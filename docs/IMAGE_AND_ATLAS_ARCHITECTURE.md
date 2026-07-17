# Image and Atlas Architecture 0.1

Decoded working images use explicit dimensions, RGBA8 storage, named color
space, and alpha mode. Every codec receives input-byte, dimension, pixel, and
decoded-byte limits before allocation. The dependency-free PPM P6 codec is a
strict reference and test fixture format, not the production authoring format.

The production PNG adapter will use libspng with dimension, decoded-size,
chunk-size, cache-size, and chunk-count limits. High-dynamic-range and layered
production formats will be isolated behind sandboxable codec processes.
OpenColorIO 2 governs transforms into named working spaces; untagged inputs
require an explicit policy decision and never silently assume a studio space.

Atlas packing is deterministic: validated frames are ordered by height, width,
then stable ID; placements are emitted in stable-ID order. All frames in one
atlas share color and alpha semantics. Animation clips reference stable frame
IDs, use integer ticks, and validate duration, reachability, referenced frames,
and event bounds. KTX 2.0/Basis is a derived target texture, never canonical
source imagery.

