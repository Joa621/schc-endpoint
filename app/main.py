from __future__ import annotations
import base64
import logging

from fastapi import FastAPI, HTTPException, Request, status

from .config import EXPECTED_FPORT
from .models import ForwardPayload
from .decompressor import decompress
from .forwarder import forward

logger = logging.getLogger(__name__)

app = FastAPI(title="SCHC Endpoint", version="1.0.0")


@app.post("/uplink", status_code=status.HTTP_200_OK)
async def uplink(request: Request) -> dict:

    print("\n========== UPLINK HANDLER ==========")

    body = await request.json()

    print("FULL JSON:")
    print(body)

    uplink_message = body.get("uplink_message", {})

    fport = uplink_message.get("f_port")
    data = uplink_message.get("frm_payload")

    print(f"FPORT = {fport}")
    print(f"BASE64 PAYLOAD = {data}")

    if fport != EXPECTED_FPORT:
        print("IGNORED: wrong FPort")
        return {
            "status": "ignored",
            "reason": f"fPort {fport} != {EXPECTED_FPORT}",
        }

    if not data:
        raise HTTPException(
            status.HTTP_400_BAD_REQUEST,
            "Missing frm_payload"
        )

    try:
        decompressed: bytes = decompress(data)

        print("\nDECOMPRESSED HEX:")
        print(decompressed.hex())

    except Exception as exc:
        print(f"\nDECOMPRESSION ERROR: {exc}")

        raise HTTPException(
            status.HTTP_422_UNPROCESSABLE_ENTITY,
            f"Decompression failed: {exc}"
        ) from exc

    forwarded = ForwardPayload(
        data=base64.b64encode(decompressed).decode(),
        devEui=body.get("end_device_ids", {}).get("dev_eui"),
        fCnt=uplink_message.get("f_cnt"),
        time=body.get("received_at"),
    )

    print("\nFORWARDING TO LOGGING APP:")
    print(forwarded.model_dump())

    try:
        await forward(forwarded.model_dump())
        print("FORWARD SUCCESS")

    except Exception as exc:
        print(f"FORWARD ERROR: {exc}")

        raise HTTPException(
            status.HTTP_502_BAD_GATEWAY,
            f"Forward failed: {exc}"
        ) from exc

    return {
        "status": "ok",
        "forwarded": forwarded.model_dump()
    }


@app.get("/health")
async def health():
    return {"status": "ok"}
