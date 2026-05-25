from __future__ import annotations
import base64
import json
import subprocess

from .config import DECOMPRESSOR_PATH


def decompress(b64_payload: str) -> bytes:
    """
    Decode a base64 SCHC payload, pass it to the C decompressor binary,
    and return the raw decompressed bytes.

    The C binary reads one line of uppercase hex from stdin and writes a
    single JSON object to stdout:
      success: {"decompressed": "<UPPERCASE_HEX>"}
      failure: {"error": "<reason>"}

    Raises RuntimeError if the binary exits non-zero or returns an error key.
    The caller is responsible for what to do with the decompressed bytes
    (e.g. base64-encode them for forwarding); no payload parsing is done here.
    """
    raw: bytes = base64.b64decode(b64_payload)
    hex_input: str = raw.hex().upper() + "\n"

    result = subprocess.run(
        [DECOMPRESSOR_PATH],
        input=hex_input,
        text=True,
        capture_output=True,
        timeout=10,
    )

    if result.returncode != 0:
        stderr_snippet = result.stderr.strip()[:200]
        raise RuntimeError(
            f"schc-decompressor exited {result.returncode}: {stderr_snippet}"
        )

    stdout = result.stdout.strip()
    if not stdout:
        raise RuntimeError("schc-decompressor produced no output")

    data = json.loads(stdout)

    if "error" in data:
        raise RuntimeError(f"schc-decompressor error: {data['error']}")

    return bytes.fromhex(data["decompressed"])
