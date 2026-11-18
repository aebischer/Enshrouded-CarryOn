static bool testAreaAllowed=true,testAltarAllowed=true;
static unsigned testZoneCalls=0,testHudFallbacks=0;
static int64_t testZoneX=0;
static bool __fastcall testArea(const void* zones,const void* point) {
 CHECK(zones);++testZoneCalls;testZoneX=read<int64_t>(point,0);return testAreaAllowed;
}
static bool __fastcall testAltar(const void* zones,const void* box,float margin) {
 CHECK(zones&&margin==0.0f);
 for(size_t i=0;i<3;++i)CHECK(read<int64_t>(box,i*8)==read<int64_t>(box,(i+3)*8));
 ++testZoneCalls;return testAltarAllowed;
}
static void* __fastcall testHintsFallback(void* out){++testHudFallbacks;write<Byte>(out,4,9);return out;}
static void __fastcall testCursorFallback(void*){++testHudFallbacks;}
static const Byte* __fastcall testZoneVisualState(void* state){return static_cast<const Byte*>(state);}
static void testTransportZoneBorders() {
 originalTransportZoneVisualState=&testZoneVisualState;
 Byte state[0x40]{},otherWorld[0x40]{};
 for(size_t i=1;i<sizeof(state);++i){state[i]=static_cast<Byte>(i);otherWorld[i]=static_cast<Byte>(255-i);}
 transportUpdated=GetTickCount64();transportEligible=true;transportActive=false;
 CHECK(transportZoneVisualState(state)!=state&&transportZoneVisualState(state)[0]==1);
 CHECK(!transportHudReady()); // Holding the pincer does not enable transport actions.
 transportActive=true;
 const auto visual=transportZoneVisualState(state);
 CHECK(visual!=state&&visual[0]==1&&state[0]==0);
 CHECK(std::memcmp(visual+1,state+1,sizeof(state)-1)==0);
 // Each call refreshes every byte from the current native state/world.
 const auto changed=transportZoneVisualState(otherWorld);
 CHECK(changed[0]==1&&otherWorld[0]==0);
 CHECK(std::memcmp(changed+1,otherWorld+1,sizeof(otherWorld)-1)==0);
 CHECK(!transportZoneVisualState(nullptr));
 // Native hammer borders remain native, whether already on or off.
 transportActive=false;CHECK(transportZoneVisualState(state)[0]==1);
 transportEligible=false;
 state[0]=1;CHECK(transportZoneVisualState(state)==state&&state[0]==1);
 state[0]=0;CHECK(transportZoneVisualState(state)==state);
 transportActive=true;transportEligible=false;CHECK(transportZoneVisualState(state)==state);
 transportEligible=true;transportUpdated=GetTickCount64()-2000;
 CHECK(transportZoneVisualState(state)==state);
 transportUpdated=0;CHECK(transportZoneVisualState(state)==state);
 transportActive=false;transportEligible=false;
 std::puts("PASS: native zone borders while pincer equipped, before/after Tab; native state preserved; tool changes, menus, stale and world changes.");
}
static void testTransportHud() {
 checkCatalogue(CarryOnTexts);
 testTransportZoneBorders();
 transportZoneQuery=&testArea;transportAltarQuery=&testAltar;
 Byte point[24]{},zones[8]{};write<int64_t>(point,0,-17ll*4294967296ll);
 CHECK(!transportZoneAllows(nullptr,point));CHECK(!transportZoneAllows(zones,nullptr));
 CHECK(testZoneCalls==0);
 CHECK(transportZoneAllows(zones,point));CHECK(testZoneX==-17ll*4294967296ll);
 testAltarAllowed=false;CHECK(!transportZoneAllows(zones,point));
 testAltarAllowed=true;testAreaAllowed=false;testZoneCalls=0;
 CHECK(!transportZoneAllows(zones,point));CHECK(testZoneCalls==1);
 testAreaAllowed=true;
 for(const auto lang:enshrouded::i18n::supportedLanguages) {
  for(const auto key:{"transport.grab","transport.place","transport.finish"}) {
   const auto caption=enshrouded::i18n::find(CarryOnTexts,key,lang);
   CHECK(caption&&*caption);
   for(bool enabled:{false,true}) {
    const auto h=transportHint(caption,enabled);
    CHECK(h.data[0x28]==(enabled?0:1));CHECK(!h.data[0x2f]);
    CHECK(read<float>(h.data,0x3c)==1.0f);
    CHECK(read<float>(h.data,0x54)==24.0f);
    const auto textValue=read<TransportText>(h.data,0x58);
    CHECK(textValue.bytes==caption&&textValue.length==std::strlen(caption));
   }
  }
 }
 originalTransportHints=&testHintsFallback;originalTransportCursorHud=&testCursorFallback;
 Byte result[8]{};
 transportActive=false;transportEligible=true;transportUpdated=GetTickCount64();
 CHECK(transportHints(result)==result&&result[4]==9);
 transportCursorHud(nullptr);CHECK(testHudFallbacks==2);
 transportActive=true;transportEligible=false;
 CHECK(!transportHudReady());transportHints(result);CHECK(testHudFallbacks==3);
 transportEligible=true;transportUpdated=GetTickCount64()-2000;
 CHECK(!transportHudReady());transportHints(result);CHECK(testHudFallbacks==4);
 transportActive=false;transportEligible=false;transportUpdated=0;
 transportZoneQuery=nullptr;transportAltarQuery=nullptr;
 std::puts("PASS: transport HUD captions, native hint configuration, zone refusals and inactive/stale fallbacks.");
}
