import argparse
import os
import sys
import time
from ctypes import POINTER, byref, c_ubyte, cast, memset, sizeof
from pathlib import Path
from typing import Any

import cv2
import numpy as np
import onnxruntime as ort


BASE_DIR = Path(__file__).resolve().parent
DEFAULT_PT_MODEL = Path(r"E:\research\1\yolo\hiki training\model_training\runs\bear_yolov8n_x5_640\weights\best.pt")
DEFAULT_ONNX_MODEL = Path(r"E:\research\1\yolo\hiki training\model_training\runs\bear_yolov8n_x5_640\weights\best.onnx")
DEFAULT_CLASS_NAME = "bear"
DEFAULT_HIK_SITE_PACKAGES = Path(r"E:\Anaconda\envs\hik_yolov8\Lib\site-packages")
DEFAULT_MVS_ROOT = Path(r"E:\MVS_Win_STD_4.6.3_260205\MVS\Development")
WINDOW_NAME = "HIK Camera Detection"
CONTROL_WINDOW = "HIK Camera Controls"

try:
    import torch as _local_torch  # type: ignore
except Exception:
    _local_torch = None


def bootstrap_external_site_packages() -> None:
    site_packages_path = Path(os.environ.get("HIK_YOLO_SITE_PACKAGES", str(DEFAULT_HIK_SITE_PACKAGES)))
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
        insert_at = 1 if _local_torch is not None else 0
        sys.path.insert(insert_at, str(site_packages_path))


bootstrap_external_site_packages()

os.environ.setdefault("YOLO_CONFIG_DIR", str((BASE_DIR / ".ultralytics").resolve()))
os.environ.setdefault("MVCAM_COMMON_RUNENV", str(DEFAULT_MVS_ROOT))

import torch
from ultralytics import YOLO

mv_import_dir = Path(os.environ["MVCAM_COMMON_RUNENV"]) / "Samples" / "Python" / "MvImport"
if str(mv_import_dir) not in sys.path:
    sys.path.append(str(mv_import_dir))

for dll_dir in (
    Path(os.environ["MVCAM_COMMON_RUNENV"]),
    Path(os.environ["MVCAM_COMMON_RUNENV"]) / "bin",
    Path(os.environ["MVCAM_COMMON_RUNENV"]) / "Win64_x64",
    Path(os.environ["MVCAM_COMMON_RUNENV"]) / "Runtime" / "Win64_x64",
):
    if hasattr(os, "add_dll_directory") and dll_dir.exists():
        os.add_dll_directory(str(dll_dir))

from CameraParams_header import MV_GENTL_CAMERALINK_DEVICE, MV_GENTL_CXP_DEVICE, MV_GENTL_GIGE_DEVICE
from CameraParams_header import MV_GENTL_XOF_DEVICE, MV_GIGE_DEVICE, MV_TRIGGER_MODE_OFF, MV_USB_DEVICE
from MvCameraControl_class import (
    MV_CC_DEVICE_INFO,
    MV_CC_DEVICE_INFO_LIST,
    MV_CC_PIXEL_CONVERT_PARAM_EX,
    MV_FRAME_OUT,
    MV_FRAME_OUT_INFO_EX,
    MVCC_FLOATVALUE,
    MvCamera,
    PixelType_Gvsp_Mono8,
    PixelType_Gvsp_RGB8_Packed,
)


def noop(_: int) -> None:
    return


def ret_to_hex(ret: int) -> str:
    return hex(ret & 0xFFFFFFFF)


def device_to_text(device_info: MV_CC_DEVICE_INFO, index: int) -> str:
    if device_info.nTLayerType in (MV_GIGE_DEVICE, MV_GENTL_GIGE_DEVICE):
        model = "".join(chr(c) for c in device_info.SpecialInfo.stGigEInfo.chModelName if c != 0)
        ip = device_info.SpecialInfo.stGigEInfo.nCurrentIp
        ip_text = ".".join(str((ip >> shift) & 0xFF) for shift in (24, 16, 8, 0))
        return f"[{index}] GigE {model} ip={ip_text}"
    if device_info.nTLayerType == MV_USB_DEVICE:
        model = "".join(chr(c) for c in device_info.SpecialInfo.stUsb3VInfo.chModelName if c != 0)
        sn = "".join(chr(c) for c in device_info.SpecialInfo.stUsb3VInfo.chSerialNumber if c != 0)
        return f"[{index}] USB {model} sn={sn}"
    if device_info.nTLayerType == MV_GENTL_CAMERALINK_DEVICE:
        model = "".join(chr(c) for c in device_info.SpecialInfo.stCMLInfo.chModelName if c != 0)
        return f"[{index}] CameraLink {model}"
    if device_info.nTLayerType == MV_GENTL_CXP_DEVICE:
        model = "".join(chr(c) for c in device_info.SpecialInfo.stCXPInfo.chModelName if c != 0)
        return f"[{index}] CXP {model}"
    if device_info.nTLayerType == MV_GENTL_XOF_DEVICE:
        model = "".join(chr(c) for c in device_info.SpecialInfo.stXoFInfo.chModelName if c != 0)
        return f"[{index}] XoF {model}"
    return f"[{index}] Unknown transport type={device_info.nTLayerType}"


def enumerate_devices() -> tuple[MV_CC_DEVICE_INFO_LIST, list[str]]:
    device_list = MV_CC_DEVICE_INFO_LIST()
    tlayer = MV_GIGE_DEVICE | MV_USB_DEVICE | MV_GENTL_CAMERALINK_DEVICE | MV_GENTL_CXP_DEVICE | MV_GENTL_XOF_DEVICE
    ret = MvCamera.MV_CC_EnumDevices(tlayer, device_list)
    if ret != 0:
        raise RuntimeError(f"EnumDevices failed: {ret_to_hex(ret)}")
    if device_list.nDeviceNum == 0:
        raise RuntimeError("No Hikrobot / MVS camera was found.")

    descriptions: list[str] = []
    for idx in range(device_list.nDeviceNum):
        info = cast(device_list.pDeviceInfo[idx], POINTER(MV_CC_DEVICE_INFO)).contents
        descriptions.append(device_to_text(info, idx))
    return device_list, descriptions


def is_mono_pixel(pixel_type: int) -> bool:
    mono_types = {
        0x01080001,
        0x01100003,
        0x010C0004,
        0x01100005,
        0x010C0006,
        0x01100007,
    }
    return pixel_type in mono_types


class HikCameraStream:
    def __init__(self, device_index: int, width: int, height: int) -> None:
        self.device_index = device_index
        self.width = width
        self.height = height
        self.device_list: MV_CC_DEVICE_INFO_LIST | None = None
        self.camera: MvCamera | None = None
        self.frame_info = MV_FRAME_OUT_INFO_EX()
        self.actual_width: int | None = None
        self.actual_height: int | None = None
        memset(byref(self.frame_info), 0, sizeof(MV_FRAME_OUT_INFO_EX))

    def open(self) -> None:
        MvCamera.MV_CC_Initialize()
        self.device_list, descriptions = enumerate_devices()
        for item in descriptions:
            print(item, flush=True)

        if self.device_index >= self.device_list.nDeviceNum:
            raise RuntimeError(
                f"Requested device index {self.device_index}, but only {self.device_list.nDeviceNum} devices exist."
            )

        self.camera = MvCamera()
        device_info = cast(self.device_list.pDeviceInfo[self.device_index], POINTER(MV_CC_DEVICE_INFO)).contents

        ret = self.camera.MV_CC_CreateHandle(device_info)
        if ret != 0:
            raise RuntimeError(f"CreateHandle failed: {ret_to_hex(ret)}")

        ret = self.camera.MV_CC_OpenDevice()
        if ret != 0:
            raise RuntimeError(f"OpenDevice failed: {ret_to_hex(ret)}")

        if device_info.nTLayerType in (MV_GIGE_DEVICE, MV_GENTL_GIGE_DEVICE):
            packet_size = self.camera.MV_CC_GetOptimalPacketSize()
            if int(packet_size) > 0:
                self.camera.MV_CC_SetIntValue("GevSCPSPacketSize", packet_size)

        self.camera.MV_CC_SetEnumValue("TriggerMode", MV_TRIGGER_MODE_OFF)
        self.camera.MV_CC_SetIntValue("Width", self.width)
        self.camera.MV_CC_SetIntValue("Height", self.height)

        ret = self.camera.MV_CC_StartGrabbing()
        if ret != 0:
            raise RuntimeError(f"StartGrabbing failed: {ret_to_hex(ret)}")

    def close(self) -> None:
        if self.camera is not None:
            self.camera.MV_CC_StopGrabbing()
            self.camera.MV_CC_CloseDevice()
            self.camera.MV_CC_DestroyHandle()
            self.camera = None
        MvCamera.MV_CC_Finalize()

    def set_exposure(self, exposure_time: float, auto_exposure: bool) -> None:
        if self.camera is None:
            return
        self.camera.MV_CC_SetEnumValue("ExposureAuto", 2 if auto_exposure else 0)
        if not auto_exposure:
            self.camera.MV_CC_SetFloatValue("ExposureTime", float(exposure_time))

    def set_gain(self, gain: float, auto_gain: bool) -> None:
        if self.camera is None:
            return
        self.camera.MV_CC_SetEnumValue("GainAuto", 2 if auto_gain else 0)
        if not auto_gain:
            self.camera.MV_CC_SetFloatValue("Gain", float(gain))

    def get_float(self, key: str) -> float | None:
        if self.camera is None:
            return None
        value = MVCC_FLOATVALUE()
        memset(byref(value), 0, sizeof(MVCC_FLOATVALUE))
        ret = self.camera.MV_CC_GetFloatValue(key, value)
        if ret != 0:
            return None
        return float(value.fCurValue)

    def read(self, timeout_ms: int = 1000) -> np.ndarray:
        if self.camera is None:
            raise RuntimeError("Camera is not open.")

        frame = MV_FRAME_OUT()
        memset(byref(frame), 0, sizeof(MV_FRAME_OUT))

        ret = self.camera.MV_CC_GetImageBuffer(frame, timeout_ms)
        if ret != 0 or frame.pBufAddr is None:
            raise RuntimeError(f"GetImageBuffer failed: {ret_to_hex(ret)}")

        try:
            src_pixel_type = frame.stFrameInfo.enPixelType
            width = frame.stFrameInfo.nWidth
            height = frame.stFrameInfo.nHeight
            self.actual_width = int(width)
            self.actual_height = int(height)

            convert_param = MV_CC_PIXEL_CONVERT_PARAM_EX()
            memset(byref(convert_param), 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM_EX))
            convert_param.nWidth = width
            convert_param.nHeight = height
            convert_param.pSrcData = frame.pBufAddr
            convert_param.nSrcDataLen = frame.stFrameInfo.nFrameLen
            convert_param.enSrcPixelType = src_pixel_type

            if is_mono_pixel(src_pixel_type):
                dst_pixel_type = PixelType_Gvsp_Mono8
                channels = 1
            else:
                dst_pixel_type = PixelType_Gvsp_RGB8_Packed
                channels = 3

            dst_len = width * height * channels
            dst_buffer = (c_ubyte * dst_len)()
            convert_param.enDstPixelType = dst_pixel_type
            convert_param.pDstBuffer = cast(dst_buffer, POINTER(c_ubyte))
            convert_param.nDstBufferSize = dst_len

            ret = self.camera.MV_CC_ConvertPixelTypeEx(convert_param)
            if ret != 0:
                raise RuntimeError(f"ConvertPixelTypeEx failed: {ret_to_hex(ret)}")

            image = np.frombuffer(dst_buffer, dtype=np.uint8)
            if channels == 1:
                image = image.reshape(height, width)
                image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
            else:
                image = image.reshape(height, width, 3)
                image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
            return image.copy()
        finally:
            self.camera.MV_CC_FreeImageBuffer(frame)


def create_control_panel(defaults: dict[str, int]) -> None:
    cv2.namedWindow(CONTROL_WINDOW, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(CONTROL_WINDOW, 560, 320)
    cv2.createTrackbar("Conf x100", CONTROL_WINDOW, defaults["conf"], 100, noop)
    cv2.createTrackbar("IoU x100", CONTROL_WINDOW, defaults["iou"], 100, noop)
    cv2.createTrackbar("Auto Exposure", CONTROL_WINDOW, defaults["auto_exposure"], 1, noop)
    cv2.createTrackbar("Exposure us x100", CONTROL_WINDOW, defaults["exposure_us_x100"], 3000, noop)
    cv2.createTrackbar("Auto Gain", CONTROL_WINDOW, defaults["auto_gain"], 1, noop)
    cv2.createTrackbar("Gain x10", CONTROL_WINDOW, defaults["gain_x10"], 300, noop)


def save_snapshot(frame: np.ndarray) -> Path:
    snapshots_dir = BASE_DIR / "runs" / "snapshots"
    snapshots_dir.mkdir(parents=True, exist_ok=True)
    output_path = snapshots_dir / f"hik_snapshot_{time.strftime('%Y%m%d_%H%M%S')}.jpg"
    cv2.imwrite(str(output_path), frame)
    return output_path


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


def preprocess(frame: np.ndarray, imgsz: int) -> tuple[np.ndarray, float, tuple[float, float]]:
    resized, ratio, pad = letterbox(frame, imgsz)
    rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
    tensor = rgb.transpose(2, 0, 1).astype(np.float32) / 255.0
    tensor = np.expand_dims(tensor, axis=0)
    return tensor, ratio, pad


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
            pass
    return arrays


def select_detection_tensor(arrays: list[np.ndarray]) -> np.ndarray:
    candidates: list[np.ndarray] = []
    for array in arrays:
        if getattr(array, "ndim", 0) < 3:
            continue
        if array.shape[0] != 1:
            continue
        candidates.append(array)

    if not candidates:
        raise RuntimeError("no candidate detection tensor found")

    def score(array: np.ndarray) -> tuple[int, int]:
        dims = list(array.shape)
        return (1 if 5 in dims else 0, int(np.prod(dims)))

    return max(candidates, key=score)


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
    raise RuntimeError(f"cannot canonicalize tensor shape: {array.shape}")


def decode_predictions(
    output: np.ndarray,
    original_shape: tuple[int, int],
    ratio: float,
    pad: tuple[float, float],
    conf_thres: float,
    iou_thres: float,
) -> tuple[list[tuple[int, int, int, int, float]], float]:
    predictions = np.asarray(output, dtype=np.float32)
    predictions = np.squeeze(predictions)
    if predictions.ndim == 3 and predictions.shape[-1] == 1:
        predictions = np.squeeze(predictions, axis=-1)
    if predictions.shape[0] != 5 and predictions.shape[1] == 5:
        predictions = predictions.T
    if predictions.shape[0] != 5:
        raise RuntimeError(f"unexpected output shape: {output.shape}")

    scores = predictions[4]
    top1_score = float(np.max(scores)) if scores.size else 0.0
    keep = scores >= conf_thres
    if not np.any(keep):
        return [], top1_score

    boxes = predictions[:4, keep].T
    scores = scores[keep]
    dw, dh = pad
    boxes_xyxy: list[list[int]] = []
    boxes_for_nms: list[list[int]] = []
    kept_scores: list[float] = []

    for (x_center, y_center, width, height), score in zip(boxes, scores):
        x1 = (x_center - width / 2 - dw) / ratio
        y1 = (y_center - height / 2 - dh) / ratio
        x2 = (x_center + width / 2 - dw) / ratio
        y2 = (y_center + height / 2 - dh) / ratio

        x1 = int(max(0, min(original_shape[1] - 1, round(x1))))
        y1 = int(max(0, min(original_shape[0] - 1, round(y1))))
        x2 = int(max(0, min(original_shape[1] - 1, round(x2))))
        y2 = int(max(0, min(original_shape[0] - 1, round(y2))))

        if x2 <= x1 or y2 <= y1:
            continue

        boxes_xyxy.append([x1, y1, x2, y2])
        boxes_for_nms.append([x1, y1, x2 - x1, y2 - y1])
        kept_scores.append(float(score))

    if not boxes_for_nms:
        return [], top1_score

    indices = cv2.dnn.NMSBoxes(boxes_for_nms, kept_scores, conf_thres, iou_thres)
    if len(indices) == 0:
        return [], top1_score

    detections: list[tuple[int, int, int, int, float]] = []
    for raw_idx in indices:
        idx = int(raw_idx[0] if isinstance(raw_idx, (list, tuple, np.ndarray)) else raw_idx)
        x1, y1, x2, y2 = boxes_xyxy[idx]
        detections.append((x1, y1, x2, y2, kept_scores[idx]))
    return detections, top1_score


def extract_top_anchor_debug(
    output: np.ndarray,
    original_shape: tuple[int, int],
    ratio: float,
    pad: tuple[float, float],
) -> dict[str, Any]:
    predictions = np.asarray(output, dtype=np.float32)
    predictions = np.squeeze(predictions)
    if predictions.ndim == 3 and predictions.shape[-1] == 1:
        predictions = np.squeeze(predictions, axis=-1)
    if predictions.shape[0] != 5 and predictions.shape[1] == 5:
        predictions = predictions.T
    if predictions.shape[0] != 5:
        raise RuntimeError(f"unexpected output shape: {output.shape}")

    scores = predictions[4]
    if scores.size == 0:
        return {
            "anchor": -1,
            "score": 0.0,
            "raw": [0.0, 0.0, 0.0, 0.0],
            "decoded_cxcywh_xyxy": None,
            "direct_xyxy": None,
        }

    anchor = int(np.argmax(scores))
    raw0 = float(predictions[0, anchor])
    raw1 = float(predictions[1, anchor])
    raw2 = float(predictions[2, anchor])
    raw3 = float(predictions[3, anchor])
    score = float(scores[anchor])

    dw, dh = pad
    x1 = (raw0 - raw2 / 2 - dw) / ratio
    y1 = (raw1 - raw3 / 2 - dh) / ratio
    x2 = (raw0 + raw2 / 2 - dw) / ratio
    y2 = (raw1 + raw3 / 2 - dh) / ratio

    x1 = int(max(0, min(original_shape[1] - 1, round(x1))))
    y1 = int(max(0, min(original_shape[0] - 1, round(y1))))
    x2 = int(max(0, min(original_shape[1] - 1, round(x2))))
    y2 = int(max(0, min(original_shape[0] - 1, round(y2))))

    decoded_cxcywh_xyxy = None
    if x2 > x1 and y2 > y1:
        decoded_cxcywh_xyxy = (x1, y1, x2, y2)

    direct_x1 = (raw0 - dw) / ratio
    direct_y1 = (raw1 - dh) / ratio
    direct_x2 = (raw2 - dw) / ratio
    direct_y2 = (raw3 - dh) / ratio

    direct_x1 = int(max(0, min(original_shape[1] - 1, round(direct_x1))))
    direct_y1 = int(max(0, min(original_shape[0] - 1, round(direct_y1))))
    direct_x2 = int(max(0, min(original_shape[1] - 1, round(direct_x2))))
    direct_y2 = int(max(0, min(original_shape[0] - 1, round(direct_y2))))

    direct_xyxy = None
    if direct_x2 > direct_x1 and direct_y2 > direct_y1:
        direct_xyxy = (direct_x1, direct_y1, direct_x2, direct_y2)

    return {
        "anchor": anchor,
        "score": score,
        "raw": [raw0, raw1, raw2, raw3],
        "decoded_cxcywh_xyxy": decoded_cxcywh_xyxy,
        "direct_xyxy": direct_xyxy,
    }


def draw_raw_debug(
    canvas: np.ndarray,
    debug_info: dict[str, Any],
    title: str,
    y_start: int,
) -> None:
    raw0, raw1, raw2, raw3 = debug_info["raw"]
    anchor = debug_info["anchor"]
    score = debug_info["score"]
    decoded_cxcywh_xyxy = debug_info["decoded_cxcywh_xyxy"]
    direct_xyxy = debug_info["direct_xyxy"]

    lines = [
        f"{title.lower()} top1 anchor={anchor} score={score:.4f}",
        f"raw0={raw0:.4f} raw1={raw1:.4f} raw2={raw2:.4f} raw3={raw3:.4f}",
    ]
    if decoded_cxcywh_xyxy is None:
        lines.append("cxcywh->xyxy: invalid")
    else:
        x1, y1, x2, y2 = decoded_cxcywh_xyxy
        lines.append(f"cxcywh->xyxy=({x1},{y1},{x2},{y2})")
        cv2.rectangle(canvas, (x1, y1), (x2, y2), (0, 120, 255), 1)
    if direct_xyxy is None:
        lines.append("direct xyxy: invalid")
    else:
        x1, y1, x2, y2 = direct_xyxy
        lines.append(f"direct xyxy=({x1},{y1},{x2},{y2})")
        cv2.rectangle(canvas, (x1, y1), (x2, y2), (255, 80, 80), 1)

    box_height = 34 * len(lines) + 16
    cv2.rectangle(canvas, (10, y_start - 22), (min(canvas.shape[1] - 10, 980), y_start - 22 + box_height), (20, 20, 20), -1)
    cv2.rectangle(canvas, (10, y_start - 22), (min(canvas.shape[1] - 10, 980), y_start - 22 + box_height), (0, 180, 255), 1)
    for idx, line in enumerate(lines):
        cv2.putText(
            canvas,
            line,
            (20, y_start + idx * 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.72,
            (255, 255, 255),
            2,
            cv2.LINE_AA,
        )


def draw_detections(
    frame: np.ndarray,
    detections: list[tuple[int, int, int, int, float]],
    fps: float,
    conf_thres: float,
    iou_thres: float,
    top1_score: float,
    class_name: str,
    title: str,
    status_suffix: str,
) -> np.ndarray:
    canvas = frame.copy()
    for x1, y1, x2, y2, score in detections:
        cv2.rectangle(canvas, (x1, y1), (x2, y2), (80, 220, 120), 2)
        cv2.putText(
            canvas,
            f"{class_name} {score:.2f}",
            (x1, max(20, y1 - 10)),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (80, 220, 120),
            2,
            cv2.LINE_AA,
        )

    cv2.putText(canvas, title, (16, 28), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 180, 0), 2, cv2.LINE_AA)
    cv2.putText(
        canvas,
        f"det={len(detections)} fps={fps:.1f} top1={top1_score:.3f} conf={conf_thres:.2f} iou={iou_thres:.2f}",
        (16, 58),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.65,
        (0, 200, 255),
        2,
        cv2.LINE_AA,
    )
    cv2.putText(canvas, status_suffix, (16, 88), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 255, 255), 2, cv2.LINE_AA)
    return canvas


class PtDetector:
    def __init__(self, model_path: Path, device: str) -> None:
        self.model = YOLO(str(model_path))
        self.model.model.eval()
        self.device = device

    def infer(self, image_tensor: np.ndarray) -> np.ndarray:
        inputs = torch.from_numpy(image_tensor).to(self.device)
        with torch.no_grad():
            outputs = self.model.model(inputs)
        arrays = flatten_outputs(outputs)
        return canonicalize_detection_tensor(select_detection_tensor(arrays))


class OnnxDetector:
    def __init__(self, model_path: Path, use_fixed_fallback: bool = False) -> None:
        resolved_model = model_path.resolve()
        candidates = [resolved_model]
        if use_fixed_fallback and resolved_model.name == "best.onnx":
            candidates.append(resolved_model.with_name("best_fixed.onnx"))

        last_error: Exception | None = None
        self.model_path: Path | None = None
        self.session = None
        for candidate in candidates:
            if not candidate.exists():
                continue
            try:
                self.session = ort.InferenceSession(str(candidate), providers=["CPUExecutionProvider"])
                self.model_path = candidate
                break
            except Exception as exc:
                last_error = exc

        if self.session is None:
            if last_error is not None:
                raise RuntimeError(
                    f"failed to load ONNX model from candidates: {', '.join(str(p) for p in candidates)}"
                ) from last_error
            raise FileNotFoundError(
                f"onnx model does not exist in candidates: {', '.join(str(p) for p in candidates)}"
            )

        self.input_name = self.session.get_inputs()[0].name

    def infer(self, image_tensor: np.ndarray) -> np.ndarray:
        outputs = self.session.run(None, {self.input_name: image_tensor})
        arrays = flatten_outputs(outputs)
        return canonicalize_detection_tensor(select_detection_tensor(arrays))


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Live Hikrobot camera detection with PT, ONNX, or side-by-side compare.")
    parser.add_argument("--backend", choices=["pt", "onnx", "compare"], default="pt")
    parser.add_argument("--model", type=Path, default=None, help="single-backend model path")
    parser.add_argument("--pt-model", type=Path, default=DEFAULT_PT_MODEL)
    parser.add_argument("--onnx-model", type=Path, default=DEFAULT_ONNX_MODEL)
    parser.add_argument("--class-name", type=str, default=DEFAULT_CLASS_NAME, help="label drawn on detection boxes")
    parser.add_argument(
        "--onnx-fallback-fixed",
        action="store_true",
        help="when using best.onnx, also try best_fixed.onnx if loading the un-fixed model fails",
    )
    parser.add_argument("--device", type=str, default="cpu")
    parser.add_argument("--device-index", type=int, default=0)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--width", type=int, default=1280)
    parser.add_argument("--height", type=int, default=1024)
    parser.add_argument("--conf", type=float, default=0.25)
    parser.add_argument("--iou", type=float, default=0.70)
    parser.add_argument("--auto-exposure", action="store_true")
    parser.add_argument("--exposure-us", type=float, default=12000.0)
    parser.add_argument("--auto-gain", action="store_true")
    parser.add_argument("--gain", type=float, default=10.0)
    return parser


def build_status_suffix(stream: HikCameraStream, auto_exposure: bool, exposure_us: float, auto_gain: bool, gain: float) -> str:
    exposure_now = stream.get_float("ExposureTime")
    gain_now = stream.get_float("Gain")
    return (
        f"exp={'auto' if auto_exposure else f'{(exposure_now or exposure_us):.0f}us'} "
        f"gain={'auto' if auto_gain else f'{(gain_now or gain):.1f}'}"
    )


def render_single(
    title: str,
    frame: np.ndarray,
    raw_output: np.ndarray,
    ratio: float,
    pad: tuple[float, float],
    conf_thres: float,
    iou_thres: float,
    class_name: str,
    fps: float,
    status_suffix: str,
) -> tuple[np.ndarray, str]:
    detections, top1_score = decode_predictions(raw_output, frame.shape[:2], ratio, pad, conf_thres, iou_thres)
    canvas = draw_detections(frame, detections, fps, conf_thres, iou_thres, top1_score, class_name, title, status_suffix)
    debug_info = extract_top_anchor_debug(raw_output, frame.shape[:2], ratio, pad)
    draw_raw_debug(canvas, debug_info, title, 126)
    status_text = f"{title.lower()} det={len(detections)} top1={top1_score:.3f} {status_suffix}"
    return canvas, status_text


def render_compare(
    frame: np.ndarray,
    pt_output: np.ndarray,
    onnx_output: np.ndarray,
    ratio: float,
    pad: tuple[float, float],
    conf_thres: float,
    iou_thres: float,
    class_name: str,
    fps: float,
    status_suffix: str,
) -> tuple[np.ndarray, str]:
    pt_detections, pt_top1 = decode_predictions(pt_output, frame.shape[:2], ratio, pad, conf_thres, iou_thres)
    onnx_detections, onnx_top1 = decode_predictions(onnx_output, frame.shape[:2], ratio, pad, conf_thres, iou_thres)
    left = draw_detections(frame, pt_detections, fps, conf_thres, iou_thres, pt_top1, class_name, "PT", status_suffix)
    right = draw_detections(frame, onnx_detections, fps, conf_thres, iou_thres, onnx_top1, class_name, "ONNX", status_suffix)
    draw_raw_debug(left, extract_top_anchor_debug(pt_output, frame.shape[:2], ratio, pad), "PT", 126)
    draw_raw_debug(right, extract_top_anchor_debug(onnx_output, frame.shape[:2], ratio, pad), "ONNX", 126)
    abs_diff = np.abs(pt_output - onnx_output)
    diff_mean = float(np.mean(abs_diff))
    diff_max = float(np.max(abs_diff))
    canvas = np.hstack([left, right])
    cv2.putText(
        canvas,
        f"pt_det={len(pt_detections)} onnx_det={len(onnx_detections)} abs_diff_mean={diff_mean:.6f} abs_diff_max={diff_max:.6f}",
        (16, canvas.shape[0] - 18),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.65,
        (255, 200, 0),
        2,
        cv2.LINE_AA,
    )
    status_text = (
        f"compare pt_det={len(pt_detections)} pt_top1={pt_top1:.3f} "
        f"onnx_det={len(onnx_detections)} onnx_top1={onnx_top1:.3f} "
        f"diff_mean={diff_mean:.6f} diff_max={diff_max:.6f} {status_suffix}"
    )
    return canvas, status_text


def main() -> None:
    args = build_argparser().parse_args()

    backend = args.backend
    pt_path = (args.model if backend == "pt" and args.model is not None else args.pt_model).resolve()
    onnx_path = (args.model if backend == "onnx" and args.model is not None else args.onnx_model).resolve()

    if backend in {"pt", "compare"} and not pt_path.exists():
        raise FileNotFoundError(f"pt model does not exist: {pt_path}")
    if backend in {"onnx", "compare"} and not onnx_path.exists():
        raise FileNotFoundError(f"onnx model does not exist: {onnx_path}")

    pt_detector = PtDetector(pt_path, args.device) if backend in {"pt", "compare"} else None
    onnx_detector = OnnxDetector(
        onnx_path,
        use_fixed_fallback=args.onnx_fallback_fixed or onnx_path.name == "best.onnx",
    ) if backend in {"onnx", "compare"} else None

    stream = HikCameraStream(device_index=args.device_index, width=args.width, height=args.height)
    stream.open()
    stream.set_exposure(args.exposure_us, args.auto_exposure)
    stream.set_gain(args.gain, args.auto_gain)

    create_control_panel(
        {
            "conf": int(args.conf * 100),
            "iou": int(args.iou * 100),
            "auto_exposure": 1 if args.auto_exposure else 0,
            "exposure_us_x100": max(1, min(int(args.exposure_us / 100.0), 3000)),
            "auto_gain": 1 if args.auto_gain else 0,
            "gain_x10": max(0, min(int(args.gain * 10.0), 300)),
        }
    )

    cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
    if backend == "compare":
        cv2.resizeWindow(WINDOW_NAME, 1600, 800)

    last_render_time = time.perf_counter()
    last_status = ""

    try:
        while True:
            conf = max(cv2.getTrackbarPos("Conf x100", CONTROL_WINDOW), 1) / 100.0
            iou = max(cv2.getTrackbarPos("IoU x100", CONTROL_WINDOW), 1) / 100.0
            auto_exposure = cv2.getTrackbarPos("Auto Exposure", CONTROL_WINDOW) == 1
            exposure_us = max(cv2.getTrackbarPos("Exposure us x100", CONTROL_WINDOW), 1) * 100.0
            auto_gain = cv2.getTrackbarPos("Auto Gain", CONTROL_WINDOW) == 1
            gain = cv2.getTrackbarPos("Gain x10", CONTROL_WINDOW) / 10.0

            stream.set_exposure(exposure_us, auto_exposure)
            stream.set_gain(gain, auto_gain)

            frame = stream.read()
            tensor, ratio, pad = preprocess(frame, args.imgsz)

            now = time.perf_counter()
            elapsed = max(now - last_render_time, 1e-6)
            last_render_time = now
            fps = 1.0 / elapsed
            status_suffix = build_status_suffix(stream, auto_exposure, exposure_us, auto_gain, gain)

            if backend == "pt":
                assert pt_detector is not None
                canvas, status_text = render_single(
                    "PT", frame, pt_detector.infer(tensor), ratio, pad, conf, iou, args.class_name, fps, status_suffix
                )
            elif backend == "onnx":
                assert onnx_detector is not None
                canvas, status_text = render_single(
                    "ONNX", frame, onnx_detector.infer(tensor), ratio, pad, conf, iou, args.class_name, fps, status_suffix
                )
            else:
                assert pt_detector is not None and onnx_detector is not None
                canvas, status_text = render_compare(
                    frame,
                    pt_detector.infer(tensor),
                    onnx_detector.infer(tensor),
                    ratio,
                    pad,
                    conf,
                    iou,
                    args.class_name,
                    fps,
                    status_suffix,
                )

            if stream.actual_width and stream.actual_height and backend != "compare":
                cv2.resizeWindow(WINDOW_NAME, stream.actual_width, stream.actual_height)

            cv2.putText(
                canvas,
                "Keys: q=quit s=snapshot e=manual exposure a=auto exposure",
                (12, 116 if backend != "compare" else 146),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 255),
                2,
                cv2.LINE_AA,
            )
            cv2.imshow(WINDOW_NAME, canvas)

            if status_text != last_status:
                print(status_text, flush=True)
                last_status = status_text

            key = cv2.waitKey(1) & 0xFF
            if key == ord("q"):
                break
            if key == ord("s"):
                snapshot_path = save_snapshot(canvas)
                print(f"snapshot saved: {snapshot_path}", flush=True)
            if key == ord("e"):
                cv2.setTrackbarPos("Auto Exposure", CONTROL_WINDOW, 0)
            if key == ord("a"):
                cv2.setTrackbarPos("Auto Exposure", CONTROL_WINDOW, 1)
    finally:
        stream.close()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
