#!/usr/bin/env python3
"""Read one MQTT message using the repository's ignored local credentials."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from paho.mqtt.subscribe import simple


def secret(source: str, name: str) -> str:
    match = re.search(rf'^#define {name} "([^"]*)"$', source, re.MULTILINE)
    if match is None:
        raise RuntimeError(f"Missing {name} in include/secrets.h")
    return match.group(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("topic")
    args = parser.parse_args()
    source = Path("include/secrets.h").read_text(encoding="utf-8")
    message = simple(
        args.topic,
        hostname=secret(source, "HOUSECAT_MQTT_HOST"),
        port=int(re.search(r"^#define HOUSECAT_MQTT_PORT (\d+)$", source, re.MULTILINE).group(1)),
        auth={
            "username": secret(source, "HOUSECAT_MQTT_USERNAME"),
            "password": secret(source, "HOUSECAT_MQTT_PASSWORD"),
        },
        msg_count=1,
    )
    print(message.payload.decode("utf-8"))


if __name__ == "__main__":
    main()
