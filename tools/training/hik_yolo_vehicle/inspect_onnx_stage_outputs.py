import argparse
from pathlib import Path
from typing import Any

import cv2
import numpy as np


IMAGE_SUFFIXES = {".bmp", ".jpg", ".jpeg", ".png", ".webp"}


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Inspect intermediate ONNX stage outputs against a trusted ONNX baseline."
    )
    parser.add_argument("--reference-onnx", type=Path, required=True, help="trusted ONNX baseline")
    parser.add_argument("--candidate-onnx", type=Path, nargs="+", required=True, help="intermediate ONNX models")
    parser.add_argument("--image-dir", type=Path, required=True, help="directory containing test images")
    parser.add_argument("--imgsz", type=int, default=640, help="square input size")
    parser.add_argument("--limit", type=int, default=3, help="maximum number of images to inspect")
    parser.add_argument("--topk", type=int, default=5, help="top anchors to inspect")
    parser.add_argument("--mean-threshold", type=float, default=0.25, help="max allowed mean absolute diff")
    parser.add_argument("--max-threshold", type=float, default=32.0, help="max allowed absolute diff")
    return parser


def iter_images(image_dir: Path) -> list[Path]:
    return sorted(
        path for path in image_dir.iterdir()
        if path.is_file() and path.suffix.lower() in IMAGE_SUFFIXES
    )


def letterbox(image: np.ndarray, new_shape: int) -> np.ndarray:
    shape = image.shape[:2]
    ratio = min(new_shape / shape[0], new_shape / shape[1])
    new_unpad = (int(round(shape[1] * ratio)), int(round(shape[0] * ratio)))
    dw = (new_shape - new_unpad[0]) / 2
    dh = (new_shape - new_unpad[1]) / 2
    if shape[::-1] != new_unpad:
        image = cv2.resize(image, new_unpad, interpolation=cv2.INTER_LINEAR)
    top = int(round(dh - 0.1))
    bottom = int(round(dh + 0.1))
    left = int(round(dw - 0.1))
    right = int(round(dw + 0.1))
    return cv2.copyMakeBorder(
        image, top, bottom, left, right, cv2.BORDER_CONSTANT, value=(114, 114, 114)
    )


def preprocess_image(image_path: Path, imgsz: int) -> np.ndarray:
    image = cv2.imread(str(image_path))
    if image is None:
        raise FileNotFoundError(f"failed to read image: {image_path}")
    image = letterbox(image, imgsz)
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    image = image.transpose(2, 0, 1).astype(np.float32) / 255.0
    return np.expand_dims(image, axis=0)


def flatten_outputs(output: Any) -> list[np.ndarray]:
    arrays: list[np.ndarray] = []
    if isinstance(output, np.ndarray):
        arrays.append(output)
    elif isinstance(output, (list, tuple)):
        for item in output:
            arrays.extend(flatten_outputs(item))
    elif output is not None:
        try:
            arrays.append(np.asarray(output))
        except Exception:
            return arrays
    return arrays


def select_detection_tensor(arrays: list[np.ndarray]) -> np.ndarray:
    candidates: list[np.ndarray] = []
    for array in arrays:
        if array.ndim < 3 or array.shape[0] != 1:
            continue
        candidates.append(array)
    if not candidates:
        raise RuntimeError("no candidate detection tensor found in model outputs")
    return max(candidates, key=lambda array: (1 if 5 in array.shape else 0, int(np.prod(array.shape))))


def canonicalize_detection_tensor(array: np.ndarray) -> np.ndarray:
    tensor = np.asarray(array, dtype=np.float32)
    tensor = np.squeeze(tensor)
    if tensor.ndim == 3 and tensor.shape[-1] == 1:
        tensor = np.squeeze(tensor, axis=-1)
    if tensor.ndim == 2:
        if tensor.shape[0] == 5:
            return tensor[np.newaxis, :, :]
        if tensor.shape[1] == 5:
            return tensor.T[np.newaxis, :, :]
    if tensor.ndim == 3:
        if tensor.shape[1] == 5:
            return tensor
        if tensor.shape[0] == 5:
            return tensor[np.newaxis, :, :]
        if tensor.shape[2] == 5:
            return np.transpose(tensor, (0, 2, 1))
    raise RuntimeError(f"cannot canonicalize detection tensor shape: {array.shape}")


def run_onnx(weights: Path, image_tensor: np.ndarray) -> np.ndarray:
    import onnxruntime as ort

    try:
        session = ort.InferenceSession(str(weights), providers=["CPUExecutionProvider"])
    except Exception as exc:
        raise RuntimeError(
            f"failed to load {weights.name} with CPU ONNX Runtime. "
            "This usually means the file already contains Horizon custom ops and must be inspected "
            "inside the Horizon/OpenExplorer environment instead."
        ) from exc
    input_name = session.get_inputs()[0].name
    outputs = session.run(None, {input_name: image_tensor})
    return select_detection_tensor(flatten_outputs(outputs))


def summarize(name: str, canonical: np.ndarray, topk: int) -> list[int]:
    score_channel = canonical[0, 4, :]
    top_indices = np.argsort(-score_channel)[:topk]
    print(
        f"[{name}] shape={tuple(canonical.shape)} "
        f"score min/max/mean={score_channel.min():.6f}/{score_channel.max():.6f}/{score_channel.mean():.6f}"
    )
    for rank, idx in enumerate(top_indices, start=1):
        values = canonical[0, :, idx]
        print(
            f"[{name}] top{rank:02d} idx={int(idx)} "
            f"raw0={values[0]:.6f} raw1={values[1]:.6f} raw2={values[2]:.6f} raw3={values[3]:.6f} score={values[4]:.6f}"
        )
        if values[2] <= 0.0 or values[3] <= 0.0:
            raise RuntimeError(f"{name} top anchor idx={int(idx)} has invalid width/height")
    return [int(index) for index in top_indices]


def main() -> None:
    args = build_argparser().parse_args()

    reference_path = args.reference_onnx.resolve()
    candidate_paths = [path.resolve() for path in args.candidate_onnx]
    images = iter_images(args.image_dir.resolve())
    if not images:
        raise FileNotFoundError(f"no images found under: {args.image_dir.resolve()}")

    selected = images[:args.limit] if args.limit > 0 else images
    print(f"[INSPECT] reference={reference_path}")
    print(f"[INSPECT] candidates={[str(path) for path in candidate_paths]}")
    print(f"[INSPECT] image_count={len(selected)} imgsz={args.imgsz}")

    for image_path in selected:
        print(f"\n=== {image_path.name} ===")
        image_tensor = preprocess_image(image_path, args.imgsz)
        reference = canonicalize_detection_tensor(run_onnx(reference_path, image_tensor))
        summarize("reference", reference, args.topk)
        for candidate_path in candidate_paths:
            candidate = canonicalize_detection_tensor(run_onnx(candidate_path, image_tensor))
            summarize(candidate_path.stem, candidate, args.topk)
            abs_diff = np.abs(reference - candidate)
            diff_mean = float(abs_diff.mean())
            diff_max = float(abs_diff.max())
            print(f"[{candidate_path.stem}] abs diff mean/max={diff_mean:.6f}/{diff_max:.6f}")
            if diff_mean > args.mean_threshold or diff_max > args.max_threshold:
                raise RuntimeError(
                    f"{candidate_path.name} drifted too far from reference on {image_path.name}: "
                    f"mean={diff_mean:.6f}, max={diff_max:.6f}"
                )

    print("\n[INSPECT] all stage checks passed", flush=True)


if __name__ == "__main__":
    main()
