# LOD quality architecture

Triangle-count reduction alone does not prove that a lower-detail mesh preserves
entity identity. The LOD-quality pass compares every lower level to LOD0 using
bounded symmetric point-to-triangle distance. Samples include all vertices and
triangle centroids in both directions, catching removed regions as well as
vertices displaced away from the source surface.

Every point-to-triangle evaluation counts against a configurable work budget.
The report records per-level and total comparisons plus maximum geometric error
in micrometers. Production policy also requires render-material compatibility by
default. Error, material mismatch, or exhausted work budget fails before target
export.

GLB output retains level number and minimum screen-coverage threshold as GSPL
node extras because core glTF does not prescribe LOD selection. Engine adapters
must translate that governed metadata into their native LOD system without
changing the thresholds silently.

Vertex-and-centroid distance is a bounded approximation rather than a formal
continuous Hausdorff proof. Future passes should add adaptive surface sampling,
normal/UV/material error, silhouette comparison, bone-weight transfer quality,
and animated correspondence across levels.
