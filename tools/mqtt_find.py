#!/usr/bin/env python3
"""List MQTT messages whose topic contains a case-insensitive fragment."""

from __future__ import annotations

import argparse
import re
import time
from pathlib import Path

import paho.mqtt.client as mqtt


def secret(source: str, name: str) -> str:
    match = re.search(rf'^#define {name} "([^"]*)"$', source, re.MULTILINE)
    if match is None:
        raise RuntimeError(f"Missing {name} in include/secrets.h")
    return match.group(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("fragment")
    parser.add_argument("--seconds", type=float, default=4.0)
    args = parser.parse_args()
    source = Path("include/secrets.h").read_text(encoding="utf-8")
    port_match = re.search(r"^#define HOUSECAT_MQTT_PORT (\d+)$", source, re.MULTILINE)
    if port_match is None:
        raise RuntimeError("Missing HOUSECAT_MQTT_PORT in include/secrets.h")

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.username_pw_set(
        secret(source, "HOUSECAT_MQTT_USERNAME"),
        secret(source, "HOUSECAT_MQTT_PASSWORD"),
    )
    needle = args.fragment.casefold()

    def on_connect(active: mqtt.Client, _userdata: object, _flags: object, _reason: object, _properties: object) -> None:
        active.subscribe("#")

    def on_message(_client: mqtt.Client, _userdata: object, message: mqtt.MQTTMessage) -> None:
        if needle in message.topic.casefold():
            print(f"{message.topic} {message.payload.decode('utf-8', errors='replace')}")

    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(secret(source, "HOUSECAT_MQTT_HOST"), int(port_match.group(1)), 10)
    client.loop_start()
    time.sleep(args.seconds)
    client.disconnect()
    client.loop_stop()


if __name__ == "__main__":
    main()
