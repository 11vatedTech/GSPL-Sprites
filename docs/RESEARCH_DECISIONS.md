# Research decisions — 2026-07-17

| Subject | Decision | Evidence and rationale |
|---|---|---|
| GSPL concepts | ADAPT seed-first identity, typed genes, canonicalization, controlled entropy, provenance, and compiler staging | Adjacent GSPL canon is evidence only; product contracts are independently defined. |
| Cross-repository reuse | REJECT | The product must build after every reference repository is removed. |
| 3D runtime interchange | ADOPT glTF 2.0 as a target adapter | Khronos specifies skins, animation channels, and morph targets; it is not rich enough to be the semantic source. https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html |
| Production authoring interchange | ADAPT OpenUSD | USD layer composition and asset resolution are valuable for non-destructive pipelines; semantic entity identity remains internal. https://openusd.org/release/intro.html |
| Portable inference | ADOPT ONNX Runtime provider abstraction | One API can prioritize CUDA and fall back to CPU. https://onnxruntime.ai/docs/execution-providers/ |
| Windows portability | SUPERSEDE DirectML-first with WinML evaluation | DirectML is sustained engineering and Microsoft directs new Windows deployment work to WinML. https://onnxruntime.ai/docs/execution-providers/DirectML-ExecutionProvider.html |
| NVIDIA optimization | ADAPT TensorRT as optional derived cache | Serialized engines are platform-specific; they cannot be canonical package artifacts. https://docs.nvidia.com/deeplearning/tensorrt/10.x.x/getting-started/support-matrix.html |

