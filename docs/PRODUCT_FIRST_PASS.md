# House Cat First-Pass Product Specification

## Product statement

**House Cat is a persistent desktop creature that understands the home, reacts to meaningful events, and gains new abilities through software and hardware extensions.**

The first pass proves the smallest complete product loop:

1. House Cat boots locally and remains useful without a network.
2. The user can understand and operate it from the five physical controls.
3. The cat is present on every screen and reacts visually.
4. Home Assistant can update ambient home data and interrupt with expiring notifications.
5. Interaction changes persistent progression.
6. Every screen works in all four orientations.
7. The same deterministic renderer runs in the native simulator and on the device.

## Product principles

### The cat is the product

Weather, alerts, missions, tools, and extensions are things the cat notices or does. They are not unrelated apps arranged around a mascot.

### One obvious action

Every screen has one primary click action. Rocker Up and Down only move or change the visible choice. Menu returns to a safe global place. Back retreats without causing damage.

### Image before prose

A large cat pose and a large domain icon communicate the state before text is read. Text labels reinforce rather than replace imagery.

### Calm e-paper interaction

House Cat poses instead of animating. Input is sampled continuously and buffered while the panel refreshes. Rapid rocker changes are coalesced into one render.

### Offline is a mode, not an error

The buddy, progression, local missions, settings, and future attached extensions remain functional without Wi-Fi or Home Assistant.

## First-pass feature boundary

### Implemented

- Original 1-bit cat art with eight poses
- Cohesive 1-bit icon language
- Portrait and landscape responsive layouts
- 0°, 90°, 180°, and 270° logical orientation
- Home cards for weather, rooms, and current mission
- Priority and TTL-aware notification queue
- Cat level, XP, bond, mood, interactions, and pet cooldown
- Menu carousel for Cat, Missions, Whiskers, Library, Lab, and Settings
- First-pass module shells that preserve the product model
- Buffered five-button input task
- Partial/full refresh budget and render coalescing
- MQTT Home Assistant bridge and device discovery
- NVS persistence for cat, settings, and mission state
- Native C++ simulator and tests
- Asset regeneration pipeline

### Intentionally scaffolded, not implemented yet

- Captive-portal provisioning
- Actual BLE/Wi-Fi scan result screens
- UART capability discovery protocol
- microSD or external storage provider
- ebook parsing
- battery measurement and power profiles
- OTA update endpoint
- trait discovery, memories, and dialogue personality engine
- audio, vibration, or lighting accessories

## Success criteria for the hardware smoke test

- Every control generates the expected action exactly once per press.
- Holding rocker Up or Down repeats at a comfortable rate.
- Presses made during an e-paper refresh appear afterward in order.
- The qualified full JD79661 waveform renders every request consistently.
- No screen omits the cat.
- All four orientations remain readable when the enclosure is physically rotated.
- A Home Assistant MQTT message appears before its TTL expires and does not replay after expiration.
- Urgent and critical notifications require click acknowledgement.
- Cat XP, bond, orientation, and mission survive a reboot.
