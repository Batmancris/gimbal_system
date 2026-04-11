import argparse
import os
from pathlib import Path

import torch
import yaml
from ultralytics import YOLO


BASE_DIR = Path(__file__).resolve().parent


def load_yaml(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def merge_dict(base: dict, override: dict) -> dict:
    merged = dict(base)
    for key, value in override.items():
        if value is not None:
            merged[key] = value
    return merged


def resolve_local_path(value: str | Path | None, *, base_dir: Path) -> Path | None:
    if value is None:
        return None
    path = Path(value)
    if path.is_absolute():
        return path
    return (base_dir / path).resolve()


def resolve_dataset_yaml(path: Path) -> Path:
    data = load_yaml(path)
    yaml_dir = path.resolve().parent

    root = data.get("path")
    if root:
        root_path = Path(root)
        if not root_path.is_absolute():
            root_path = (yaml_dir / root_path).resolve()
    else:
        root_path = yaml_dir

    data["path"] = root_path.as_posix()

    tmp_dir = BASE_DIR / ".tmp"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    resolved_yaml = tmp_dir / f"{path.stem}_resolved.yaml"
    resolved_yaml.write_text(yaml.safe_dump(data, sort_keys=False, allow_unicode=True), encoding="utf-8")
    return resolved_yaml


def ensure_matching_labels(image_dir: Path, label_dir: Path, split_name: str) -> None:
    image_files = [item for item in image_dir.iterdir() if item.is_file()]
    if not image_files:
        raise FileNotFoundError(f"no images found in split '{split_name}': {image_dir}")

    missing_labels: list[str] = []
    for image_file in image_files:
        label_file = label_dir / f"{image_file.stem}.txt"
        if not label_file.exists():
            missing_labels.append(image_file.name)
        if len(missing_labels) >= 10:
            break

    if missing_labels:
        preview = ", ".join(missing_labels[:10])
        raise FileNotFoundError(
            f"missing label files for split '{split_name}' under {label_dir}. "
            f"Examples: {preview}"
        )


def validate_dataset_layout(path: Path) -> Path:
    data = load_yaml(path)
    yaml_dir = path.resolve().parent

    root = data.get("path")
    if root:
        dataset_root = resolve_local_path(root, base_dir=yaml_dir)
    else:
        dataset_root = yaml_dir

    assert dataset_root is not None
    if not dataset_root.exists():
        raise FileNotFoundError(f"dataset root does not exist: {dataset_root}")

    required_splits = ("train", "val")
    optional_splits = ("test",)
    for split_name in (*required_splits, *optional_splits):
        split_value = data.get(split_name)
        if not split_value:
            if split_name in required_splits:
                raise KeyError(f"missing required dataset split '{split_name}' in {path}")
            continue

        image_dir = resolve_local_path(split_value, base_dir=dataset_root)
        assert image_dir is not None
        if not image_dir.exists():
            raise FileNotFoundError(f"image directory for split '{split_name}' does not exist: {image_dir}")

        label_dir = dataset_root / "labels" / split_name
        if not label_dir.exists():
            raise FileNotFoundError(f"label directory for split '{split_name}' does not exist: {label_dir}")

        ensure_matching_labels(image_dir, label_dir, split_name)

    names = data.get("names")
    if not names:
        raise KeyError(f"dataset config is missing class names: {path}")

    return dataset_root


def log_stage(message: str) -> None:
    print(f"[TRAIN] {message}", flush=True)


def ensure_model_ready(model_name: str) -> str:
    model_path = resolve_local_path(model_name, base_dir=BASE_DIR)
    if model_path and model_path.exists():
        return str(model_path)

    if model_name.endswith(".pt"):
        log_stage(f"Model weights not found locally, Ultralytics will try to download: {model_name}")
        return model_name

    raise FileNotFoundError(f"model weights not found: {model_name}")


def resolve_device(device: str | None) -> str:
    requested = str(device or "0").strip()
    if requested.lower() == "cpu":
        return "cpu"

    if torch.cuda.is_available():
        return requested

    raise RuntimeError(
        "CUDA device requested but not available in the current Python environment. "
        f"Requested device={requested}, torch={torch.__version__}, torch.version.cuda={torch.version.cuda}. "
        "Install a CUDA-enabled PyTorch build for this environment, or explicitly pass --device cpu if you really want CPU training."
    )


def resolve_output_dir(project_value: str | None) -> Path:
    return resolve_local_path(project_value or "runs", base_dir=BASE_DIR) or (BASE_DIR / "runs").resolve()


def normalize_cache_value(value: object) -> object:
    if not isinstance(value, str):
        return value

    lowered = value.strip().lower()
    if lowered in {"true", "1", "yes", "on"}:
        return True
    if lowered in {"false", "0", "no", "off"}:
        return False
    return lowered


def summarize_runtime(device: str) -> None:
    cpu_count = os.cpu_count() or 0
    torch_cuda = torch.version.cuda or "cpu"
    log_stage(
        f"Python={os.sys.version.split()[0]}, torch={torch.__version__}, "
        f"torch_cuda={torch_cuda}, cpu_threads={cpu_count}, device={device}"
    )
    if device != "cpu" and torch.cuda.is_available():
        primary_index = int(device.split(',')[0])
        gpu_name = torch.cuda.get_device_name(primary_index)
        total_mem_gib = torch.cuda.get_device_properties(primary_index).total_memory / (1024 ** 3)
        log_stage(f"GPU detected: {gpu_name} ({total_mem_gib:.1f} GiB)")


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="YOLOv8 train/val/export entry")
    parser.add_argument("--mode", choices=["train", "val", "export", "check"], default="train")
    parser.add_argument("--train-config", type=Path, default=BASE_DIR / "configs" / "train_yolov8.yaml")
    parser.add_argument("--data-config", type=Path, default=BASE_DIR / "configs" / "data_vehicle.yaml")
    parser.add_argument("--weights", type=str, default=None)
    parser.add_argument("--imgsz", type=int, default=None)
    parser.add_argument("--epochs", type=int, default=None)
    parser.add_argument("--batch", type=int, default=None)
    parser.add_argument("--device", type=str, default=None)
    parser.add_argument("--workers", type=int, default=None)
    parser.add_argument("--project", type=str, default=None)
    parser.add_argument("--name", type=str, default=None)
    parser.add_argument("--resume", action="store_true", help="resume training from the last checkpoint")
    parser.add_argument("--cache", type=str, default=None, help="cache mode override, e.g. true/false/ram/disk")
    parser.add_argument("--close-mosaic", dest="close_mosaic", type=int, default=None)
    parser.add_argument("--format", type=str, default=None, help="export format, e.g. onnx")
    return parser


def main() -> None:
    args = build_argparser().parse_args()
    log_stage("Loading configs")
    train_cfg_path = args.train_config.resolve()
    data_cfg_path = resolve_dataset_yaml(args.data_config.resolve())
    train_cfg = load_yaml(train_cfg_path)

    overrides = {
        "model": args.weights,
        "imgsz": args.imgsz,
        "epochs": args.epochs,
        "batch": args.batch,
        "device": args.device,
        "workers": args.workers,
        "project": args.project,
        "name": args.name,
        "cache": args.cache,
        "close_mosaic": args.close_mosaic,
    }
    cfg = merge_dict(train_cfg, overrides)
    cfg["cache"] = normalize_cache_value(cfg.get("cache"))
    log_stage(f"Resolved train config: {train_cfg_path}")
    log_stage(f"Resolved data config: {data_cfg_path}")
    dataset_root = validate_dataset_layout(data_cfg_path)
    resolved_device = resolve_device(cfg.get("device"))
    summarize_runtime(resolved_device)
    resume_requested = args.resume or bool(cfg.get("resume", False))
    project_dir = resolve_output_dir(cfg.get("project"))
    log_stage(
        "Runtime settings: "
        f"mode={args.mode}, model={cfg.get('model', 'yolov8.yaml')}, "
        f"imgsz={cfg.get('imgsz', 640)}, batch={cfg.get('batch', 16)}, "
        f"workers={cfg.get('workers', 8)}, device={resolved_device}, resume={resume_requested}, "
        f"cache={cfg.get('cache', False)}, close_mosaic={cfg.get('close_mosaic', 0)}"
    )
    log_stage(f"Dataset root: {dataset_root}")
    log_stage(f"Output project dir: {project_dir}")

    model_name = cfg.get("model", "yolov8.yaml")
    if resume_requested:
        resume_model = cfg.get("resume_model")
        if not resume_model:
            resume_model = str(project_dir / cfg.get("name", "vehicle_yolov8") / "weights" / "last.pt")
        resume_path = resolve_local_path(resume_model, base_dir=BASE_DIR)
        assert resume_path is not None
        if not resume_path.exists():
            raise FileNotFoundError(f"resume checkpoint not found: {resume_path}")
        model_name = str(resume_path)

    model_name = ensure_model_ready(model_name)
    log_stage(f"Loading model from: {model_name}")

    if args.mode == "check":
        log_stage("Environment and dataset checks passed")
        return

    model = YOLO(model_name)

    if args.mode == "train":
        log_stage("Handing off to Ultralytics trainer")
        model.train(
            data=str(data_cfg_path),
            imgsz=cfg.get("imgsz", 640),
            epochs=cfg.get("epochs", 100),
            batch=cfg.get("batch", 16),
            device=resolved_device,
            workers=cfg.get("workers", 8),
            project=str(project_dir),
            name=cfg.get("name", "vehicle_yolov8"),
            pretrained=cfg.get("pretrained", True),
            optimizer=cfg.get("optimizer", "auto"),
            lr0=cfg.get("lr0", 0.01),
            lrf=cfg.get("lrf", 0.01),
            momentum=cfg.get("momentum", 0.937),
            weight_decay=cfg.get("weight_decay", 0.0005),
            warmup_epochs=cfg.get("warmup_epochs", 3.0),
            hsv_h=cfg.get("hsv_h", 0.015),
            hsv_s=cfg.get("hsv_s", 0.7),
            hsv_v=cfg.get("hsv_v", 0.4),
            degrees=cfg.get("degrees", 0.0),
            translate=cfg.get("translate", 0.1),
            scale=cfg.get("scale", 0.5),
            fliplr=cfg.get("fliplr", 0.5),
            mosaic=cfg.get("mosaic", 1.0),
            close_mosaic=cfg.get("close_mosaic", 0),
            mixup=cfg.get("mixup", 0.0),
            cache=cfg.get("cache", False),
            patience=cfg.get("patience", 50),
            cos_lr=cfg.get("cos_lr", False),
            amp=cfg.get("amp", True),
            exist_ok=cfg.get("exist_ok", True),
            resume=resume_requested,
        )
        return

    if args.mode == "val":
        val_weights = cfg.get("val_model", cfg.get("model", "yolov8.yaml"))
        val_path = resolve_local_path(val_weights, base_dir=BASE_DIR)
        if val_path is not None:
            val_weights = str(val_path)
        YOLO(val_weights).val(
            data=str(data_cfg_path),
            imgsz=cfg.get("imgsz", 640),
            batch=cfg.get("batch", 16),
            device=resolved_device,
            workers=cfg.get("workers", 8),
            project=str(project_dir),
            name=cfg.get("val_name", f"{cfg.get('name', 'vehicle_yolov8')}_val"),
            exist_ok=True,
        )
        return

    export_weights = cfg.get("export_model", cfg.get("val_model", cfg.get("model", "yolov8.yaml")))
    export_path = resolve_local_path(export_weights, base_dir=BASE_DIR)
    if export_path is not None:
        export_weights = str(export_path)
    YOLO(export_weights).export(
        format=args.format or cfg.get("export_format", "onnx"),
        imgsz=cfg.get("imgsz", 640),
        device=resolved_device,
        simplify=cfg.get("simplify", True),
        dynamic=cfg.get("dynamic", False),
        half=cfg.get("half", False),
    )


if __name__ == "__main__":
    main()
