# schc-endpoint

HTTP endpoint that sits between a TTN LoRaWAN server and a separate logging application.
It receives uplink frames, decompresses SCHC-compressed payloads back to raw IPv6/UDP/CoAP bytes,
and forwards those bytes (plus LoRaWAN metadata) to the logging app.

**The endpoint does not parse sensor values.**
Temperature, pH, battery level extraction is the logging application's responsibility.

---

## Architecture

```
Underwater sensor node
  |  (acoustic modem — AHOI protocol)
  v
sink-lora-forwarder        [C app on the gateway Raspberry Pi]
  |  forwards raw SCHC bytes over LoRaWAN (FPort 10)
  v
LoRaWAN network (RAK11720 / LoRaWAN server)
  v
TTN                 [LoRaWAN Network Server]
  |  HTTP integration — POST /uplink  (JSON, base64-encoded SCHC payload)
  v
schc-endpoint              [THIS PROJECT — Python FastAPI]
  |  calls schc-decompressor (C binary, same rules as sink-demo-app)
  |  forwards: { "data": "<base64 decompressed IPv6/UDP/CoAP>",
  |              "devEui": "...", "fCnt": N, "time": "..." }
  v
logging-app                [separate project — parses IPv6/UDP/CoAP, stores sensor data]
```

The SCHC rules are identical to those in `sink-demo-app` (RX/sink perspective) of the last semester:
- **Rule 28** — full IPv6 + UDP + CoAP header compression
- **Rule 150** — no-compression passthrough (rule ID byte + raw bytes)

---

## Project layout

```
schc-endpoint/
├── CMakeLists.txt                  # C decompressor build root
├── conanfile.txt                   # C deps: schc-full-sdk + zlog
├── config.cmake                    # LOG_CONFIG_FILE path
├── config/log.conf                 # zlog configuration
├── include/schc_endpoint/
│   ├── logger_helper.h
│   └── schc_service.h
├── src/
│   ├── CMakeLists.txt
│   ├── logger_helper.c
│   ├── schc_service.c              # Rule 28 + Rule 150 (from sink-demo-app)
│   └── main.c                      # reads hex stdin → writes {"decompressed":"<HEX>"}
├── app/
│   ├── config.py                   # env-var settings
│   ├── models.py                   # TTNUplink, ForwardPayload
│   ├── decompressor.py             # calls C binary via subprocess
│   ├── forwarder.py                # httpx POST to logging app
│   └── main.py                     # FastAPI: POST /uplink, GET /health
├── tests/
│   ├── conftest.py                 # packet builders + pytest fixtures
│   ├── test_uplink.py              # unit tests (C binary mocked)
│   ├── test_decompressor.py        # integration tests (requires built binary)
│   └── sample_uplink.json          # realistic ChirpStack uplink body
├── .env.example
└── requirements.txt
```

---

## Build — C decompressor

The C binary (`schc-decompressor`) uses the same `schc-full-sdk` and `zlog` Conan packages
as the other projects in this workspace.

```bash
cd schc-endpoint

# 1. Install C dependencies via Conan
mkdir build && cd build
conan install .. --build=missing -s build_type=Release

# 2. Configure CMake (single-config generator — binary lands at build/src/schc-decompressor)
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# 3. Build
cmake --build .

# Binary is now at: schc-endpoint/build/src/schc-decompressor
```

Quick smoke test (the binary reads hex from stdin, writes JSON to stdout):

```bash
# Rule-150 frame: 0x96 prefix + raw IPv6/UDP/CoAP bytes (from tests/sample_uplink.json data field)
echo "9660000000002211FF2001 0DB8000000010000000000000001200 10DB8000000020000000000000002123456780022000050023031B673656E736F72046461 7461FF0000C8410000F04050" \
  | tr -d ' ' \
  | ./build/src/schc-decompressor
# Expected: {"decompressed":"6000000000221 1FF..."}
```

---

## Install — Python dependencies

```bash
cd schc-endpoint

pip install -r requirements.txt
```

---

## Configure

```bash
cp .env.example .env
# Edit .env:
#   DECOMPRESSOR_PATH — path to the built binary (default: ./build/src/schc-decompressor)
#   LOGGING_APP_URL   — URL of your logging application's ingest endpoint
```

---

## Run — FastAPI app

```bash
cd schc-endpoint

DECOMPRESSOR_PATH=./build/src/schc-decompressor \
LOGGING_APP_URL=http://localhost:9090/sensor \
uvicorn app.main:app --host 0.0.0.0 --port 8080
```

The app exposes:

| Method | Route     | Description |
|--------|-----------|-------------|
| POST   | `/uplink` | TTN HTTP integration webhook (uplink events) |
| GET    | `/health` | Liveness check — returns `{"status": "ok"}` |

---

## Simulate a TTN uplink with curl

The payload below is a **Rule-150 SCHC frame** (no-compression passthrough) wrapping a
minimal IPv6/UDP/CoAP packet with a 9-byte packed `sensor_data_t` payload.
Use `tests/sample_uplink.json` as reference, or generate fresh bytes with
`python -c "from tests.conftest import b64_schc; print(b64_schc())"`.

```bash
curl -s -X POST http://localhost:8080/uplink \
  -H "Content-Type: application/json" \
  -d @tests/sample_uplink.json \
  | python -m json.tool
```

Expected response:

```json
{
  "status": "ok",
  "forwarded": {
    "data": "<base64 of decompressed IPv6/UDP/CoAP bytes>",
    "devEui": "0102030405060708",
    "fCnt": 42,
    "time": "2024-06-01T12:00:00.000Z"
  }
}
```

---

## What the endpoint forwards to the logging app

The endpoint POSTs the following JSON body to `LOGGING_APP_URL`:

```json
{
  "data": "<base64-encoded decompressed IPv6/UDP/CoAP bytes>",
  "devEui": "<LoRaWAN device EUI>",
  "fCnt":   42,
  "time":   "2024-06-01T12:00:00.000Z"
}
```

The `data` field carries the raw wire-format IPv6/UDP/CoAP packet exactly as
the SCHC decompressor reconstructed it.  **The logging application is responsible
for parsing this packet** to extract sensor values (temperature, pH, battery level).

---

## Run tests

```bash
# Unit tests — no binary required
pytest tests/test_uplink.py -v

# Integration tests — requires the built binary
DECOMPRESSOR_PATH=./build/src/schc-decompressor \
pytest tests/test_decompressor.py -v

# All tests
DECOMPRESSOR_PATH=./build/src/schc-decompressor pytest -v
```

---

## TTN integration setup

In ,TTN navigate to **Application → Integrations → HTTP** and add:

- **Uplink URL**: `http://<this-host>:8080/uplink` 

Only configure the Uplink URL.  Other event types (join, ack, status) are not
handled by this endpoint and should point elsewhere or be left unconfigured.

TTN will POST a JSON body matching the shape in `tests/sample_uplink.json`
for every received uplink frame.
