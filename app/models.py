from __future__ import annotations
from typing import Optional
from pydantic import BaseModel


class DeviceInfo(BaseModel):
    devEui: str = ""
    deviceName: str = ""

    model_config = {"extra": "allow"}


class ChirpStackUplink(BaseModel):
    """Subset of the ChirpStack v4 HTTP integration uplink event body."""
    deduplicationId: Optional[str] = None
    time: Optional[str] = None
    deviceInfo: Optional[DeviceInfo] = None
    fPort: Optional[int] = None
    fCnt: Optional[int] = None
    data: Optional[str] = None   # base64-encoded SCHC payload

    model_config = {"extra": "allow"}


class ForwardPayload(BaseModel):
    """Body POSTed to the logging application after decompression."""
    data: str                     # base64-encoded decompressed IPv6/UDP/CoAP bytes
    devEui: Optional[str] = None
    fCnt: Optional[int] = None
    time: Optional[str] = None
