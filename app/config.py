"""
Runtime configuration loaded from environment variables.

DECOMPRESSOR_PATH  Path to the built schc-decompressor binary.
LOGGING_APP_URL    URL of the downstream logging application that receives
                   structured sensor readings (HTTP POST, JSON body).
BIND_HOST          Address FastAPI listens on (default 0.0.0.0).
BIND_PORT          Port FastAPI listens on (default 8080).
EXPECTED_FPORT     LoRaWAN FPort that carries SCHC frames (default 10).
"""
import os

DECOMPRESSOR_PATH: str = os.getenv(
    "DECOMPRESSOR_PATH", "./build/src/schc-decompressor"
)
LOGGING_APP_URL: str = os.getenv(
    "LOGGING_APP_URL", "http://localhost:9090/sensor"
)
BIND_HOST: str = os.getenv("BIND_HOST", "0.0.0.0")
BIND_PORT: int = int(os.getenv("BIND_PORT", "8080"))
EXPECTED_FPORT: int = int(os.getenv("EXPECTED_FPORT", "10"))
