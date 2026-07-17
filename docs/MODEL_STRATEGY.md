# Model Strategy 0.1

Models are replaceable production resources, never domain authority. Every
descriptor records stable identity and version, immutable source revision,
model-file SHA-256, SPDX license and permitted uses, tasks, bounded typed tensor
contracts, minimum RAM/VRAM, supported backend/precision pairs, and declared
determinism class. Registration is immutable and idempotent.

Selection is deterministic. It first enforces task, commercial rights, memory,
VRAM, and minimum determinism; then ranks explicit backend and precision
preferences; stable model identity/version break ties. No fallback is silent:
absence of a compatible descriptor returns no selection and the synthesis plan
must emit a diagnostic.

The first governed provider is ONNX Runtime 1.26.0 CPU/FP32 on Windows x64 and
performs verified local inference under `INFERENCE_PROVIDER_ARCHITECTURE.md`.
CUDA is the primary NVIDIA acceleration direction, TensorRT remains only a derived optimized cache, WinML provides
portable Windows acceleration, OpenVINO where validated for Intel hardware, and
CoreML for future directly tested Apple support. Descriptor support does not
claim that a backend or model is installed; provider discovery and model-file
hash verification are separate runtime gates.

No production model has been selected by this specification. Admission requires
license review, reproducible file acquisition, local hash verification, quality
evaluation on governed rights-safe fixtures, measured RAM/VRAM/latency, and
failure/determinism characterization.
