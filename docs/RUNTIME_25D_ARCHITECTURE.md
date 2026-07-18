# Operational 2.5D Runtime

The 2.5D runtime converts a validated `Projection25dDefinition` and fixed-point camera state into a deterministic presentation frame. It is a projection layer: it does not mutate semantic simulation state.

## Frame evaluation

- Discrete multi-angle billboards select the nearest authored or governed generated view. Existing deterministic angular tie-breaking remains authoritative.
- Fixed-axis and camera-facing billboards require exactly one view, eliminating collection-order-dependent selection.
- Visible planes receive view-specific asset overrides, fixed-point parallax offsets, and bounded signed camera deformation. Plane instances are ordered by depth and stable ID.
- Hybrid geometry is emitted only while its attachment plane is visible. It inherits the plane's presentation offset and deformation.
- Collision volumes are copied in stable ID order without parallax or camera deformation. Their coordinates remain semantic authority, independent of presentation.

All camera coordinates use microunits, yaw uses millidegrees, depth uses millimeters, and proportional factors use parts per million. Evaluation performs no floating-point arithmetic.

## Failure behavior

Evaluation rejects an invalid projection and camera yaw outside `[0, 360000)`. Validation diagnostics remain the source of definition failures; runtime errors do not silently substitute assets or views.
