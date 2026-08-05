#include "gspl_sprites/rights.hpp"
#include "gspl_sprites/morphology.hpp"
#include "gspl_sprites/identity.hpp"
#include "gspl_sprites/control.hpp"
#include "gspl_sprites/viewer_model.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl/ir_authority.hpp"
#include "gspl_sprites/animation.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;
static void check(bool v, const char* msg) {
  if (!v) { std::fprintf(stderr, "FAIL: %s\n", msg); ++failures; }
  else { std::fprintf(stdout, "PASS: %s\n", msg); }
}

int main() {
  using namespace gspl::sprites;

  /* ── RIGHTS PARITY ── */
  for (RightsClass c : {RightsClass::original_user_creation, RightsClass::user_owned,
                         RightsClass::licensed, RightsClass::public_domain,
                         RightsClass::permissive, RightsClass::research_only,
                         RightsClass::restricted, RightsClass::unknown,
                         RightsClass::prohibited}) {
    auto name = rights_class_to_string(c);
    auto back = rights_class_from_string(name);
    check(back.has_value() && *back == c, "rights round-trip");
    check(name.find("ORIGINAL") != std::string_view::npos ||
          name.find("USER_OWNED") != std::string_view::npos ||
          name.find("LICENSED") != std::string_view::npos ||
          name.find("PUBLIC_DOMAIN") != std::string_view::npos ||
          name.find("PERMISSIVE") != std::string_view::npos ||
          name.find("RESEARCH_ONLY") != std::string_view::npos ||
          name.find("RESTRICTED") != std::string_view::npos ||
          name.find("UNKNOWN_RIGHTS") != std::string_view::npos ||
          name.find("PROHIBITED") != std::string_view::npos,
          "rights name recognized");
  }
  check(!rights_class_from_string("nonsense").has_value(), "rights: unknown lowercase");
  check(!rights_class_from_string("").has_value(), "rights: empty string");
  check(lower_rights_class("ORIGINAL_USER_CREATION/NCC-1701") == RightsClass::original_user_creation,
        "rights: lower with registry suffix");
  check(lower_rights_class("garbage") == RightsClass::unknown, "rights: lower unknown");

  /* ── IDENTITY TAXONOMY ── */
  check(kIdentityTaxonomy.size() == 7, "identity taxonomy: size");
  for (int i = 0; i < 7; ++i) {
    auto desc = identity_descriptor(static_cast<IdentityKind>(i));
    check(desc != nullptr, "identity descriptor exists");
    check(desc->kind == static_cast<IdentityKind>(i), "identity descriptor kind matches");
  }
  check(identity_descriptor(IdentityKind::semantic_entity)->immutable, "semantic entity immutable");
  check(identity_descriptor(IdentityKind::semantic_entity)->serialized, "semantic entity serialized");
  check(identity_descriptor(IdentityKind::runtime_state)->changes_during_runtime, "runtime changes");

  /* ── IR AUTHORITY ── */
  check(gspl::ir_path_role_name(gspl::IrPathRole::canonical) == "canonical", "ir: canonical role name");
  check(gspl::ir_path_role_name(gspl::IrPathRole::compatibility) == "compatibility", "ir: compatibility role");
  check(gspl::ir_path_role_name(gspl::IrPathRole::fixture) == "fixture", "ir: fixture role");

  /* ── LEGACY PATH CLASSIFICATION ── */
  check(pipeline_path_role_name(PipelinePathRole::canonical) == "canonical", "pipeline: canonical");
  check(pipeline_path_role_name(PipelinePathRole::compatibility) == "compatibility", "pipeline: compat");
  check(pipeline_path_role_name(PipelinePathRole::legacy) == "legacy", "pipeline: legacy");
  check(pipeline_path_role_name(PipelinePathRole::fixture) == "fixture", "pipeline: fixture");
  check(pipeline_path_role_name(PipelinePathRole::deprecation_candidate) == "deprecation-candidate",
        "pipeline: deprecation");

  /* ── CONTROL AUTHORITY ── */
  for (ControlAuthority a : {ControlAuthority::player, ControlAuthority::living,
                              ControlAuthority::ai, ControlAuthority::scripted,
                              ControlAuthority::network, ControlAuthority::cinematic,
                              ControlAuthority::hybrid}) {
    check(!control_authority_name(a).empty(), "control authority name non-empty");
  }
  LivingRuntimeProgram program;
  program.id = "test-living";
  auto policy = living_control_policy(program, "instance-1");
  check(policy.authority == ControlAuthority::living, "living policy authority");
  check(policy.policy_id == "test-living", "living policy id");
  check(policy.entity_instance_id == "instance-1", "living policy instance");
  check(policy.deterministic, "living policy deterministic");
  check(validate_control_policy(policy).ok(), "control policy valid");
  ControlPolicy invalid;
  invalid.policy_id = "";
  check(!validate_control_policy(invalid).ok(), "invalid: empty policy id");
  invalid.policy_id = "x";
  invalid.entity_instance_id = "";
  check(!validate_control_policy(invalid).ok(), "invalid: empty instance id");
  auto canon1 = canonicalize_control_policy(policy);
  auto canon2 = canonicalize_control_policy(policy);
  check(canon1 == canon2, "control canonicalize deterministic");

  /* ── MORPHOLOGY ── */
  {
    MorphologyPart part;
    part.bone_id = "head";
    part.primitive = "circle";
    part.color = "#ff0000";
    part.x = 1.5;
    auto json = canonicalize_morphology_part(part);
    check(json.find("\"boneId\":\"head\"") != std::string::npos, "morph: boneId");
    check(json.find("\"primitive\":\"circle\"") != std::string::npos, "morph: primitive");
  }
  {
    MorphologyMap map;
    map["head"].bone_id = "headbone";
    map["head"].primitive = "circle";
    map["tail"].bone_id = "tailbone";
    auto mapJson = canonicalize_morphology_map(map);
    check(mapJson.find("\"head\"") != std::string::npos, "morph map: head entry");
    auto map2 = canonicalize_morphology_map(map);
    check(mapJson == map2, "morph map deterministic");
  }
  {
    MorphologyMap morph;
    morph["eyes"].bone_id = "head";
    morph["eyes"].semantic_role = "sensor";
    auto preimage = canonicalize_effective_morphology_preimage(morph);
    check(!preimage.empty(), "morph preimage non-empty");
    check(preimage.find("sensor") != std::string::npos, "morph preimage has semantic role");
    auto pre2 = canonicalize_effective_morphology_preimage(morph);
    check(preimage == pre2, "morph preimage deterministic");
  }
  {
    MorphologyMap m0, m1;
    m0["a"].bone_id = "a";
    m1["b"].bone_id = "b";
    std::vector<MorphologyMap> transforms = {m0, m1};
    auto pre = canonicalize_transformations_preimage(transforms);
    check(pre.find("2\n0\n") == 0, "transform preimage: count header");
    auto pre2 = canonicalize_transformations_preimage(transforms);
    check(pre == pre2, "transform preimage deterministic");
  }

  /* ── VIEWER HOSTILE ── */
  check(!LivingPackageViewer::load("nonexistent_xyz_dir").ok(), "viewer: nonexistent");
  check(!LivingPackageViewer::load("C:/Windows/System32").ok(), "viewer: system dir");
  check(!LivingPackageViewer::load("").ok(), "viewer: empty path");

  std::printf("\n%d failures\n", failures);
  return failures != 0 ? 1 : 0;
}
