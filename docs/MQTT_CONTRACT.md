# House Cat MQTT Contract v0.1

The device base topic is:

```text
housecat/<device-id>
```

The default first-pass device ID is `housecat-desk-01`.

## Topics

| Topic suffix | Direction | Retain | Purpose |
|---|---|---:|---|
| `command/notification` | HA → Cat | No | Transient breaking event |
| `command/home` | HA → Cat | Yes | Latest normalized home snapshot |
| `command/mission` | HA → Cat | Yes | Current mission |
| `command/action` | HA → Cat | No | Remote equivalent of a physical action |
| `state` | Cat → HA | Yes | Current buddy/device state |
| `event` | Cat → HA | No | Interaction and acknowledgement event |
| `availability` | Cat → HA | Yes/LWT | `online` or `offline` |

House Cat also subscribes to `homeassistant/status` and republishes discovery and state when Home Assistant announces `online`.

## Notification

```json
{
  "id": "sam-home-1724271000",
  "title": "Sam is home!",
  "body": "Hoo-ray!",
  "kind": "person",
  "priority": "notice",
  "ttl_s": 180,
  "ack": false
}
```

Kinds: `generic`, `person`, `vehicle`, `home`, `warning`, `mission`.

Priorities, lowest to highest: `ambient`, `info`, `notice`, `important`, `urgent`, `critical`.

`ttl_s: 0` means no automatic expiration. Use that only for a condition whose clearing or acknowledgement is meaningful. Transient notifications should not be retained.

## Home snapshot

```json
{
  "outside_f": 86,
  "inside_f": 72,
  "condition": "sunny",
  "condition_label": "Sunny",
  "rooms": [
    {"name": "Office", "temperature_f": 70, "humidity": 39},
    {"name": "Living", "temperature_f": 73, "humidity": 42}
  ]
}
```

The first-pass renderer displays two rooms and the parser accepts four. Home Assistant should perform entity selection and averaging; the ESP receives a small semantic snapshot rather than enumerating the entire HA registry.

## Mission

```json
{
  "id": "watch-ioniq",
  "title": "Watch the Ioniq",
  "detail": "Tell me when charging is done.",
  "progress": 72,
  "target": 100,
  "complete": false
}
```

## Remote action

Payloads are plain strings:

```text
pet
up
down
select
menu
home
back
```

These enter the same application state machine as physical input.

## Device state

The retained device state also exposes the pal's life loop:

```json
{
  "food": 70,
  "rest": 70,
  "fun": 70,
  "energy": 70,
  "body": "balanced",
  "body_balance": 0,
  "activity": "idle",
  "meals": 0,
  "work_s": 0,
  "play_s": 0,
  "sleep_s": 0,
  "session_s": 0,
  "focus_phase": "off",
  "focus_remaining_s": 0
}
```

`food`, `rest`, and `fun` degrade once per minute. `work_s`, `play_s`, and
`sleep_s` are monotonic accumulated totals. Home Assistant discovery creates
need, condition, activity, focus, and total-duration sensors plus buttons for
meal, work, play, sleep, and idle actions.

Publish one of `feed`, `pet`, `work`, `play`, `sleep`, or `idle` to
`housecat/housecat-desk-01/command/action`. Work and leisure/sleep modes toggle;
`idle` always stops the active mode. Home snapshots may include Unix
`epoch_s`, allowing the persisted routine to catch up safely after a reboot.

```json
{
  "name": "Kitty",
  "level": 4,
  "xp": 260,
  "bond": 67,
  "mood": "content",
  "screen": "home",
  "wifi_rssi": -48,
  "notification_queue": 0,
  "mission_complete": false
}
```

## Home Assistant discovery

The firmware publishes one bundled device-discovery payload to:

```text
homeassistant/device/<device-id>/config
```

It currently creates Level, Bond, Mood, Screen, Wi-Fi signal, and Pet entities. Discovery is retained and is also resent after Home Assistant's MQTT birth message.
