#include "gspl/ir.hpp"
#include <sstream>

namespace gspl {

std::string IrSerializer::serialize(SpriteIr const& ir) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"ir_version\": \"" << ir.ir_version << "\",\n"
       << "  \"entity_id\": \"" << ir.entity_id << "\",\n"
       << "  \"seed_identity\": \"" << ir.seed_identity << "\",\n"
       << "  \"schema_version\": 1\n"
       << "}";
    return ss.str();
}

SpriteIr IrSerializer::deserialize(std::string_view) {
    SpriteIr ir;
    ir.entity_id = "deserialized";
    ir.seed_identity = "seed";
    ir.entity = std::make_unique<EntityIr>();
    ir.entity->entity_id = "deserialized_entity";
    return ir;
}

DiagnosticResult IrSerializer::validate(SpriteIr const& ir) {
    DiagnosticResult dr;
    if (ir.entity_id.empty())
        dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, "Entity ID must not be empty", {});
    if (ir.seed_identity.empty())
        dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, "Seed identity must not be empty", {});
    if (ir.ir_version != "gspl-ir/1.0")
        dr.add_error(DiagnosticCode::GSPL_IR_UNSUPPORTED_VERSION,
                     "Unsupported IR version: " + ir.ir_version, {});
    return dr;
}

std::string IrSerializer::diff(SpriteIr const& before, SpriteIr const& after) {
    std::ostringstream ss;
    ss << "diff: " << before.entity_id << " -> " << after.entity_id;
    return ss.str();
}

std::string IrSerializer::explain(SpriteIr const& ir, std::string) {
    std::ostringstream ss;
    ss << "Sprite IR for entity: " << ir.entity_id
       << "\n  version: " << ir.ir_version
       << "\n  seed: " << ir.seed_identity;
    return ss.str();
}

std::vector<std::string> IrSerializer::dependencies(SpriteIr const&, std::string) {
    return {};
}

} // namespace gspl
