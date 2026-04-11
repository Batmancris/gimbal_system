import argparse
import os
import sys
import time
from ctypes import POINTER, byref, c_ubyte, cast, memset, sizeof
from pathlib import Path

import cv2
import numpy as np


BASE_DIR = Path(__file__).resolve().parent
DEFAULT_MODEL = BASE_DIR / "runs" / "vehicle_yolov8x_4090" / "weights" / "best.pt"
DEFAULT_MVS_ROOT = Path(r"E:\MVS_Win_STD_4.6.3_260205\MVS\Development")
WINDOW_NAME = "HIK Camera Detection"
CONTROL_WINDOW = "HIK Camera Controls"

os.environ.setdefault("YOLO_CONFIG_DIR", str((BASE_DIR / ".ultralytics").resolve()))
os.environ.setdefault("MVCAM_COMMON_RUNENV", str(DEFAULT_MVS_ROOT))

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
from ultralytics import YOLO


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
            raise RuntimeError(f"Requested device index {self.device_index}, but only {self.device_list.nDeviceNum} devices exist.")

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
            convert_param.pDstBuffer = dst_buffer
            convert_param.nDstBufferSize = dst_len

            ret = self.camera.MV_CC_ConvertPixelTypeEx(convert_param)
            if ret != 0:
                raise RuntimeError(f"ConvertPixelTypeEx failed: {ret_to_hex(ret)}")

            if channels == 1:
                image = np.frombuffer(dst_buffer, dtype=np.uint8, count=dst_len).reshape(height, width)
                image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
            else:
                image = np.frombuffer(dst_buffer, dtype=np.uint8, count=dst_len).reshape(height, width, 3)
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


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Live YOLO detection directly from Hikrobot MVS camera.")
    parser.add_argument("--model", type=Path, default=DEFAULT_MODEL)
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


def main() -> None:
    args = build_argparser().parse_args()
    model_path = args.model.resolve()
    if not model_path.exists():
        raise FileNotFoundError(f"model file does not exist: {model_path}")

    model = YOLO(str(model_path))
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
            results = model.predict(source=frame, imgsz=args.imgsz, conf=conf, iou=iou, verbose=False)
            result = results[0]
            annotated = result.plot()
            if stream.actual_width and stream.actual_height:
                cv2.resizeWindow(WINDOW_NAME, stream.actual_width, stream.actual_height)

            detection_count = len(result.boxes) if result.boxes is not None else 0
            now = time.perf_counter()
            elapsed = max(now - last_render_time, 1e-6)
            last_render_time = now
            fps = 1.0 / elapsed

            exposure_now = stream.get_float("ExposureTime")
            gain_now = stream.get_float("Gain")
            status_text = (
                f"det={detection_count} fps={fps:.1f} conf={conf:.2f} iou={iou:.2f} "
                f"exp={'auto' if auto_exposure else f'{(exposure_now or exposure_us):.0f}us'} "
                f"gain={'auto' if auto_gain else f'{(gain_now or gain):.1f}'}"
            )

            cv2.putText(annotated, status_text, (12, 28), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2, cv2.LINE_AA)
            cv2.putText(
                annotated,
                "Keys: q=quit s=snapshot e=manual exposure a=auto exposure",
                (12, 56),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 255),
                2,
                cv2.LINE_AA,
            )
            cv2.imshow(WINDOW_NAME, annotated)

            if status_text != last_status:
                print(status_text, flush=True)
                last_status = status_text

            key = cv2.waitKey(1) & 0xFF
            if key == ord("q"):
                break
            if key == ord("s"):
                snapshot_path = save_snapshot(annotated)
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
