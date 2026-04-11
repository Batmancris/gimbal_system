#!/usr/bin/env python3
"""Minimal TianAim capture-session scaffold."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def parse_scalar(value: str) -> Any:
    lowered = value.lower()
    if lowered in {"true", "false"}:
        return lowered == "true"
    if lowered in {"null", "none"}:
        return None
    try:
        return int(value)
    except ValueError:
        pass
    try:
        return float(value)
    except ValueError:
        pass
    return value.strip("\"'")


def load_simple_yaml(path: Path) -> dict[str, Any]:
    raw_lines = path.read_text(encoding="utf-8").splitlines()
    lines: list[tuple[int, str]] = []
    for raw_line in raw_lines:
        stripped = raw_line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        indent = len(raw_line) - len(raw_line.lstrip(" "))
        lines.append((indent, stripped))

    def parse_block(index: int, indent: int) -> tuple[Any, int]:
        if index >= len(lines):
            return {}, index

        container: Any
        if lines[index][1].startswith("- "):
            container = []
            while index < len(lines):
                line_indent, content = lines[index]
                if line_indent < indent:
                    break
                if line_indent != indent or not content.startswith("- "):
                    break
                container.append(parse_scalar(content[2:].strip()))
                index += 1
            return container, index

        container = {}
        while index < len(lines):
            line_indent, content = lines[index]
            if line_indent < indent:
                break
            if line_indent != indent:
                raise ValueError(f"Unexpected indentation near: {content}")

            key, _, value = content.partition(":")
            key = key.strip()
            value = value.strip()
            index += 1

            if value:
                container[key] = parse_scalar(value)
                continue

            if index < len(lines) and lines[index][0] > line_indent:
                nested, index = parse_block(index, lines[index][0])
                container[key] = nested
            else:
                container[key] = {}

        return container, index

    parsed, _ = parse_block(0, lines[0][0] if lines else 0)
    if not isinstance(parsed, dict):
        raise ValueError("Top-level YAML structure must be a mapping.")
    return parsed


@dataclass
class CaptureConfig:
    project_name: str
    dataset_root: Path
    manifest_root: Path
    camera_vendor: str
    camera_model: str
    camera_serial: str
    camera_id: str
    image_format: str
    session_prefix: str
    labels_expected: list[str]
    environment: dict[str, Any]

    @classmethod
    def from_dict(cls, data: dict[str, Any], repo_root: Path) -> "CaptureConfig":
        camera = data.get("camera", {})
        capture = data.get("capture", {})
        environment = data.get("environment", {})
        labels = capture.get("labels_expected", [])
        return cls(
            project_name=str(data.get("project_name", "TianAim")),
            dataset_root=repo_root / str(data.get("dataset_root", "datasets/raw")),
            manifest_root=repo_root / str(data.get("manifest_root", "datasets/manifests")),
            camera_vendor=str(camera.get("vendor", "Hikvision")),
            camera_model=str(camera.get("model", "TODO")),
            camera_serial=str(camera.get("serial_number", "TODO")),
            camera_id=str(camera.get("camera_id", "cam0")),
            image_format=str(capture.get("image_format", "jpg")),
            session_prefix=str(capture.get("session_prefix", "capture")),
            labels_expected=list(labels if isinstance(labels, list) else []),
            environment=dict(environment if isinstance(environment, dict) else {}),
        )


class HikCaptureAdapter:
    """Placeholder interface for future Hik SDK integration."""

    def capture(self) -> None:
        raise NotImplementedError(
            "Real Hik SDK capture is not wired yet. Use --source-dir to import existing images."
        )


def build_session_id(prefix: str) -> str:
    return f"{datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')}_{prefix}"


def collect_source_images(source_dir: Path) -> list[Path]:
    allowed = {".jpg", ".jpeg", ".png", ".bmp"}
    return sorted(
        path for path in source_dir.iterdir()
        if path.is_file() and path.suffix.lower() in allowed
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a TianAim capture session.")
    parser.add_argument("--config", required=True, help="Path to capture config YAML.")
    parser.add_argument("--source-dir", help="Optional directory of existing images to import.")
    parser.add_argument("--session-id", help="Optional fixed session id.")
    args = parser.parse_args()

    script_path = Path(__file__).resolve()
    repo_root = script_path.parents[2]
    config_path = (repo_root / args.config).resolve() if not Path(args.config).is_absolute() else Path(args.config)
    config = CaptureConfig.from_dict(load_simple_yaml(config_path), repo_root)

    session_id = args.session_id or build_session_id(config.session_prefix)
    session_dir = config.dataset_root / session_id
    manifest_path = config.manifest_root / f"{session_id}.json"

    session_dir.mkdir(parents=True, exist_ok=True)
    config.manifest_root.mkdir(parents=True, exist_ok=True)

    frames: list[dict[str, Any]] = []
    if args.source_dir:
        for index, image_path in enumerate(collect_source_images(Path(args.source_dir).resolve()), start=1):
            timestamp_ns = int(datetime.now(timezone.utc).timestamp() * 1_000_000_000) + index
            target_name = (
                f"{session_id}_{config.camera_id}_{timestamp_ns}_{index:06d}"
                f"{image_path.suffix.lower()}"
            )
            shutil.copy2(image_path, session_dir / target_name)
            frames.append(
                {
                    "file_name": target_name,
                    "frame_index": index,
                    "timestamp_ns": timestamp_ns,
                    "source": str(image_path),
                    "annotations": [],
                }
            )
    else:
        (session_dir / "README.txt").write_text(
            "Capture session scaffold created. Integrate Hik SDK or rerun with --source-dir.\n",
            encoding="utf-8",
        )

    manifest = {
        "schema_version": "1.0",
        "project": config.project_name,
        "session_id": session_id,
        "camera": {
            "vendor": config.camera_vendor,
            "model": config.camera_model,
            "serial_number": config.camera_serial,
            "camera_id": config.camera_id,
        },
        "capture_mode": "import_existing_images" if args.source_dir else "session_scaffold_only",
        "image_format": config.image_format,
        "labels_expected": config.labels_expected,
        "environment": config.environment,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "frame_count": len(frames),
        "frames": frames,
        "todo": [
            "Integrate real Hik SDK adapter for live capture.",
            "Record exposure, gain, and calibration metadata when available."
        ],
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"Created session: {session_dir}")
    print(f"Wrote manifest: {manifest_path}")
    if not args.source_dir:
        print("No frames imported. This run created a minimal session scaffold.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
