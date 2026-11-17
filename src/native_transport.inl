// Included inside native_content's private namespace. Addresses and layouts
// are restricted by the executable SHA-256 check in initialize().
using TransportRow=void(__fastcall*)(void*,void*,void*);
using TypeIndex=const uint16_t*(__fastcall*)(void*,const uint32_t*);
struct ComponentView {Byte* data;size_t size;};
using ReadComponent=ComponentView*(__fastcall*)(ComponentView*,void*,const uint16_t*,uint32_t);
using EntityRecord=void*(__fastcall*)(void*,uint32_t);
using MutableComponent=ComponentView*(__fastcall*)(ComponentView*,void*,void*,void*,const uint16_t*,bool);
TransportRow originalTransportRow=nullptr;
TypeIndex transportTypeIndex=nullptr;
ReadComponent transportRead=nullptr;
EntityRecord transportEntity=nullptr;
MutableComponent transportMutable=nullptr;
std::atomic<bool> transportChests{true};
std::atomic<bool> transportObjects{true};
std::atomic<bool> transportInteractive{true};
std::mutex transportCatalogMutex;
// Never wait on resource preparation from a second gameplay callback.
std::mutex transportResourceWorkMutex,transportCatalogWorkMutex;
std::map<Guid,uint32_t> transportPlacementCatalog;
// Process-long immutable blobs. Native input callbacks may retain a descriptor
// pointer until the next frame, so never replace/erase a published allocation.
std::map<Guid,Bytes> transportFloorPlacements;
std::set<Guid> transportUnsupportedFloorModels;
Bytes transportFloorProfile;
std::set<void*> transportCatalogRegistries;
struct TransportCatalogProgress {std::vector<uint32_t> pending;ULONGLONG attemptedAt=0;size_t cursor=0;};
std::map<void*,TransportCatalogProgress> transportCatalogProgress;
std::map<Guid,ULONGLONG> transportFloorRetry;
std::map<Guid,ULONGLONG> transportTemplateRetry;
std::map<Guid,bool> transportDoorTemplates,transportWellTemplates;
bool transportMovementEnabled(){return transportChests.load()||transportObjects.load()||transportInteractive.load();}
std::atomic<bool> transportInspectObjects{false};
std::atomic<bool> transportActive{false};
std::atomic<bool> transportEligible{false};
std::atomic<bool> transportHolding{false},transportPlaceReady{false};
std::atomic<bool> transportTravelPaused{false};
std::atomic<uint64_t> transportSessionGeneration{1};
struct TransportTravelState {
 uint64_t generation=0;
 uintptr_t world=0;
 ULONGLONG updatedAt=0,rowAt=0;
 Byte screen=0;
 bool active=false,eligible=false;
};
std::mutex transportTravelMutex;
TransportTravelState transportTravelState;
TransportTravelState transportCachedTravelState() {
 std::lock_guard<std::mutex> lock(transportTravelMutex);return transportTravelState;
}
// Called on the UI thread, within the lifetime of the native UI context.
void transportPublishScreen(const void* context,ULONGLONG now) {
 const auto services=context?read<const void*>(context,0xd0):nullptr;
 const auto game=services?read<const void*>(services,0):nullptr;
 const auto client=game?read<const void*>(game,0x38):nullptr;
 if(!client)return;
 std::lock_guard<std::mutex> lock(transportTravelMutex);
 transportTravelState.screen=read<Byte>(client,0x1cd8);
 transportTravelState.updatedAt=now;
 transportTravelState.generation=transportSessionGeneration.load();
}
using TransportUiTick=void(__fastcall*)(void*);
TransportUiTick originalTransportUiTick=nullptr;
void __fastcall transportUiTick(void* context) {
 transportPublishScreen(context,GetTickCount64());
 originalTransportUiTick(context);
 transportPublishScreen(context,GetTickCount64());
}
void (*transportMoveFrameCallback)(void*,bool,bool)=nullptr;
void (*transportMoveRenderCallback)(void*,void*)=nullptr;
// UI snapshot only. Never writes native cursor flags or build permissions.
std::atomic<bool> transportZoneKnown{false},transportInsideZone{false},transportHasTarget{false};
using TransportZoneQuery=bool(__fastcall*)(const void*,const void*);
TransportZoneQuery transportZoneQuery=nullptr;
using TransportAltarQuery=bool(__fastcall*)(const void*,const void*,float);
TransportAltarQuery transportAltarQuery=nullptr;
bool transportZoneAllows(const void* zones,const Byte* point) {
 if(!zones||!point||!transportZoneQuery||!transportAltarQuery)return false;
 int64_t corners[6]{};
 for(size_t i=0;i<3;++i)corners[i]=corners[i+3]=read<int64_t>(point,i*8);
 return transportZoneQuery(zones,corners)&&transportAltarQuery(zones,corners,0.0f);
}
std::atomic<uint32_t> transportTabEvents{0},transportCancelEvents{0};
std::mutex transportKeyboardMutex;
transport::KeyCapture transportTabKey,transportCancelKey;
std::atomic<ULONGLONG> transportUpdated{0};
std::atomic<uint64_t> transportRows{0};
std::mutex transportMutex;
transport::Selection transportSelection;
uintptr_t transportWorld=0;
uint32_t transportPainted=0;
Byte transportOriginalColor=0,transportAppliedColor=0;
bool transportMissingColorLogged=false;

// Hashes from reflected type descriptors (+0x90), not guessed item names.
constexpr uint32_t UpgradeStorageType=0x8a207846;
constexpr uint32_t HighlightType=0xcdfc4fa8;
constexpr uint32_t InventoryType=0xb19528a9;
struct TransportPropClass {uint32_t item=0;bool door=false,well=false;};
TransportPropClass transportClassifyProp(void* world,uint32_t entity,const Byte* descriptor=nullptr);
const Byte* transportTemplate(void* world,uint32_t entity);
struct TransportInspectedObject {
 uint64_t id=0;
 Byte kind=0;
 Guid model{};
 std::array<char,192> name{};
};
void transportInspectName(const Byte* descriptor,std::array<char,192>& output) {
 output={};if(!descriptor)return;
 // Pinned live descriptor: owned native string at +0x10, byte length +0x18.
 // Copy on the ECS thread; no borrowed name pointer survives into the HUD.
 const auto name=read<const char*>(descriptor,0x10);
 const auto length=read<uint64_t>(descriptor,0x18);
 if(!name||!length||length>output.size())return;
 for(size_t i=0;i<length&&i+1<output.size();++i) {
  const auto c=static_cast<unsigned char>(name[i]);
  if(!c)break;
  // Internal identifiers only: prevent text markup/control interpretation.
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.')){output={};return;}
  output[i]=static_cast<char>(c);
 }
}
// Owned by transportMutex; values only, no ECS pointers cross into the HUD.
TransportInspectedObject transportInspectedObject;
TransportInspectedObject transportInspectTarget(void* world,const Byte* targets) {
 TransportInspectedObject result{};
 if(!world||!targets)return result;
 // Generic hit first, then decor/storage fallbacks. No movability, claim,
 // inventory or comfort filter: this is observation, never authorization.
 for(const size_t slot:{size_t(0),size_t(4),size_t(2)}) {
  const auto id=read<uint64_t>(targets,slot*16);if(!id)continue;
  result.id=id;result.kind=read<Byte>(targets,slot*16+8);
  if(result.kind==0&&id<=UINT32_MAX) {
   const auto descriptor=transportTemplate(world,static_cast<uint32_t>(id));
   if(descriptor){result.model=read<Guid>(descriptor,0);transportInspectName(descriptor,result.name);}
  }
  return result;
 }
 return result;
}
uint32_t transportModelType=0;
uint32_t transportVoxelType=0;
// Pinned client's reflected keen::HudScreenType: Invalid=0, InGame=1.
constexpr Byte TransportInGame=1;
constexpr Byte TransportHighlight=2; // reflected HighlightColorIndex::ValidState
uint32_t transportRenderType=0; // bound from the pinned descriptor

const uint16_t* transportIndex(void* world,uint32_t type) {
 if(!world||!type)return nullptr;
 auto registry=read<Byte*>(world,0);if(!registry)return nullptr;
 auto index=transportTypeIndex(registry+0x30,&type);
 if(!index)index=transportTypeIndex(registry+0x88,&type);
 return index&&*index<640?index:nullptr;
}
ComponentView transportComponent(void* world,uint32_t entity,uint32_t type,size_t size) {
 ComponentView result{};if(!entity)return result;
 auto index=transportIndex(world,type);if(!index)return result;
 transportRead(&result,world,index,entity);
 if(!result.data||result.size!=size)return {};
 return result;
}
uint32_t transportHeldItem(void* world,const Byte* slots) {
 // actionItem is the item executing an action, not necessarily the selected
 // tool. Tools without an action sequence leave it empty. Follow the native
 // actionbar selection: two inventory entities, eight ItemStack entries each.
 if(!slots)return 0;
 const auto selected=read<Byte>(slots,192);if(selected>=16)return 0;
 const auto entity=read<uint32_t>(slots,160+(selected/8)*4);
 const auto inventory=transportComponent(world,entity,InventoryType,96);
 return inventory.data?read<uint32_t>(inventory.data,(selected%8)*12):0;
}
bool transportHas(void* context,void* world,uint32_t entity,uint32_t type) {
 const auto index=transportIndex(world,type);if(!index||!entity)return false;
 auto record=transportEntity(context,entity);if(!record)return false;
 auto archetype=read<const Byte*>(record,0x18);if(!archetype)return false;
 return (read<uint64_t>(archetype,(*index/64)*8)&(uint64_t(1)<<(*index%64)))!=0;
}
Byte* transportColor(void* context,void* world,uint32_t entity) {
 // Read and validate the dynamic half before requesting the native write mark.
 if(!transportComponent(world,entity,HighlightType,1).data)return nullptr;
 auto index=transportIndex(world,HighlightType);
 auto record=transportEntity(context,entity);if(!record||!index)return nullptr;
 auto descriptor=read<void*>(context,0);if(!descriptor)return nullptr;
 auto tracker=read<void*>(descriptor,0x1c0);
 auto stamps=read<void*>(descriptor,0);
 if(!tracker||!stamps)return nullptr;
 ComponentView out{};
 transportMutable(&out,tracker,stamps,record,index,read<Byte>(descriptor,0xbb)!=0);
 return out.data&&out.size==1?out.data:nullptr;
}
void transportRestore(void* context,void* world) {
 if(transportPainted) {
  if(auto color=transportColor(context,world,transportPainted))
   if(*color==transportAppliedColor)*color=transportOriginalColor;
 }
 transportPainted=0;
}
float transportDistance(const Byte* a,const Byte* b) {
 double sum=0;
 for(size_t axis=0;axis<3;++axis) {
  // Convert before subtraction: subtracting two int64 coordinates may overflow.
  const double delta=(static_cast<double>(read<int64_t>(a,axis*8))-
                      static_cast<double>(read<int64_t>(b,axis*8)))/4294967296.0;
  sum+=delta*delta;
 }
 return static_cast<float>(std::sqrt(sum));
}
transport::Candidate transportTargetCandidate(void* context,void* world,const Byte* targets,const Byte* playerTransform) {
 if(!targets||!playerTransform)return {};
  // Interaction target is selected by the game's own reach/occlusion query.
  // Do not use cursorEntityId: it denotes the placement preview, not the chest.
  // Preserve the chest interaction slot's priority. The native decor query
  // exposes this counter in slots 0/4, rather than the storage slot 2.
  for(const size_t slot:{size_t(2),size_t(0),size_t(4)}) {
  const auto id=read<uint64_t>(targets,slot*16);
  const auto kind=read<Byte>(targets,slot*16+8);
  if(kind==0 && id && id<=UINT32_MAX &&
     ((slot==2&&transportChests.load()&&transportHas(context,world,static_cast<uint32_t>(id),UpgradeStorageType))||
      transportClassifyProp(world,static_cast<uint32_t>(id)).item)) {
   const auto transform=transportComponent(world,static_cast<uint32_t>(id),transportRenderType,176);
   const auto model=transportComponent(world,static_cast<uint32_t>(id),transportModelType,4);
   transport::Placement pose{};
   if(transform.data){pose.rotation=read<std::array<float,4>>(transform.data,24);pose.scale=read<std::array<float,3>>(transform.data,40);}
   if(transform.data&&pose.movable()&&model.data&&read<uint32_t>(model.data,0)) {
    return {id,true,true,transportDistance(playerTransform,transform.data)};

   }
  }
  }
 return {};
}
bool transportFrame(void* row,void* cursorContext,bool tab,bool cancel,bool focused,ULONGLONG now) {
 std::lock_guard<std::mutex> lock(transportMutex);
 auto context=read<void*>(row,0);if(!context)return false;
 auto descriptor=read<void*>(context,0);if(!descriptor)return false;
 auto world=read<void*>(descriptor,0x30);if(!world)return false;
 // The row's SlotSelection and NetworkSelectedTargets are native read views.
 auto slots=read<const Byte*>(row,0x40);
 auto targets=read<const Byte*>(row,0x28);
 auto ui=read<const Byte*>(cursorContext,0x90);
 const Byte screen=ui?read<Byte>(ui,0x1cd8):0;
 auto playerTransform=read<const Byte*>(row,0x38);
 const auto prior=transportUpdated.load(std::memory_order_relaxed);
 const bool changedWorld=transportWorld!=reinterpret_cast<uintptr_t>(world);
 const bool stale=prior && (now<prior||now-prior>transport_time::SelectionFresh);
 if(changedWorld) {transportPainted=0;transportSelection={};}
 else if(stale&&!transportTravelPaused.load()&&!(transportHolding.load()&&screen==5)) {
  transportRestore(context,world);transportSelection={};
 }
 transportWorld=reinterpret_cast<uintptr_t>(world);
 transport::Frame frame{};
 const auto heldItem=transportHeldItem(world,slots);
 frame.world=transportWorld;frame.pincer=heldItem==hash(Item);
 frame.gameplay=screen==TransportInGame;
 frame.suspended=transportHolding.load()&&(screen==5||(screen==51&&transportTravelPaused.load()));
 transportTravelPaused=frame.suspended;
 frame.focused=focused;frame.tab=tab;frame.cancel=cancel;
 frame.enabled=transportMovementEnabled()&&!transportHolding.load(std::memory_order_relaxed);
 frame.reach=3.0f; // Additional conservative cap on the native interaction target.
 const Byte* zonePoint=playerTransform;
 if(frame.pincer&&frame.gameplay&&frame.focused&&frame.enabled&&!cancel&&
    (transportSelection.active()||tab)&&targets&&playerTransform) {
  frame.candidate=transportTargetCandidate(context,world,targets,playerTransform);
  if(frame.candidate.entity)zonePoint=transportComponent(world,static_cast<uint32_t>(frame.candidate.entity),transportRenderType,176).data;
 }
 const bool wasActive=transportSelection.active();
 const auto previous=transportSelection.selected();
 const auto change=transportSelection.update(frame);
 const auto inspected=transportInspectObjects.load(std::memory_order_relaxed)&&change.active&&
  frame.pincer&&frame.gameplay&&frame.focused?transportInspectTarget(world,targets):TransportInspectedObject{};
 if(inspected.id&&(changedWorld||inspected.id!=transportInspectedObject.id||inspected.kind!=transportInspectedObject.kind||inspected.model!=transportInspectedObject.model)) {
  char model[33]{};for(size_t i=0;i<16;++i)sprintf_s(model+i*2,3,"%02x",unsigned(inspected.model[i]));
  char message[384];sprintf_s(message,"CarryOn: inspected object name=%s id=%llu kind=%u template=%s",
   inspected.name[0]?inspected.name.data():"-",static_cast<unsigned long long>(inspected.id),unsigned(inspected.kind),model);log(message);
 }
 transportInspectedObject=inspected;
 if(change.clear)transportRestore(context,world);
 const auto selected=transportSelection.selected();
 // Read-only global dependency #2: BuildZoneSystem, row +0xb8.
 const auto zones=read<const void*>(row,0xb8);
 if(!selected)zonePoint=playerTransform;
 const bool zoneKnown=change.active&&zones&&zonePoint&&transportZoneQuery&&transportAltarQuery;
 transportZoneKnown.store(zoneKnown,std::memory_order_relaxed);
 transportInsideZone.store(zoneKnown&&transportZoneAllows(zones,zonePoint),std::memory_order_relaxed);
 transportHasTarget.store(selected!=0,std::memory_order_relaxed);
 if(selected && transportPainted!=selected) {
  if(auto color=transportColor(context,world,static_cast<uint32_t>(selected))) {
    transportPainted=static_cast<uint32_t>(selected);
    transportOriginalColor=*color;transportAppliedColor=TransportHighlight;*color=TransportHighlight;
    char message[140];sprintf_s(message,"CarryOn: native highlight applied entity=%u previous=%u color=%u",
     transportPainted,unsigned(transportOriginalColor),unsigned(transportAppliedColor));log(message);
  }
  if(!transportPainted&&!transportMissingColorLogged) {
   const auto component=transportComponent(world,static_cast<uint32_t>(selected),HighlightType,1);
   log(component.data?"CarryOn: chest HighlightColor exists but native writable access failed":
    "CarryOn: chest has no dynamic HighlightColor; using model-render outline");
   transportMissingColorLogged=true;
  }
 } else if(transportPainted) {
  // Some render updates restore the base color each frame. Reapply only over
  // that saved base color; a distinct effect keeps ownership of its own color.
  auto current=transportComponent(world,transportPainted,HighlightType,1);
  if(current.data && *current.data==transportOriginalColor &&
     transportOriginalColor!=transportAppliedColor)
   if(auto color=transportColor(context,world,transportPainted))*color=transportAppliedColor;
 }
 transportActive.store(change.active,std::memory_order_relaxed);
 transportEligible.store(frame.pincer&&frame.gameplay&&frame.focused,std::memory_order_relaxed);
 transportUpdated.store(now,std::memory_order_release);
 {
  std::lock_guard<std::mutex> travelLock(transportTravelMutex);
  transportTravelState={transportSessionGeneration.load(),transportWorld,now,now,screen,
   change.active,frame.pincer&&frame.gameplay&&frame.focused};
 }
 if(!frame.focused) {
  std::lock_guard<std::mutex> keyboardLock(transportKeyboardMutex);
  transportTabKey.reset();transportCancelKey.reset();
  transportTabEvents=0;transportCancelEvents=0;
 }
 if(wasActive!=change.active)log(change.active?"CarryOn: transport mode entered":"CarryOn: transport mode exited");
 if constexpr(TransportDiagnostics) if(previous!=transportSelection.selected()) {
  char message[120];sprintf_s(message,"CarryOn: chest target=%llu highlighted=%u",
   static_cast<unsigned long long>(transportSelection.selected()),transportPainted);log(message);
  if(selected) {
   const auto model=transportComponent(world,static_cast<uint32_t>(selected),transportModelType,4);
   const auto voxel=transportComponent(world,static_cast<uint32_t>(selected),transportVoxelType,8);
   sprintf_s(message,"CarryOn: target rendering model=%u voxel=%u",
    model.data?read<uint32_t>(model.data,0):0,voxel.data?read<uint32_t>(voxel.data,0):0);log(message);
  }
 }
 return !changedWorld&&!stale&&frame.pincer&&frame.gameplay&&frame.focused;
}
bool transportBoundAction(Byte action);
void __fastcall transportRow(void* row,void* cursorContext,void* query) {
 if(transportRows.fetch_add(1,std::memory_order_relaxed)==0)
  log("CarryOn: transport cursor callback reached");
 originalTransportRow(row,cursorContext,query);
 const bool focused=gameHasFocus();const auto now=GetTickCount64();
 const auto presses=transportTabEvents.exchange(0),cancel=transportCancelEvents.exchange(0);
 // The native input adapter queues command edges until the next ECS frame.
 // Two taps before a frame cancel each other. Repeats never enter this count.
 const bool acceptPress=transportFrame(row,cursorContext,false,cancel!=0,focused,now);
 if(acceptPress&&(presses&1)&&!cancel)transportFrame(row,cursorContext,true,false,focused,now);
 if(transportMoveFrameCallback)transportMoveFrameCallback(row,
  transportBoundAction(10),transportBoundAction(14));
}
bool transportModeActive() {
 const auto updated=transportUpdated.load(std::memory_order_acquire),now=GetTickCount64();
 return transport_time::fresh(now,updated,transport_time::SelectionFresh)&&transportActive.load(std::memory_order_relaxed);
}
// Existing placed models do not necessarily have HighlightColor. Submit the
// same render-only command as static_render_model, without changing archetypes.
struct RenderSpan {const void* data;size_t count;};
using RenderHighlight=void(__fastcall*)(void*,const RenderSpan*,const RenderSpan*);
using RenderSystem=void(__fastcall*)(void*);
using InitRow=void(__fastcall*)(void*,void*,size_t);
RenderHighlight transportRenderHighlight=nullptr;
RenderSystem originalTransportRender=nullptr;
RenderSystem originalTransportVoxelRender=nullptr;
InitRow transportInitRow=nullptr;
uintptr_t transportRenderWorld=0;
uint32_t transportRenderEntity=0,transportRenderHandle=0;
uint32_t transportModel(void* world,uint32_t entity) {
 const auto model=transportComponent(world,entity,transportModelType,4);
 if(model.data&&read<uint32_t>(model.data,0))return read<uint32_t>(model.data,0);
 const auto voxel=transportComponent(world,entity,transportVoxelType,8);
 return voxel.data?read<uint32_t>(voxel.data,0):0;
}
void transportDraw(void* scene,uint32_t handle,Byte color) {
 const RenderSpan ids{&handle,1},colors{&color,1};
 transportRenderHighlight(scene,&ids,&colors);
}
void transportRenderFrame(void* world,void* scene,ULONGLONG now) {
 std::lock_guard<std::mutex> lock(transportMutex);
 const auto identity=reinterpret_cast<uintptr_t>(world);
 static uint32_t lastDiagnostic=0;
 const auto targetDiagnostic=static_cast<uint32_t>(transportSelection.selected());
 if constexpr(TransportDiagnostics) if(targetDiagnostic!=lastDiagnostic) {
  char message[160];sprintf_s(message,"CarryOn: render callback target=%u world_match=%u scene=%u model=%u",
   targetDiagnostic,unsigned(identity==transportWorld),unsigned(scene!=nullptr),
   world&&targetDiagnostic?transportModel(world,targetDiagnostic):0);log(message);
  lastDiagnostic=targetDiagnostic;
 }
 if(transportRenderWorld!=identity) {transportRenderEntity=0;transportRenderHandle=0;}
 transportRenderWorld=identity;
 if(!world||!scene)return;
 const auto updated=transportUpdated.load(std::memory_order_acquire);
 uint32_t wanted=identity==transportWorld&&transport_time::fresh(now,updated,transport_time::SelectionFresh)?
  static_cast<uint32_t>(transportSelection.selected()):0;
 // Components with their own color continue through the native ECS path.
 if(wanted&&transportComponent(world,wanted,HighlightType,1).data)wanted=0;
 const auto handle=wanted?transportModel(world,wanted):0;
 if(transportRenderEntity&&(transportRenderEntity!=wanted||transportRenderHandle!=handle)) {
  // Re-resolve the entity, and never touch a recycled model handle.
  if(transportModel(world,transportRenderEntity)==transportRenderHandle) {
   const auto own=transportComponent(world,transportRenderEntity,HighlightType,1);
   transportDraw(scene,transportRenderHandle,own.data?*own.data:0);
  }
  transportRenderEntity=0;transportRenderHandle=0;
 }
 if(wanted&&handle) {
  transportDraw(scene,handle,TransportHighlight);
  if constexpr(TransportDiagnostics) if(transportRenderEntity!=wanted) {
   char message[120];sprintf_s(message,"CarryOn: render outline entity=%u model=%u",wanted,handle);log(message);
  }
  transportRenderEntity=wanted;transportRenderHandle=handle;
 }
}
void __fastcall transportRender(void* context) {
 static std::atomic<bool> reached{false};
 if(!reached.exchange(true))log("CarryOn: player cursor render callback reached");
 originalTransportRender(context);
 // client_cursor_entity runs every player frame (static_render_model only runs
 // when its spawn query is scheduled). Its EcsFxController at row +0x70 owns
 // RenderScene at +8, as used by the native helper 0x29d9e0 -> 0xf6f80.
 alignas(16) Byte row[0xc0]{};
 transportInitRow(context,row,sizeof(row));
 auto descriptor=read<void*>(context,0);
 auto effects=read<void*>(row,0x70);
 if(descriptor&&effects) {
  auto world=read<void*>(descriptor,0x30);auto scene=read<void*>(effects,8);
  transportRenderFrame(world,scene,GetTickCount64());
  if(transportMoveRenderCallback)transportMoveRenderCallback(world,scene);
 }
}
void __fastcall transportVoxelRender(void* context) {
 originalTransportVoxelRender(context);
 // render_voxel_model writes its native color every frame. Apply our visual
 // override afterwards, using its RenderScene dependency (row +0x58).
 alignas(16) Byte row[0x60]{};
 transportInitRow(context,row,sizeof(row));
 auto descriptor=read<void*>(context,0);
 if(descriptor)transportRenderFrame(read<void*>(descriptor,0x30),read<void*>(row,0x58),GetTickCount64());
}
// All commands use native action IDs; configured keyboard/gamepad bindings stay native.
constexpr Byte TransportModeAction=129,TransportCancelAction=126;
using NativeActionDown=bool(__fastcall*)(void*,Byte);
NativeActionDown originalActionDown=nullptr;
struct TransportCommands {bool grab=false,place=false;ULONGLONG at=0;uint64_t generation=0;};
std::mutex transportCommandsMutex;
TransportCommands transportCommands;
void transportPublishCommands(void* mapping,ULONGLONG now) {
 std::lock_guard<std::mutex> lock(transportCommandsMutex);
 transportCommands={mapping&&originalActionDown&&originalActionDown(mapping,10),
  mapping&&originalActionDown&&originalActionDown(mapping,14),now,transportSessionGeneration.load()};
}
bool transportCommand(Byte action,bool down,bool focused,ULONGLONG now) {
 if(action!=TransportModeAction&&action!=TransportCancelAction)return false;
 const auto updated=transportUpdated.load(std::memory_order_acquire);
 const bool fresh=focused&&transport_time::fresh(now,updated,transport_time::SelectionFresh);
 const bool eligible=fresh&&transportEligible.load(std::memory_order_relaxed);
 std::lock_guard<std::mutex> lock(transportKeyboardMutex);
 const bool mode=action==TransportModeAction;
 const auto result=(mode?transportTabKey:transportCancelKey).event(
  down,eligible&&(mode||transportActive.load(std::memory_order_relaxed)));
 if(result.pressed)(mode?transportTabEvents:transportCancelEvents).fetch_add(1,std::memory_order_relaxed);
 return result.consume;
}
bool transportBoundAction(Byte action) {
 std::lock_guard<std::mutex> lock(transportCommandsMutex);
 return transportCommands.generation==transportSessionGeneration.load()&&
  transport_time::fresh(GetTickCount64(),transportCommands.at,transport_time::InputFresh)&&
  (action==10?transportCommands.grab:action==14?transportCommands.place:false);
}
void bindTransport() {
 originalActionDown=reinterpret_cast<NativeActionDown>(base+0xea2990);
 transportZoneQuery=reinterpret_cast<TransportZoneQuery>(base+0xea3a50);
 transportAltarQuery=reinterpret_cast<TransportAltarQuery>(base+0xea3610);
 transportTypeIndex=reinterpret_cast<TypeIndex>(base+0x9b1db0);
 transportRead=reinterpret_cast<ReadComponent>(base+0x9b2b50);
 transportEntity=reinterpret_cast<EntityRecord>(base+0x9b25f0);
 transportMutable=reinterpret_cast<MutableComponent>(base+0x9bf460);
 transportRenderType=read<uint32_t>(reinterpret_cast<void*>(base),0x194ce20+0x90);
 transportModelType=read<uint32_t>(reinterpret_cast<void*>(base),0x1869430+0x90);
 // FNV-1a of the reflected name keen::ecs::VoxelModel.
 transportVoxelType=typeHash("keen::ecs::VoxelModel");
 transportRenderHighlight=reinterpret_cast<RenderHighlight>(base+0xf6e90);
 transportInitRow=reinterpret_cast<InitRow>(base+0x9c7b80);
}
