# House Cat Serial Test Console

The serial console is available at 115200 baud in both firmware builds. It is
designed to qualify the complete experience without Wi-Fi, MQTT, or Home Assistant.

Commands execute through the same `HouseCatApp` state machine used by physical controls and MQTT. They are not a separate mock UI.

## Navigation and buddy commands

| Command | Result |
|---|---|
| `up` | Rocker Up |
| `down` | Rocker Down |
| `select` or `ok` | Rocker Click |
| `menu` or `home` | Home/Menu button |
| `back` | Back/Exit button |
| `pet` | Pet the cat without navigating |

## Scenario injection

| Command | Example | Result |
|---|---|---|
| `person [title\|body]` | `person Maya is home!\|Best news all day.` | Person-arrival notification |
| `car [title\|body]` | `car Ioniq 5 N charged!\|Ready to go.` | Important vehicle notification |
| `notify title\|body` | `notify Washer finished\|Move the clothes.` | Normal notification |
| `alert title\|body` | `alert Water detected!\|Check the kitchen.` | Required urgent alert |
| `weather OUT IN CONDITION` | `weather 93 72 sunny` | Home snapshot temperatures and condition |
| `mission P T title\|detail` | `mission 2 5 Find radios\|Scan nearby BLE devices.` | Mission progress, target, and copy |

Supported weather conditions are `sunny`, `clear`, `cloudy`, `partlycloudy`, `rain`, and `rainy`.

## Display and orientation

| Command | Result |
|---|---|
| `rotate 0` | Upright portrait |
| `rotate 90` | Landscape |
| `rotate 180` | Inverted portrait |
| `rotate 270` | Opposite landscape |
| `partial` | Force a coalescible repaint request (presented with the qualified full waveform) |
| `full` or `repaint` | Force a full refresh |

The safe PlatformIO environment converts `partial` requests to full refreshes. The serial log identifies the active refresh profile at boot.

## Diagnostics and maintenance

| Command | Result |
|---|---|
| `help` or `?` | Print the complete command list |
| `status` | Buddy, UI, mission, notification, network, heap, and PSRAM state |
| `buttons` | Live active-low state of all five controls |
| `diagnostics` or `diag` | Repeat full startup diagnostics |
| `reboot` | Software restart |
| `factory-reset` | Clear the `housecat` NVS namespace and restart |

## Line format

Commands end with Enter. Text fields use a vertical bar to separate title and body/detail:

```text
person Sam is home!|Hoo-ray!
```

Input is capped at 384 characters. Unknown commands do not change application state.
