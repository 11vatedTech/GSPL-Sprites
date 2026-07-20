# 2D Frame Distinctness via Pixel Hashing

## Requirements
1. Each SpriteFrame carries SHA-256 hash of RGBA pixel data
2. Different frame IDs produce different pixel hashes
3. All frames within a clip are distinct (no duplicate-content frames)
4. Ascend transformation produces progressively different frames
5. Per-frame collision AABB and ability timing match frame content
6. Alpha, outline, depth, emissive channels generated alongside RGBA

## Scenarios
1. Idle clip hashes differ from attack clip hashes
2. Each frame in idle animation has different pixel hash
3. Ascend tick 1 frame differs from ascend tick N frame
4. Muzzle socket in metadata matches rendered muzzle location
5. Collision AABB encloses rendered pixels for each frame
