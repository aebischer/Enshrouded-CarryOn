#include "enshrouded_mod.h"
#include "configuration.h"
#include "native_content.h"
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
        carryon::native::configureTransport(false,false,false,false);
        carryon::Configuration next;
        if(!carryon::parse(world_id,settings_utf8,next))
            return ENSHROUDED_MOD_FAILURE;
        configuration=next;
        carryon::native::configureTransport(next.move_chests,next.inspect_objects,next.move_objects,next.move_interactive);
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
        return carryon::native::initialize(context->write_log) ? ENSHROUDED_MOD_SUCCESS : ENSHROUDED_MOD_FAILURE;
    } catch (...) {return ENSHROUDED_MOD_FAILURE;}
}
