# Resource Limits Enforcement

## Requirements
1. Seed size: 1MB max, exceeded → diagnostic
2. Forms: 16 max, exceeded → diagnostic
3. Transformations: 32 max, exceeded → diagnostic
4. Bones: 64 max, exceeded → diagnostic
5. Sockets: 32 max, exceeded → diagnostic
6. Animation clips: 32 max, exceeded → diagnostic
7. Frames: 1024 max, 2048×2048 max dimensions, exceeded → diagnostic
8. 2.5D planes: 32 max, exceeded → diagnostic
9. Vertices: 65535 max, exceeded → diagnostic
10. Package size: 256MB max, exceeded → diagnostic
11. Runtime entities: 64 max, exceeded → diagnostic

## Scenarios
1. Each limit boundary test: at-limit input accepted
2. Each limit over-boundary test: exceeded input produces diagnostic
3. Diagnostics include limit name, current value, maximum value
