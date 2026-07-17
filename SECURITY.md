# Security policy and initial threat boundary

Report vulnerabilities privately to the repository owner. Do not disclose an
unpatched issue publicly.

The compiler treats every seed, media file, archive, model, plugin, and package
as untrusted. The core has no network access. Source input is bounded to 1 MiB,
4,096 lines, 8 KiB per line, and 64 abilities per entity. Export is denied for
unknown, prohibited, restricted, and research-only rights classifications.

Future media ingestion must run decoders behind dimension, decoded-byte,
duration, recursion, and timeout limits. Archive extraction must reject absolute
paths, traversal, device paths, links, duplicate normalized paths, and expansion
bombs. Model descriptors are data only; model and plugin packages may not
execute arbitrary initialization code. Cache and package manifests require
content hashes and provenance verification before use.

