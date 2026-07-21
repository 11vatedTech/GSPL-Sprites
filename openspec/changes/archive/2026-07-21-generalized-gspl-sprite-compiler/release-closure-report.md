# Release-Closure Final Report

**Change:** generalized-gspl-sprite-compiler
**Archive:** `openspec/changes/archive/2026-07-21-generalized-gspl-sprite-compiler/`
**Author:** 11vatedTech
**Date:** 2026-07-21

---

## 1. Change Overview

Complete the GSPL sprite compiler pipeline for production use: deterministic, bounded, validated, and verifiable across all 2D/2.5D/3D representations. The change builds on the infrastructure hardening foundation (cfc1ef0) with a generalized compiler accepting arbitrary SpriteSeeds, full resource-limit enforcement, mutation test coverage, cross-representation consistency, and a dual-compiler CI pipeline.

## 2. Deliverables

### New Capabilities (5 new main specs)
| Capability | Location | Description |
|---|---|---|
| generalized-compiler | `openspec/specs/generalized-compiler/spec.md` | Accept arbitrary SpriteSeeds, compile to SpriteIR, synthesize all three representations deterministically |
| resource-limits-enforcement | `openspec/specs/resource-limits-enforcement/spec.md` | Bounded resource consumption at every pipeline stage |
| frame-content-identity | `openspec/specs/frame-content-identity/spec.md` | GSPL_FRAME_V1 SHA-256 hashing, frame distinctness, PixelAABB |
| headless-evidence-contract | `openspec/specs/headless-evidence-contract/spec.md` | Versioned JSON evidence, committed oracle, oracle verification |
| preview-authority | `openspec/specs/preview-authority/spec.md` | Transformation-state-machine-driven preview |

### Modified Capabilities (updated 3 existing specs)
- **headless-evidence**: `_schema_version` field, `status_id`/`ability_name` serialization, oracle verification
- **seed-forms**: Form validation enforces resource limits; transformation state machine drives transitions
- **compile-forms**: Cross-representation compilation with resource-limit enforcement

### Source Code Changes
| File | Change |
|---|---|
| `src/core.cpp` | compile() entity-agnostic; package size enforcement (256MB); resource limits in parse_seed/validate/build_package_internal |
| `src/synthesis.cpp` | synthesize_unified_entity morphs on morphology presence (not entity_id); synthesis-level resource limits live |
| `src/preview.cpp` | A/S key handlers call begin_transformation; render functions read tstate.current_form |
| `src/headless_evidence.cpp` | write_trace_json outputs _schema_version, tick, kind, form_before/after, health, status_id, ability_name |
| `src/image.cpp` | compute_frame_hash preimage = "GSPL_FRAME_V1" + dims + format + RGBA |
| `tests/mutation_tests.cpp` | 26 scenarios (up from 24) — preview toggle, malformed headless output |
| `tests/voltfox_integration_tests.cpp` | Section 8 cross-representation consistency test |
| `tests/headless_evidence_tests.cpp` | Oracle verification; _schema_version check |
| `tests/evidence_oracle.json` | Committed deterministic oracle |
| `.github/workflows/ci.yml` | Restricted to windows-2025 (project requires Windows x64 for ONNX Runtime) |

## 3. Verification Evidence

### CI (final SHA a78c387)
- **MSVC (windows-2025):** CI run #8 (c892679) — Configure ✅ Build ✅ Test ✅
- **GCC (MinGW):** Validated locally — 48/48 tests, 0 warnings
- **Linux GCC:** Not currently supported — ONNX Runtime Windows-x64 dependency boundary

### OpenSpec Validation
- **`openspec validate --all`:** 15 items pass

### Task Completion
- **23/24 IMPLEMENTED**
- **1 APPROVED DEFERRAL:** task 4.4 (Device Guard mitigation docs — environmental, not code)

## 4. Known Issues & Gaps

| Issue | Severity | Status |
|---|---|---|
| Windows Device Guard blocks freshly-linked executables (BAD_COMMAND) | Environmental | Mitigation: rebuild + retry; task 4.4 deferred for documentation |
| Frame hash propagation is 2D-only (2.5D/3D lack frame_hash field) | Design constraint | Semantic state identity replaces pixel hashes for 2.5D/3D |
| MSVC CI confirmed post-push; GCC tested locally only | Verification | CI now windows-2025 only; GCC builds are local-only |
| ONNX Runtime requires Windows x64 (CMakeLists.txt fatal-errors on Linux) | Platform constraint | CI restricted to windows-2025; documented in build system |

## 5. Commit History (cfc1ef0..a78c387)

```
a78c387 archive: finalize generalized-gspl-sprite-compiler change
c892679 ci: restrict to windows-2025 (project requires Windows x64 for ONNX Runtime)
5b2002c fix: commit missing OpenSpec change artifact files (proposal, design, delta specs)
fb50907 docs: sync delta specs to main, update task completion status
6c926c5 test: cross-representation consistency, frame hash determinism, synthesis limits
949427f feat: mutation tests 25-26, synthesis resource limits, package size enforcement
cfc1ef0 infra: harden resource limits, frame identity, preview authority, headless contract
```

## 6. Repository State

- **Branch:** main
- **HEAD:** a78c387 (0 ahead, 0 behind origin/main)
- **Working tree:** clean
- **Active changes:** none (archived)
- **Archived changes:** 4 (voltfox-reference-entity, voltfox-living-sprite-vertical, voltfox-living-sprite-vertical-v2, generalized-gspl-sprite-compiler)

---

**Prepared by:** opencode (11vatedTech)
