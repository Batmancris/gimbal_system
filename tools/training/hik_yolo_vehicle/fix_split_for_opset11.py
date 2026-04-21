import argparse
import sys

import numpy as np
import onnx
from onnx import helper, numpy_helper


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Rewrite Split/Resize/Reshape ops for opset-11 style compatibility."
    )
    parser.add_argument("--input", required=True, help="input ONNX path")
    parser.add_argument("--output", required=True, help="output ONNX path")
    args = parser.parse_args()

    model = onnx.load(args.input)
    initializers = {item.name: numpy_helper.to_array(item) for item in model.graph.initializer}

    split_fixed = 0
    resize_fixed = 0
    reshape_fixed = 0

    for node in model.graph.node:
        if node.op_type != "Split" or len(node.input) != 2:
            continue
        split_input = node.input[1]
        if split_input not in initializers:
            continue
        split_values = [int(v) for v in np.asarray(initializers[split_input]).tolist()]
        kept_attrs = [attr for attr in node.attribute if attr.name != "split"]
        kept_attrs.append(helper.make_attribute("split", split_values))
        data_input = node.input[0]
        del node.input[:]
        node.input.extend([data_input])
        del node.attribute[:]
        node.attribute.extend(kept_attrs)
        split_fixed += 1

    roi_name = "_hb_empty_roi"
    if not any(item.name == roi_name for item in model.graph.initializer):
        model.graph.initializer.append(
            numpy_helper.from_array(np.array([], dtype=np.float32), name=roi_name)
        )

    for node in model.graph.node:
        if node.op_type == "Resize" and len(node.input) >= 2 and node.input[1] == "":
            node.input[1] = roi_name
            resize_fixed += 1
        if node.op_type == "Reshape":
            kept_attrs = [attr for attr in node.attribute if attr.name != "allowzero"]
            if len(kept_attrs) != len(node.attribute):
                del node.attribute[:]
                node.attribute.extend(kept_attrs)
                reshape_fixed += 1

    used_inputs = {name for node in model.graph.node for name in node.input}
    kept_initializers = [item for item in model.graph.initializer if item.name in used_inputs]
    del model.graph.initializer[:]
    model.graph.initializer.extend(kept_initializers)

    onnx.save(model, args.output)
    print(f"Rewrote Split nodes: {split_fixed}")
    print(f"Rewrote Resize nodes: {resize_fixed}")
    print(f"Rewrote Reshape nodes: {reshape_fixed}")
    print(f"Saved fixed model to: {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
