#pragma once
#include <array>
#include <cstring>
#include <string_view>

namespace carryon {
struct Configuration {
    std::array<char,33> world{};
    bool move_chests=true;
    bool move_objects=true;
    bool move_interactive=true;
    bool inspect_objects=false;
};
inline bool valid_world(const char* value) noexcept {
    if (!value || strnlen_s(value,33)!=32) return false;
    for (size_t i=0;i<32;++i)
        if (!((value[i]>='0' && value[i]<='9') ||
              (value[i]>='a' && value[i]<='f'))) return false;
    return true;
}
// Publish a configuration only after every field has passed validation.
// No allocation or borrowed pointer is retained.
inline bool parse(const char* world,const char* text,Configuration& out) noexcept {
    if (!valid_world(world) || !text) return false;
    const size_t length=strnlen_s(text,1024);
    if (length==1024) return false;
    Configuration next;
    bool seen=false,seenInspector=false,seenObjects=false,seenInteractive=false;
    std::string_view remaining(text,length);
    while (!remaining.empty()) {
        const size_t end=remaining.find('\n');
        const auto line=remaining.substr(0,end);
        const size_t equals=line.find('=');
        if (equals==std::string_view::npos) return false;
        const auto key=line.substr(0,equals),value=line.substr(equals+1);
        if(value!="true" && value!="false") return false;
        if(key=="move_chests"&&!seen){next.move_chests=value=="true";seen=true;}
        else if(key=="move_objects"&&!seenObjects){next.move_objects=value=="true";seenObjects=true;}
        else if(key=="move_interactive"&&!seenInteractive){next.move_interactive=value=="true";seenInteractive=true;}
        else if(key=="inspect_objects"&&!seenInspector){next.inspect_objects=value=="true";seenInspector=true;}
        else return false;
        if(end==std::string_view::npos) break;
        remaining.remove_prefix(end+1);
    }
    if(!seen) return false;
    std::memcpy(next.world.data(),world,32);
    out=next;
    return true;
}
}
