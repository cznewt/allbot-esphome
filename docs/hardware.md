# Hardware notes

This port drops the original Velleman / Whadda controller stack (Arduino UNO + VRSSM servo shield + VRBS1/VRBS2 battery shield) in favor of an ESP32 driving a PCA9685 16-channel PWM breakout. The mechanical kits are unchanged — you assemble the robot as Whadda documents, then swap the electronics.

Upstream assembly manuals (authoritative for the mechanical build):

- [VR408 – 4-legged ALLBOT](https://manuals.whadda.com/article.php?id=383)
- [VR009 – ALLBOT with four wheels](https://manuals.whadda.com/article.php?id=1182)

## Electronics overview

| Function | Original kit | This port |
| --- | --- | --- |
| MCU | Arduino UNO R3 | ESP32 devkit |
| Servo PWM | VRSSM servo shield (direct pins) | PCA9685 over I²C |
| Power | VRBS1 / VRBS2 battery shield | Any 5 V / 2 A+ supply; servo rail usually 6 V via a dedicated BEC |
| Buzzer | Piezo on D13 via `digitalWrite` pulses | GPIO via ESPHome `ledc` + `rtttl` |
| Remote | IR serial command protocol `<CC N S>` | Home Assistant actions / ESPHome automations |

You keep every servo, swing arm, and the backbone unchanged. Cut the original shield stack out.

## Wiring the PCA9685

One board covers any kit up to 16 servos (VR204 / VR408 / VR412 / VR612 / Wheels). VR618 at 18 servos needs two boards — solder the A0 address jumper on the second so it answers at `0x41`.

```
ESP32          PCA9685
GPIO21 ───────── SDA
GPIO22 ───────── SCL
3V3    ───────── VCC (logic)
GND    ───────── GND
6 V    ───────── V+  (servo rail — separate supply, common ground with the ESP32)
```

Signal wires from each servo plug into the corresponding channel (0–15). Mind the polarity — the standard Whadda/Velleman pinout is **brown = GND · red = V+ · yellow = signal**.

The examples in `example/` wire servos to channels in the order `hip_fl, hip_fr, hip_rl, hip_rr, knee_fl, …`. Follow that order or adjust the `channel:` in YAML to match your actual wiring.

## Servo calibration

> The Whadda manuals flag servo centering as a **critical** step: if a joint is clipped onto an off-center spline the leg geometry is wrong and every gait looks broken. Do this before attaching any swing arms.

1. Flash a temporary YAML that only defines your PCA9685 outputs and the `allbot:` block with each servo at `initial_angle: 90`.
2. Connect the servos one at a time (no arms attached) and power up. Each horn should snap to the mechanical centre — mark or note its position.
3. Clip the swing arms on per the kit manual with the servo still at 90°.
4. Flip to the real per-kit YAML (`initial_angle: 45` or the kit-specific value) and re-flash. The robot should come up in its resting pose.

The `flipped` and `offset` fields in `allbot.servos` mirror the original library's `BOT.attach(motor, pin, angle, flipped, offset)` semantics one-for-one, so existing calibration tables transfer directly.

## Buzzer

The original sketches toggle D13 with `delayMicroseconds` to synthesize a tone. ESPHome does the same work with `rtttl` — wire a small piezo between a free GPIO and ground through a transistor or directly if the piezo is low-current:

```yaml
output:
  - platform: ledc
    id: buzzer_out
    pin: GPIO13
    frequency: 1000Hz

rtttl:
  id: buzzer
  output: buzzer_out
```

Then trigger it from an HA action alongside `allbot.run_action` — see the `scared` and `chirp` handlers in `example/vr408.yaml`.

## Power

- **Servo rail (V+):** 5–6 V, at least 2 A for a quadruped, 3 A+ for hexapods. A micro servo can pull 500 mA during a stall — do not power the rail from the ESP32's 5 V pin.
- **Logic (VCC):** 3.3 V from the ESP32 board regulator is fine for the PCA9685 logic side.
- **Common ground:** tie the servo supply ground to the ESP32 ground. Failure to do so leaves the PWM reference floating and the servos will jitter or ignore commands.

## Safety / mechanical notes

- Leave the joint locking nuts **finger-tight** — the Whadda manuals call this out explicitly. A fully-torqued nut binds the swing arm and the servo will stall.
- Route servo leads away from the feet / wheels before you power up. The first gait test with a fresh harness is where cables get shredded.
- Keep the robot on a book or stand during the first `run_action` — a mis-flipped servo can drive the leg into the ground at full torque.
- The `allbot.stop` action halts a running gait but leaves each servo at its last commanded angle; if a leg is in a stall position, call `run_action: standup` (or your equivalent rest pose) to unload it.
