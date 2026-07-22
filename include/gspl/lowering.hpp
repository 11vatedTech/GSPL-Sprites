#pragma once
#include "gspl/semantics.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/animation.hpp"
#include <string>
#include <vector>

namespace gspl {

struct LoweringDiagnostic {
    enum class Code {
        OK,
        RIGHTS_INVALID,
        COLOR_INVALID,
        FORM_UNREPRESENTABLE,
        TRANSFORMATION_INVALID,
        COLLISION_INVALID,
        ABILITY_INVALID,
        SEMANTIC_LOSS,
        INTERNAL_FAILURE,
        BONE_INVALID,
        ANIMATION_INVALID,
        RUNTIME_INVALID
    };
    Code code{Code::OK};
    std::string message;
    bool is_error() const { return code != Code::OK; }
};

// Direct lowering: CanonicalEntity → production gspl::sprites::SpriteIr
// This is the authoritative path. Bypasses SpriteSeed entirely.
class SpriteIrLowering {
public:
    struct Result {
        gspl::sprites::SpriteIr sprite_ir;
        std::vector<LoweringDiagnostic> diagnostics;
        bool ok() const {
            return std::none_of(diagnostics.begin(), diagnostics.end(),
                [](auto const& d) { return d.is_error(); });
        }
    };
    static Result lower(CanonicalEntity const& entity);
    static gspl::sprites::RightsClass lower_rights(std::string const& classification);
    static gspl::sprites::AbilitySeed lower_ability(CanonicalAbility const& ability);
    static gspl::sprites::MorphologyPart lower_part(CanonicalPart const& part);
    static gspl::sprites::FormAttributes lower_form_attributes(CanonicalForm const& form);
    static gspl::sprites::RuntimeAttributes lower_runtime(CanonicalRuntime const& runtime);
    static std::uint64_t parse_hex_color(std::string const& hex, std::vector<LoweringDiagnostic>& diags);
private:
    static gspl::sprites::BoneDefinition lower_bone(CanonicalSkeletalBone const& bone);
    static gspl::sprites::SocketDefinition lower_socket(CanonicalSocket const& socket);
    static gspl::sprites::SkeletalClip lower_clip(CanonicalAnimationClip const& clip);
    static gspl::sprites::AnimationState lower_state(CanonicalAnimationState const& state);
    static gspl::sprites::AnimationTransition lower_transition(CanonicalTransition const& trans);
    static gspl::sprites::CollisionShape lower_collision_shape(CanonicalCollisionShape const& shape);
    static gspl::sprites::CollisionWindow lower_collision_window(CanonicalCollisionWindow const& window);
    static gspl::sprites::Comparison lower_comparison(std::string const& comp);
};

// Compatibility lowering via SpriteSeed (for legacy testing and existing APIs)
class SpriteSeedLowering {
public:
    static gspl::sprites::SpriteSeed lower(CanonicalEntity const& entity);
    static gspl::sprites::RightsClass rights_to_enum(std::string const& classification);
    static std::uint64_t parse_hex_color(std::string const& hex);
private:
    static gspl::sprites::AbilitySeed lower_ability(CanonicalAbility const& ability);
    static gspl::sprites::FormSeed lower_form(CanonicalForm const& form);
    static gspl::sprites::TransformationSeed lower_transformation(CanonicalTransformation const& trans);
    static gspl::sprites::MorphologyPart lower_part(CanonicalPart const& part);
    static gspl::sprites::FormAttributes lower_form_attributes(CanonicalForm const& form);
    static gspl::sprites::RuntimeAttributes lower_runtime(CanonicalRuntime const& runtime);
};

} // namespace gspl
