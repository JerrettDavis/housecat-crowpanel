# Home Assistant Setup

1. Change every occurrence of `housecat-desk-01` if the firmware uses another device ID.
2. Replace the example person, weather, temperature, humidity, leak, and vehicle entity IDs.
3. Copy `housecat_package.yaml` into a configured Home Assistant packages directory, or move the scripts and automations into the UI/YAML locations you use.
4. Reload scripts and automations or restart Home Assistant.
5. Run `script.housecat_send_home_snapshot` once.
6. Run `script.housecat_notify` with a short TTL to validate the breaking-notification path.

The firmware publishes MQTT device discovery for its own diagnostic entities. This includes the nearby Wi-Fi count, GPIO 40/41 levels, and buttons that start a passive Wi-Fi scan or read-only GPIO probe. The package is for messages Home Assistant sends *to* the buddy.

Transient notification messages are intentionally not retained. Home snapshots and current missions are retained because they represent the latest state.

The home snapshot script prefers Home Assistant's standard `weather.forecast_home` entity and otherwise uses the first available weather entity. It refuses to publish when no numeric outdoor temperature is available, converts Celsius sources to Fahrenheit, and refreshes every five minutes. This prevents the panel from presenting a fabricated fallback temperature.
