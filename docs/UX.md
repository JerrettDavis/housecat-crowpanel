# House Cat Physical UI and UX

## Control grammar

| Control | Everywhere | Home | Menu | Detail | Notification | Settings |
|---|---|---|---|---|---|---|
| Rocker Up | Previous visible choice | Previous card | Previous module | Alternate detail | No destructive action | Rotate counter-clockwise |
| Rocker Down | Next visible choice | Next card | Next module | Alternate detail | No destructive action | Rotate clockwise |
| Rocker Click | Confirm the obvious action | Pet Kitty | Open module | Play/interact | Acknowledge | Save orientation |
| Menu/Home | Safe global navigation | Open menu | Return home | Return home | Alert remains foreground | Return home after leaving alert |
| Back | Retreat one level | No-op | Return home | Return to menu | Dismiss only non-required alerts | Cancel draft |

## Toddler-grade interaction rules

1. **No hidden gestures.** First pass has no chords, timing puzzles, or multi-click commands.
2. **No destructive primary click.** Click pets, opens, plays, acknowledges, or saves a clearly previewed setting.
3. **One large visual noun.** Every menu page has one oversized icon and one short uppercase label.
4. **Persistent character anchor.** The cat is always visible and its pose reflects the meaning of the screen.
5. **Picture plus word.** Controls and information pair iconography with short text.
6. **Stable navigation.** Menu is always the escape hatch; Back always goes one level up.
7. **No tiny app grid.** The menu is a carousel so there is only one choice to understand at a time.
8. **No refresh punishment.** The input task buffers actions while e-paper is busy; rendering waits briefly for rocker movement to settle.

## Screen hierarchy

### Home

The default screen combines a large cat with one of three cards:

- Weather: outdoor condition and temperature, then indoor average
- Rooms: up to two room readings in the first-pass layout
- Mission: current objective and progress

Rocker Up/Down changes cards. Click pets the cat. Menu opens the module carousel.

### Menu

One module fills the screen with its icon, title, and plain-language purpose. Rocker changes the item. Click opens it.

### Notification

A notification preempts the current screen, but remembers where the user was. The visual hierarchy is:

1. cat reaction
2. event icon
3. short title
4. supporting sentence
5. large inverted acknowledgement bar

Non-critical, non-required messages may be dismissed with Back. Required and critical messages remain until rocker-click acknowledgement.

### Cat

Shows level progress and bond. Click pets. Additional pages can later expose discovered traits and memories without changing navigation semantics.

### Missions

Shows one active mission, short description, and progress. The first-pass local demo increments on click; later mission providers own progress.

### Settings

The first settings screen is orientation because enclosure placement is foundational. Rocker previews the four choices; click commits and forces a clean full refresh; Back cancels.

## Refresh behavior

- Navigation and ordinary interaction request a coalescible repaint.
- Notifications and orientation commits request full refresh.
- Important changes request immediate presentation; the JD79661 adapter uses its qualified full waveform for both request classes.
- Ordinary input waits 150 ms before rendering, capped at 450 ms from the first change.
- A full-refresh event bypasses the settle delay.
- The input FreeRTOS task remains active while the JD79661 is refreshing.

## Language guidelines

- Labels: one or two familiar words, usually uppercase.
- Message title: ideally under 18 characters in portrait.
- Message body: one short sentence.
- Avoid abbreviations unless they are universally recognized, such as Wi-Fi.
- Let the cat supply warmth; do not make safety notifications jokey.
