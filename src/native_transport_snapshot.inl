// Template helpers run on the owning world's thread. The source snapshot runs
// on the server post-step boundary and returns copied values only.
transport::Placement transportReadPlacement(const void* bytes);
using TransportEntitySlot=size_t(__fastcall*)(void*,const uint32_t*);
TransportEntitySlot transportEntitySlot=nullptr;
uint32_t transportInventorySetupType=0,transportStaticColliderType=0;
uint32_t transportComfortProviderType=0;
uint32_t transportAttachmentType=0;
const Byte* transportTemplate(void* world,uint32_t entity) {
 if(!world||!entity||!transportEntitySlot)return nullptr;
 const auto slot=transportEntitySlot(static_cast<Byte*>(world)+0x148,&entity);
 if(slot>=read<size_t>(world,0x158))return nullptr;
 const auto records=read<const Byte*>(world,0x188);if(!records)return nullptr;
 const auto record=read<const Byte*>(records,slot*sizeof(void*));
 return record?read<const Byte*>(record,0x28):nullptr;
}
bool transportTemplateSupported(const Byte* descriptor,bool well) noexcept {
 if(!descriptor||read<Byte>(descriptor,0x20)!=(well?13:10))return false;
 const auto model=read<Guid>(descriptor,0);const auto now=GetTickCount64();
 try {
  {
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   const auto& cache=well?transportWellTemplates:transportDoorTemplates;
   const auto known=cache.find(model);if(known!=cache.end())return known->second;
  }
  std::unique_lock<std::mutex> worker(transportResourceWorkMutex,std::try_to_lock);
  // Cached results remain usable while another resource is being prepared.
  {
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   auto& cache=well?transportWellTemplates:transportDoorTemplates;
   const auto known=cache.find(model);if(known!=cache.end())return known->second;
   if(!worker.owns_lock())return false;
   const auto retry=transportTemplateRetry.find(model);
   if(retry!=transportTemplateRetry.end()&&now-retry->second<1000)return false;
   transportTemplateRetry[model]=now;
  }
  const auto db=transportResourceDb.load();if(!db)return false;
  const auto bytes=source(db,Key{model,0x39768775,0,0});
  bool supported=false;
  if(well)supported=isWaterWellTemplate(bytes);
  else {
   const auto components=array(bytes,16,12);unsigned traits=0;bool noSave=false;
   for(uint32_t i=0;i<components.count;++i) {
    const auto type=u32(bytes,components.at+size_t(i)*12);
    if(type==typeHash("keen::ecs::InteractionToggle"))traits|=1;
    if(type==typeHash("keen::ecs::ServerAnimationGraphSlotComponent"))traits|=2;
    if(type==typeHash("keen::ecs::SubEntities"))traits|=4;
    if(type==typeHash("keen::ecs::Collider"))traits|=8;
    if(type==typeHash("keen::ecs::DoNotSave"))noSave=true;
   }
   supported=bytes[8]==10&&traits==15&&!noSave;
  }
  std::lock_guard<std::mutex> lock(transportCatalogMutex);
  (well?transportWellTemplates:transportDoorTemplates).emplace(model,supported);
  transportTemplateRetry.erase(model);return supported;
 }catch(const std::exception&){return false;}
}
bool transportIsDoor(const Byte* descriptor){return transportTemplateSupported(descriptor,false);}
bool transportIsWell(const Byte* descriptor){return transportTemplateSupported(descriptor,true);}
uint8_t transportRestTemplate(const Byte* descriptor) noexcept {
 if(!descriptor||read<Byte>(descriptor,0x20)!=8)return RestNone;
 const auto model=read<Guid>(descriptor,0);const auto now=GetTickCount64();
 try {
  {
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   const auto found=transportRestTemplates.find(model);if(found!=transportRestTemplates.end())return found->second;
  }
  std::unique_lock<std::mutex> work(transportResourceWorkMutex,std::try_to_lock);if(!work.owns_lock())return RestNone;
  {
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   const auto found=transportRestTemplates.find(model);if(found!=transportRestTemplates.end())return found->second;
   const auto retry=transportTemplateRetry.find(model);
   if(retry!=transportTemplateRetry.end()&&now-retry->second<1000)return RestNone;
   transportTemplateRetry[model]=now;
  }
  const auto db=transportResourceDb.load();if(!db)return RestNone;
  RestFurnitureDefinition definition{};
  auto result=restFurnitureTemplate(source(db,Key{model,0x39768775,0,0}),&definition);
  if(result&RestCompound) {
   uint8_t common=0;
   for(size_t i=0;i<definition.count;++i) {
    auto& child=definition.children[i];
    child.flags=restFurnitureTemplate(source(db,Key{child.model,0x39768775,0,0}),nullptr,true);
    const auto kind=uint8_t(child.flags&(RestBed|RestSeat));
    if(!kind||(child.flags&(RestCompound|RestSavedAssignments))||(common&&kind!=common)){result=RestNone;break;}
    common=kind;
   }
   if(result) {
    if((result&(RestBed|RestSeat))&&(result&(RestBed|RestSeat))!=common)result=RestNone;
    else if((result&RestSavedAssignments)&&(!(common&RestBed)||definition.count!=2))result=RestNone;
    else {result|=common;definition.flags=result;}
   }
  }
  std::lock_guard<std::mutex> lock(transportCatalogMutex);
  if(result&RestCompound)transportRestAssemblies[model]=definition;
  transportRestTemplates.emplace(model,result);transportTemplateRetry.erase(model);return result;
 }catch(const std::exception&){return RestNone;}
}
bool transportRestAssembly(const Byte* descriptor,RestFurnitureDefinition& out) {
 if(!(transportRestTemplate(descriptor)&RestCompound))return false;
 std::lock_guard<std::mutex> lock(transportCatalogMutex);
 const auto found=transportRestAssemblies.find(read<Guid>(descriptor,0));
 if(found==transportRestAssemblies.end())return false;out=found->second;return true;
}
uint32_t transportRestRoot(void* world,uint32_t entity) {
 // Client helpers replicate DespawnLink, but not OwnerRelationship/SubEntities.
 // Server snapshots separately validate both links and the exact child array.
 const auto link=transportComponent(world,entity,typeHash("keen::ecs::DespawnLink"),8);
 const auto parent=link.data?read<uint32_t>(link.data,0):0;
 if(!parent||parent==entity)return entity;
 RestFurnitureDefinition definition{};
 if(!transportRestAssembly(transportTemplate(world,parent),definition))return entity;
 const auto descriptor=transportTemplate(world,entity);if(!descriptor)return entity;
 const auto model=read<Guid>(descriptor,0);
 for(size_t i=0;i<definition.count;++i)if(definition.children[i].model==model)return parent;
 return entity;
}
bool transportDecorFamily(Byte family){return family==8||family==10||family==13;}
uint32_t transportDecorPlacement(const Byte* descriptor,uint32_t entity) noexcept {
 if(!descriptor||(entity&0x80000000u)||!transportDecorFamily(read<Byte>(descriptor,0x20)))return 0;
 const auto model=read<Guid>(descriptor,0);const auto now=GetTickCount64();
 try {
  {
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   const auto found=transportPlacementCatalog.find(model);if(found!=transportPlacementCatalog.end())return found->second;
  }
  std::unique_lock<std::mutex> worker(transportResourceWorkMutex,std::try_to_lock);
  {
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   const auto found=transportPlacementCatalog.find(model);if(found!=transportPlacementCatalog.end())return found->second;
   // A published immutable blob may already exist if a later cache insertion failed.
   const auto built=transportFloorPlacements.find(model);
   if(built!=transportFloorPlacements.end()) {
    const auto id=u32(built->second,0);transportPlacementCatalog.emplace(model,id);return id;
   }
   if(!worker.owns_lock()||transportCatalogRegistries.empty()||transportUnsupportedFloorModels.count(model))return 0;
   const auto retry=transportFloorRetry.find(model);
   if(retry!=transportFloorRetry.end()&&now-retry->second<1000)return 0;
   transportFloorRetry[model]=now;
  }
  const auto db=transportResourceDb.load();if(!db)return 0;
  const Key profile{guid("e3a56abbb97f3946ad1fbd266c5ccace"),ItemType,0,0};
  if(transportFloorProfile.empty())transportFloorProfile=source(db,profile);
  const auto resource=source(db,Key{model,0x39768775,0,0});
  Bytes bytes;
  try{bytes=makeFloorPlacement(transportFloorProfile,resource,model);}
  catch(const std::bad_alloc&){return 0;}
  catch(const std::runtime_error& error) {
   if(transportInspectObjects.load()) {
    char key[33]{};for(size_t i=0;i<16;++i)sprintf_s(key+i*2,3,"%02x",unsigned(model[i]));
    char message[384];sprintf_s(message,"CarryOn: floor placement refused template=%s reason=%s",key,error.what());log(message);
   }
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   transportUnsupportedFloorModels.insert(model);transportFloorRetry.erase(model);return 0;
  }
  const auto id=u32(bytes,0);
  {
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   transportFloorPlacements.emplace(model,std::move(bytes));transportPlacementCatalog.emplace(model,id);
   transportFloorRetry.erase(model);
  }
  log("CarryOn: floor placement descriptor prepared");return id;
 }catch(const std::exception&){return 0;}
}
TransportPropClass transportClassifyProp(void* world,uint32_t entity,const Byte* descriptor) {
 TransportPropClass result{};
 if(!descriptor)descriptor=transportTemplate(world,entity);
 if(!descriptor||!transportDecorFamily(read<Byte>(descriptor,0x20))||
    transportComponent(world,entity,transportVoxelType,8).data)return result;
 const auto family=read<Byte>(descriptor,0x20);
 const bool interactive=family==10||family==13||
  transportComponent(world,entity,typeHash("keen::ecs::ClientInteractionOffer"),12).data||
  transportComponent(world,entity,typeHash("keen::ecs::InteractionAttachment"),48).data;
 if(interactive) {
  if(!transportInteractive.load())return result;
  result.door=transportIsDoor(descriptor);result.well=transportIsWell(descriptor);
  if(!result.door&&!result.well)result.rest=transportRestTemplate(descriptor);
  if(!result.door&&!result.well&&!result.rest)return result;
 } else if(!transportObjects.load())return result;
 result.item=transportDecorPlacement(descriptor,entity);return result;
}
uint32_t transportPlacementItem(void* world,uint32_t entity) {
 const auto descriptor=transportTemplate(world,entity);
 if(descriptor&&transportDecorFamily(read<Byte>(descriptor,0x20)))return transportDecorPlacement(descriptor,entity);
 const auto used=transportComponent(world,entity,transportUsedItemType,4);
 return used.data?read<uint32_t>(used.data,0):0;
}
bool transportRestState(void* world,uint32_t entity,uint8_t flags,uint32_t& assignment,
 std::array<Byte,48>& attachmentState,std::array<Byte,40>& interactionState) {
  const auto attachment=transportComponent(world,entity,typeHash("keen::ecs::InteractionAttachment"),48);
  const auto offer=transportComponent(world,entity,typeHash("keen::ecs::InteractionOfferRuntime"),56);
  if(!attachment.data)return false;
  std::memcpy(attachmentState.data(),attachment.data,attachmentState.size());
  if(flags&RestNoOffer)return !offer.data;
  if(!offer.data)return false;
  const auto sequence=read<uint32_t>(offer.data,8);
  if(sequence!=((flags&RestBed)?0xc2ee9b13u:0x2804c2f7u)||read<uint32_t>(offer.data,12))return false;
  if(flags&RestLock) {
   const auto lock=transportComponent(world,entity,typeHash("keen::ecs::DynamicInteractionLock"),2);
   if(!lock.data||read<uint16_t>(lock.data,0))return false;
  }
  if(flags&RestNpcTarget) {
   const auto npc=transportComponent(world,entity,typeHash("keen::ecs::DynamicActiveNpcTarget"),32);
   if(!npc.data||read<uint32_t>(npc.data,24))return false;
  }
  // lastAcceptData is historical; only a guest still attached to THIS object
  // blocks the move. Never clear or rewrite actor/NPC attachment components.
  if(const auto guest=read<uint32_t>(offer.data,24)) {
   const auto link=transportComponent(world,guest,typeHash("keen::ecs::InteractionAttachmentGuest"),8);
   if(link.data&&(read<uint32_t>(link.data,0)==entity||read<uint32_t>(link.data,4)==entity))return false;
  }
  if(flags&RestAssignment) {
   const auto bed=transportComponent(world,entity,typeHash("keen::ecs::NpcBed"),4);
   if(!bed.data)return false;assignment=read<uint32_t>(bed.data,0);
  }
  std::memcpy(attachmentState.data(),attachment.data,attachmentState.size());
  std::memcpy(interactionState.data(),offer.data,interactionState.size());
 return true;
}
struct TransportSourceSnapshot {
 transport::MoveSource source{};
 std::array<Byte,84> inventorySetup{};
 uint32_t item=0;
 uint64_t collider=0;
 bool decor=false,door=false,well=false;
 uint8_t rest=0;
 uint32_t bedAssignment=0;
 std::array<Byte,48> restAttachment{};
 std::array<Byte,40> restInteraction{};
 std::array<Byte,24> toggleState{};
 std::array<uint32_t,16> children{};
 std::array<std::array<Byte,48>,16> childAttachments{};
 std::array<Guid,16> childTemplates{};
 std::array<uint64_t,16> childColliders{};
 uint8_t restChildCount=0;
 std::array<transport::Placement,16> childPoses{};
 std::array<std::array<float,3>,16> childOffsets{};
 std::array<std::array<float,4>,16> childRotations{};
 std::array<std::array<Byte,40>,16> childInteractions{};
 std::array<uint32_t,16> childAssignments{};
 std::array<Byte,12> savedAssignments{};
 std::array<Byte,16> comfortProvider{};
 std::array<Byte,16> attachmentState{};
};
bool transportSnapshotSource(void* world,const void* zones,uint64_t session,uint32_t entity,
 TransportSourceSnapshot& out,const TransportClaim* selectedClaim=nullptr) {
 out={};
 if(!world||!zones||!session||!entity||!transportEntitySlot)return false;
 const auto templateId=transportTemplate(world,entity);if(!templateId)return false;
 out.decor=transportDecorFamily(read<Byte>(templateId,0x20));
 const auto classification=out.decor?transportClassifyProp(world,entity,templateId):TransportPropClass{};
 if(out.decor?!classification.item:!transportChests.load())return false;
 const auto transform=transportComponent(world,entity,transportCurrentTransformType,56);
 const auto inventory=transportComponent(world,entity,transportInventorySetupType,84);
 const auto item=transportComponent(world,entity,transportUsedItemType,4);
 out.door=classification.door;out.well=classification.well;out.rest=classification.rest;
 if(out.rest) {
  if(!transportRestState(world,entity,out.rest,out.bedAssignment,out.restAttachment,out.restInteraction))return false;
 }
 if(out.well) {
  // Preserve live reserve/refill components in place. Do not snapshot/restore
  // quantities: water may refill naturally while the preview is being moved.
  const auto fuel=transportComponent(world,entity,typeHash("keen::ecs::Fuel"),16);
  const auto refill=transportComponent(world,entity,typeHash("keen::ecs::DynamicFuelSetup"),4);
  const auto npc=transportComponent(world,entity,typeHash("keen::ecs::DynamicActiveNpcTarget"),32);
  const auto animal=transportComponent(world,entity,typeHash("keen::ecs::DynamicAnimalTarget"),8);
  if(!fuel.data||!refill.data||!npc.data||!animal.data||read<uint32_t>(npc.data,24)||read<uint32_t>(animal.data,4))return false;
 }
 const auto collider=transportComponent(world,entity,out.door?typeHash("keen::ecs::Collider"):transportStaticColliderType,out.door?24:16);
 if(!transform.data||!collider.data)return false;
 if(out.door) {
  const auto toggle=transportComponent(world,entity,typeHash("keen::ecs::ServerToggleState"),24);
  const auto children=transportComponent(world,entity,typeHash("keen::ecs::DynamicSubEntities"),64);
  if(!toggle.data||!children.data||read<Byte>(collider.data,17)!=0)return false;
  std::memcpy(out.toggleState.data(),toggle.data,24);std::memcpy(out.children.data(),children.data,64);
  for(size_t i=0;i<out.children.size();++i)if(const auto child=out.children[i]) {
   if(child==entity)return false;
   for(size_t j=0;j<i;++j)if(out.children[j]==child)return false;
   const auto attachment=transportComponent(world,child,typeHash("keen::ecs::ServerSlotAttachment"),112);
   const auto childCollider=transportComponent(world,child,typeHash("keen::ecs::Collider"),24);
   const auto childTemplate=transportTemplate(world,child);
   if(!attachment.data||read<uint32_t>(attachment.data,44)!=entity||!childCollider.data||!childTemplate)return false;
   std::memcpy(out.childAttachments[i].data(),attachment.data,48);
   out.childTemplates[i]=read<Guid>(childTemplate,0);out.childColliders[i]=read<uint64_t>(childCollider.data,8);
   if(!out.childColliders[i])return false;
  }
 }

 // A chest must already have initialized storage. This does not classify an
 // arbitrary storage-bearing entity as a chest: caller must verify its item.
 if(out.decor) {
  const auto index=transportIndex(world,transportComfortProviderType);
  const auto provider=index?transportStaticRead(world,index,entity):nullptr;

  const auto attachment=transportComponent(world,entity,transportAttachmentType,16);
  // Native enum: 0 Uninitialized, 1 Detached, 2 Attached. Bottles can be
  // settled Detached with an infinite searchStartTime. Attached props may
  // reference a support: retain that link during preview, invalidate at commit.
  if(attachment.data) {
   const auto state=read<Byte>(attachment.data,12);
   const bool settled=state==2||(state==1&&read<int64_t>(attachment.data,0)==INT64_MAX);
   const auto support=read<uint32_t>(attachment.data,8);
   if(!settled||(support&&(state!=2||support==entity))||read<Byte>(attachment.data,13)||
      read<Byte>(attachment.data,14)||read<Byte>(attachment.data,15))return false;
   std::memcpy(out.attachmentState.data(),attachment.data,out.attachmentState.size());
  }
  if(provider)std::memcpy(out.comfortProvider.data(),provider,out.comfortProvider.size());
  out.item=classification.item;
 } else {
  if(!inventory.data||!item.data||!read<Byte>(inventory.data,0x53))return false;
  out.item=read<uint32_t>(item.data,0);
  std::memcpy(out.inventorySetup.data(),inventory.data,out.inventorySetup.size());
 }
 const auto claim=selectedClaim?*selectedClaim:transportFindClaim(zones,transform.data);if(!claim)return false;
 const auto claimEntry=transportClaimEntry(zones,claim);
 if(!claimEntry||!transportPointInZone||!transportPointInZone(claimEntry,transform.data,0.0f))return false;
 auto& source=out.source;
 source.session=session;source.entity=entity;
 source.templateId=read<transport::Identity>(templateId,0);
 source.claim=read<transport::Identity>(claim.id.data(),0);source.claimEntity=claim.altar;
 source.original.position=read<std::array<int64_t,3>>(transform.data,0);
 source.original.rotation=read<std::array<float,4>>(transform.data,24);
 source.original.scale=read<std::array<float,3>>(transform.data,40);
 out.collider=read<uint64_t>(collider.data,8);
 if(!source.templateId||!source.original.movable()||!out.item||!out.collider){out={};return false;}
 if(out.rest&RestCompound) {
  RestFurnitureDefinition definition{};
  const auto children=transportComponent(world,entity,typeHash("keen::ecs::DynamicSubEntities"),64);
  if(!children.data||!transportRestAssembly(templateId,definition))return false;
  std::memcpy(out.children.data(),children.data,64);out.restChildCount=definition.count;
  for(size_t i=0;i<out.children.size();++i) {
   const auto child=out.children[i];
   if(i>=definition.count){if(child)return false;continue;}
   if(!child||child==entity)return false;
   for(size_t j=0;j<i;++j)if(out.children[j]==child)return false;
   const auto descriptor=transportTemplate(world,child);
   const auto owner=transportComponent(world,child,typeHash("keen::ecs::OwnerRelationship"),4);
   const auto link=transportComponent(world,child,typeHash("keen::ecs::DespawnLink"),8);
   const auto pose=transportComponent(world,child,transportCurrentTransformType,56);
   const auto shape=transportComponent(world,child,transportStaticColliderType,16);
   const auto& expected=definition.children[i];
   if(!descriptor||read<Guid>(descriptor,0)!=expected.model||!owner.data||read<uint32_t>(owner.data,0)!=entity||
      !link.data||read<uint32_t>(link.data,0)!=entity||!pose.data||!shape.data)return false;
   out.childTemplates[i]=expected.model;out.childColliders[i]=read<uint64_t>(shape.data,8);
   out.childPoses[i]=transportReadPlacement(pose.data);out.childOffsets[i]=expected.offset;out.childRotations[i]=expected.rotation;
   transport::Placement authored{};
   if(!out.childColliders[i]||!transport::restChildPlacement(source.original,expected.offset,expected.rotation,authored)||
      !transport::restChildMatches(out.childPoses[i],authored)||
      !transportRestState(world,child,expected.flags,out.childAssignments[i],out.childAttachments[i],out.childInteractions[i]))return false;
  }
  if(out.rest&RestSavedAssignments) {
   const auto saved=transportComponent(world,entity,typeHash("keen::ecs::StoreAssociatedNpcIds"),12);
   if(!saved.data)return false;std::memcpy(out.savedAssignments.data(),saved.data,12);
  }
 }
 // 'movable' and 'available' deliberately remain false until item classification
 // and the actor/interaction-lock checks have succeeded on this same thread.
 return true;
}
bool transportSameSourceState(const TransportSourceSnapshot& before,const TransportSourceSnapshot& now) {
 return before.item==now.item&&before.collider==now.collider&&before.decor==now.decor&&before.door==now.door&&before.well==now.well&&
  before.rest==now.rest&&before.bedAssignment==now.bedAssignment&&before.restAttachment==now.restAttachment&&before.restInteraction==now.restInteraction&&
  before.restChildCount==now.restChildCount&&before.childOffsets==now.childOffsets&&before.childRotations==now.childRotations&&
  before.childAssignments==now.childAssignments&&before.childInteractions==now.childInteractions&&before.savedAssignments==now.savedAssignments&&
  before.toggleState==now.toggleState&&before.children==now.children&&before.childAttachments==now.childAttachments&&
  before.childTemplates==now.childTemplates&&before.childColliders==now.childColliders&&
  before.inventorySetup==now.inventorySetup&&before.comfortProvider==now.comfortProvider&&before.attachmentState==now.attachmentState;
}
