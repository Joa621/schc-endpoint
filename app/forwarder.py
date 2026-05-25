from __future__ import annotations
import httpx

from .config import LOGGING_APP_URL


async def forward(reading: dict) -> None:
    """POST structured sensor reading to the downstream logging application."""
    async with httpx.AsyncClient() as client:
        resp = await client.post(LOGGING_APP_URL, json=reading, timeout=5.0)
        resp.raise_for_status()
