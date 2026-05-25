"""
Shared fixtures and packet-construction helpers.

Packet layout used in tests (Rule 150 — no-compression passthrough):
  [0x96][IPv6 40B][UDP 8B][CoAP header + options + 0xFF + sensor payload]

sensor_data_t is defined with __attribute__((packed)) in the original project:
  float temp (4B) + float pH (4B) + uint8_t bat (1B) = exactly 9 bytes.
"""
from __future__ import annotations
import base64
import struct
from unittest.mock import AsyncMock, patch

import pytest
from fastapi.testclient import TestClient

from app.main import app


# ---------------------------------------------------------------------------
# Packet-construction helpers
# ---------------------------------------------------------------------------

def build_sensor_bytes(temp: float, pH: float, bat: int) -> bytes:
    """
    Build packed sensor_data_t bytes: 4B LE float + 4B LE float + 1B uint8_t = 9 bytes.
    Matches __attribute__((packed)) in sensor_service.h.
    """
    return struct.pack("<ffB", temp, pH, bat)


def build_coap_frame(sensor_payload: bytes) -> bytes:
    """
    CoAP NON POST with Uri-Path "sensor"/"data" and a sensor payload.
      Byte 0: 0x50  (VER=1, TYPE=NON=1, TKL=0)
      Byte 1: 0x02  (POST)
      Bytes 2-3: message ID 0x3031
      Option 11 (Uri-Path) "sensor": 0xB6 + b"sensor"   (delta=11, len=6)
      Option 11 (Uri-Path) "data":   0x04 + b"data"      (delta=0,  len=4)
      0xFF  payload marker
      sensor_payload
    """
    frame  = bytes([0x50, 0x02, 0x30, 0x31])
    frame += bytes([0xB6]) + b"sensor"
    frame += bytes([0x04]) + b"data"
    frame += bytes([0xFF])
    frame += sensor_payload
    return frame


def build_ipv6_udp_coap(temp: float = 25.0,
                         pH: float = 7.5,
                         bat: int = 80) -> bytes:
    """
    Build a complete IPv6/UDP/CoAP packet with a packed 9-byte sensor payload.
    UDP checksum is left as 0x0000 (not validated by the logging app parser;
    the SCHC SDK sets it via CDA_COMPUTE_CHECKSUM when decompressing Rule 28).
    """
    coap    = build_coap_frame(build_sensor_bytes(temp, pH, bat))
    udp_len = 8 + len(coap)

    src_ip = bytes([0x20,0x01,0x0d,0xb8, 0x00,0x00,0x00,0x01,
                    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x01])   # sensor
    dst_ip = bytes([0x20,0x01,0x0d,0xb8, 0x00,0x00,0x00,0x02,
                    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x02])   # sink

    ipv6  = bytes([0x60, 0x00, 0x00, 0x00])   # ver=6, TC=0, FL=0
    ipv6 += struct.pack(">H", udp_len)         # payload length = UDP header + CoAP
    ipv6 += bytes([0x11, 0xFF])                # next_header=UDP, hop_limit=255
    ipv6 += src_ip + dst_ip

    udp  = struct.pack(">HH", 0x1234, 0x5678)  # src_port=0x1234, dst_port=0x5678
    udp += struct.pack(">HH", udp_len, 0)      # length, checksum=0

    return ipv6 + udp + coap


def build_rule150_frame(**kwargs) -> bytes:
    """Rule-150 SCHC frame: [0x96][raw IPv6/UDP/CoAP bytes]."""
    return bytes([0x96]) + build_ipv6_udp_coap(**kwargs)


def b64_schc(**kwargs) -> str:
    """Base64-encoded Rule-150 SCHC frame for use as ChirpStack 'data' field."""
    return base64.b64encode(build_rule150_frame(**kwargs)).decode()


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture
def client() -> TestClient:
    return TestClient(app)


@pytest.fixture
def mock_decompress():
    """
    Replace the real C decompressor with a fixed decompressed IPv6/UDP/CoAP
    byte string.  Returns bytes, matching the contract of decompressor.decompress().
    """
    with patch(
        "app.main.decompress",
        return_value=build_ipv6_udp_coap(),
    ) as m:
        yield m


@pytest.fixture
def mock_forward():
    """Silently absorb forward calls."""
    with patch("app.main.forward", new_callable=AsyncMock) as m:
        yield m
