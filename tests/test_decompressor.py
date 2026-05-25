"""
Integration tests for the schc-decompressor C binary.

These tests are automatically skipped when the binary has not been built.
Build first:

    cd schc-endpoint
    mkdir build && cd build
    conan install .. --build=missing -s build_type=Release
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
    cmake --build .

Then run:

    DECOMPRESSOR_PATH=./build/src/schc-decompressor pytest tests/test_decompressor.py -v

The binary outputs {"decompressed":"<UPPERCASE_HEX>"} on success.
It does NOT parse sensor values — that is the logging app's job.
"""
from __future__ import annotations
import json
import os
import subprocess

import pytest

from .conftest import build_rule150_frame, build_ipv6_udp_coap

BINARY: str = os.getenv("DECOMPRESSOR_PATH", "./build/src/schc-decompressor")


def _binary_available() -> bool:
    return os.path.isfile(BINARY) and os.access(BINARY, os.X_OK)


def _run(schc_bytes: bytes) -> tuple[int, dict | None, str]:
    """Run the binary; return (returncode, parsed_json | None, stderr)."""
    hex_in = schc_bytes.hex().upper() + "\n"
    result = subprocess.run(
        [BINARY],
        input=hex_in,
        text=True,
        capture_output=True,
        timeout=10,
    )
    data = None
    if result.stdout.strip():
        try:
            data = json.loads(result.stdout.strip())
        except json.JSONDecodeError:
            pass
    return result.returncode, data, result.stderr


@pytest.mark.skipif(not _binary_available(),
                    reason=f"binary not found or not executable: {BINARY}")
class TestDecompressorBinary:

    # ------------------------------------------------------------------
    # Rule 150 (no-compression passthrough)
    # ------------------------------------------------------------------

    def test_rule150_returns_decompressed_key(self) -> None:
        """The output JSON must contain 'decompressed', not sensor fields."""
        rc, data, _ = _run(build_rule150_frame())
        assert rc == 0, f"exited {rc}"
        assert data is not None
        assert "decompressed" in data
        # The endpoint must NOT produce sensor fields
        assert "temp" not in data
        assert "pH"   not in data
        assert "bat"  not in data

    def test_rule150_output_is_hex_string(self) -> None:
        rc, data, _ = _run(build_rule150_frame())
        assert rc == 0
        hex_str = data["decompressed"]
        assert isinstance(hex_str, str)
        assert len(hex_str) > 0
        # Must be valid hex
        decoded = bytes.fromhex(hex_str)
        assert len(decoded) > 0

    def test_rule150_passthrough_roundtrip(self) -> None:
        """Rule-150 decompressed output must equal the raw IPv6/UDP/CoAP bytes."""
        raw_ipv6 = build_ipv6_udp_coap(temp=25.0, pH=7.5, bat=80)
        rc, data, _ = _run(bytes([0x96]) + raw_ipv6)
        assert rc == 0
        assert bytes.fromhex(data["decompressed"]) == raw_ipv6

    def test_rule150_different_payload(self) -> None:
        raw_ipv6 = build_ipv6_udp_coap(temp=12.5, pH=8.1, bat=42)
        rc, data, _ = _run(bytes([0x96]) + raw_ipv6)
        assert rc == 0
        assert bytes.fromhex(data["decompressed"]) == raw_ipv6

    # ------------------------------------------------------------------
    # Error paths
    # ------------------------------------------------------------------

    def test_invalid_hex_returns_error(self) -> None:
        result = subprocess.run(
            [BINARY], input="ZZZZ\n", text=True,
            capture_output=True, timeout=10,
        )
        if result.returncode == 0:
            data = json.loads(result.stdout.strip())
            assert "error" in data
        # either exit != 0 OR {"error":...} in stdout is acceptable

    def test_empty_input_returns_error(self) -> None:
        result = subprocess.run(
            [BINARY], input="\n", text=True,
            capture_output=True, timeout=10,
        )
        if result.returncode == 0:
            data = json.loads(result.stdout.strip())
            assert "error" in data

    def test_rule150_too_short_returns_error(self) -> None:
        """Bare rule-id byte with no payload — schc_service_decompress must reject it."""
        rc, data, _ = _run(bytes([0x96]))
        if rc == 0:
            assert data is not None and "error" in data
