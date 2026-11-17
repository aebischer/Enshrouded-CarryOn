// Pinned native UI adapters. These render hints only; no action dispatcher,
// cursor creation, item destruction, inventory edit or placement is called.
struct TransportText {const char* bytes;uint64_t length;uint64_t hash;};
TransportText transportText(const char* s) {
 if(!s)s="";
 return {s,std::strlen(s),nativeTextHash(s)};
}
template<class Fn> Fn transportUi(uintptr_t rva){return reinterpret_cast<Fn>(base+rva);}
struct TransportUiFrame {
 alignas(16) Byte data[0x30]{};
 TransportUiFrame(const char* id,bool horizontal,float gap) {
  const auto name=transportText(id);
  transportUi<void*(__fastcall*)(void*,const TransportText*,float)>(horizontal?0xa3cf70:0xa3c740)(data,&name,gap);
 }
 ~TransportUiFrame(){transportUi<void(__fastcall*)(void*)>(0xa40080)(data);}
};
struct TransportHint {alignas(16) Byte data[0x78]{};};
TransportHint transportHint(const char* caption,bool enabled) {
 TransportHint hint{};
 write<uint16_t>(hint.data,4,0x201);hint.data[6]=1;
 // Native disabled-state flag dims both the binding glyph and its caption.
 hint.data[0x28]=enabled?0:1;
 write<float>(hint.data,0x3c,1.0f);
 write<float>(hint.data,0x44,500.0f);
 write<float>(hint.data,0x54,24.0f);
 write(hint.data,0x58,transportText(caption));
 write<uint32_t>(hint.data,0x70,0xffffffffu);
 write<float>(hint.data,0x74,1.0f);
 return hint;
}
const char* transportCaption(const char* key) {
 return enshrouded::i18n::find(CarryOnTexts,key,language.load(std::memory_order_relaxed));
}
using TransportHints=void*(__fastcall*)(void*);
using TransportCursorHud=void(__fastcall*)(void*);
TransportHints originalTransportHints=nullptr;
TransportCursorHud originalTransportCursorHud=nullptr;
using TransportPlacementPanel=void(__fastcall*)(void*,float*,float*);
using TransportHudItemLookup=const Byte*(__fastcall*)(uint32_t,void*);
using TransportPlacementHudAllowed=bool(__fastcall*)(const void*);
TransportPlacementPanel originalTransportPlacementPanel=nullptr;
TransportHudItemLookup originalTransportHudItemLookup=nullptr;
TransportPlacementHudAllowed originalTransportPlacementHudAllowed=nullptr;
// Set only around the native inventory-placement HUD, never around gameplay.
thread_local const Byte* transportPlacementHudItem=nullptr;
const Byte* transportHudItemForCaller(uintptr_t caller,uint32_t id,void* registry) {
 if(transportPlacementHudItem&&caller==base+0xb6bc42)return transportPlacementHudItem;
 return originalTransportHudItemLookup(id,registry);
}
const Byte* __fastcall transportHudItemLookup(uint32_t id,void* registry) {
 return transportHudItemForCaller(reinterpret_cast<uintptr_t>(_ReturnAddress()),id,registry);
}
bool __fastcall transportPlacementHudAllowed(const void* context) {
 // Native context copies keep the registry-owned ItemInfo at +0.
 if(context&&transportPlacementHudItem&&read<const Byte*>(context,0)==transportPlacementHudItem)return true;
 return originalTransportPlacementHudAllowed(context);
}
void __fastcall transportPlacementPanel(void* input,float* bounds,float* adjustmentBounds);
bool transportHudReady() {
 return transportModeActive()&&transportEligible.load(std::memory_order_relaxed);
}
using TransportZoneVisualState=const Byte*(__fastcall*)(void*);
TransportZoneVisualState originalTransportZoneVisualState=nullptr;
bool transportZoneBordersReady() {
 const auto updated=transportUpdated.load(std::memory_order_acquire),now=GetTickCount64();
 // Equipped pincer in gameplay; Tab controls actions, not zone visibility.
 return transport_time::fresh(now,updated,transport_time::SelectionFresh)&&transportEligible.load(std::memory_order_relaxed);
}
const Byte* __fastcall transportZoneVisualState(void* client) {
 const auto native=originalTransportZoneVisualState(client);
 if(!native||!transportZoneBordersReady())return native;
 // 0x3aa980 is the pinned client's render-state getter. Its only native
 // caller (0x860237) reads byte 0 and the three vectors at +0x10..+0x3f.
 // Byte 0 enables the existing zone-border render flag (0x8605c8).
 // Return a per-render-thread snapshot: never change the shared game state,
 // building permissions, cursor mode, zone geometry or native preview data.
 alignas(16) thread_local Byte visual[0x40];
 std::memcpy(visual,native,sizeof(visual));
 visual[0]=1;
 return visual;
}
void* __fastcall transportHints(void* result) {
 if(!transportHudReady())return originalTransportHints(result);
 // Match the native user's "hide action hints" setting.
 const auto context=read<const Byte*>(reinterpret_cast<void*>(base),0x283f4a8);
 const auto services=context?read<const Byte*>(context,0xd0):nullptr;
 const bool hidden=services&&read<uint64_t>(services,0x33aa8)&&read<Byte>(services,0x336b0+0x2d7);
 if(!hidden) {
  {
  TransportUiFrame column("CarryOn.transport.footer",false,12.0f);
  {
  TransportUiFrame row("CarryOn.transport.commands",true,48.0f);
  const bool canGrab=!transportHolding.load()&&transportZoneKnown.load()&&transportInsideZone.load()&&transportHasTarget.load();
  const Byte actions[]={10,14,129};
  const char* keys[]={"transport.grab","transport.place","transport.finish"};
  const bool enabled[]={canGrab,transportHolding.load()&&transportPlaceReady.load(),true};
  for(size_t i=0;i<3;++i) {
   auto hint=transportHint(transportCaption(keys[i]),enabled[i]);
   transportUi<void(__fastcall*)(Byte,void*)>(0xbd4130)(actions[i],hint.data);
  }
  }
  }

 }
 // Native result is an optional action: leave its discriminator false.
 write<Byte>(result,4,0);return result;
}
bool transportInspectorText(char* output,size_t capacity) {
 if(!output||!capacity)return false;
 output[0]=0;
 if(!transportInspectObjects.load(std::memory_order_relaxed)||!transportHudReady())return false;
 TransportInspectedObject object;
 {std::lock_guard<std::mutex> lock(transportMutex);object=transportInspectedObject;}
 if(!object.id)return false;
 char model[33]{};
 if(object.model!=Guid{})for(size_t i=0;i<16;++i)sprintf_s(model+i*2,3,"%02x",unsigned(object.model[i]));
 else strcpy_s(model,"-");
 return _snprintf_s(output,capacity,_TRUNCATE,"%s%s%s: %llu (0x%llX / %u)\n%s: %s",
  object.name.data(),object.name[0]?"\n":"",
  transportCaption("inspect.target"),static_cast<unsigned long long>(object.id),
  static_cast<unsigned long long>(object.id),unsigned(object.kind),transportCaption("inspect.model"),model)>0;
}
void transportInspectorHud() {
 char text[640]{};if(!transportInspectorText(text,sizeof(text)))return;
 const auto context=read<const Byte*>(reinterpret_cast<void*>(base),0x283f4a8);if(!context)return;
 const auto viewport=transportUi<const float*(__fastcall*)(const void*)>(0xa4b6c0)(context);
 if(!viewport||viewport[0]<=0||viewport[1]<=0)return;
 const float point[2]={viewport[0]*0.5f,viewport[1]*0.65f};
 alignas(16) Byte feedback[0xe0]{};
 write(feedback,0x48,transportText(text));
 TransportUiFrame overlay("CarryOn.transport.inspector",false,0);
 transportUi<Byte(__fastcall*)(void*,const float*)>(0xb3d6d0)(feedback,point);
}
void __fastcall transportCursorHud(void* arg) {
 originalTransportCursorHud(arg);
 transportInspectorHud();
 if(!transportHudReady()||!transportZoneKnown.load()||transportInsideZone.load())return;
 const auto context=read<const Byte*>(reinterpret_cast<void*>(base),0x283f4a8);
 if(!context)return;
 const auto bundle=read<const Byte*>(context,0xc8);if(!bundle)return;
 // Same localisation tag and text renderer used by the hammer's zone refusal.
 const auto id=transportUi<uint32_t(__fastcall*)(const void*)>(0x42ea30)(bundle+0x3a30);
 TransportText message{};
 transportUi<void*(__fastcall*)(void*,uint32_t)>(0xaff9f0)(&message,id);
 if(!message.length)return;
 const auto viewport=transportUi<const float*(__fastcall*)(const void*)>(0xa4b6c0)(context);
 if(!viewport||viewport[0]<=0||viewport[1]<=0)return;
 const float point[2]={viewport[0]*0.5f,viewport[1]*0.5f};
 alignas(16) Byte feedback[0xe0]{};
 write(feedback,0x48,message);
 TransportUiFrame overlay("CarryOn.transport.zoneFeedback",false,0);
 transportUi<Byte(__fastcall*)(void*,const float*)>(0xb3d6d0)(feedback,point);
 const float invalidRect[4]={point[0]-10,point[1]-10,20,20};
 transportUi<void(__fastcall*)(const float*)>(0xb6df10)(invalidRect);
}
