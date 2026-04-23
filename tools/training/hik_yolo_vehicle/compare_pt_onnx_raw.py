import argparse
import os
import sys
from pathlib import Path
from typing import Any

import numpy as np

BASE_DIR = Path(__file__).resolve().parent
DEFAULT_PT = Path(r"E:\research\1\yolo\hiki training\model_training\runs\bear_yolov8n_x5_640\weights\best.pt")
DEFAULT_ONNX = Path(r"E:\research\1\yolo\hiki training\model_training\runs\bear_yolov8n_x5_640\weights\best.onnx")
os.environ.setdefault("YOLO_CONFIG_DIR", str((BASE_DIR / ".ultralytics").resolve()))
DEFAULT_HIK_PYTHON = Path(r"E:\Anaconda\envs\hik_yolov8\python.exe")
DEFAULT_HIK_SITE_PACKAGES = DEFAULT_HIK_PYTHON.parent / "Lib" / "site-packages"

try:
    import torch as _local_torch  # type: ignore
except Exception:
    _local_torch = None


def bootstrap_external_site_packages() -> None:
    extra_site_packages = os.environ.get("HIK_YOLO_SITE_PACKAGES", str(DEFAULT_HIK_SITE_PACKAGES))
    site_packages_path = Path(extra_site_packages)
    if not site_packages_path.exists():
        return

    env_root = site_packages_path.parent.parent
    dll_dirs = (
        env_root,
        env_root / "Library" / "bin",
        env_root / "DLLs",
        site_packages_path / "torch" / "lib",
    )

    for dll_dir in dll_dirs:
        if hasattr(os, "add_dll_directory") and dll_dir.exists():
            os.add_dll_directory(str(dll_dir))

    if str(site_packages_path) not in sys.path:
        sys.path.insert(0, str(site_packages_path))


bootstrap_external_site_packages()

import cv2


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Compare raw YOLOv8 outputs from PyTorch and ONNX on the same image.")
    parser.add_argument("--image", type=Path, required=True, help="input image path")
    parser.add_argument("--pt", type=Path, default=DEFAULT_PT, help="PyTorch weights path")
    parser.add_argument("--onnx", type=Path, default=DEFAULT_ONNX, help="ONNX model path")
    parser.add_argument("--imgsz", type=int, default=640, help="square input size")
    parser.add_argument("--topk", type=int, default=10, help="number of candidates to print")
    parser.add_argument("--device", type=str, default="cpu", help="torch device, e.g. cpu or cuda:0")
    return parser


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
        image,
        top,
        bottom,
        left,
        right,
        cv2.BORDER_CONSTANT,
        value=(114, 114, 114),
    )
    return image, ratio, (dw, dh)


def preprocess_image(image_path: Path, imgsz: int) -> tuple[np.ndarray, np.ndarray]:
    image = cv2.imread(str(image_path))
    if image is None:
        raise FileNotFoundError(f"failed to read image: {image_path}")
    original = image.copy()
    image, _, _ = letterbox(image, imgsz)
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    image = image.transpose(2, 0, 1).astype(np.float32) / 255.0
    image = np.expand_dims(image, axis=0)
    return original, image


def flatten_outputs(output: Any) -> list[np.ndarray]:
    arrays: list[np.ndarray] = []
    if isinstance(output, np.ndarray):
        arrays.append(output)
    elif isinstance(output, (list, tuple)):
        for item in output:
            arrays.extend(flatten_outputs(item))
    elif output is None:
        return arrays
    else:
        try:
            arrays.append(np.asarray(output))
        except Exception:
            return arrays
    return arrays


def select_detection_tensor(arrays: list[np.ndarray]) -> np.ndarray:
    candidates: list[np.ndarray] = []
    for array in arrays:
        if array.ndim < 3:
            continue
        if array.shape[0] != 1:
            continue
        candidates.append(array)

    if not candidates:
        raise RuntimeError("no candidate detection tensor found in model outputs")

    def score(array: np.ndarray) -> tuple[int, int]:
        dims = list(array.shape)
        has_five = 5 in dims
        volume = int(np.prod(dims))
        return (1 if has_five else 0, volume)

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
            tensor = tensor[np.newaxis, :, :]
        elif tensor.shape[1] == 5:
            tensor = tensor.T[np.newaxis, :, :]
        else:
            raise RuntimeError(f"cannot canonicalize 2D tensor shape: {array.shape}")
        return tensor

    if tensor.ndim == 3:
        if tensor.shape[0] == 5:
            tensor = tensor[np.newaxis, :, :,]
            return tensor
        if tensor.shape[1] == 5:
            return tensor
        if tensor.shape[2] == 5:
            return np.transpose(tensor, (0, 2, 1))

    raise RuntimeError(f"cannot canonicalize detection tensor shape: {array.shape}")


def summarize_tensor(name: str, tensor: np.ndarray, topk: int) -> None:
    canonical = canonicalize_detection_tensor(tensor)
    score_channel = canonical[0, 4, :]
    top_indices = np.argsort(-score_channel)[:topk]

    print(f"[{name}] raw shape: {tuple(tensor.shape)}")
    print(f"[{name}] canonical shape: {tuple(canonical.shape)}")
    print(
        f"[{name}] score channel min/max/mean: "
        f"{score_channel.min():.6f} / {score_channel.max():.6f} / {score_channel.mean():.6f}"
    )

    for rank, idx in enumerate(top_indices, start=1):
        values = canonical[0, :, idx]
        print(
            f"[{name}] top{rank:02d} idx={int(idx)} "
            f"x={values[0]:.6f} y={values[1]:.6f} w={values[2]:.6f} h={values[3]:.6f} score={values[4]:.6f}"
        )


def run_pt(weights: Path, image_tensor: np.ndarray, device: str) -> np.ndarray:
    import torch
    from ultralytics import YOLO

    model = YOLO(str(weights))
    model.model.eval()
    inputs = torch.from_numpy(image_tensor).to(device)

    with torch.no_grad():
        outputs = model.model(inputs)

    arrays = flatten_outputs(outputs)
    if not arrays:
        raise RuntimeError("PyTorch model produced no readable outputs")
    return select_detection_tensor(arrays)


def run_onnx(weights: Path, image_tensor: np.ndarray) -> np.ndarray:
    import onnxruntime as ort

    session = ort.InferenceSession(str(weights), providers=["CPUExecutionProvider"])
    input_name = session.get_inputs()[0].name
    outputs = session.run(None, {input_name: image_tensor})
    arrays = flatten_outputs(outputs)
    if not arrays:
        raise RuntimeError("ONNX model produced no readable outputs")
    return select_detection_tensor(arrays)


def main() -> None:
    args = build_argparser().parse_args()

    image_path = args.image.resolve()
    pt_path = args.pt.resolve()
    onnx_path = args.onnx.resolve()

    if not image_path.exists():
        raise FileNotFoundError(f"image does not exist: {image_path}")
    if not pt_path.exists():
        raise FileNotFoundError(f"pt weights do not exist: {pt_path}")
    if not onnx_path.exists():
        raise FileNotFoundError(f"onnx weights do not exist: {onnx_path}")

    _, image_tensor = preprocess_image(image_path, args.imgsz)

    print(f"[INPUT] image: {image_path}")
    print(f"[INPUT] pt: {pt_path}")
    print(f"[INPUT] onnx: {onnx_path}")
    print(f"[INPUT] tensor shape: {tuple(image_tensor.shape)}")

    pt_tensor = run_pt(pt_path, image_tensor, args.device)
    onnx_tensor = run_onnx(onnx_path, image_tensor)

    summarize_tensor("PT", pt_tensor, args.topk)
    summarize_tensor("ONNX", onnx_tensor, args.topk)

    pt_canonical = canonicalize_detection_tensor(pt_tensor)
    onnx_canonical = canonicalize_detection_tensor(onnx_tensor)
    abs_diff = np.abs(pt_canonical - onnx_canonical)

    print(
        "[DIFF] abs diff min/max/mean: "
        f"{abs_diff.min():.6f} / {abs_diff.max():.6f} / {abs_diff.mean():.6f}"
    )
    score_diff = np.abs(pt_canonical[0, 4, :] - onnx_canonical[0, 4, :])
    print(
        "[DIFF] score diff min/max/mean: "
        f"{score_diff.min():.6f} / {score_diff.max():.6f} / {score_diff.mean():.6f}"
    )


if __name__ == "__main__":
    main()
