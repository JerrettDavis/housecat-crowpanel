#!/usr/bin/env python3
"""Publish one MQTT message using the repository's ignored local credentials."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from paho.mqtt.publish import single


def secret(source: str, name: str) -> str:
    match = re.search(rf'^#define {name} "([^"]*)"$', source, re.MULTILINE)
    if match is None:
        raise RuntimeError(f"Missing {name} in include/secrets.h")
    return match.group(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("topic")
    parser.add_argument("payload", nargs="?", help="payload text; omit to read stdin")
    parser.add_argument("--retain", action="store_true")
    args = parser.parse_args()
    source = Path("include/secrets.h").read_text(encoding="utf-8")
    port_match = re.search(r"^#define HOUSECAT_MQTT_PORT (\d+)$", source, re.MULTILINE)
    if port_match is None:
        raise RuntimeError("Missing HOUSECAT_MQTT_PORT in include/secrets.h")
    single(
        args.topic,
        payload=args.payload if args.payload is not None else sys.stdin.read(),
        retain=args.retain,
        hostname=secret(source, "HOUSECAT_MQTT_HOST"),
        port=int(port_match.group(1)),
        auth={
            "username": secret(source, "HOUSECAT_MQTT_USERNAME"),
            "password": secret(source, "HOUSECAT_MQTT_PASSWORD"),
        },
    )


if __name__ == "__main__":
    main()
