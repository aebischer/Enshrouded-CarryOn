#pragma once
#include <cstdint>
#include <cmath>

namespace carryon::transport {
struct KeyResult {bool consume=false,pressed=false;};
class KeyCapture {
    bool down_=false, owned_=false;
public:
    KeyResult event(bool down,bool eligible) noexcept {
        if(!down) {const bool owned=owned_;down_=false;owned_=false;return {owned,false};}
        if(down_)return {owned_,false};
        down_=true;owned_=eligible;return {owned_,owned_};
    }
    void reset() noexcept {down_=false;owned_=false;}
};
// Pure policy. The native adapter must supply a fresh, validated candidate each
// frame and apply the returned visual transition on the game's owning thread.
// Never retain a component pointer here, or treat an inventory item as an entity.
struct Candidate {
    uint64_t entity=0; // Full native identity, including generation.
    bool movable=false;
    bool visible=false;
    float distance=0;
};
struct Frame {
    uint64_t world=0; // Owning world identity supplied by the adapter (native world address).
    bool pincer=false;
    bool gameplay=false;
    bool suspended=false; // Map/teleport only while an existing object is held.
    bool focused=false;
    bool enabled=false;
    bool tab=false;
    bool cancel=false;
    float reach=0; // Additional distance cap after the native interaction query.
    Candidate candidate{};
};
struct Change {
    uint64_t clear=0;
    bool active=false;
};
class Selection {
    uint64_t world_=0, selected_=0;
    bool active_=false, eligible_=false, tab_=false;
public:
    Change update(const Frame& f) noexcept {
        const auto oldTarget=selected_;
        const bool eligible=f.world && f.pincer && f.gameplay && f.focused;
        const bool sameSession=f.world==world_;
        // Arming requires a released key in an eligible frame. Holding Tab
        // while equipping, entering a world or closing a menu cannot activate.
        const bool pressed=sameSession && eligible_ && eligible && f.tab && !tab_;
        if(!sameSession || (!eligible && !(f.suspended&&f.world&&f.pincer&&f.focused)) || f.cancel)active_=false;
        else if(pressed)active_=!active_;
        world_=f.world; eligible_=eligible; tab_=f.tab;
        const auto& c=f.candidate;
        selected_=active_ && eligible && f.enabled && c.entity && c.movable && c.visible &&
            std::isfinite(c.distance) && std::isfinite(f.reach) &&
            c.distance>=0 && f.reach>0 && c.distance<=f.reach ? c.entity : 0;
        Change result{};result.active=active_&&eligible;
        // On a world transition the former entity belongs to a dead session:
        // do not resolve its ID in the new world. Its renderer owns teardown.
        if(sameSession && oldTarget!=selected_)result.clear=oldTarget;
        return result;
    }
    bool active() const noexcept {return active_;}
    uint64_t selected() const noexcept {return selected_;}
};
}
