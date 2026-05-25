"""
Unit tests for the /uplink FastAPI endpoint.
The C decompressor and the logging-app forward call are both mocked so these
tests run without any compiled binary or external service.

The endpoint's job:
  - receive ChirpStack uplink
  - decompress SCHC → raw bytes
  - forward {data: base64, devEui, fCnt, time} to the logging app
It does NOT parse sensor values (temp/pH/bat).
"""
from __future__ import annotations
import base64
from unittest.mock import AsyncMock, patch

import pytest
from fastapi.testclient import TestClient

from .conftest import b64_schc, build_ipv6_udp_coap


# ---------------------------------------------------------------------------
# Happy path
# ---------------------------------------------------------------------------

def test_uplink_ok(client: TestClient, mock_decompress, mock_forward) -> None:
    body = {
        "fPort": 10,
        "data": b64_schc(),
        "fCnt": 42,
        "time": "2024-01-15T10:30:00Z",
        "deviceInfo": {"devEui": "0102030405060708", "deviceName": "sensor-1"},
    }
    r = client.post("/uplink", json=body)

    assert r.status_code == 200
    resp = r.json()
    assert resp["status"] == "ok"

    forwarded = resp["forwarded"]
    # The forwarded payload carries base64-encoded decompressed bytes — not sensor values
    assert "data" in forwarded
    assert forwarded["devEui"] == "0102030405060708"
    assert forwarded["fCnt"] == 42
    assert forwarded["time"] == "2024-01-15T10:30:00Z"

    # The base64 data must decode back to the bytes that mock_decompress returned
    expected_bytes = build_ipv6_udp_coap()
    assert base64.b64decode(forwarded["data"]) == expected_bytes


def test_uplink_no_device_info(client: TestClient, mock_decompress, mock_forward) -> None:
    body = {"fPort": 10, "data": b64_schc(), "fCnt": 1}
    r = client.post("/uplink", json=body)
    assert r.status_code == 200
    assert r.json()["forwarded"]["devEui"] is None


# ---------------------------------------------------------------------------
# Wrong FPort → ignored, no decompression attempted
# ---------------------------------------------------------------------------

def test_uplink_wrong_fport(client: TestClient) -> None:
    r = client.post("/uplink", json={"fPort": 1, "data": b64_schc()})
    assert r.status_code == 200
    body = r.json()
    assert body["status"] == "ignored"
    assert "fPort" in body["reason"]


def test_uplink_null_fport(client: TestClient) -> None:
    r = client.post("/uplink", json={"fPort": None, "data": b64_schc()})
    assert r.status_code == 200
    assert r.json()["status"] == "ignored"


# ---------------------------------------------------------------------------
# Missing / empty payload → 400
# ---------------------------------------------------------------------------

def test_uplink_missing_data(client: TestClient) -> None:
    r = client.post("/uplink", json={"fPort": 10})
    assert r.status_code == 400


def test_uplink_null_data(client: TestClient) -> None:
    r = client.post("/uplink", json={"fPort": 10, "data": None})
    assert r.status_code == 400


# ---------------------------------------------------------------------------
# Decompressor failure → 422
# ---------------------------------------------------------------------------

def test_uplink_decompress_runtime_error(client: TestClient, mock_forward) -> None:
    with patch("app.main.decompress", side_effect=RuntimeError("bad frame")):
        r = client.post("/uplink", json={"fPort": 10, "data": b64_schc()})
    assert r.status_code == 422
    assert "bad frame" in r.json()["detail"]


def test_uplink_decompress_generic_error(client: TestClient, mock_forward) -> None:
    with patch("app.main.decompress", side_effect=ValueError("json parse error")):
        r = client.post("/uplink", json={"fPort": 10, "data": b64_schc()})
    assert r.status_code == 422


# ---------------------------------------------------------------------------
# Forward failure → 502
# ---------------------------------------------------------------------------

def test_uplink_forward_failure(client: TestClient, mock_decompress) -> None:
    with patch("app.main.forward", new_callable=AsyncMock,
               side_effect=Exception("connection refused")):
        r = client.post("/uplink", json={"fPort": 10, "data": b64_schc()})
    assert r.status_code == 502
    assert "connection refused" in r.json()["detail"]


# ---------------------------------------------------------------------------
# Health check
# ---------------------------------------------------------------------------

def test_health(client: TestClient) -> None:
    r = client.get("/health")
    assert r.status_code == 200
    assert r.json() == {"status": "ok"}
