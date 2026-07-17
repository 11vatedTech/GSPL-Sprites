# Image and Atlas Architecture 0.1

Decoded working images use explicit dimensions, RGBA8 storage, named color
space, and alpha mode. Every codec receives input-byte, dimension, pixel, and
decoded-byte limits before allocation. The dependency-free PPM P6 codec is a
strict reference and test fixture format, not the production authoring format.

The production PNG adapter uses pinned libspng 0.7.4 and zlib 1.3.2 sources with
verified archive hashes. It enforces input bytes, dimensions, pixels, decoded
bytes, chunk size, chunk cache, chunk count, and critical CRC integrity before
or during decode. Untagged PNG data remains `unknown` and cannot enter an atlas
or be encoded as sRGB without an explicit transform/policy decision.
High-dynamic-range and layered
production formats will be isolated behind sandboxable codec processes.
OpenColorIO 2 governs transforms into named working spaces; untagged inputs
require an explicit policy decision and never silently assume a studio space.

Atlas packing is deterministic: validated frames are ordered by height, width,
then stable ID; placements are emitted in stable-ID order. All frames in one
atlas share color and alpha semantics. Animation clips reference stable frame
IDs, use integer ticks, and validate duration, reachability, referenced frames,
and event bounds. KTX 2.0/Basis is a derived target texture, never canonical
source imagery.

libspng 0.7.4's setter for `SPNG_CHUNK_COUNT_LIMIT` rejects positive overrides
before parsing because it compares them to the current parsed count. A bounded
container preflight therefore enforces the caller's actual chunk-count limit and
also requires a valid signature, complete chunk envelopes, a final zero-length
IEND, and no trailing bytes. Chunk byte and cache limits remain independently
enforced by libspng.
