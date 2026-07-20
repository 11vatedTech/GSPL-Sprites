# Seed Language: Base/Storm Forms and Ascend/Descend

## Requirements
1. Seed defines `base` and `storm` forms via `[form.base]`/`[form.storm]`
2. Seed defines `ascend` (base→storm) and `descend` (storm→base) transformations
3. Storm morphology deltas alter silhouette, markings, aura, emissive, ability envelope
4. Schema version handled with backward compatibility
5. Invalid form/transformation diagnostics (undefined source, dangling target, duplicates, circular)

## Scenarios
1. Valid seed with base/storm and ascend/descend parses and compiles
2. Storm delta produces different morphology than base
3. Schema version round-trips through parse→canonicalize→parse
4. Undefined source form produces diagnostic
5. Circular transformation accepted when explicitly declared
