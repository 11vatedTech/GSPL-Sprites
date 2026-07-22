#pragma once
#include "gspl/ast.hpp"
#include "gspl/genes.hpp"
#include "gspl/types.hpp"
#include "gspl/diagnostics.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace gspl {

struct CanonicalPart {
    std::string name;
    std::string parent;
    double x{}, y{}, z{};
    double size_x{1}, size_y{1}, size_z{1};
    std::string color;
    double rotation_degrees{};
    bool emissive{};
    bool electrical_marking{};
};

struct CanonicalAbility {
    std::string id;
    std::string effect;
    std::uint32_t cost{};
    std::uint32_t cooldown_ticks{};
    std::uint32_t active_ticks{};
    std::string origin_socket;
    double speed_mm_per_tick{};
    double collision_radius_mm{};
    std::string status_id;
    std::uint32_t status_duration_ticks{};
};

struct CanonicalForm {
    std::string id;
    std::vector<std::string> transformation_ids;
    std::uint32_t resource_capacity{100};
    double collision_scale{1.0};
    double ability_envelope{1.0};
    std::uint32_t max_health{100};
};

struct CanonicalTransformation {
    std::string id;
    std::string from_form;
    std::string to_form;
    std::string trigger_condition;
    std::uint32_t duration_ticks{};
    std::uint32_t resource_cost{};
};

struct CanonicalAnimationClip {
    std::string name;
    bool loop{};
    struct Track {
        std::string bone;
        std::vector<std::pair<std::uint32_t, std::string>> keys;
    };
    std::vector<Track> tracks;
    std::vector<std::pair<std::uint32_t, std::string>> clip_events;
};

struct CanonicalAnimationState {
    std::string name;
    std::string clip_name;
};

struct CanonicalTransition {
    std::string from_state;
    std::string to_state;
    std::string ability_id;
    std::string comparison; // "LESS", "GREATER_EQUAL", "EQUAL", etc.
    std::uint32_t threshold{};
    std::uint32_t resource_cost{};
    std::uint32_t cooldown_ticks{};
};

struct CanonicalCollisionShape {
    std::string id;
    std::string shape_type; // "CIRCLE", "AXIS_ALIGNED_BOX"
    std::string socket;
    double radius_mm{};
    double offset_x{}, offset_y{};
    double scale_x{1}, scale_y{1};
};

struct CanonicalCollisionWindow {
    std::string ability_id;
    std::string shape_id;
    std::uint32_t start_tick{};
    std::uint32_t duration_ticks{};
    bool active{};
};

struct CanonicalSkeletalBone {
    std::string id;
    std::string parent;
    double x{}, y{}, z{};
    double scale_x{1}, scale_y{1};
    double length_mm{};
    double min_rotation{}, max_rotation{};
};

struct CanonicalSocket {
    std::string id;
    std::string bone;
    double x{}, y{}, z{};
    double scale_x{1}, scale_y{1};
};

struct CanonicalAnimationIntent {
    std::string behavior_state;
    std::string clip_name;
};

struct CanonicalRuntime {
    std::uint32_t aggression{50};
    std::uint32_t curiosity{50};
    std::uint32_t energy{50};
    std::uint32_t loyalty{50};
    std::vector<CanonicalAnimationIntent> animation_intents;
};

struct CanonicalResource {
    std::string id;
    std::string resource_type;
    std::uint32_t min{};
    std::uint32_t max{100};
    std::uint32_t initial{100};
};

struct CanonicalEntity {
    /* PRESERVED */ std::string stable_id;
    /* PRESERVED */ std::string name;
    /* PRESERVED */ std::string classification;
    /* PRESERVED */ std::string rights;
    /* PRESERVED */ bool rights_allow_export{true};
    /* PRESERVED */ std::uint64_t entropy_root{};
    /* PRESERVED */ std::string primary_color;
    /* PRESERVED */ std::string accent_color;
    /* PRESERVED */ std::string storm_primary_color;
    /* PRESERVED */ std::string storm_accent_color;
    /* PRESERVED */ std::string emissive_color;
    /* PRESERVED */ std::string aura_color;
    /* PRESERVED */ std::vector<CanonicalForm> forms;
    /* PRESERVED */ std::vector<CanonicalTransformation> transformations;
    /* PRESERVED */ std::map<std::string, CanonicalPart, std::less<>> morphology;
    /* PRESERVED */ std::map<std::string, std::map<std::string, CanonicalPart, std::less<>>, std::less<>> form_morphology_overrides;
    /* PRESERVED */ std::vector<CanonicalAbility> abilities;
    /* PRESERVED */ std::vector<CanonicalAbility> storm_abilities;
    /* PRESERVED */ std::vector<CanonicalSkeletalBone> bones;
    /* PRESERVED */ std::vector<CanonicalSocket> sockets;
    /* PRESERVED */ std::vector<CanonicalAnimationClip> clips;
    /* PRESERVED */ std::vector<CanonicalAnimationState> states;
    /* PRESERVED */ std::vector<CanonicalTransition> transitions;
    /* PRESERVED */ std::string initial_state;
    /* PRESERVED */ std::vector<CanonicalCollisionShape> collision_shapes;
    /* PRESERVED */ std::vector<CanonicalCollisionWindow> collision_windows;
    /* PRESERVED */ std::optional<CanonicalRuntime> runtime;
    /* PRESERVED */ std::vector<CanonicalResource> resources;
    /* PRESERVED */ std::vector<GeneInstance> genes;
    /* PRESERVED */ std::string provenance_hash;
    /* PRESERVED */ std::string provenance_source;
    /* PRESERVED */ std::string schema_version{"gspl.canonical-entity/1.0"};

    DiagnosticResult diagnostics;
};

class CanonicalEntitySerializer {
public:
    static std::string to_json(CanonicalEntity const& entity);
    static std::optional<CanonicalEntity> from_json(std::string_view json, DiagnosticResult& diag);
    static std::string to_yaml(CanonicalEntity const& entity);
};

class CanonicalEntityValidator {
public:
    DiagnosticResult validate(CanonicalEntity const& entity) const;
private:
    DiagnosticResult check_identity(CanonicalEntity const& entity) const;
    DiagnosticResult check_rights(CanonicalEntity const& entity) const;
    DiagnosticResult check_forms(CanonicalEntity const& entity) const;
    DiagnosticResult check_transformations(CanonicalEntity const& entity) const;
    DiagnosticResult check_morphology(CanonicalEntity const& entity) const;
    DiagnosticResult check_animation(CanonicalEntity const& entity) const;
    DiagnosticResult check_collision(CanonicalEntity const& entity) const;
    DiagnosticResult check_runtime(CanonicalEntity const& entity) const;
};

// identity: deterministic hash excluding memory/process-dependent state
class CanonicalEntityIdentity {
public:
    explicit CanonicalEntityIdentity(CanonicalEntity const& entity);
    std::string hash() const { return hash_; }
    std::string serialized() const { return serialized_; }
private:
    std::string compute(CanonicalEntity const& entity) const;
    std::string hash_;
    std::string serialized_;
};

class CanonicalEntityDiff {
public:
    struct Entry {
        std::string field;
        std::string before;
        std::string after;
        bool changed{};
    };
    static std::vector<Entry> diff(CanonicalEntity const& before, CanonicalEntity const& after);
    static std::string to_text(std::vector<Entry> const& entries);
    static bool identical(std::vector<Entry> const& entries);
};

// Canonicalizer: consumes resolved typed AST + gene instances → CanonicalEntity
class Canonicalizer {
public:
    explicit Canonicalizer(SourceManager const& sources);
    CanonicalEntity lower(ModuleDecl const& module, std::vector<GeneInstance> const& genes);
    DiagnosticResult const& diagnostics() const { return diag_; }
private:
    SourceManager const& sources_;
    DiagnosticResult diag_;
    void lower_entity(EntityDecl const& entity, CanonicalEntity& out);
    void lower_gene_decl(GeneDecl const& gene, CanonicalEntity& out);
    void lower_form(FormDecl const& form, CanonicalEntity& out);
    void lower_transformation(TransformationDecl const& trans, CanonicalEntity& out);
    void lower_morphology(MorphologyDecl const& morph, CanonicalEntity& out);
    void lower_ability(AbilityDecl const& ability, CanonicalEntity& out);
    void lower_rights(RightsDecl const& rights, CanonicalEntity& out);
    void lower_resource(ResourceDecl const& resource, CanonicalEntity& out);
    void apply_genes(std::vector<GeneInstance> const& genes, CanonicalEntity& out);
};

} // namespace gspl
