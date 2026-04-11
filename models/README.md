# models

This directory is the product-facing anchor for model artifacts and metadata.

Recommended substructure when the workflow matures:

```text
models/
├── checkpoints/
├── exports/
├── calibration/
└── reports/
```

Guidance:

- keep small configs, manifests, and reports in Git
- avoid committing large weights unless explicitly approved
- record which dataset split and training config produced each export
