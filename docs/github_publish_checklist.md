# GitHub Publish Checklist

Updated: 2026-04-26

## Repository Scope

训练/量化/采集工具已拆分为独立工具仓，本仓库仅保留云台主系统代码。

The public GitHub README should present this repository as the TianAim gimbal main system repository, not as a combined training/capture/quantization workspace.

## Required Before Publish

- Root README is readable and matches the final repository scope.
- `docs/repo_cleanup/tools_split_note.md` explains the split-tool decision.
- `docs/repo_cleanup/target_structure.md` defines the final directory map.
- `docs/repo_cleanup/large_file_risk.md` lists binary and licensing risk.
- No raw datasets, videos, local caches, training weights, or quantization outputs are added.
- No runtime code changes are mixed into cleanup commits.

## Branch Checks

- Confirm whether `feature/formula-mini-kt-cloud-follow` is the release base.
- Keep `main` protected as the GitHub PR target.
- Treat `origin/chore/repo-restructure` as superseded only after review.
- Prefer a new clean branch for this publish-prep work.

## Binary Checks

- Confirm Hikvision SDK redistribution permission.
- Confirm STM32 HAL/CMSIS/FreeRTOS license notices.
- Confirm whether current quantized model bins should remain tracked.
- Decide whether Git LFS, release assets, or download instructions should be used.

## Final Local Checks

```bash
git status --short
git diff --stat
rg -n "训练/量化/采集工具已拆分" README.md docs
```

The last command should confirm that README and docs use the split-tool boundary statement consistently.
