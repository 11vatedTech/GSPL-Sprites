#include "gspl/studio/ipc_envelope.hpp"
#include "gspl/studio/ipc_channel.hpp"
#include "gspl/studio/shared_memory.hpp"
#include <cassert>
#include <cstdio>

void test_envelope_roundtrip() {
    gspl::studio::IpcEnvelope env;
    env.message_id = 42;
    env.method = "test_method";
    env.payload = "{\"key\":\"value\"}";

    auto serialized = env.serialize();
    auto deserialized = gspl::studio::IpcEnvelope::deserialize(serialized);

    assert(deserialized.magic == gspl::studio::IpcEnvelope::MAGIC);
    assert(deserialized.version == gspl::studio::IpcEnvelope::PROTOCOL_VERSION);
    assert(deserialized.message_id == 42);
    assert(deserialized.method == "test_method");
    assert(deserialized.payload == "{\"key\":\"value\"}");
    assert(!deserialized.has_shared_memory);
}

void test_envelope_with_shared_memory() {
    gspl::studio::IpcEnvelope env;
    env.method = "preview_frame";
    env.payload = "{}";
    env.has_shared_memory = true;
    env.shared_memory_key = "frame_001";
    env.shared_memory_size = 1048576;

    auto serialized = env.serialize();
    auto deserialized = gspl::studio::IpcEnvelope::deserialize(serialized);

    assert(deserialized.has_shared_memory);
    assert(deserialized.shared_memory_key == "frame_001");
    assert(deserialized.shared_memory_size == 1048576);
}

void test_envelope_special_chars() {
    gspl::studio::IpcEnvelope env;
    env.method = "diag\nostic";
    env.payload = "{\"msg\":\"hello \\\"world\\\"\"}";

    auto serialized = env.serialize();
    auto deserialized = gspl::studio::IpcEnvelope::deserialize(serialized);

    assert(deserialized.method == "diag\nostic");
    assert(deserialized.payload == "{\"msg\":\"hello \\\"world\\\"\"}");
}

int main() {
    test_envelope_roundtrip();
    test_envelope_with_shared_memory();
    test_envelope_special_chars();
    std::printf("All IPC tests passed.\n");
    return 0;
}
