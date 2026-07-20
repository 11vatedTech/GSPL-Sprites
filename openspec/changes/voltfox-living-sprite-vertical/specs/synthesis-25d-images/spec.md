# Actual 2.5D Rendered Plane Images

## Requirements
1. synthesize_projection25d_voltfox() produces ImageRgba8 buffers per depth plane
2. Each plane image has SHA-256 content hash stored in metadata
3. Every plane ID referenced in metadata has corresponding rendered image data
4. Plane content derives from morphology parts (torso, head, ears, tail, limbs, aura)
5. Palette change produces different plane images
6. Storm form produces different plane content than base form
7. Planes sorted by depth with stable intervals
8. Parallax offset metadata stored per plane

## Scenarios
1. All 11 morphology parts produce corresponding depth plane images
2. Plane image hashes differ between base and storm forms
3. Palette change produces different plane hashes
4. Every plane ID resolves to actual image data
5. Depth order stable and matches morphological z-order
