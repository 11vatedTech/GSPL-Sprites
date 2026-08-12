#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/markings.hpp"
#include "gspl_sprites/performance.hpp"
#include "gspl_sprites/visual_common.hpp"
#include "gspl_sprites/visual_geometry.hpp"
#include "gspl_sprites/visual_morphology.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Visual Canon ──
 * The authoritative machine-readable description of how an entity becomes
 * visually recognizable: structure, proportions, landmarks, silhouette,
 * surfaces, materials, colors, facial regions, markings, attachments,
 * deformation limits, resolution behavior and identity invariants.
 *
 * The canon is a DERIVED manifestation-level authority (Visual
 * Manifestation Identity taxonomy). It never replaces CanonicalEntity;
 * entities differ by canon DATA, never by compiler branches. See
 * docs/architecture/VISUAL_CANON_ARCHITECTURE.md. */

enum class StructureKind { region, element, joint, mass, appendage };
[[nodiscard]] std::string_view structure_kind_name(StructureKind kind) noexcept;
[[nodiscard]] std::optional<StructureKind> structure_kind_from_name(std::string_view name) noexcept;

enum class RelationshipKind {
  parent, child, attachment, articulation, adjacency,
  symmetry, correspondence, containment, overlap, surface_ownership
};
[[nodiscard]] std::string_view relationship_kind_name(RelationshipKind kind) noexcept;
[[nodiscard]] std::optional<RelationshipKind> relationship_kind_from_name(std::string_view name) noexcept;

struct CanonRelationship {
  RelationshipKind kind{RelationshipKind::adjacency};
  std::string target;
};

struct CanonStructure {
  std::string id;
  StructureKind kind{StructureKind::element};
  std::string parent;                 // "" = root
  std::string role;                   // extensible semantic role
  std::string primitive{"ellipse"};   // ellipse/capsule/rounded_rect/polygon/ring/open_path/closed_path
  std::string layer{"body"};          // visual layer (see layer_from_name)
  std::string material_class{"skin"};
  std::string color_role{"primary"};
  double x{}, y{}, z{};
  double size_x{1.0};
  double size_y{1.0};
  double size_z{1.0};
  double rotation_degrees{};
  std::int32_t z_order{};
  bool emissive{};
  bool silhouette_contribution{true};
  bool support_capable{};             // declared ground/flight support contact (data-driven)
  std::string bone_id;
  std::string socket_id;
  std::string projection_behavior{"default"};
  std::string render_group;
  std::vector<CanonRelationship> relationships;
};

/* Proportion measure: either the distance between two landmarks
 * ("landmark_dist:<a>:<b>") or a part axis size ("size:<part>:<axis>"). */
[[nodiscard]] std::string make_landmark_dist_measure(std::string_view a, std::string_view b);
[[nodiscard]] std::string make_size_measure(std::string_view part, std::string_view axis);

/* ── Relational Construction Constraints ──
 * Canonical identity is authored primarily through typed relationships
 * (anchor, size ratio, symmetry, orientation, chain), NOT absolute pixel
 * coordinates. Absolute x/y/z in CanonStructure are classified projection
 * hints/root anchors; when a structure participates in a construction
 * constraint, the constraint DERIVES the geometry instead. The solver is
 * deterministic (fixed iteration order by constraint id + target id) and
 * fails closed on contradictions. See docs/architecture/VISUAL_CANON_ARCHITECTURE.md. */

enum class ConstructionAxis { x, y, z };
[[nodiscard]] std::string_view construction_axis_name(ConstructionAxis axis) noexcept;
[[nodiscard]] std::optional<ConstructionAxis> construction_axis_from_name(std::string_view name) noexcept;

enum class ConstructionConstraintKind {
  anchor,         // position = reference position + offset
  size_ratio,     // size = reference size * factor (generative proportion)
  symmetry,       // position mirrors reference across a parent-local axis plane
  orientation,    // rotation = reference rotation + offset
  chain,          // position = reference position + reference_size * factor (extend along axis)
};
[[nodiscard]] std::string_view construction_kind_name(ConstructionConstraintKind kind) noexcept;
[[nodiscard]] std::optional<ConstructionConstraintKind> construction_kind_from_name(std::string_view name) noexcept;

struct ConstructionConstraint {
  std::string id;
  ConstructionConstraintKind kind{ConstructionConstraintKind::size_ratio};
  std::string structure;   // constrained structure id
  std::string reference;   // reference structure id
  ConstructionAxis axis{ConstructionAxis::x};  // x/y/z (symmetry: mirror plane normal)
  // Optional distinct axis for the reference measure (cross-axis ratios such
  // as "target size_x derived from reference size_y"). Empty = same axis.
  std::optional<ConstructionAxis> reference_axis;
  double factor{1.0};
  double offset{0.0};
  bool hard{true};         // contradiction fails closed; soft resolves via deterministic precedence
  std::string scope;       // optional "form:<id>" restriction
};

struct VisualCanon;  // forward declaration (solver operates on canon)

/* Deterministic construction: applies all applicable construction
 * constraints to the canon structures and returns the derived geometry
 * plus any diagnostics. Contradictory hard constraints fail closed.
 * This is the generative core behind canon_to_morphology. */
[[nodiscard]] ValidationResult solve_construction_constraints(
    const VisualCanon& canon, std::string_view form_id);

/* Structural chain: ordered list of ancestor ids from the named structure
 * up to (not including) the root, deterministically. */
[[nodiscard]] std::vector<std::string> structural_chain(const VisualCanon& canon,
                                                        std::string_view structure_id);

struct ProportionRule {
  std::string id;
  std::string numerator;    // measure string
  std::string denominator;  // measure string
  double preferred{1.0};
  double min{0.0};
  double max{std::numeric_limits<double>::infinity()};
  double deformation_min{0.0};
  double deformation_max{std::numeric_limits<double>::infinity()};
  bool hard{};
  // Generative proportion: when set, construction DERIVES this target from
  // the rule (target = denominator * preferred). "size:<part>:<axis>" only
  // (distance-based rules stay validation-only). Changing `preferred`
  // changes constructed geometry deterministically.
  std::string derived;      // "size:<part>:<axis>" or "" = measurement only
  bool derived_hard{true};  // derived value is identity-critical
  std::string scope;        // optional "form:<id>" or "style:<name>" restriction
};

struct VisualLandmark {
  std::string id;
  std::string role;         // extensible: eye_center, ear_tip, nose_tip, wheel_center, ...
  std::string owner;        // canon structure id
  double ox{}, oy{}, oz{};  // offset in owner local frame
  std::string symmetry;     // partner landmark id ("")
  bool silhouette_anchor{};
  bool required{};
  double deformability{1.0};
  std::vector<std::string> ordering_after;  // landmark ids that must precede this one
};

enum class SilhouetteFeatureKind { mass, anchor, protrusion, concavity, negative_space };
[[nodiscard]] std::string_view silhouette_feature_kind_name(SilhouetteFeatureKind kind) noexcept;
[[nodiscard]] std::optional<SilhouetteFeatureKind> silhouette_feature_kind_from_name(std::string_view name) noexcept;

struct SilhouetteFeature {
  std::string id;
  SilhouetteFeatureKind kind{SilhouetteFeatureKind::mass};
  std::uint32_t priority{};
  std::string structure_ref;
  double min_contribution{0.0};  // minimum silhouette share (0..1)
  double min_separation{0.0};    // minimum gap for negative spaces
};

struct SurfaceRegion {
  std::string id;
  std::string structure_ref;
  std::string role;             // extensible semantic surface area
};

struct MaterialBinding {
  std::string id;
  std::string structure_ref;
  std::string material_class;   // fur/skin/metal/glass/electricity/...
  std::string material_id;      // "" = auto
};

struct ColorRegion {
  std::string id;
  std::string structure_ref;
  std::string color_role;       // canonical role name (see color_role_from_name)
  std::string form_id;          // "" = all forms
  std::string hex;              // optional concrete "#rrggbb"
};

enum class ExpressiveFeatureKind {
  eye, brow, mouth, visor, antenna, light, panel, ear, tail, custom
};
[[nodiscard]] std::string_view expressive_feature_kind_name(ExpressiveFeatureKind kind) noexcept;
[[nodiscard]] std::optional<ExpressiveFeatureKind> expressive_feature_kind_from_name(std::string_view name) noexcept;

struct ExpressiveFeature {
  std::string id;
  ExpressiveFeatureKind kind{ExpressiveFeatureKind::eye};
  std::string region_id;
  double gaze{};         // -1..1 lateral gaze
  double aperture{1.0};  // 0..1 openness
  double intensity{0.0}; // 0..1 channel intensity (light/emission)
  double rotation{};     // degrees
  double squash{1.0};    // facial mass squash factor
};

/* ── Emotion response (data-driven expressive mapping) ──
 * A canon may map a semantic emotion/expression name to concrete
 * ExpressiveFeature parameter overrides. Organisms move eyes/ears/mouth;
 * mechanical entities move visor/antenna/panel channels — through the SAME
 * generic machinery (feature ids resolve via the canonical feature graph).
 * Unknown emotions simply produce no overrides (feature defaults apply). */
struct EmotionResponse {
  std::string feature_id;   // ExpressiveFeature id within the canon
  double gaze{};            // -1..1 lateral gaze override
  double aperture{1.0};     // 0..1 openness override
  double intensity{0.0};    // 0..1 channel intensity override
  double rotation{};        // degrees override
  double squash{1.0};       // mass squash override
};

struct ExpressiveRegion {
  std::string id;
  std::string structure_ref;
  std::vector<ExpressiveFeature> features;
};

struct MarkingBinding {
  std::string id;
  MarkingKind kind{MarkingKind::stripe};
  std::string structure_ref;
  std::string color_role{"custom"};
  double opacity{1.0};
  double scale{1.0};
  std::string form_id;  // "" = all forms
  std::string intent;   // extensible: "identity", "warning", "energy", ...
};

struct AttachmentPoint {
  std::string id;
  std::string structure_ref;
  std::string socket_id;
  std::string role;
};

/* ── Typed deformation envelope ──
 * Dimensional correctness: translation (world units), rotation (degrees)
 * and scale (dimensionless ratio) each carry their own bound instead of
 * one scalar compared against every channel. per-axis values default to
 * `allowed_deviation` when 0 (legacy scalar semantics preserved). */
struct DeformationEnvelope {
  std::string structure_id;
  double allowed_deviation{0.0};     // legacy scalar bound (default for all axes)
  double allowed_translation{0.0};   // world units; 0 = allowed_deviation
  double allowed_rotation{0.0};      // degrees;    0 = allowed_deviation
  double allowed_scale{0.0};         // |scale-1|;  0 = allowed_deviation
  double action_scale{1.0};          // action-driven multiplier
  double expression_scale{1.0};      // expression-driven multiplier
  double style_scale{1.0};           // style-driven multiplier
  double transformation_scale{1.0};  // transformation-driven multiplier
  double hard_translation{std::numeric_limits<double>::infinity()};
  double hard_rotation{std::numeric_limits<double>::infinity()};
  double hard_scale{std::numeric_limits<double>::infinity()};
  bool rigid{};                      // identity-critical: never exceeds boundary
  bool volume_preserving{};          // request volume conservation
};

struct VisualFeature {
  std::string id;
  std::string structure_ref;
  std::uint32_t semantic_priority{};
  double recognition_importance{1.0};
  std::uint32_t min_resolution{};    // minimum canvas size at which this survives
  std::string substitution_rule;     // "merge:<parent>" | "omit" | ""
  std::string merge_rule;            // "merge" | ""
  std::string omission_rule;         // "omit" | ""
};

enum class IdentityInvariantKind {
  proportion, landmark_ordering, landmark_existence, silhouette_anchor,
  attachment, color_role_topology, material_truth, marking_topology,
  hard_boundary
};
[[nodiscard]] std::string_view identity_invariant_kind_name(IdentityInvariantKind kind) noexcept;
[[nodiscard]] std::optional<IdentityInvariantKind> identity_invariant_kind_from_name(std::string_view name) noexcept;

/* Severity semantics: advisory = informational (never fails), soft = quality
 * violation (diagnostic only, does NOT fail compilation), hard = identity
 * violation (fails closed). The `hard` field maps to severity==hard. */
enum class InvariantSeverity { advisory, soft, hard };
[[nodiscard]] std::string_view invariant_severity_name(InvariantSeverity severity) noexcept;
[[nodiscard]] std::optional<InvariantSeverity> invariant_severity_from_name(std::string_view name) noexcept;

struct IdentityInvariant {
  std::string id;
  IdentityInvariantKind kind{IdentityInvariantKind::proportion};
  std::vector<std::string> refs;   // proportion ids / landmark ids / structure ids
  double tolerance{0.0};
  InvariantSeverity severity{InvariantSeverity::hard};  // single authority
  std::string scope;               // optional "form:<id>" restriction
};

/* ── Support policy (data-driven balance/support reasoning) ──
 * Declares how an entity relates to its environment for support analysis:
 *   grounded          — ground contact via declared support_capable structures
 *   flight            — airborne; no ground-support analysis
 *   buoyant           — fluid medium; no ground-support analysis
 *   free              — zero-gravity/abstract; no support analysis
 *   auto              — infer from declared support_capable structures
 * Never derived from appendage roles (ears/tails/antennae are not support). */
enum class SupportPolicy { auto_, grounded, flight, buoyant, free };
[[nodiscard]] std::string_view support_policy_name(SupportPolicy policy) noexcept;
[[nodiscard]] std::optional<SupportPolicy> support_policy_from_name(std::string_view name) noexcept;

struct VisualCanon {
  std::string schema{"gspl.visual-canon/0.1"};
  std::string entity_id;
  std::string name;
  std::string rights_class;                 // canonical rights name (provenance readability)
  std::string provenance;                   // free-form provenance note
  std::vector<std::string> forms;           // form ids (base first)
  std::map<std::string, CanonStructure, std::less<>> structures;
  std::vector<ConstructionConstraint> construction;  // relational construction (generative)
  std::vector<ProportionRule> proportions;
  std::map<std::string, VisualLandmark, std::less<>> landmarks;
  std::vector<SilhouetteFeature> silhouette_features;
  std::vector<SurfaceRegion> surfaces;
  std::vector<MaterialBinding> materials;
  std::vector<ColorRegion> color_regions;
  std::vector<ExpressiveRegion> facial_regions;
  std::vector<MarkingBinding> markings;
  std::vector<AttachmentPoint> attachments;
  std::map<std::string, DeformationEnvelope, std::less<>> deformation_envelopes;
  std::vector<VisualFeature> resolution_features;
  std::vector<IdentityInvariant> identity_invariants;
  std::string gaze_driver;              // structure id that orients toward gaze ("" = auto-detect)
  std::string attention_driver;          // structure id for attention/awareness ("" = gaze_driver)
  SupportPolicy support_policy{SupportPolicy::auto_};  // typed (validated closed set)
  // Emotion name -> typed expressive channel overrides (data-driven; generic
  // across organisms and mechanical entities).
  std::map<std::string, std::vector<EmotionResponse>, std::less<>> emotion_responses;
  // Role-name -> concrete color palettes (deterministic; roles never vanish).
  std::map<std::string, std::string, std::less<>> base_palette;   // role -> "#rrggbb"
  std::map<std::string, std::map<std::string, std::string, std::less<>>, std::less<>> form_palettes; // form -> role -> hex
};

struct CanonLimits {
  std::uint32_t max_structures{512};
  std::uint32_t max_proportions{512};
  std::uint32_t max_construction{512};
  std::uint32_t max_landmarks{512};
  std::uint32_t max_features{512};
  std::uint32_t max_markings{256};
  std::uint32_t max_invariants{256};
};

/* Validation fails closed: cycles, dangling refs, nonfinite values,
 * inverted ranges, unknown enum spellings. */
[[nodiscard]] ValidationResult validate_visual_canon(const VisualCanon& canon,
                                                     const CanonLimits& limits = {});
[[nodiscard]] std::string canonicalize_visual_canon(const VisualCanon& canon);
[[nodiscard]] std::optional<VisualCanon> parse_visual_canon(std::string_view text,
                                                            const CanonLimits& limits = {});
[[nodiscard]] std::string visual_canon_identity(const VisualCanon& canon);

/* ── Canon → morphology (deterministic construction) ──
 * Builds the form's VisualMorphologyV2 from canon structures. Geometry is
 * derived through the Relational Construction System (construction
 * constraints, generative proportions); absolute coordinates are only
 * projection hints/root anchors used when no constraint applies. The
 * body_scale maps canonical units to canvas units. No entity-specific
 * behavior. */
[[nodiscard]] VisualMorphologyV2 canon_to_morphology(const VisualCanon& canon,
                                                     std::string_view form_id,
                                                     double body_scale = 1.0);

/* Compatibility: derive a canon from an existing v2 morphology so any
 * content can flow through canon-driven compilation without semantic
 * reinterpretation. */
[[nodiscard]] VisualCanon default_canon_from_morphology(const VisualMorphologyV2& morph,
                                                        std::string_view entity_id);

/* ── Canon measure / landmark resolution ──
 * Deterministic geometry queries over canon + morphology. */
[[nodiscard]] std::optional<Vec2> resolve_landmark_position(const VisualCanon& canon,
                                                            const VisualMorphologyV2& morph,
                                                            std::string_view landmark_id);
[[nodiscard]] std::optional<double> measure_value(const VisualCanon& canon,
                                                  const VisualMorphologyV2& morph,
                                                  std::string_view measure);
[[nodiscard]] std::optional<double> proportion_value(const VisualCanon& canon,
                                                     const VisualMorphologyV2& morph,
                                                     std::string_view proportion_id);

/* ── Deformation enforcement ──
 * Clamps performance motions per envelope. Rigid structures or hard
 * boundaries that are exceeded fail closed (diagnostics). Deviation
 * beyond allowed (but inside the boundary) is permitted and reported. */
[[nodiscard]] ValidationResult enforce_deformation_envelopes(const VisualCanon& canon,
                                                             PerformanceState& perf,
                                                             std::string_view action_phase = {},
                                                             double expression = 0.0,
                                                             double style_factor = 1.0,
                                                             double transformation_factor = 1.0);

/* ── Identity invariant checks ──
 * Validates proportion/landmark/silhouette/material/color invariants
 * against the constructed morphology. Hard invariants fail closed;
 * severity is typed (DiagnosticSeverity) — never encoded in the code string. */
[[nodiscard]] ValidationResult check_identity_invariants(const VisualCanon& canon,
                                                         const VisualMorphologyV2& morph);

/* ── Facial/expressive canon application ──
 * expression_motions: collect expression-driven PartMotions for the facial
 * canon BEFORE the deformation enforcement gate. Returns aperture, gaze,
 * rotation, etc. as motions; does NOT mutate morph. Consumed by the compiler
 * to close the post-gate expression bypass. Emotion (if provided) resolves
 * through canon.emotion_responses (data-driven) before feature defaults. */
[[nodiscard]] std::vector<PartMotion> expression_motions(const VisualCanon& canon,
                                                         std::string_view emotion,
                                                         double gaze_x, double gaze_y);

/* ── Semantic LOD ──
 * Executes authored resolution rules (semantic_priority, recognition_importance,
 * min_resolution, substitution_rule, merge_rule, omission_rule) on the
 * constructed morphology for a target resolution. Identity-critical features
 * (high recognition_importance or priority 0) are preserved even when their
 * min_resolution is crossed; authored rules decide omit/merge/substitute for
 * the rest. Deterministic; diagnostics report every action. */
[[nodiscard]] ValidationResult apply_semantic_lod(const VisualCanon& canon,
                                                  VisualMorphologyV2& morph,
                                                  std::uint32_t target_resolution);

} // namespace gspl::sprites::visual
