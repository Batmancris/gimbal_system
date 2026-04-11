import argparse
import os
from pathlib import Path

import yaml

BASE_DIR = Path(__file__).resolve().parent
DEFAULT_DATA_CONFIG = BASE_DIR / "configs" / "data_vehicle.yaml"
DEFAULT_WEIGHTS = BASE_DIR / "runs" / "vehicle_yolov8x_4090" / "weights" / "best.pt"
os.environ.setdefault("YOLO_CONFIG_DIR", str((BASE_DIR / ".ultralytics").resolve()))

from ultralytics import YOLO


def load_yaml(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as file:
        return yaml.safe_load(file) or {}


def resolve_local_path(value: str | Path | None, *, base_dir: Path) -> Path | None:
    if value is None:
        return None
    path = Path(value)
    if path.is_absolute():
        return path
    return (base_dir / path).resolve()


def resolve_dataset_yaml(path: Path) -> tuple[Path, Path, Path]:
    data = load_yaml(path)
    yaml_dir = path.resolve().parent

    root = data.get("path")
    dataset_root = resolve_local_path(root, base_dir=yaml_dir) if root else yaml_dir
    if dataset_root is None or not dataset_root.exists():
        raise FileNotFoundError(f"dataset root does not exist: {dataset_root}")

    test_value = data.get("test")
    if not test_value:
        raise KeyError(f"dataset config is missing 'test' split: {path}")

    test_dir = resolve_local_path(test_value, base_dir=dataset_root)
    if test_dir is None or not test_dir.exists():
        raise FileNotFoundError(f"test image directory does not exist: {test_dir}")

    data["path"] = dataset_root.as_posix()

    tmp_dir = BASE_DIR / ".tmp"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    resolved_yaml = tmp_dir / f"{path.stem}_resolved.yaml"
    resolved_yaml.write_text(
        yaml.safe_dump(data, sort_keys=False, allow_unicode=True),
        encoding="utf-8",
    )
    return resolved_yaml, dataset_root, test_dir


def print_metrics(metrics: object) -> None:
    results_dict = getattr(metrics, "results_dict", {}) or {}
    if results_dict:
        print("[TEST] Validation metrics")
        for key, value in results_dict.items():
            if isinstance(value, (int, float)):
                print(f"  {key}: {value:.6f}")
            else:
                print(f"  {key}: {value}")
        return

    box = getattr(metrics, "box", None)
    if box is None:
        print("[TEST] Metrics object returned, but no readable values were found.")
        return

    print("[TEST] Validation metrics")
    for key in ("mp", "mr", "map50", "map"):
        value = getattr(box, key, None)
        if value is not None:
            print(f"  box.{key}: {value:.6f}")


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Evaluate the trained vehicle detector on the test split.")
    parser.add_argument("--weights", type=Path, default=DEFAULT_WEIGHTS)
    parser.add_argument("--data-config", type=Path, default=DEFAULT_DATA_CONFIG)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--batch", type=int, default=16)
    parser.add_argument("--device", type=str, default="cpu")
    parser.add_argument("--workers", type=int, default=0)
    parser.add_argument("--conf", type=float, default=0.25)
    parser.add_argument("--iou", type=float, default=0.7)
    parser.add_argument("--project", type=Path, default=BASE_DIR / "runs")
    parser.add_argument("--name", type=str, default="vehicle_yolov8x_4090_test")
    parser.add_argument("--save-predictions", action="store_true")
    parser.add_argument("--save-txt", action="store_true")
    return parser


def main() -> None:
    args = build_argparser().parse_args()

    weights_path = args.weights.resolve()
    if not weights_path.exists():
        raise FileNotFoundError(f"weights file does not exist: {weights_path}")

    data_yaml, dataset_root, test_dir = resolve_dataset_yaml(args.data_config.resolve())

    print(f"[TEST] Weights: {weights_path}")
    print(f"[TEST] Dataset root: {dataset_root}")
    print(f"[TEST] Test images: {test_dir}")

    model = YOLO(str(weights_path))
    metrics = model.val(
        data=str(data_yaml),
        split="test",
        imgsz=args.imgsz,
        batch=args.batch,
        device=args.device,
        workers=args.workers,
        project=str(args.project.resolve()),
        name=args.name,
        exist_ok=True,
        conf=args.conf,
        iou=args.iou,
        plots=True,
        verbose=True,
    )
    print_metrics(metrics)

    if args.save_predictions:
        prediction_name = f"{args.name}_predictions"
        print(f"[TEST] Saving annotated predictions to: {args.project.resolve() / prediction_name}")
        model.predict(
            source=str(test_dir),
            imgsz=args.imgsz,
            conf=args.conf,
            iou=args.iou,
            device=args.device,
            project=str(args.project.resolve()),
            name=prediction_name,
            exist_ok=True,
            save=True,
            save_txt=args.save_txt,
            verbose=False,
        )


if __name__ == "__main__":
    main()
