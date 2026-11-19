#include <Windows.h>
#include "enshrouded_mod.h"
#include "../src/configuration.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static int logs=0;
static void __cdecl log_message(const char* text) {
    CHECK(text && std::strstr(text,"unsupported executable"));
    ++logs;
}
int main() {
    constexpr char world[]="0123456789abcdef0123456789abcdef";
    constexpr char other[]="abcdef0123456789abcdef0123456789";
    constexpr char settings[]="move_chests=true\n";
    carryon::Configuration config;
    CHECK(carryon::parse(world,settings,config));
    CHECK(config.move_chests);
    CHECK(carryon::parse(world,"move_chests=false\ninspect_objects=true\n",config)&&config.inspect_objects&&!config.move_chests);
    CHECK(!carryon::parse(world,"move_chests=true\ninspect_objects=true\ninspect_objects=false\n",config));
    CHECK(carryon::parse(other,settings,config)&&!config.inspect_objects&&config.move_chests);
    CHECK(!carryon::parse(world,"move_chests=false\nmove_chests=true\n",config));
    CHECK(config.move_chests);
    CHECK(!carryon::parse(world,"move_chests=yes",config));
    CHECK(carryon::parse(world,"move_chests=true\nmove_objects=false\n",config)&&!config.move_objects&&config.move_chests);
    CHECK(!carryon::parse(world,"move_chests=true\nmove_objects=false\nmove_objects=true\n",config));
    CHECK(!carryon::parse(world,"unknown=true",config));
    CHECK(carryon::parse(world,"move_chests=true\nmove_objects=false\nmove_interactive=true\n",config)&&config.move_interactive&&!config.move_objects);
    CHECK(carryon::parse(other,"move_chests=true\nmove_objects=true\nmove_interactive=false\n",config)&&!config.move_interactive&&config.move_objects);
    CHECK(!carryon::parse(world,"move_chests=true\nmove_interactive=true\nmove_interactive=false\n",config));
    CHECK(!carryon::parse("INVALID",settings,config));
    CHECK(!carryon::parse(nullptr,settings,config));
    CHECK(!carryon::parse(world,nullptr,config));
    std::string oversized(1024,'x');
    CHECK(!carryon::parse(world,oversized.c_str(),config));
    HMODULE dll=LoadLibraryW(L"..\\dist\\Mods\\EnshroudedModLoader\\carryon\\carryon.dll");
    CHECK(dll);
    auto configure=reinterpret_cast<EnshroudedModConfigureFn>(GetProcAddress(dll,"EnshroudedModConfigure"));
    auto initialize=reinterpret_cast<EnshroudedModInitializeFn>(GetProcAddress(dll,"EnshroudedModInitialize"));
    CHECK(configure && initialize);
    EnshroudedModContext ctx{sizeof(ctx),ENSHROUDED_MOD_ABI_VERSION,world,log_message};
    CHECK(initialize(nullptr)==0);
    CHECK(initialize(&ctx)==0 && logs==0);
    CHECK(configure(world,settings)==1);
    ctx.world_id=other;
    CHECK(initialize(&ctx)==0 && logs==0);
    ctx.world_id=world;ctx.abi_version=999;
    CHECK(initialize(&ctx)==0 && logs==0);
    ctx.abi_version=1;
    CHECK(initialize(&ctx)==0 && logs==1); // Non-game executables must never be hooked.
    CHECK(configure(other,"move_chests=false\n")==1);
    CHECK(initialize(&ctx)==0 && logs==1); // Previous world's configuration not reused.
    CHECK(configure(other,"invalid")==0);
    ctx.world_id=other;
    CHECK(initialize(&ctx)==0 && logs==1); // Failed configure invalidates old state.
    CHECK(FreeLibrary(dll));
    std::puts("PASS: configuration, exported SDK ABI, world isolation and unsupported-executable refusal.");
}
