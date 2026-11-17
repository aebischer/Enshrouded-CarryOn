#include "../src/transport_selection.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <initializer_list>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using namespace carryon::transport;
static Frame ready() {
    Frame f{};f.world=1;f.pincer=true;f.gameplay=true;f.focused=true;
    f.enabled=true;f.reach=3;f.candidate={0x100000001ull,true,true,2};return f;
}
static void enter(Selection& s,Frame& f) {
    f.tab=false;s.update(f);f.tab=true;s.update(f);CHECK(s.active());
}
int main() {
    {
        Selection travel;auto tf=ready();enter(travel,tf);
        tf.tab=false;tf.gameplay=false;tf.suspended=true;
        auto paused=travel.update(tf);CHECK(travel.active()&&!paused.active&&!travel.selected());
        tf.gameplay=true;tf.suspended=false;CHECK(travel.update(tf).active);
        tf.gameplay=false;tf.suspended=true;travel.update(tf);tf.pincer=false;
        travel.update(tf);CHECK(!travel.active());
        tf=ready();enter(travel,tf);tf.gameplay=false;tf.suspended=true;tf.world=2;
        travel.update(tf);CHECK(!travel.active());
    }
    KeyCapture key;
    CHECK(!key.event(true,false).consume);
    CHECK(!key.event(true,true).consume); // Never steal a held native key.
    CHECK(!key.event(false,true).consume);
    auto captured=key.event(true,true);CHECK(captured.consume && captured.pressed);
    captured=key.event(true,true);CHECK(captured.consume && !captured.pressed);
    captured=key.event(false,false);CHECK(captured.consume && !captured.pressed);
    CHECK(!key.event(false,false).consume);
    Selection s;auto f=ready();auto c=s.update(f);
    CHECK(!c.active && !s.selected());
    f.tab=true;c=s.update(f);CHECK(c.active && s.selected()==f.candidate.entity);
    c=s.update(f);CHECK(c.active && !c.clear); // no key repeat
    const auto first=f.candidate.entity;
    f.candidate.entity=0x200000001ull;c=s.update(f); // Same index, new generation.
    CHECK(c.clear==first && s.selected()==f.candidate.entity);
    f.candidate.visible=false;c=s.update(f);CHECK(c.clear && !s.selected());
    f.candidate.visible=true;f.candidate.distance=3.01f;s.update(f);CHECK(!s.selected());
    f.candidate.distance=3;s.update(f);CHECK(s.selected());
    f.candidate.movable=false;s.update(f);CHECK(!s.selected());
    f.candidate.movable=true;
    for(float distance:{-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        f.candidate.distance=distance;s.update(f);CHECK(!s.selected());
    }
    f.candidate.distance=2;f.enabled=false;s.update(f);CHECK(!s.selected());
    f.enabled=true;s.update(f);CHECK(s.selected());
    f.tab=false;s.update(f);f.tab=true;c=s.update(f);CHECK(!c.active && c.clear);
    enter(s,f);f.pincer=false;c=s.update(f);CHECK(!c.active && c.clear);
    f.pincer=true;s.update(f);CHECK(!s.active()); // Tab still held
    enter(s,f);f.gameplay=false;c=s.update(f);CHECK(!c.active && c.clear);
    f.gameplay=true;s.update(f);CHECK(!s.active());
    enter(s,f);f.focused=false;c=s.update(f);CHECK(!c.active && c.clear);
    f.focused=true;s.update(f);CHECK(!s.active());
    enter(s,f);f.cancel=true;c=s.update(f);CHECK(!c.active && c.clear);
    f.cancel=false;f.tab=false;s.update(f);
    enter(s,f);f.world=2;c=s.update(f);CHECK(!c.active && !c.clear);
    CHECK(!s.selected()); // Never resolve the old ID in a different world.
    enter(s,f);f.world=0;c=s.update(f);CHECK(!c.active && !c.clear && !s.selected());
    std::puts("PASS: transport policy (Tab edge, menus, focus, world lifetime, chest filter, reach, occlusion, generation).");
}
