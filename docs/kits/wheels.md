# Wheels (VR009) — 4-wheeled ALLBOT

Upstream manual: [manuals.whadda.com/article.php?id=1182](https://manuals.whadda.com/article.php?id=1182).

The Wheels kit isn't a standalone chassis — it's an add-on that converts a VR408 quadruped (or a VR204 biped plus VR007 plastics) into a wheeled rover. Two VR009 wheel packs plus the donor kit give you eight servos: four steered hips and four continuous-rotation wheel drives.

## Kit at a glance

- 4 × standard 9 g servos (steered hips) — reused from VR408 / VR204
- 4 × **continuous-rotation** (360°) servos — the wheel drives, supplied by VR009
- Swing arms, wheel hubs, self-tapping screws for the wheels

## Continuous-rotation servo behaviour

This is the one place the Wheels kit diverges meaningfully from the other ALLBOTs. A continuous-rotation servo doesn't have a target angle — it has a **target speed**:

| Commanded angle | Behaviour |
| --- | --- |
| ~90° | Stopped (neutral) |
| < 90° | Reverses, with speed proportional to the offset from 90° |
| > 90° | Drives forward, with speed proportional to the offset from 90° |

The original Whadda sketch reads a trimmer pot to modulate the duty cycle; `example/wheels.yaml` hard-codes 45° / 135° for full-speed reverse / forward and keeps 90° as neutral. Tune those numbers if your specific servos aren't centered.

**Calibration:** set every wheel servo to 90° in the temp YAML, power up, and trim the small pot on the side of the servo until the wheel stops rotating. This is the continuous-rotation equivalent of "centre the spline" on the hip servos.

## Servo map used by `example/wheels.yaml`

| Joint | Servo id | PCA9685 channel | `flipped` | `offset` | Resting angle |
| --- | --- | --- | --- | --- | --- |
| Front-left hip  | `hip_fl`   | 0 | no  | 0  | 80 |
| Front-right hip | `hip_fr`   | 1 | yes | 0  | 80 |
| Rear-left hip   | `hip_rl`   | 2 | yes | 0  | 80 |
| Rear-right hip  | `hip_rr`   | 3 | no  | −5 | 80 |
| Front-left wheel  | `wheel_fl` | 4 | yes | 2  | 90 |
| Front-right wheel | `wheel_fr` | 5 | no  | 0  | 90 |
| Rear-left wheel   | `wheel_rl` | 6 | yes | 1  | 90 |
| Rear-right wheel  | `wheel_rr` | 7 | no  | 1  | 90 |

The per-servo `offset` values mirror the calibration comments in the original `WHEELS.ino` — they compensate for small mechanical differences between individual servos. Replace with your own offsets after you trim the wheel pots.

## Drive primitives

`example/wheels.yaml` defines five actions instead of per-step gaits:

- `forward_start`, `backward_start` — splay hips to a neutral drive stance, then engage all four wheels.
- `spin_left_start`, `spin_right_start` — splay hips further out, then drive left/right wheels in opposite directions for a zero-radius turn.
- `halt` — stop the wheels and return hips to the drive stance.

Each is a two-step sequence (hips first, wheels second) so the robot doesn't "scrub" sideways while the wheels spool up.

**Important:** unlike the jointed kits, a wheels action doesn't end — the continuous servos keep spinning at the last commanded speed until you issue `halt` or `stop`. Use a HA automation + timer if you want to drive for a fixed duration:

```yaml
- service: esphome.allbot_wheels_forward
- delay: 2s
- service: esphome.allbot_wheels_halt
```

## Typical first-run sequence

1. Flash `example/wheels.yaml`.
2. Prop the chassis up so the wheels spin free.
3. Call `halt` — all four wheels should sit still. If any drifts, trim that servo's pot.
4. Call `forward_start`, watch the wheels spin, then `halt`.
5. Repeat for `spin_left_start` / `spin_right_start`. Left wheels should drive one way, right wheels the other.
6. Put it on the floor and enjoy.
