# Governed inference provider architecture

The first production provider executes verified ONNX models through ONNX
Runtime 1.26.0's CPU execution provider on Windows x64. It is a real baseline,
not a mock: the integration suite loads and runs an upstream ONNX graph and
checks its numeric result.

Admission is fail closed. The immutable model descriptor must be valid and must
explicitly declare CPU/FP32 support. The model path must be a regular,
non-symlink file within its byte limit, and its SHA-256 must equal the descriptor
before a session is created. Loaded graph input/output names, element types,
ranks, and fixed dimensions are reconciled with the governed tensor contract.
Requests must provide every input exactly once and satisfy dimension, element,
per-tensor, and aggregate byte bounds. Results repeat those checks and record
provider version, backend, precision, and verified model identity.

Sessions use sequential graph execution, an explicit bounded intra-operation
thread count, one inter-operation thread, disabled worker spinning, and extended
graph optimization. The default is one intra-operation thread to preserve
deterministic resource behavior inside the existing bounded job scheduler.

The in-process provider accepts only models already admitted into the immutable
registry and verified by hash. ONNX Runtime may allocate an output before the
adapter can inspect its realized dimensions; therefore untrusted or newly
downloaded models must not execute in this process. A later provider-host process
will add OS-level memory, time, filesystem, and termination isolation before
untrusted model admission is allowed. CUDA and TensorRT remain separate explicit
providers; the CPU adapter never silently activates them.
