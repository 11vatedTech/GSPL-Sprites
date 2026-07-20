#pragma once

#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/common.hpp"
#include "gspl_sprites/sprite2d.hpp"
#include "gspl_sprites/visual_set.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites {

enum class RightsClass {
  original_user_creation,
  user_owned,
  licensed,
  public_domain,
  permissive,
  research_only,
  restricted,
  unknown,
  prohibited
};
enum class RuntimeState { idle, observing, charging, attacking, recovering };

struct AbilitySeed {
  std::string id;
  std::string effect;
  std::uint32_t cost{};
  std::uint32_t cooldown_ticks{};
  std::uint32_t active_ticks{};
};

struct MorphologyPart {
  double x{};
  double y{};
  double z{};
  double size_x{1.0};
  double size_y{1.0};
  double size_z{1.0};
  std::string color;
  double rotation_degrees{};
  std::string parent;
  bool emissive{};
  bool electrical_marking{};
};

struct TransformationSeed {
  std::string id;
  std::string from_form;
  std::string to_form;
  std::string trigger_condition;
  std::uint32_t duration_ticks{};
  std::uint32_t resource_cost{};
};

struct FormSeed {
  std::string id;
  std::vector<std::string> transformation_ids;
};

struct RuntimeAttributes {
  std::uint32_t aggression{50};
  std::uint32_t curiosity{50};
  std::uint32_t energy{50};
  std::uint32_t loyalty{50};
  std::vector<std::pair<std::string, std::string>> animation_intents;
};

struct FormAttributes {
  std::uint32_t resource_capacity{100};
  double collision_scale{1.0};
  double ability_envelope{1.0};
  std::uint32_t max_health{100};
};

struct SpriteSeed {
  std::string schema;
  std::string stable_id;
  std::string name;
  std::string classification;
  RightsClass rights{RightsClass::unknown};
  std::uint64_t entropy_root{};
  std::string primary_color;
  std::string accent_color;
  std::string storm_primary_color;
  std::string storm_accent_color;
  std::string emissive_color;
  std::string aura_color;
  std::vector<AbilitySeed> abilities;
  std::vector<AbilitySeed> storm_abilities;
  std::optional<RigDefinition> rig;
  std::vector<SkeletalClip> clips;
  std::optional<AnimationStateGraph> animation_graph;
  std::vector<CollisionShape> collision_shapes;
  std::vector<CollisionWindow> collision_windows;
  std::vector<FormSeed> forms;
  std::map<std::string, FormAttributes> form_attributes;
  std::vector<TransformationSeed> transformations;
  std::map<std::string, MorphologyPart, std::less<>> morphology;
  std::map<std::string, std::map<std::string, MorphologyPart, std::less<>>, std::less<>> form_morphology_overrides;
  std::optional<RuntimeAttributes> runtime;
};

struct FormDefinition {
  std::string name;
  std::vector<std::string> transformation_names;
};

struct TransformationDelta {
  std::string name;
  std::string from_form;
  std::string to_form;
  std::string trigger_condition;
};

struct AnimationIntent {
  std::string behavior_state;
  std::string clip_name;
};

struct ProjectileDefinition {
  std::string id;
  std::string origin_socket;
  double speed_mm_per_tick{};
  double collision_radius_mm{};
  std::uint32_t damage{};
  std::string status_id;
  std::uint32_t status_duration_ticks{};
};

struct SpriteIr {
  std::string seed_identity;
  std::string entity_id;
  std::string name;
  std::string classification;
  RightsClass rights{RightsClass::unknown};
  std::string provenance_hash;
  std::string schema_version{"gspl.sprite-seed/0.2"};
  std::vector<AbilitySeed> abilities;
  std::string primary_color;
  std::string accent_color;
  std::string storm_primary_color;
  std::string storm_accent_color;
  std::string emissive_color;
  std::string aura_color;
  std::optional<RigDefinition> rig;
  std::vector<SkeletalClip> clips;
  std::optional<AnimationStateGraph> animation_graph;
  std::vector<CollisionShape> collision_shapes;
  std::vector<CollisionWindow> collision_windows;
  std::vector<FormDefinition> form_definitions;
  std::map<std::string, FormAttributes> form_attributes;
  std::vector<TransformationDelta> transformation_deltas;
  std::map<std::string, MorphologyPart, std::less<>> morphology;
  std::map<std::string, std::map<std::string, MorphologyPart, std::less<>>, std::less<>> form_morphology_overrides;
  std::vector<AnimationIntent> animation_intents;
  std::vector<ProjectileDefinition> projectiles;
  std::vector<AbilitySeed> storm_abilities;
  RuntimeAttributes runtime;
};

struct RuntimeEntity {
  RuntimeState state{RuntimeState::idle};
  std::uint32_t energy{100};
  std::uint32_t remaining_ticks{};
  std::uint32_t cooldown_ticks{};
};

struct PackageGovernanceEvidence {
  std::string authoring_provenance_json{"{\"project\":null,\"references\":[]}"};
  std::string target_compatibility_json{"{\"reports\":[]}"};
};

[[nodiscard]] SpriteSeed parse_seed(std::string_view source);
[[nodiscard]] ValidationResult validate(const SpriteSeed &seed);
[[nodiscard]] std::string canonicalize(const SpriteSeed &seed);
[[nodiscard]] std::string sha256(std::string_view input);
[[nodiscard]] SpriteIr compile(const SpriteSeed &seed);
[[nodiscard]] bool activate(RuntimeEntity &entity, const AbilitySeed &ability);
void tick(RuntimeEntity &entity) noexcept;
[[nodiscard]] std::string render_svg(const SpriteIr &ir);
void build_package(const SpriteSeed &seed, const std::filesystem::path &output);
void build_package(const SpriteSeed &seed,
                   const PackageGovernanceEvidence &governance,
                   const std::filesystem::path &output);
void build_package(const SpriteSeed &seed, std::span<const FrameSource> frames,
                   const SpriteSheetOptions &options,
                   const std::filesystem::path &output);
void build_package(const SpriteSeed &seed, const AuthoredVisualSet &visual_set,
                   const std::filesystem::path &output);

} // namespace gspl::sprites
