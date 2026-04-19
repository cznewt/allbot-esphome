# ALLBOT for ESPHome

ESPHome external component port of [Velleman's ALLBOT-lib](https://github.com/Velleman/ALLBOT-lib) — the Arduino library that drives the Velleman ALLBOT modular robotic kits (VR204, VR408, VR412, VR612, VR618, Wheels).

The original library is a blocking Arduino sketch. This port rewrites it as a non-blocking ESPHome component so ALLBOTs can be declared entirely in YAML, triggered from Home Assistant services or any ESPHome automation, and run on ESP32 / ESP8266.

## Install

```yaml
external_components:
  - source: github://cznewt/allbot-esphome
    components: [allbot]
```

Or reference a local checkout:

```yaml
external_components:
  - source:
      type: local
      path: ../components
    components: [allbot]
```

## Configure

Each servo is wired to an `output: platform: ledc` (ESP32) or `esp8266_pwm` (ESP8266) entry running at 50 Hz. The `allbot` component then references those outputs and adds per-servo `flipped` / `offset` — identical semantics to Velleman's `BOT.attach(i, pin, angle, flipped, offset)`.

Gaits are lists of steps. Each step is a set of simultaneous `moves` with a `duration`. The `duration` maps 1-to-1 to the `speed` argument of the original `BOT.animate(speed)` call: the engine moves each servo `1°` per millisecond until the step completes. A step with no `moves` is a pure delay — handy for porting the `delay(speedms/2)` pauses in `leanforward` et al.

```yaml
output:
  - platform: ledc
    id: pwm_hip_fl
    pin: GPIO13
    frequency: 50Hz

allbot:
  id: bot
  servos:
    - id: hip_fl
      output: pwm_hip_fl
      flipped: false
      offset: 0
      initial_angle: 45
  actions:
    walk_forward:
      - duration: 100ms
        moves:
          - {servo: knee_rr, angle: 80}
          - {servo: knee_fl, angle: 80}
      - duration: 100ms
        moves:
          - {servo: hip_rr, angle: 80}
          - {servo: hip_fl, angle: 20}
```

## Servo options

| Key | Default | Description |
| --- | --- | --- |
| `id` | — | Name used to reference the servo from `actions.*.moves[].servo`. |
| `output` | — | ID of a `output::FloatOutput` entry (50 Hz PWM). |
| `initial_angle` | `45` | Angle (°) written at boot and the resting pose. |
| `flipped` | `false` | Mirror the servo — useful for left/right symmetric joints. |
| `offset` | `0` | Mechanical zero correction in degrees (−90…90). |
| `min_pulse_width` | `544us` | Pulse width at 0° (matches the Arduino `Servo` default). |
| `max_pulse_width` | `2400us` | Pulse width at 180°. |

## Automation actions

- `allbot.move: {id, servo, angle}` — queue a target angle for servo index `servo`. Takes effect on the next `allbot.animate`.
- `allbot.animate: {id, duration}` — play the queued moves over `duration`. Non-blocking: returns immediately.
- `allbot.run_action: {id, action}` — start a named gait. Non-blocking.
- `allbot.stop: {id}` — abort any running gait; servos hold their current angle.
- `allbot.is_busy: {id}` — condition; true while a gait is playing.

From a lambda:

```cpp
id(bot)->run_action("walk_forward");
id(bot)->stop();
```

## Hardware recommendation: PCA9685

One PCA9685 16-channel PWM breakout covers most ALLBOT kits (only VR618 at 18 servos needs two boards). It gives every servo a hardware-timed 50 Hz PWM channel over I²C — freeing the ESP32 GPIOs and avoiding the per-pin frequency conflicts you get with `ledc` when also driving a buzzer.

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22

pca9685:
  - id: pca
    address: 0x40
    frequency: 50Hz

output:
  - platform: pca9685
    pca9685_id: pca
    id: pwm_hip_fl
    channel: 0
```

Any `output: platform: ledc` entry also works if you prefer native ESP32 pins.

## Chirp / sound

The piezo buzzer in the original sketch is separate from the servo bus. In ESPHome, wire it to a `ledc` output and drive it through the `rtttl` component — no special support in the `allbot` component is needed:

```yaml
output:
  - platform: ledc
    id: buzzer_out
    pin: GPIO13
    frequency: 1000Hz

rtttl:
  id: buzzer
  output: buzzer_out

api:
  actions:
    - action: chirp
      then:
        - rtttl.play: {rtttl: "chirp:d=32,o=7,b=180:c,c,c"}
    - action: scared
      then:
        - allbot.run_action: {id: bot, action: scared}
        - rtttl.play: {rtttl: "scared:d=16,o=6,b=180:c,e,g,c7,e,g,c7"}
```

## Examples

Every kit in the original library has a reference config under [`example/`](example):

| File | Kit | Servos | Notes |
| --- | --- | --- | --- |
| [`vr204.yaml`](example/vr204.yaml) | VR204 biped | 4 | `walk/look/scared/chirp` |
| [`vr408.yaml`](example/vr408.yaml) | VR408 quadruped | 8 | `walk/turn/lean/wave/scared/chirp` |
| [`vr412.yaml`](example/vr412.yaml) | VR412 quadruped w/ ankles | 12 | `standup/walk/turn/lean/look/wave/scared/chirp` |
| [`vr612.yaml`](example/vr612.yaml) | VR612 hexapod | 12 | `walk/turn/lean/look/wave/scared/chirp` |
| [`vr618.yaml`](example/vr618.yaml) | VR618 hexapod w/ ankles | 18 | `standup/walk/turn/lean/wave/chirp` (two PCA9685s) |
| [`wheels.yaml`](example/wheels.yaml) | Wheels rover | 8 (4 hips + 4 continuous-rotation wheels) | `forward/backward/spin_left/spin_right/halt/chirp` |

Each file exposes every gait as a Home Assistant action via the `api:` block, replacing the `<CC N S>` serial protocol from the original sketches. Ported gait sequences keep the original angle tables 1:1; durations match the `BOT.animate(speedms)` calls at a reasonable fixed speed (100 ms default), so you can retime them in YAML without touching C++.

## Kit pin maps

The example targets an ESP32 devkit. If you are using the original Velleman VRSSM shield, adjust pin IDs to whatever your ESP board exposes.

## Documentation

- [`docs/hardware.md`](docs/hardware.md) — PCA9685 wiring, power, servo calibration, buzzer, safety notes. Read this first if you are swapping the VRSSM/UNO stack for an ESP32.

Per-kit pages — servo maps, gait lists, first-run procedures, and kit-specific tuning tips:

- [`docs/kits/vr204.md`](docs/kits/vr204.md) — VR204 biped (4 servos)
- [`docs/kits/vr408.md`](docs/kits/vr408.md) — VR408 quadruped (8 servos)
- [`docs/kits/vr412.md`](docs/kits/vr412.md) — VR412 quadruped with ankles (12 servos)
- [`docs/kits/vr612.md`](docs/kits/vr612.md) — VR612 hexapod (12 servos, tripod gait)
- [`docs/kits/vr618.md`](docs/kits/vr618.md) — VR618 hexapod with ankles (18 servos, two PCA9685 boards)
- [`docs/kits/wheels.md`](docs/kits/wheels.md) — Wheels (VR009) rover (continuous-rotation drive)

Upstream Whadda manuals are authoritative for the mechanical assembly — this repo covers only the electronics swap and the software.

## Credits

Original logic: Velleman `ALLBOT-lib` (MIT, 2017). This port preserves the `prepare`/`tick` animation semantics (1° per ms) so gaits transcribed directly from the `*.ino` examples keep their original timing.
