# Mouse Axis Calibration Design

## Context

The smoothed firmware converts MPU6050 `gz` samples to horizontal HID motion and `gy` samples to vertical HID motion. Device testing shows that horizontal movement is acceptable, while vertical movement is reversed and travels about half as far for a comparable gesture.

Code inspection confirms that both axes currently use the same gain. The direction matches the legacy firmware's final HID output, but that legacy mapping does not match the physical orientation of this device.

## Selected Design

Give `MouseMotionFilter` separate horizontal and vertical sensitivities:

- Horizontal output remains `-gz * (4 / 250)`.
- Vertical output becomes `-gy * (8 / 250)`.
- Fractional residual accumulation, HID clamping, reset behavior, BLE reporting cadence, and MPU sampling remain unchanged.

The vertical gain starts at twice the horizontal gain because the measured vertical travel is approximately half the horizontal travel. This is an empirical device calibration and can be adjusted later without affecting the horizontal axis.

## Alternatives Considered

1. Flip Y only at `Mouse.move()`. This corrects direction but leaves the unequal travel unresolved and scatters coordinate policy across two components.
2. Increase one shared sensitivity. This changes horizontal movement that the user already considers acceptable.
3. Use independent axis gains in `MouseMotionFilter` (selected). This keeps direction and scaling at the conversion boundary and isolates future calibration.

## Interface and Data Flow

`MouseMotionFilter` accepts separate X and Y sensitivity values at construction. `update(gy, gz)` applies the corresponding sign and gain before accumulating fractional residuals. The existing caller continues to pass the resulting signed HID delta directly to `Mouse.move(x, y)`.

## Testing

Board tests will first be changed to require:

- Positive `gy` produces negative Y output.
- The configured Y gain can produce twice the magnitude of X for equal raw inputs.
- Existing accumulation, clamping, and reset behavior still work.

The changed test must fail against the current implementation before production code is changed. After implementation, all board tests must pass, the formal firmware must compile, and the resulting firmware must upload successfully to COM7.

## Scope

This change does not alter MPU offsets, DMP configuration, BLE report frequency, button behavior, or horizontal sensitivity. Physical feel after flashing remains the final validation; if the approximate two-times calibration is imperfect, only the Y gain will be tuned in a later iteration.
