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
};

struct TransformationSeed {
  std::string id;
  std::string from_form;
  std::string to_form;
  std::string trigger_condition;
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

struct SpriteSeed {
  std::string schema;
  std::string stable_id;
  std::string name;
  std::string classification;
  RightsClass rights{RightsClass::unknown};
  std::uint64_t entropy_root{};
  std::string primary_color;
  std::string accent_color;
  std::vector<AbilitySeed> abilities;
  std::optional<RigDefinition> rig;
  std::vector<SkeletalClip> clips;
  std::optional<AnimationStateGraph> animation_graph;
  std::vector<CollisionShape> collision_shapes;
  std::vector<CollisionWindow> collision_windows;
  std::vector<FormSeed> forms;
  std::vector<TransformationSeed> transformations;
  std::map<std::string, MorphologyPart, std::less<>> morphology;
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

struct SpriteIr {
  std::string seed_identity;
  std::string entity_id;
  std::vector<AbilitySeed> abilities;
  std::string primary_color;
  std::string accent_color;
  std::optional<RigDefinition> rig;
  std::vector<SkeletalClip> clips;
  std::optional<AnimationStateGraph> animation_graph;
  std::vector<CollisionShape> collision_shapes;
  std::vector<CollisionWindow> collision_windows;
  std::vector<FormDefinition> form_definitions;
  std::vector<TransformationDelta> transformation_deltas;
  std::map<std::string, MorphologyPart, std::less<>> morphology;
  std::vector<AnimationIntent> animation_intents;
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
