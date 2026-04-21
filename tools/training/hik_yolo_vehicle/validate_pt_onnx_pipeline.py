import argparse
from pathlib import Path
from typing import Any

import cv2
import numpy as np


IMAGE_SUFFIXES = {".bmp", ".jpg", ".jpeg", ".png", ".webp"}


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Batch-validate that PT and ONNX raw outputs stay semantically aligned."
    )
    parser.add_argument("--pt", type=Path, required=True, help="PyTorch weights path")
    parser.add_argument("--onnx", type=Path, nargs="+", required=True, help="one or more ONNX models to validate")
    parser.add_argument("--image-dir", type=Path, required=True, help="directory containing test images")
    parser.add_argument("--imgsz", type=int, default=640, help="square input size")
    parser.add_argument("--limit", type=int, default=3, help="maximum number of images to validate")
    parser.add_argument("--topk", type=int, default=5, help="top anchors to inspect")
    parser.add_argument("--device", type=str, default="cpu", help="torch device, e.g. cpu or cuda:0")
    parser.add_argument("--mean-threshold", type=float, default=0.02, help="max allowed mean absolute diff")
    parser.add_argument("--max-threshold", type=float, default=3.0, help="max allowed absolute diff")
    return parser


def iter_images(image_dir: Path) -> list[Path]:
    return sorted(
        path for path in image_dir.iterdir()
        if path.is_file() and path.suffix.lower() in IMAGE_SUFFIXES
    )


def letterbox(image: np.ndarray, new_shape: int) -> tuple[np.ndarray, float, tuple[float, float]]:
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
    image = cv2.copyMakeBorder(
        image, top, bottom, left, right, cv2.BORDER_CONSTANT, value=(114, 114, 114)
    )
    return image, ratio, (dw, dh)


def preprocess_image(image_path: Path, imgsz: int) -> np.ndarray:
    image = cv2.imread(str(image_path))
    if image is None:
        raise FileNotFoundError(f"failed to read image: {image_path}")
    image, _, _ = letterbox(image, imgsz)
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

    def score(array: np.ndarray) -> tuple[int, int]:
        dims = list(array.shape)
        return (1 if 5 in dims else 0, int(np.prod(dims)))

    return max(candidates, key=score)


def canonicalize_detection_tensor(array: np.ndarray) -> np.ndarray:
    tensor = np.asarray(array, dtype=np.float32)
    tensor = np.squeeze(tensor)
    if tensor.ndim == 1:
        raise RuntimeError(f"unexpected 1D detection tensor shape: {array.shape}")
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


def run_pt(weights: Path, image_tensor: np.ndarray, device: str) -> np.ndarray:
    import torch
    from ultralytics import YOLO

    model = YOLO(str(weights))
    model.model.eval()
    inputs = torch.from_numpy(image_tensor).to(device)
    with torch.no_grad():
        outputs = model.model(inputs)
    return select_detection_tensor(flatten_outputs(outputs))


def run_onnx(weights: Path, image_tensor: np.ndarray) -> np.ndarray:
    import onnxruntime as ort

    session = ort.InferenceSession(str(weights), providers=["CPUExecutionProvider"])
    input_name = session.get_inputs()[0].name
    outputs = session.run(None, {input_name: image_tensor})
    return select_detection_tensor(flatten_outputs(outputs))


def summarize_topk(name: str, canonical: np.ndarray, topk: int) -> list[int]:
    score_channel = canonical[0, 4, :]
    top_indices = np.argsort(-score_channel)[:topk]
    print(
        f"[{name}] score min/max/mean="
        f"{score_channel.min():.6f}/{score_channel.max():.6f}/{score_channel.mean():.6f}"
    )
    for rank, idx in enumerate(top_indices, start=1):
        values = canonical[0, :, idx]
        print(
            f"[{name}] top{rank:02d} idx={int(idx)} "
            f"cx={values[0]:.6f} cy={values[1]:.6f} w={values[2]:.6f} h={values[3]:.6f} score={values[4]:.6f}"
        )
    return [int(index) for index in top_indices]


def check_topk_width_height(name: str, canonical: np.ndarray, top_indices: list[int]) -> None:
    for idx in top_indices:
        width = float(canonical[0, 2, idx])
        height = float(canonical[0, 3, idx])
        if width <= 0.0 or height <= 0.0:
            raise RuntimeError(f"{name} top anchor idx={idx} has invalid width/height: {width}, {height}")


def main() -> None:
    args = build_argparser().parse_args()

    pt_path = args.pt.resolve()
    image_dir = args.image_dir.resolve()
    onnx_paths = [path.resolve() for path in args.onnx]
    images = iter_images(image_dir)
    if not images:
        raise FileNotFoundError(f"no images found under: {image_dir}")

    selected = images[:args.limit] if args.limit > 0 else images
    print(f"[VALIDATE] pt={pt_path}")
    print(f"[VALIDATE] onnx={[str(path) for path in onnx_paths]}")
    print(f"[VALIDATE] image_dir={image_dir}")
    print(f"[VALIDATE] limit={len(selected)} imgsz={args.imgsz}")

    for image_path in selected:
        print(f"\n=== {image_path.name} ===")
        image_tensor = preprocess_image(image_path, args.imgsz)
        pt_canonical = canonicalize_detection_tensor(run_pt(pt_path, image_tensor, args.device))
        pt_top = summarize_topk("PT", pt_canonical, args.topk)
        check_topk_width_height("PT", pt_canonical, pt_top)

        for onnx_path in onnx_paths:
            onnx_canonical = canonicalize_detection_tensor(run_onnx(onnx_path, image_tensor))
            name = onnx_path.stem
            top_indices = summarize_topk(name, onnx_canonical, args.topk)
            check_topk_width_height(name, onnx_canonical, top_indices)
            abs_diff = np.abs(pt_canonical - onnx_canonical)
            diff_mean = float(abs_diff.mean())
            diff_max = float(abs_diff.max())
            print(f"[{name}] abs diff mean/max={diff_mean:.6f}/{diff_max:.6f}")
            if diff_mean > args.mean_threshold or diff_max > args.max_threshold:
                raise RuntimeError(
                    f"{onnx_path.name} drifted too far from PT on {image_path.name}: "
                    f"mean={diff_mean:.6f}, max={diff_max:.6f}"
                )

    print("\n[VALIDATE] all checks passed", flush=True)


if __name__ == "__main__":
    main()
