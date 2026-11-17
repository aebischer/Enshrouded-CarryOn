// Pinned native geometry/render operations, separate from entity mutation.
using TransportPointInZone=bool(__fastcall*)(const void*,const void*,float);
using TransportStaticRead=const Byte*(__fastcall*)(void*,const uint16_t*,uint32_t);
using TransportModelCreate=uint32_t(__fastcall*)(void*,const Guid*,const void*);
using TransportModelRemove=void(__fastcall*)(void*,const RenderSpan*);
using TransportModelMove=void(__fastcall*)(void*,const RenderSpan*,const RenderSpan*);
using TransportModelOpacity=void(__fastcall*)(void*,uint32_t,float);
using TransportModelStaticPose=void(__fastcall*)(void*,uint32_t,bool);
TransportPointInZone transportPointInZone=nullptr;
TransportStaticRead transportStaticRead=nullptr;
TransportModelCreate transportModelCreate=nullptr;
TransportModelRemove transportModelRemove=nullptr;
TransportModelMove transportModelMove=nullptr;
TransportModelOpacity transportModelOpacity=nullptr;
TransportModelStaticPose transportModelStaticPose=nullptr;
uint32_t transportModelResourceType=0,transportCurrentTransformType=0,transportUsedItemType=0;

struct TransportClaim {
 Guid id{};
 uint32_t altar=0;
 explicit operator bool() const {return altar&&id!=Guid{};}
};
// Resolve by altar GUID AND entity ID. Never use the index in the zone array:
// it can change when another altar is placed or removed.
const Byte* transportClaimEntry(const void* zones,const TransportClaim& claim) {
 if(!zones||!claim)return nullptr;
 const auto count=read<size_t>(zones,0x18);const auto zoneEntries=read<const Byte*>(zones,0x10);
 if(!zoneEntries||count>1024)return nullptr;
 for(size_t i=0;i<count;++i) {
  const auto entry=zoneEntries+i*0x48;
  if(!read<Byte>(entry,0x46)&&read<Guid>(entry,0x30)==claim.id&&read<uint32_t>(entry,0x40)==claim.altar)
   return entry;
 }
 return nullptr;
}
TransportClaim transportFindClaim(const void* zones,const void* point) {
 if(!zones||!point||!transportPointInZone)return {};
 const auto count=read<size_t>(zones,0x18);const auto zoneEntries=read<const Byte*>(zones,0x10);
 if(!zoneEntries||count>1024)return {};
 for(size_t i=0;i<count;++i) {
  const auto entry=zoneEntries+i*0x48;
  TransportClaim claim{read<Guid>(entry,0x30),read<uint32_t>(entry,0x40)};
  if(claim&&!read<Byte>(entry,0x46)&&transportPointInZone(entry,point,0.0f))return claim;
 }
 return {};
}
bool transportBoxInClaim(const void* zones,const TransportClaim& claim,const int64_t* bounds);
TransportClaim transportDestinationClaim(const void* zones,const int64_t* bounds) {
 if(!zones||!bounds)return {};
 for(size_t axis=0;axis<3;++axis)if(bounds[axis]>bounds[axis+3])return {};
 const auto count=read<size_t>(zones,0x18);const auto zoneEntries=read<const Byte*>(zones,0x10);
 if(!zoneEntries||count>1024)return {};
 for(size_t i=0;i<count;++i) {
  const auto entry=zoneEntries+i*0x48;
  if(read<Byte>(entry,0x46))continue;
  TransportClaim claim{read<Guid>(entry,0x30),read<uint32_t>(entry,0x40)};
  if(!claim)continue;
  if(transportBoxInClaim(zones,claim,bounds))return claim;
 }
 return {};
}
bool transportBoxInClaim(const void* zones,const TransportClaim& claim,const int64_t* bounds) {
 const auto entry=transportClaimEntry(zones,claim);
 if(!entry||!bounds||!transportPointInZone)return false;
 for(size_t axis=0;axis<3;++axis)if(bounds[axis]>bounds[axis+3])return false;
 // All corners, also for native spherical zones. Testing just the centre
 // would let a large or rotated chest cross its altar's boundary.
 for(unsigned corner=0;corner<8;++corner) {
  int64_t point[3]{};
  for(size_t axis=0;axis<3;++axis)point[axis]=bounds[axis+((corner&(1u<<axis))?3:0)];
  if(!transportPointInZone(entry,point,0.0f))return false;
 }
 return true;
}
struct TransportPreview {
 uintptr_t world=0;
 void* scene=nullptr;
 Guid resource{};
 uint32_t handle=0;
 void forget() {world=0;scene=nullptr;resource={};handle=0;}
 void clear(uintptr_t liveWorld,void* liveScene) {
  // A dead scene owns its teardown. Never free a recycled handle in a new one.
  if(handle&&world==liveWorld&&scene==liveScene&&liveScene&&transportModelRemove) {
   const RenderSpan handles{&handle,1};transportModelRemove(liveScene,&handles);
  }
  forget();
 }
 bool draw(uintptr_t liveWorld,void* liveScene,const Guid& model,const Byte* transform,bool valid) {
  if(!liveWorld||!liveScene||!transform||(model==Guid{})||!transportModelCreate||
     !transportModelRemove||!transportModelMove||!transportModelOpacity||!transportModelStaticPose||!transportRenderHighlight) {
   clear(liveWorld,liveScene);return false;
  }
  if(world!=liveWorld||scene!=liveScene||!(resource==model))clear(liveWorld,liveScene);
  if(!handle) {
   world=liveWorld;scene=liveScene;resource=model;
   handle=transportModelCreate(scene,&resource,transform);
   if(!handle){forget();return false;}
   // Match static_render_model's no-ClientAnimationGraphInput path. A preview
   // has no animation controller to initialize the chest's skeletal pose.
   transportModelStaticPose(scene,handle,true);
   transportModelOpacity(scene,handle,0.45f);
  }
  const RenderSpan handles{&handle,1},transforms{transform,1};
  transportModelMove(scene,&handles,&transforms);
  transportDraw(scene,handle,valid?2:1); // Native ValidState / InvalidState (3 is MutedState).
  return true;
 }
};
void bindTransportPlacement() {
 transportPointInZone=reinterpret_cast<TransportPointInZone>(base+0xea3ab0);
 transportStaticRead=reinterpret_cast<TransportStaticRead>(base+0x9b3b70);
 transportModelCreate=reinterpret_cast<TransportModelCreate>(base+0xf38e0);
 transportModelRemove=reinterpret_cast<TransportModelRemove>(base+0xf67f0);
 transportModelMove=reinterpret_cast<TransportModelMove>(base+0xf7270);
 transportModelOpacity=reinterpret_cast<TransportModelOpacity>(base+0xf6dd0);
 transportModelStaticPose=reinterpret_cast<TransportModelStaticPose>(base+0xf6eb0);
 transportCurrentTransformType=read<uint32_t>(reinterpret_cast<void*>(base),0x194a480+0x90);
 transportUsedItemType=read<uint32_t>(reinterpret_cast<void*>(base),0x1a3f170+0x90);
 transportModelResourceType=typeHash("keen::ecs::ModelResource");
}

