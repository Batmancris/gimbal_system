# capture

## Purpose

This directory contains the TianAim dataset capture workflow skeleton.

Current contents:

- `capture_session.py`
- `config.yaml`

## Minimal Usage

Create a new capture session skeleton:

```bash
python3 tools/capture/capture_session.py --config tools/capture/config.yaml
```

Import existing images into a managed session:

```bash
python3 tools/capture/capture_session.py \
  --config tools/capture/config.yaml \
  --source-dir /path/to/images
```

## Current Limitation

Real Hik SDK integration is not wired in this repository yet. The script currently creates a reproducible session directory, copies or registers images, and emits a manifest.
