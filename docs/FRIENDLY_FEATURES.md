# Friendly feature map

House Cat should feel attentive without pretending the panel senses things it
does not. The panel itself provides five buttons, e-paper, Wi-Fi/BLE, two spare
GPIO pins, and persistent flash. Home Assistant supplies the household context.

## Implemented companionship pass

- **Pal check-in:** a fourth Home card turns time, weather, indoor temperature,
  and humidity into one short, friendly observation. The message is generated
  by Home Assistant and delivered with the retained home snapshot.
- **Reading continuity:** the Library automatically bookmarks every displayed
  page. Reopening the cached title resumes locally from LittleFS, including when
  Wi-Fi is unavailable.
- **Tactile interaction:** Select on Home still pets the cat from every card,
  keeping the primary interaction consistent and rewarding.

## Good next additions

1. **Arrival greetings** from opted-in Home Assistant `person` entities.
2. **Quiet hours** that use a sleepy pose and suppress non-urgent interruptions.
3. **Tiny routines** such as bedtime, medication, recycling, and plant-care
   missions, acknowledged with the physical Select button.
4. **Comfort nudges** when indoor readings stay unusually hot, cold, or humid,
   with hysteresis in Home Assistant to avoid repeated alerts.
5. **GPIO buddy accessories** such as a large external button, contact switch,
   or low-voltage plant-moisture module.
6. **Known-device hellos over BLE**, opt-in and local-only, after the simpler
   Home Assistant presence route has proven useful.

The design rule is calm usefulness: retained ambient context belongs on Home;
only actionable or safety-critical events should interrupt the current screen.
