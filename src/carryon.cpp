#include "enshrouded_mod.h"
#include "configuration.h"
#include <mutex>

namespace {
std::mutex state_mutex;
carryon::Configuration configuration;
bool configured=false;
}
ENSHROUDED_MOD_EXPORT int ENSHROUDED_MOD_CALL EnshroudedModConfigure(
    const char* world_id,const char* settings_utf8) {
    try {
        std::lock_guard<std::mutex> lock(state_mutex);
        configured=false;
        carryon::Configuration next;
        if(!carryon::parse(world_id,settings_utf8,next))
            return ENSHROUDED_MOD_FAILURE;
        configuration=next;
        configured=true;
        return ENSHROUDED_MOD_SUCCESS;
    } catch (...) {return ENSHROUDED_MOD_FAILURE;}
}
ENSHROUDED_MOD_EXPORT int ENSHROUDED_MOD_CALL EnshroudedModInitialize(
    const EnshroudedModContext* context) {
    try {
        if(!context || context->size<sizeof(*context) ||
           context->abi_version!=ENSHROUDED_MOD_ABI_VERSION ||
           !context->write_log || !carryon::valid_world(context->world_id))
            return ENSHROUDED_MOD_FAILURE;
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            if(!configured || std::strcmp(configuration.world.data(),context->world_id)!=0)
                return ENSHROUDED_MOD_FAILURE;
        }
        // Integration milestone only. No guessed native calls or world mutation.
        // Replace this refusal only when the native relocation path is verified.
        context->write_log("CarryOn 0.1.0-dev: native object relocation is not implemented; initialization refused.");
        return ENSHROUDED_MOD_FAILURE;
    } catch (...) {return ENSHROUDED_MOD_FAILURE;}
}
