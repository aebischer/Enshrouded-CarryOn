struct MoveFixture {
 Byte world[0x190]{},client[0x190]{},registry[0x100]{},record[0x38]{},server[0x1798]{},health[40]{};
 bool missingHealth=false,sourceGone=false,manySources=false;
 Byte zones[0x20]{},zone[0x48]{},transform[56]{},actorTransform[56]{},inventory[84]{},slots[216]{},bar[96]{},lock[2]{};
 Byte item[4]{},collider[16]{},resource[0x40]{},system[0x1a8]{},manager[0x90]{},collision[0x80]{};
 Byte info[0x400]{},pincer[4]{},descriptor[0x38]{},context[0x6a0]{},config[0xa0]{};
 Byte actor[0xc38]{};
 Byte render[176]{},model[4]{},clientTransform[56]{};
 bool counter=false,missingComfort=false;
 bool door=false,well=false,missingFuel=false;
 Byte fuel[16]{},refill[4]{},npcTarget[32]{},animalTarget[8]{};
 Byte offer[12]{},dynamicCollider[24]{},toggle[24]{},children[64]{},childAttachment[112]{},childCollider[24]{},childRecord[0x38]{},childTemplate[0x28]{};
 bool badAttachmentWrite=false;
 unsigned attachmentWrites=0;
 const Byte* placementOverride=nullptr;
 Byte counterDescriptor[0x28]{},comfort[16]{},attachment[16]{};
 bool separateClientTransform=false,quantizedRotation=false,clientSourceAbsent=false;
 unsigned renderWrites=0,modelMoves=0;
 const Byte* records[2]{record,childRecord};const Byte* collisions[1]{collision};
 void* iterator=descriptor;
 Guid templateId{},claimId{};
 unsigned writes=0,moves=0;uint32_t refusal=0;bool focused=true,badWrite=false,rotated=false;
 uintptr_t fullWorld() const {return reinterpret_cast<uintptr_t>(world)-0xe179c0;}
};
static MoveFixture* moveFixture=nullptr;
static Byte runtimeScreen=1;
static ULONGLONG runtimeUiAge=0,runtimeRowAge=0;
static bool runtimeOldSession=false;
static TransportTravelState runtimeTravelSnapshot(){
 const auto now=GetTickCount64();
 return {transportSessionGeneration.load()-(runtimeOldSession?1:0),transportMove.world.client,
  now-runtimeUiAge,now-runtimeRowAge,runtimeScreen,transportActive.load(),transportEligible.load()};
}
static bool runtimeFocus(){return moveFixture->focused;}
static const uint16_t* __fastcall runtimeIndex(void*,const uint32_t* hashValue) {
 static const uint16_t indices[]{0,1,2,3,4,5,6,7,8,9,10,11,12};
 static const uint16_t fuel=21,refill=22,npc=23,animal=24;
 static const uint16_t health=25;
 if(*hashValue==typeHash("keen::ecs::Health"))return &health;
 if(*hashValue==typeHash("keen::ecs::Fuel"))return &fuel;
 if(*hashValue==typeHash("keen::ecs::DynamicFuelSetup"))return &refill;
 if(*hashValue==typeHash("keen::ecs::DynamicActiveNpcTarget"))return &npc;
 if(*hashValue==typeHash("keen::ecs::DynamicAnimalTarget"))return &animal;
 static const uint16_t comfort=13,attachment=14,offer=15,collider=16,toggle=17,children=18,slot=19,clientCollider=20;
 if(*hashValue==typeHash("keen::ecs::ClientInteractionOffer"))return &offer;
 if(*hashValue==typeHash("keen::ecs::Collider"))return &collider;
 if(*hashValue==typeHash("keen::ecs::ServerToggleState"))return &toggle;
 if(*hashValue==typeHash("keen::ecs::DynamicSubEntities"))return &children;
 if(*hashValue==typeHash("keen::ecs::ServerSlotAttachment"))return &slot;
 if(*hashValue==typeHash("keen::ecs::ClientCollider"))return &clientCollider;
 if(*hashValue==13)return &comfort;if(*hashValue==14)return &attachment;
 if(*hashValue>=1&&*hashValue<=12)return indices+*hashValue;
 if(*hashValue==InventoryType)return indices+6;
 if(*hashValue==typeHash("keen::ecs::StaticClientCollider"))return indices+10;
 return nullptr;
}
static size_t __fastcall runtimeEntitySlot(void* table,const uint32_t* entity){
 if(moveFixture->manySources&&*entity>=2000&&*entity<2032)return 0;
 if(moveFixture->sourceGone&&*entity==1217)return 1;
 if(moveFixture->clientSourceAbsent&&table==moveFixture->client+0x148&&*entity==1217)return 1;
 return *entity==1217?0:1;
}
static uint32_t __fastcall runtimeCurrentEntity(const void*,const void*,const void*){return 9;}
static ComponentView* __fastcall runtimeRead(ComponentView* out,void* world,const uint16_t* index,uint32_t entity) {
 auto& f=*moveFixture;*out={};
 if(f.manySources&&entity>=2000&&entity<2032)entity=1217;
 if(f.clientSourceAbsent&&world==f.client&&entity==1217)return out;
 if(entity==9) {
  if(*index==25&&world==f.world&&!f.missingHealth)*out={f.health,40};
  if(*index==1)*out={f.actorTransform,56};if(*index==5)*out={f.slots,216};return out;
 }
 if(entity==500&&*index==6){*out={f.bar,96};return out;}
 if(f.door&&entity==1218) {
  if(*index==16)*out={f.childCollider,24};if(*index==19)*out={f.childAttachment,112};return out;
 }
 if(entity!=1217)return out;
 if(f.well) {
  if(*index==21&&!f.missingFuel)*out={f.fuel,16};if(*index==22)*out={f.refill,4};
  if(*index==23)*out={f.npcTarget,32};if(*index==24)*out={f.animalTarget,8};
  if(*index==15)*out={f.offer,12};if(*index>=15)return out;
 }
 if(f.door) {
  if(*index==15)*out={f.offer,12};if(*index==16)*out={f.dynamicCollider,24};
  if(*index==17)*out={f.toggle,24};if(*index==18)*out={f.children,64};if(*index==20)*out={f.collider,16};
  if(*index>=15)return out;
 }
 if(f.counter&&(*index==2||*index==3||*index==7))return out;
 if(*index==14){*out={f.attachment,16};return out;}
 switch(*index){case 1:*out={f.separateClientTransform&&world==f.client?f.clientTransform:f.transform,56};break;case 2:*out={f.inventory,84};break;
 case 3:*out={f.item,4};break;case 4:case 10:*out={f.collider,16};break;case 7:*out={f.lock,2};break;
 case 11:*out={f.render,176};break;case 12:*out={f.model,4};break;}
 return out;
}
static const Byte* __fastcall runtimeStatic(void*,const uint16_t* index,uint32_t entity) {
 if(entity!=1217)return nullptr;
 if(*index==13)return moveFixture->missingComfort?nullptr:moveFixture->comfort;
 return *index==8?moveFixture->resource:*index==9?moveFixture->templateId.data():nullptr;
}
static ComponentView* __fastcall runtimeWrite(ComponentView* out,void* world,void* tracker,const uint16_t* index,uint32_t entity,Byte flags) {
 auto& f=*moveFixture;
 CHECK(reinterpret_cast<uintptr_t>(tracker)==reinterpret_cast<uintptr_t>(world)+0x7bc030);
 if(world==f.client) {
  CHECK(*index==11&&entity==1217&&!flags);++f.renderWrites;
  *out={f.render,f.badWrite?size_t(175):size_t(176)};return out;
 }
 CHECK(world==f.world);
 if(*index==14){CHECK(entity==1217&&!flags);++f.attachmentWrites;*out={f.attachment,f.badAttachmentWrite?size_t(15):size_t(16)};return out;}
 CHECK(*index==1&&entity==1217&&!flags);++f.writes;*out={f.transform,f.badWrite?size_t(55):size_t(56)};return out;
}
static size_t __fastcall runtimeCollisionSlot(void*,const uint64_t* handle){CHECK(*handle==163);return 0;}
static void __fastcall runtimeCollisionMove(void* manager,void*,bool replay,uint64_t handle,const float* transform,const void* resource) {
 auto& f=*moveFixture;CHECK(manager==f.manager&&!replay&&handle==163&&resource==f.resource+8);
 const float expectedX=f.moves?static_cast<float>(read<int64_t>(f.transform,0))*0x1p-32f:8.0f;
 CHECK(transform[0]==expectedX&&transform[1]==-2&&transform[3]==1);
 CHECK(transform[7]==(f.quantizedRotation?(f.moves?read<float>(f.clientTransform,36):transportMove.transaction.request().destination.rotation[3]):(f.rotated?0.70710677f:1.0f)));++f.moves;
}
static void __fastcall runtimeModelMove(void* scene,const RenderSpan* handles,const RenderSpan* transforms) {
 auto& f=*moveFixture;CHECK(scene==f.client&&handles->count==1&&transforms->count==1);
 CHECK(read<uint32_t>(handles->data,0)==77);
 CHECK(read<int64_t>(transforms->data,0)==read<int64_t>(f.transform,0));++f.modelMoves;
}
static const Byte* __fastcall runtimeItem(uint32_t item,void*){return item==(moveFixture->counter?TransportCounterItem:42)?moveFixture->info:nullptr;}
static void __fastcall runtimeConfig(void* config,void*,const void* info,const void* effective,bool alternate,bool primary) {
 CHECK(info==(moveFixture->placementOverride?moveFixture->placementOverride:moveFixture->info)&&effective==info&&!alternate&&primary);write(config,0x60,info);
}
static void __fastcall runtimeValidate(uint32_t* flags,void*,const void* config,const void*,Byte permissions,Byte special,float) {
 CHECK(permissions==0x91&&special==0xa7);
 if(read<const Byte*>(config,0x60)==(moveFixture->placementOverride?moveFixture->placementOverride:moveFixture->info))*flags=moveFixture->refusal;
}
static void __fastcall runtimeCastConfig(void*,void*,const void*) {}
static void __fastcall runtimeCast(void* hit,void*,const void*,const void*) {write<Byte>(hit,0x24,0xa7);write<Byte>(hit,0x28,1);}
static void __fastcall runtimeSolve(uint32_t* flags,void* transform,bool*,bool*,float*,void* context,const void*,const void*) {
 const auto actor=read<const Byte*>(context,0x108);CHECK(actor!=moveFixture->actor);
 CHECK(read<uint32_t>(actor,0xc00)&(1u<<24));CHECK(!read<uint32_t>(moveFixture->actor,0xc00));
 *flags=0;write<int64_t>(transform,0,int64_t(8)<<32);write<int64_t>(transform,8,-(int64_t(2)<<32));
 if(moveFixture->rotated)write(transform,24,std::array<float,4>{0,0.70710677f,0,0.70710677f});
}
static void runtimeSetup(MoveFixture& f) {
 runtimeScreen=1;runtimeUiAge=0;runtimeRowAge=0;runtimeOldSession=false;transportTravelSnapshot=&runtimeTravelSnapshot;transportTravelPaused=false;
 moveFixture=&f;transportMove=TransportMoveRuntime{};transportObjects=true;transportPlacementCatalog.clear();transportCatalogRegistries.clear();
 transportFloorPlacements.clear();transportUnsupportedFloorModels.clear();transportFloorProfile.clear();transportResourceDb=nullptr;
 transportCatalogProgress.clear();transportFloorRetry.clear();transportTemplateRetry.clear();
 transportDoorTemplates.clear();transportWellTemplates.clear();transportInteractive=true;
 transportTypeIndex=&runtimeIndex;transportRead=&runtimeRead;transportStaticRead=&runtimeStatic;
 transportEntitySlot=&runtimeEntitySlot;transportCurrentEntity=&runtimeCurrentEntity;transportWorldWrite=&runtimeWrite;
 transportColliderSlot=&runtimeCollisionSlot;transportColliderMove=&runtimeCollisionMove;
 transportItemLookup=&runtimeItem;transportCursorConfig=&runtimeConfig;originalTransportValidate=&runtimeValidate;
 transportCastConfig=&runtimeCastConfig;transportCast=&runtimeCast;transportSolve=&runtimeSolve;
 transportMoveFocused=&runtimeFocus;transportPointInZone=&previewContains;
 transportCurrentTransformType=1;transportInventorySetupType=2;transportUsedItemType=3;transportStaticColliderType=4;
 transportComfortProviderType=13;transportAttachmentType=14;
 transportSlotsType=5;transportLockType=7;transportColliderResourceType=8;transportModelResourceType=9;
 transportRenderType=11;transportModelType=12;transportModelMove=&runtimeModelMove;
 write<uint32_t>(f.model,0,77);write<uint32_t>(f.render,168,123);write<uint32_t>(f.render,172,456);
 f.templateId[0]=9;f.claimId[0]=7;
 write(f.world,0,static_cast<Byte*>(f.registry));write(f.client,0,static_cast<Byte*>(f.registry));
 write<size_t>(f.world,0x158,1);write(f.world,0x188,f.records+0);write(f.record,0x28,f.templateId.data());
 write<size_t>(f.client,0x158,1);write(f.client,0x188,f.records+0);
 write(f.zones,0x10,static_cast<Byte*>(f.zone));write<size_t>(f.zones,0x18,1);
 write(f.zone,0x30,f.claimId);write<uint32_t>(f.zone,0x40,1027);
 for(size_t axis=0;axis<3;++axis){write<int64_t>(f.zone,axis*8,-(int64_t(100)<<32));write<int64_t>(f.zone,(axis+3)*8,int64_t(100)<<32);write<float>(f.info,0x3dc+axis*4,1);}
 const auto pose=transportNativePlacement(transport::Placement{});std::memcpy(f.transform,pose.data(),56);std::memcpy(f.actorTransform,pose.data(),56);
 std::memcpy(f.render,pose.data(),56);
 write<int64_t>(f.actorTransform,0,int64_t(2)<<32);
 f.inventory[0x53]=1;write<uint32_t>(f.inventory,0,1217);write<uint32_t>(f.inventory,4,1218);
 write<uint32_t>(f.item,0,42);write<uint64_t>(f.collider,8,163);
 write<uint32_t>(f.slots,160,500);write<uint32_t>(f.bar,0,hash(Item));
 write<int32_t>(f.resource,0,8);write<uint32_t>(f.resource,4,1);
 write(f.system,0x1a0,static_cast<Byte*>(f.manager));write<size_t>(f.manager,0x58,1);write(f.manager,0x88,f.collisions+0);
 write<uint32_t>(f.collision,0x54,1217);write<uint64_t>(f.collision,0x58,163);write(f.collision,0x68,static_cast<Byte*>(f.resource+8));f.collision[0x78]=1;
 transportServerCollision.world=reinterpret_cast<uintptr_t>(f.world);transportServerCollision.system=reinterpret_cast<uintptr_t>(f.system);transportServerCollision.pathGrid=0;
 transportClientCollision.world=reinterpret_cast<uintptr_t>(f.client);transportClientCollision.system=reinterpret_cast<uintptr_t>(f.system);transportClientCollision.pathGrid=0;
 write(f.server,0x1790,static_cast<Byte*>(f.zones));write<uint32_t>(f.info,0,42);write(f.info,0x3c0,f.templateId);
 write<uint32_t>(f.pincer,0,hash(Item));write(f.config,0x60,static_cast<Byte*>(f.pincer));f.config[0x70]=1;
 write(f.descriptor,0x30,static_cast<Byte*>(f.world));write(f.context,0,&f.iterator);
 write(f.context,0x108,static_cast<Byte*>(f.actor));
 write(f.context,0x190,static_cast<Byte*>(f.zones));write(f.context,0x1b0,static_cast<Byte*>(f.registry));
 transportActive=true;transportEligible=true;transportChests=true;transportUpdated=GetTickCount64();
 write<uint32_t>(f.health,0,0x8eb84995);write<uint16_t>(f.health,4,8);f.health[6]=1;f.health[7]=8;
 write<int32_t>(f.health,8,100);write<int32_t>(f.health,36,100);
 transportMovePostStep(f.server,f.fullWorld());
 transportMove.world.client=reinterpret_cast<uintptr_t>(f.client);transportMove.world.actor=9;
}
static void runtimeGrab(MoveFixture& f) {
 transportMove.input.grab=true;transportMove.input.requestedEntity=1217;transportMove.input.requestedItem=f.counter?TransportCounterItem:42;transportMove.input.requestedAt=GetTickCount64();
 transportMovePostStep(f.server,f.fullWorld());
 CHECK(transportMove.transaction.stage()==transport::MoveStage::Preview&&transportHolding.load());
 CHECK(!f.writes&&!f.moves&&read<int64_t>(f.transform,0)==0);
}
static void runtimeQueue() {
 transportMove.preview.sample.special=0xa7;
 auto destination=transportMove.source.source.original;destination.position={int64_t(8)<<32,-(int64_t(2)<<32),0};
 const auto frame=++transportMove.preview.frame;
 transportMove.transaction.preview(destination,frame,true,true,transportMove.source.source.claim,transportMove.source.source.claimEntity);transport::MoveRequest request{};
 CHECK(transportMove.transaction.queue(frame,request));transportMove.input.requestedAt=GetTickCount64();
}
static void runtimeValidatePending(MoveFixture& f) {
 uint32_t flags=0;transportValidateMove(&flags,f.context,f.config,f.transform,0x91,0xa7,0.25f);
}
static const Byte* controlsReceivedItem=nullptr;
static unsigned controlsCalls=0;
static void __fastcall runtimeControls(void* input,void* states,void* digital,void* actor,void* mapping,
 void* settings,void* camera,uint32_t domain,uint64_t time,void* device,uint32_t tick,const Byte* item) {
 CHECK(states==reinterpret_cast<void*>(1)&&digital==reinterpret_cast<void*>(2)&&mapping==reinterpret_cast<void*>(3));
 CHECK(settings==reinterpret_cast<void*>(4)&&camera==reinterpret_cast<void*>(5)&&device==reinterpret_cast<void*>(6));
 CHECK(actor&&domain<=1&&time==123456&&tick==789);
 CHECK(!read<uint32_t>(actor,0xc00)); // No construction state set on the real actor.
 CHECK(transportMoveMutex.try_lock());transportMoveMutex.unlock();
 controlsReceivedItem=item;++controlsCalls;
 if(item==moveFixture->info)write<int32_t>(input,0x5c4,90);
}
static void runtimeCounter(MoveFixture& f) {
 f.counter=true;write(f.counterDescriptor,0,TransportCounterTemplate);f.counterDescriptor[0x20]=8;
 static const char name[]="Prop_Furniture_Comfort1_Bar_01_Wood";
 write(f.counterDescriptor,0x10,static_cast<const char*>(name));write<uint64_t>(f.counterDescriptor,0x18,sizeof(name));
 transportPlacementCatalog[TransportCounterTemplate]=TransportCounterItem;
 write(f.record,0x28,static_cast<Byte*>(f.counterDescriptor));
 write(f.info,0x3c0,TransportCounterTemplate);write<uint32_t>(f.info,0,TransportCounterItem);
 write<uint32_t>(f.comfort,0,2);write<uint32_t>(f.comfort,4,3);write<float>(f.comfort,8,5);f.comfort[12]=1;
 f.attachment[12]=2;
}
static void testCounterRuntime() {
 for(unsigned refusal=0;refusal<8;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeCounter(f);
  CHECK(transportClassifyProp(f.client,1217).item);
  Byte targets[5*16]{};
  for(size_t slot:{size_t(0),size_t(4)}) {
   std::memset(targets,0,sizeof(targets));write<uint64_t>(targets,slot*16,1217);
   CHECK(transportTargetCandidate(nullptr,f.client,targets,f.actorTransform).entity==1217);
   targets[slot*16+8]=1;CHECK(!transportTargetCandidate(nullptr,f.client,targets,f.actorTransform).entity);
  }
  CHECK(transportPlacementItem(f.client,1217)==TransportCounterItem);
  CHECK(!transportDecorPlacement(f.counterDescriptor,0x800004c1));
  if(refusal==1)f.counterDescriptor[0]=0; // Similar name/type cannot authorize another model.
  if(refusal==2)f.counterDescriptor[0x20]=3;
  if(refusal==3)f.missingComfort=true;
  if(refusal==4)write<uint32_t>(f.attachment,8,1217); // Self-support is invalid.
  if(refusal==5)f.attachment[13]=1;
  if(refusal==6)f.attachment[14]=1;
  if(refusal==7)f.attachment[12]=0;
  TransportSourceSnapshot snapshot{};
  CHECK(transportSnapshotSource(f.world,f.zones,1,1217,snapshot)==(!refusal||refusal==3));
  CHECK(!f.writes&&!f.moves);
 }
 for(unsigned refusal=0;refusal<6;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeCounter(f);runtimeGrab(f);
  const auto comfort=read<std::array<Byte,16>>(f.comfort,0);
  const auto attachment=read<std::array<Byte,16>>(f.attachment,0);
  f.rotated=true;write(f.descriptor,0x30,static_cast<Byte*>(f.client));
  transportMove.input.place=true;runtimeValidatePending(f);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued&&!f.writes&&!f.moves);
  if(refusal==1)f.refusal=1u<<8;
  if(refusal==2)f.refusal=1u<<14;
  if(refusal==3)f.info[0x3c0]=0;
  write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);
  if(refusal==4)f.attachment[15]=1;
  if(refusal==5)f.comfort[0]=3;
  transportMovePostStep(f.server,f.fullWorld());
  if(refusal){CHECK(!f.writes&&!f.moves);continue;}
  CHECK(f.writes==1&&f.moves==1);
  CHECK((read<std::array<Byte,16>>(f.comfort,0)==comfort));
  CHECK((read<std::array<Byte,16>>(f.attachment,0)==attachment));
  CHECK(read<Guid>(f.counterDescriptor,0)==TransportCounterTemplate);
  CHECK(transportMove.clientCollisions.size()==1);
  transportMoveRender(f.client,f.client);
  CHECK(f.moves==2&&f.modelMoves==1&&transportMove.clientCollisions.empty());
  for(size_t offset:{size_t(0),size_t(56),size_t(112)})CHECK(std::memcmp(f.render+offset,f.transform,56)==0);
  TransportSourceSnapshot after{};CHECK(transportSnapshotSource(f.world,f.zones,2,1217,after));
  CHECK(after.decor&&after.source.original==transportReadPlacement(f.transform));
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportPlaceReady=false;
 std::puts("PASS: allowlisted dynamic counter, native descriptor, preserved comfort/attachment, placement refusals and render/collision synchronization without UsedItem or inventory.");
}
static void runtimeDoor(MoveFixture& f) {
 runtimeCounter(f);f.door=true;f.counterDescriptor[0x20]=10;transportDoorTemplates[TransportCounterTemplate]=true;
 write<uint64_t>(f.dynamicCollider,8,163);f.collision[0x78]=0;
 write<uint32_t>(f.children,0,1218);write<uint32_t>(f.childAttachment,44,1217);
 write<uint64_t>(f.childCollider,8,164);f.childTemplate[0]=11;
 write(f.childRecord,0x28,static_cast<Byte*>(f.childTemplate));write<size_t>(f.world,0x158,2);
}
static unsigned placementHudCalls=0,placementHudGateFallbacks=0,placementHudLookups=0;
static const Byte* placementHudExpected=nullptr;
static Byte placementHudOriginal[8]{};
static bool __fastcall runtimeHudGate(const void*){++placementHudGateFallbacks;return false;}
static const Byte* __fastcall runtimeHudItem(uint32_t id,void* registry) {
 CHECK(id==123&&registry==placementHudOriginal);++placementHudLookups;return placementHudOriginal;
}
static void __fastcall runtimePlacementPanel(void* input,float* bounds,float* adjustmentBounds) {
 ++placementHudCalls;
 CHECK(input==placementHudOriginal&&bounds&&bounds[3]==24.0f&&adjustmentBounds==bounds+4);
 CHECK(transportPlacementHudItem==placementHudExpected);
 const auto selected=transportHudItemForCaller(base+0xb6bc42,123,placementHudOriginal);
 CHECK(selected==(placementHudExpected?placementHudExpected:placementHudOriginal));
 CHECK(transportHudItemForCaller(base+0xb6bc43,123,placementHudOriginal)==placementHudOriginal);
 CHECK(transportMoveMutex.try_lock());transportMoveMutex.unlock();
 if(placementHudExpected) {
  Byte context[16]{};write(context,0,placementHudExpected);
  CHECK(transportPlacementHudAllowed(context));
  Byte copied[16]{};std::memcpy(copied,context,sizeof(copied));
  CHECK(transportPlacementHudAllowed(copied));
  write(copied,0,static_cast<const Byte*>(placementHudOriginal));CHECK(!transportPlacementHudAllowed(copied));
 }
 bounds[0]=12;adjustmentBounds[0]=34;
}
static void runtimeWell(MoveFixture& f);
static void testPlacementHud() {
 originalTransportPlacementPanel=&runtimePlacementPanel;
 originalTransportHudItemLookup=&runtimeHudItem;originalTransportPlacementHudAllowed=&runtimeHudGate;
 for(unsigned kind=0;kind<4;++kind) {
  MoveFixture f;runtimeSetup(f);
  if(kind==1)runtimeCounter(f);if(kind==2){runtimeDoor(f);transportObjects=false;}if(kind==3){runtimeWell(f);transportObjects=false;}
  runtimeGrab(f);write(f.descriptor,0x30,static_cast<Byte*>(f.client));runtimeValidatePending(f);
  CHECK(transportMove.preview.item);
  placementHudCalls=placementHudGateFallbacks=placementHudLookups=0;
  const auto pose=read<std::array<Byte,56>>(f.transform,0);
  float rect[8]={0,0,900,24};
  placementHudExpected=transportMove.preview.item;
  transportPlacementPanel(placementHudOriginal,rect,rect+4);
  CHECK(placementHudCalls==1&&placementHudGateFallbacks==1&&placementHudLookups==1&&!transportPlacementHudItem);
  CHECK(rect[0]==12&&rect[4]==34);
  CHECK((read<std::array<Byte,56>>(f.transform,0)==pose&&!f.writes&&!f.moves));
  CHECK(!transportPlacementHudAllowed(nullptr)&&placementHudGateFallbacks==2);
  // Scope ends even though the original renderer retains its normal output.
  CHECK(transportHudItemForCaller(base+0xb6bc42,123,placementHudOriginal)==placementHudOriginal);
  placementHudExpected=nullptr;transportMove.preview.updatedAt=GetTickCount64()-2000;
  transportPlacementPanel(placementHudOriginal,rect,rect+4);CHECK(placementHudCalls==2);
  transportMove.preview.updatedAt=GetTickCount64();
  if(kind==0)transportChests=false;else if(kind==1)transportObjects=false;else transportInteractive=false;
  transportPlacementPanel(placementHudOriginal,rect,rect+4);CHECK(placementHudCalls==3);
  transportHolding=false;transportPlacementPanel(placementHudOriginal,rect,rect+4);CHECK(placementHudCalls==4);
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;
 std::puts("PASS: complete native placement panel, exact lookup callsite, copied UI contexts, output rectangles, stale/disabled guards and no entity writes.");
}
static void testDoorRuntime() {
 for(bool decor:{false,true})for(bool usable:{false,true}) {
  MoveFixture f;runtimeSetup(f);runtimeDoor(f);transportObjects=decor;transportInteractive=usable;
  TransportSourceSnapshot snapshot{};CHECK(transportSnapshotSource(f.world,f.zones,1,1217,snapshot)==usable);
  transportDoorTemplates[TransportCounterTemplate]=false;CHECK(!transportSnapshotSource(f.world,f.zones,1,1217,snapshot));
 }
 for(bool opened:{false,true})for(unsigned refusal=0;refusal<7;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeDoor(f);transportObjects=false;f.toggle[0]=opened?1:0;
  const auto link=read<std::array<Byte,112>>(f.childAttachment,0);const auto rootToggle=read<std::array<Byte,24>>(f.toggle,0);
  runtimeGrab(f);CHECK(transportMove.source.door);
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));transportMove.input.place=true;runtimeValidatePending(f);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued&&!f.writes&&!f.moves);
  if(refusal==1)f.toggle[0]^=1;
  if(refusal==2)write<uint32_t>(f.childAttachment,44,999);
  if(refusal==3)f.childTemplate[0]=12;
  if(refusal==4)write<uint64_t>(f.childCollider,8,0);
  if(refusal==5)transportInteractive=false;
  if(refusal==6)f.dynamicCollider[17]=1;
  write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);
  transportMovePostStep(f.server,f.fullWorld());
  if(refusal){CHECK(!f.moves&&!f.writes);continue;}
  CHECK(f.moves==1&&f.writes==1&&!f.attachmentWrites);
  CHECK((read<std::array<Byte,112>>(f.childAttachment,0)==link));
  CHECK((read<std::array<Byte,24>>(f.toggle,0)==rootToggle));
  CHECK(read<uint32_t>(f.children,0)==1218);
  transportMoveRender(f.client,f.client);CHECK(f.moves==2&&f.modelMoves==1&&transportMove.clientCollisions.empty());
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportPlaceReady=false;
 std::puts("PASS: independent usable/decor gates, door root dynamic collision/render move, toggle/child identity and attachment preservation, changed assembly refusal.");
}
static void runtimeWell(MoveFixture& f) {
 runtimeCounter(f);f.well=true;f.counterDescriptor[0x20]=13;transportWellTemplates[TransportCounterTemplate]=true;
 // Exact attribute header observed on well 1479; quantities may change naturally.
 write<uint64_t>(f.fuel,0,0x0201000805b0bf75ull);write<uint32_t>(f.fuel,8,0x1900);write<uint32_t>(f.fuel,12,0x1900);
 write<float>(f.refill,0,0.25f);f.npcTarget[0]=f.animalTarget[0]=1;
}
static void testWellRuntime() {
 for(bool decor:{false,true})for(bool usable:{false,true}) {
  MoveFixture f;runtimeSetup(f);runtimeWell(f);transportObjects=decor;transportInteractive=usable;
  TransportSourceSnapshot snapshot{};CHECK(transportSnapshotSource(f.world,f.zones,1,1217,snapshot)==usable);
  transportWellTemplates[TransportCounterTemplate]=false;CHECK(!transportSnapshotSource(f.world,f.zones,1,1217,snapshot));
 }
 for(unsigned refusal=0;refusal<5;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeWell(f);transportObjects=false;
  runtimeGrab(f);CHECK(transportMove.source.well&&!transportMove.source.door);
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));transportMove.input.place=true;runtimeValidatePending(f);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued);
  // Refill during preview must survive commit, never revert to the grab snapshot.
  write<uint32_t>(f.fuel,8,1234);write<float>(f.refill,0,0.75f);
  const auto fuel=read<std::array<Byte,16>>(f.fuel,0);const auto refill=read<uint32_t>(f.refill,0);
  if(refusal==1)write<uint32_t>(f.npcTarget,24,999);
  if(refusal==2)write<uint32_t>(f.animalTarget,4,999);
  if(refusal==3)f.missingFuel=true;
  if(refusal==4)transportInteractive=false;
  write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);transportMovePostStep(f.server,f.fullWorld());
  CHECK((read<std::array<Byte,16>>(f.fuel,0)==fuel&&read<uint32_t>(f.refill,0)==refill));
  if(refusal){CHECK(!f.moves&&!f.writes);continue;}
  CHECK(f.moves==1&&f.writes==1);
  transportMoveRender(f.client,f.client);CHECK(f.moves==2&&f.modelMoves==1&&transportMove.clientCollisions.empty());
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportPlaceReady=false;
 std::puts("PASS: water well setting isolation, native static collision/render movement, live reserve/refill preservation, NPC/animal and missing-component refusal.");
}
static void testTeleportRuntime() {
 // The native global UI pointer is intentionally unavailable: the default
 // accessor must use only the screen published from the cursor row.
 transportTravelState.screen=5;CHECK(transportCachedTravelState().screen==5);
 transportTravelState.screen=1;CHECK(transportCachedTravelState().screen==1);

 for(unsigned kind=0;kind<4;++kind)for(unsigned refusal=0;refusal<4;++refusal) {
  MoveFixture f;runtimeSetup(f);
  if(kind==1)runtimeCounter(f);if(kind==2)runtimeDoor(f);if(kind==3)runtimeWell(f);
  runtimeGrab(f);
  // Queue a placement, then opening the map must invalidate that old request.
  runtimeQueue();runtimeScreen=5;transportActive=false;transportEligible=false;
  transportMovePostStep(f.server,f.fullWorld());
  CHECK(transportHolding&&transportMove.preview.suspended&&transportTravelPaused);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Preview&&!f.moves);
  runtimeScreen=51;transportMove.input.requestedAt=GetTickCount64()-10000;
  transportMovePostStep(f.server,f.fullWorld());CHECK(transportHolding&&!f.moves);
  if(refusal==1){f.focused=false;transportMovePostStep(f.server,f.fullWorld());CHECK(!transportHolding);continue;}
  if(refusal==2){write<uint32_t>(f.bar,0,0);transportMovePostStep(f.server,f.fullWorld());CHECK(!transportHolding);continue;}
  runtimeScreen=1;transportActive=true;transportEligible=true;transportUpdated=GetTickCount64();
  transportMovePostStep(f.server,f.fullWorld());CHECK(transportHolding&&!transportMove.preview.suspended&&!transportTravelPaused);
  Byte claims[0x90]{};std::memcpy(claims,f.zone,0x48);std::memcpy(claims+0x48,f.zone,0x48);
  write<int64_t>(claims,24,int64_t(1)<<32); // Source no longer contains destination x=8.
  write<int64_t>(claims+0x48,0,int64_t(5)<<32);Guid arrival{};arrival[0]=44;
  write(claims+0x48,0x30,arrival);write<uint32_t>(claims+0x48,0x40,2048);
  write(f.zones,0x10,static_cast<Byte*>(claims));write<size_t>(f.zones,0x18,2);
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));transportMove.input.place=true;runtimeValidatePending(f);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued);
  CHECK(transportMove.transaction.request().destinationClaimEntity==2048);
  if(refusal==3)write<uint32_t>(claims+0x48,0x40,2049);
  write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);transportMovePostStep(f.server,f.fullWorld());
  if(refusal==3){CHECK(!f.moves&&!f.writes);continue;}
  CHECK(f.moves==1&&f.writes==1&&!transportHolding);
  f.clientSourceAbsent=true;transportMoveRender(f.client,f.client);
  CHECK(f.moves==1&&transportMove.clientCollisions.size()==1);
  f.clientSourceAbsent=false;transportMoveRender(f.client,f.client);
  CHECK(f.moves==2&&f.modelMoves==1&&transportMove.clientCollisions.empty());
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportTravelPaused=false;
 std::puts("PASS: map/loading suspension, stale placement invalidation, cross-claim commit for four families, changed-altar/focus/tool refusal, late client replication.");
}
static void runtimeSuspend(MoveFixture& f,Byte screen) {
 runtimeScreen=screen;transportActive=false;transportEligible=false;
 transportMovePostStep(f.server,f.fullWorld());
 CHECK(transportHolding&&transportMove.preview.suspended&&!f.moves&&!f.writes);
}
static void runtimeResume(MoveFixture& f) {
 runtimeScreen=1;transportActive=true;transportEligible=true;transportUpdated=GetTickCount64();
 transportMovePostStep(f.server,f.fullWorld());
 CHECK(transportHolding&&!transportMove.preview.suspended&&!transportTravelPaused);
}
static unsigned runtimeDestroyed=0,runtimeUiCalls=0;
static bool runtimeExpectDestroyed=false;
static void __fastcall runtimeDestroy(void*) {
 ++runtimeDestroyed;
 if(runtimeExpectDestroyed)CHECK(!transportHolding&&transportMove.clientCollisions.empty()&&!transportMove.world.server&&!transportWorld);
 // The real destructor must run outside all mod locks.
 CHECK(transportMoveMutex.try_lock());transportMoveMutex.unlock();
 CHECK(transportMutex.try_lock());transportMutex.unlock();
}
static void __fastcall runtimeUiOriginal(void* context) {
 ++runtimeUiCalls;
 auto services=read<void*>(context,0xd0);auto game=read<void*>(services,0);auto client=read<void*>(game,0x38);
 write<Byte>(client,0x1cd8,51);
}
static void testLiveActorHealth() {
 MoveFixture f;runtimeSetup(f);runtimeCounter(f);
 // Exact server Health bytes from the live player who could not pick up decor.
 // Neither client nor server player had NetworkHealth; client had no Health either.
 const Byte live[]={0x95,0x49,0xb8,0x8e,0x08,0x00,0x01,0x08,0x29,0x02,0,0,0,0,0,0,
  0xfa,0,0,0,0x2f,0x01,0,0,0xf4,0x01,0,0,0x20,0x03,0,0,0,0,0,0,0x29,0x02,0,0};
 std::memcpy(f.health,live,sizeof(live));
 CHECK(transportActorEligible(f.world,9));
 CHECK(!transportActorEligible(f.client,9)); // Client data never authorizes a server move.
 runtimeGrab(f);
 write(f.descriptor,0x30,static_cast<Byte*>(f.client));runtimeValidatePending(f);
 CHECK(transportMove.preview.updatedAt&&transportHolding);
 for(unsigned invalid=0;invalid<8;++invalid) {
  std::memcpy(f.health,live,sizeof(live));
  switch(invalid) {
   case 0:write<int32_t>(f.health,8,0);break;
   case 1:write<int32_t>(f.health,8,-1);break;
   case 2:write<int32_t>(f.health,36,0);break;
   case 3:f.health[6]=0;break;
   case 4:write<uint16_t>(f.health,4,12);break;
   case 5:f.health[7]=7;break;
   case 6:write<uint32_t>(f.health,0,0);break;
   case 7:f.missingHealth=true;break;
  }
  CHECK(!transportActorEligible(f.world,9));
 }
 f.missingHealth=false;std::memcpy(f.health,live,sizeof(live));
 write<int32_t>(f.health,8,0);transportMovePostStep(f.server,f.fullWorld());
 CHECK(!transportHolding&&!f.moves&&!f.writes);
 std::puts("PASS: live 553/553 server Health without NetworkHealth, client preview without health components, invalid attributes and death cancellation.");
}
static void testTransportInterruptions() {
 // Health is read from a real component contract, not an injected eligibility
 // bool. Check before grab and between validation and commit, in every phase.
 for(unsigned kind=0;kind<4;++kind)for(unsigned phase=0;phase<4;++phase)for(unsigned interruption=0;interruption<9;++interruption) {
  MoveFixture f;runtimeSetup(f);
  if(kind==1)runtimeCounter(f);if(kind==2)runtimeDoor(f);if(kind==3)runtimeWell(f);
  runtimeGrab(f);
  if(phase>0)runtimeSuspend(f,5);
  if(phase>1)runtimeSuspend(f,51);
  if(phase>2)runtimeResume(f);
  if(phase==0||phase==3){runtimeQueue();runtimeValidatePending(f);}
  switch(interruption) {
   case 0:write<int32_t>(f.health,8,0);break;
   case 1:f.missingHealth=true;break;
   case 2:write<uint32_t>(f.bar,0,0);break;
   case 3:f.focused=false;break;
   case 4:runtimeScreen=2;break;
   case 5:runtimeOldSession=true;break;
   case 6:f.sourceGone=true;break;
   case 7:write<Byte>(f.zone,0x46,1);break;
   case 8:write<int64_t>(f.transform,0,int64_t(1)<<32);break;
  }
  transportMovePostStep(f.server,f.fullWorld());
  CHECK(!transportHolding&&transportMove.transaction.stage()==transport::MoveStage::Idle&&!f.writes&&!f.moves);
 }
 {
  MoveFixture f;runtimeSetup(f);write<int32_t>(f.health,8,0);
  transportMove.input.grab=true;transportMove.input.requestedAt=GetTickCount64();transportMove.input.requestedEntity=1217;transportMove.input.requestedItem=42;
  transportMovePostStep(f.server,f.fullWorld());CHECK(!transportHolding&&!f.moves&&!f.writes);
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeScreen=51;
  transportMovePostStep(f.server,f.fullWorld());CHECK(!transportHolding); // NoUi alone is not permission to travel.
 }
 for(Byte screen:{Byte(5),Byte(51)}) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeSuspend(f,5);
  if(screen==51)runtimeSuspend(f,51);
  transportMove.input.requestedAt=GetTickCount64()-600000;
  for(unsigned i=0;i<100;++i)transportMovePostStep(f.server,f.fullWorld());
  CHECK(transportHolding); // Live UI heartbeat: no duration limit on map/loading.
  runtimeUiAge=transport_time::UiFresh+1;
  transportMovePostStep(f.server,f.fullWorld());CHECK(!transportHolding&&!f.moves&&!f.writes);
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeSuspend(f,5);runtimeSuspend(f,51);
  runtimeUiAge=600000;transportMove.world.lastStep=GetTickCount64()-600000;
  transportMovePostStep(f.server,f.fullWorld());CHECK(transportHolding); // Engine stall, allow a new UI frame.
  runtimeUiAge=0;runtimeResume(f);
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeSuspend(f,5);
  runtimeScreen=1;runtimeRowAge=10000;transportMovePostStep(f.server,f.fullWorld());CHECK(transportHolding);
  transportMove.preview.resumeWaitAt=GetTickCount64()-transport_time::PreviewStartup-1;
  transportMovePostStep(f.server,f.fullWorld());CHECK(!transportHolding&&!f.moves&&!f.writes);
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);
  for(unsigned trip=0;trip<3;++trip) {
   runtimeQueue();const auto old=transportMove.transaction.request();runtimeValidatePending(f);
   runtimeSuspend(f,5);if(trip)runtimeSuspend(f,51);runtimeResume(f);
   CHECK(!transportMove.transaction.owns(old)&&!transportMove.validation.ticket&&!transportMove.input.place);
  }
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));transportMove.input.place=true;runtimeValidatePending(f);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued);
  write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);transportMovePostStep(f.server,f.fullWorld());
  CHECK(f.writes==1&&f.moves==1&&!transportHolding);
 }
 // UI observer refreshes map/loading even when no targeting row is running.
 {
  Byte context[0xd8]{},services[8]{},game[0x40]{},client[0x1cd9]{};
  write(context,0xd0,static_cast<void*>(services));write(services,0,static_cast<void*>(game));write(game,0x38,static_cast<void*>(client));
  write<Byte>(client,0x1cd8,5);transportPublishScreen(context,123);
  CHECK(transportCachedTravelState().screen==5&&transportCachedTravelState().updatedAt==123);
  originalTransportUiTick=&runtimeUiOriginal;runtimeUiCalls=0;transportUiTick(context);
  CHECK(runtimeUiCalls==1&&transportCachedTravelState().screen==51&&transportCachedTravelState().generation==transportSessionGeneration.load());
  originalTransportUiTick=nullptr;
 }
 // The session ends at actual ECS destruction, even when the next allocation
 // reuses the same address and entity IDs. Unrelated worlds must not interfere.
 for(bool clientFirst:{false,true}) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeQueue();runtimeValidatePending(f);
  const auto old=transportMove.transaction.request();const auto generation=transportSessionGeneration.load();
  originalTransportDestroyWorld=&runtimeDestroy;runtimeDestroyed=0;runtimeExpectDestroyed=false;
  transportCommands={true,true,GetTickCount64(),generation};CHECK(transportBoundAction(10));
  transportDestroyWorld(f.registry);CHECK(transportHolding&&transportSessionGeneration.load()==generation);
  runtimeExpectDestroyed=true;transportDestroyWorld(clientFirst?f.client:f.world);
  CHECK(runtimeDestroyed==2&&transportSessionGeneration.load()>generation&&!transportMove.transaction.owns(old));
  CHECK(!transportBoundAction(10)&&!transportBoundAction(14));
  transportActive=true;transportEligible=true;transportUpdated=GetTickCount64();
  transportMovePostStep(f.server,f.fullWorld());CHECK(transportMove.world.session!=old.source.session&&!f.moves&&!f.writes);
  CHECK(!transportMove.preview.item&&!transportMove.preview.actor&&!transportMove.preview.renderer.handle);
  originalTransportDestroyWorld=nullptr;
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportTravelPaused=false;
 std::puts("PASS: 144 interruption paths (four object families), native health/death, stale UI, long live loading, stalled engine, repeated travel, bounded resume, ECS teardown/address reuse and UI forwarding.");
}
static void testClaimAndSyncRecovery() {
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeQueue();runtimeValidatePending(f);
  transportMove.input.requestedAt=GetTickCount64()-transport_time::CommitLifetime-1;
  transportMovePostStep(f.server,f.fullWorld());
  CHECK(!f.moves&&!f.writes&&transportMove.transaction.stage()==transport::MoveStage::Idle);
 }
 {
  MoveFixture f;runtimeSetup(f);write<float>(f.transform,44,2);
  TransportSourceSnapshot source{};CHECK(!transportSnapshotSource(f.world,f.zones,1,1217,source));
 }
 for(unsigned refusal=0;refusal<3;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);const auto source=transportMove.source.source;
  Byte claims[0x90]{};std::memcpy(claims,f.zone,0x48);std::memcpy(claims+0x48,f.zone,0x48);
  Guid other{};other[0]=88;write(claims,0x30,other);write<uint32_t>(claims,0x40,3333);
  write(f.zones,0x10,static_cast<Byte*>(claims));write<size_t>(f.zones,0x18,2);
  CHECK(transportFindClaim(f.zones,f.transform).altar==3333); // Overlap order changed.
  if(refusal==1)write<Byte>(claims+0x48,0x46,1);
  if(refusal==2)write<int64_t>(claims+0x48,0,int64_t(1)<<32);
  transportMovePostStep(f.server,f.fullWorld());
  CHECK(transportHolding.load()==!refusal&&!f.moves&&!f.writes);
  if(!refusal)CHECK(transportMove.source.source.claim==source.claim&&transportMove.source.source.claimEntity==source.claimEntity);
 }
 for(unsigned obsolete=0;obsolete<4;++obsolete) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeQueue();runtimeValidatePending(f);transportMovePostStep(f.server,f.fullWorld());
  CHECK(f.writes==1&&transportMove.clientCollisions.size()==1);
  f.clientSourceAbsent=true;auto& pending=transportMove.clientCollisions[0];pending.queuedAt=GetTickCount64()-600000;
  transportMoveRender(f.client,f.client);CHECK(pending.reported&&pending.lastAttempt&&f.modelMoves==0);
  const auto attempted=pending.lastAttempt;transportMoveRender(f.client,f.client);CHECK(pending.lastAttempt==attempted);
  if(obsolete==1)f.sourceGone=true;
  if(obsolete==2)f.templateId[0]=11;
  if(obsolete==3)write<int64_t>(f.transform,0,int64_t(20)<<32);
  transportMovePostStep(f.server,f.fullWorld());
  CHECK(transportMove.clientCollisions.size()==(obsolete==1||obsolete==2?0u:1u));
  if(obsolete==3)CHECK(pending.destination.position[0]==int64_t(20)<<32);
  if(!obsolete||obsolete==3) {
   f.clientSourceAbsent=false;pending.lastAttempt=GetTickCount64()-transport_time::SyncRetry-1;
   transportMoveRender(f.client,f.client);CHECK(transportMove.clientCollisions.empty()&&f.modelMoves==1);
  }
  CHECK(f.writes==1);
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportPlaceReady=false;
 std::puts("PASS: overlapping claim order, exact source claim removal/resize, delayed replication preserved, absent-object warning/backoff and obsolete server acknowledgements pruned.");
}
static void testSupportedDecorRuntime() {
 // Actual pink flowerpot 3393 on sideboard 3372: ffffffffffffff7f2c0d000002000000.
 for(unsigned refusal=0;refusal<7;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeCounter(f);
  write<int64_t>(f.attachment,0,INT64_MAX);write<uint32_t>(f.attachment,8,3372);
  const auto original=read<std::array<Byte,16>>(f.attachment,0);
  runtimeGrab(f);
  CHECK((read<std::array<Byte,16>>(f.attachment,0)==original&&!f.attachmentWrites));
  if(refusal==1) {
   transportMove.cancel();CHECK((read<std::array<Byte,16>>(f.attachment,0)==original));
   CHECK(!f.moves&&!f.writes&&!f.attachmentWrites);continue;
  }
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));transportMove.input.place=true;runtimeValidatePending(f);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued&&!f.moves&&!f.attachmentWrites);
  if(refusal==2)f.refusal=1u<<8;
  if(refusal==3)write<uint32_t>(f.attachment,8,3373);
  if(refusal==4)f.badAttachmentWrite=true;
  if(refusal==5)f.badWrite=true;
  if(refusal==6)f.attachment[15]=1;
  write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);
  const auto beforeCommit=read<std::array<Byte,16>>(f.attachment,0);
  transportMovePostStep(f.server,f.fullWorld());
  if(refusal) {
   CHECK(!f.moves&&read<int64_t>(f.transform,0)==0);
   CHECK((read<std::array<Byte,16>>(f.attachment,0)==beforeCommit));
   if(refusal==4) {
    transportMove.input.requestedAt=GetTickCount64()-transport_time::CommitLifetime-1;
    transportMovePostStep(f.server,f.fullWorld());CHECK(transportMove.transaction.stage()==transport::MoveStage::Idle);
   }
   continue;
  }
  CHECK(f.writes==1&&f.attachmentWrites==1&&f.moves==1);
  CHECK(!read<uint32_t>(f.attachment,8)&&!read<int64_t>(f.attachment,0));
  CHECK(f.attachment[12]==2&&!f.attachment[13]&&!f.attachment[14]&&!f.attachment[15]);
  transportMoveRender(f.client,f.client);CHECK(f.modelMoves==1&&f.moves==2);
  // Immediate second grab, without unloading/reloading the world.
  transportMove.input.grab=true;transportMove.input.requestedEntity=1217;
  transportMove.input.requestedItem=TransportCounterItem;transportMove.input.requestedAt=GetTickCount64();
  write<int64_t>(f.actorTransform,0,int64_t(8)<<32);write<int64_t>(f.actorTransform,8,-(int64_t(2)<<32));
  transportMovePostStep(f.server,f.fullWorld());CHECK(transportMove.transaction.stage()==transport::MoveStage::Preview);
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportPlaceReady=false;
 std::puts("PASS: supported prop grab, cancel/refusal preserves support, commit invalidates only old support, failed writes expire, immediate re-grab and render sync.");
}
static void testQuantizedReplication() {
 // Actual failing sample from entity 2771, 2026-11-16; no invented equality.
 transport::Placement server{},client{};
 server.position={int64_t(4390.99169921875*4294967296.0),int64_t(779)<<32,int64_t(2797.068359375*4294967296.0)};
 server.rotation={0,0.500069260597229f,0,0.8659854531288147f};
 client=server;client.position[0]-=int64_t(1)<<21;
 client.rotation={0,0.49954119324684143f,0,0.8662901520729065f};
 CHECK(transportReplicatedPlacement(client,server));
 for(auto& c:client.rotation)c=-c;
 CHECK(transportReplicatedPlacement(client,server));
 client.rotation={0,0.49954119324684143f,0,0.8662901520729065f};
 auto different=client;different.rotation={0,0,0,1};CHECK(!transportReplicatedPlacement(different,server));
 different=client;different.position[0]-=int64_t(1)<<32;CHECK(!transportReplicatedPlacement(different,server));
 different=client;different.scale[0]=1.1f;CHECK(!transportReplicatedPlacement(different,server));
 different=client;different.rotation={0,0.50753836f,0,0.86162916f};CHECK(!transportReplicatedPlacement(different,server)); // 61 degrees.
 // Exercise the real retry queue: old client pose cannot acknowledge the move;
 // after quantized replication, synchronize using that client pose, not a
 // forced copy of the higher-precision server pose.
 for(bool counter:{false,true}) {
  MoveFixture f;runtimeSetup(f);if(counter)runtimeCounter(f);runtimeGrab(f);
  f.separateClientTransform=true;f.quantizedRotation=true;
  std::memcpy(f.clientTransform,f.transform,56);
  auto destination=server;destination.position={int64_t(8)<<32,-(int64_t(2)<<32),0};
  transportMove.transaction.preview(destination,1,true,true,transportMove.source.source.claim,transportMove.source.source.claimEntity);transport::MoveRequest request{};
  CHECK(transportMove.transaction.queue(1,request));transportMove.preview.sample.special=0xa7;
  transportMove.input.requestedAt=GetTickCount64();runtimeValidatePending(f);
  transportMovePostStep(f.server,f.fullWorld());CHECK(f.writes==1&&f.moves==1);
  const auto authoritative=read<std::array<Byte,56>>(f.transform,0);
  transportMoveRender(f.client,f.client);CHECK(!f.modelMoves&&transportMove.clientCollisions.size()==1);
  auto replicated=destination;replicated.rotation=client.rotation;
  const auto bytes=transportNativePlacement(replicated);std::memcpy(f.clientTransform,bytes.data(),56);
  transportMoveRender(f.client,f.client);
  CHECK(f.modelMoves==1&&f.moves==2&&transportMove.clientCollisions.empty());
  CHECK((read<std::array<Byte,56>>(f.transform,0)==authoritative));
  for(size_t offset:{size_t(0),size_t(56),size_t(112)})CHECK(std::memcmp(f.render+offset,f.clientTransform,56)==0);
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportPlaceReady=false;
 std::puts("PASS: live quantized rotation regression, quaternion sign, stale pose rejection, chest/counter retry completion and authoritative pose preservation.");
}
static Byte catalogInfos[2][0x400]{};
static unsigned catalogLookupCalls=0;
static const Byte* __fastcall catalogLookup(uint32_t id,void*) {
 ++catalogLookupCalls;
 for(auto& info:catalogInfos)if(read<uint32_t>(info,0)==id)return info;
 return nullptr;
}
static void testPlacementCatalog() {
 auto saved=std::move(entries[0].bytes);const auto lookup=transportItemLookup;
 entries[0].bytes=Bytes(40);put32(entries[0].bytes,0,8);put32(entries[0].bytes,4,2);
 Guid a=TransportCounterTemplate,b=a;b[0]^=1;
 putGuid(entries[0].bytes,8,a);putGuid(entries[0].bytes,24,b);
 write<uint32_t>(catalogInfos[0],0,hash(a));write(catalogInfos[0],0x3c0,a);
 write<uint32_t>(catalogInfos[1],0,hash(b)); // A non-placeable native item.
 transportItemLookup=&catalogLookup;transportPlacementCatalog.clear();transportCatalogRegistries.clear();catalogLookupCalls=0;
 transportRefreshPlacementCatalog(reinterpret_cast<void*>(123));
 CHECK(catalogLookupCalls==2&&transportPlacementCatalog.size()==1&&transportPlacementCatalog[a]==hash(a));
 transportRefreshPlacementCatalog(reinterpret_cast<void*>(123));CHECK(catalogLookupCalls==2);
 transportRefreshPlacementCatalog(reinterpret_cast<void*>(124));CHECK(catalogLookupCalls==4&&transportPlacementCatalog.size()==1);
 // A late resource must be retried without rescanning already resolved IDs.
 write<uint32_t>(catalogInfos[1],0,0);
 auto late=reinterpret_cast<void*>(125);transportRefreshPlacementCatalog(late);
 CHECK(catalogLookupCalls==6&&!transportCatalogRegistries.count(late));
 transportRefreshPlacementCatalog(late);CHECK(catalogLookupCalls==6);
 write<uint32_t>(catalogInfos[1],0,hash(b));transportCatalogProgress[late].attemptedAt=GetTickCount64()-1001;
 transportRefreshPlacementCatalog(late);CHECK(catalogLookupCalls==7&&transportCatalogRegistries.count(late));
 entries[0].bytes=std::move(saved);transportItemLookup=lookup;transportPlacementCatalog.clear();transportCatalogRegistries.clear();
 std::puts("PASS: registry-driven placement catalog, non-placeable skip, per-registry single scan and value-only descriptor mapping.");
}
static void testObjectSettingsPolicy() {
 for(bool chestEnabled:{false,true})for(bool decorEnabled:{false,true}) {
  MoveFixture f;runtimeSetup(f);transportChests=chestEnabled;transportObjects=decorEnabled;
  TransportSourceSnapshot snapshot{};
  CHECK(transportSnapshotSource(f.world,f.zones,1,1217,snapshot)==chestEnabled);
  runtimeCounter(f);
  CHECK(transportSnapshotSource(f.world,f.zones,1,1217,snapshot)==decorEnabled);
 }
 MoveFixture f;runtimeSetup(f);runtimeCounter(f);
 // An arbitrary allowed Prop with a matching native placement descriptor.
 f.counterDescriptor[0]=0xab;const auto model=read<Guid>(f.counterDescriptor,0);
 transportPlacementCatalog[model]=42;
 CHECK(transportClassifyProp(f.client,1217).item&&transportPlacementItem(f.client,1217)==42);
 static const char name[]="Deco_Without_Prefix";
 write(f.counterDescriptor,0x10,static_cast<const char*>(name));write<uint64_t>(f.counterDescriptor,0x18,sizeof(name));
 CHECK(transportClassifyProp(f.client,1217).item);
 transportPlacementCatalog.erase(model);CHECK(!transportClassifyProp(f.client,1217).item);
 transportPlacementCatalog[model]=42;f.counterDescriptor[0x20]=3;CHECK(!transportClassifyProp(f.client,1217).item);
 f.counterDescriptor[0x20]=8;CHECK(!transportDecorPlacement(f.counterDescriptor,0x80000001));
 CHECK(!f.writes&&!f.moves);
 std::puts("PASS: independent chest/decor settings, no external file or name filter, generic mapped Prop, missing descriptor and scene persistence refusal.");
}
static void testObjectInspector() {
 MoveFixture f;runtimeSetup(f);
 Byte targets[5*16]{};write<uint64_t>(targets,0,1217);
 // Unlisted template: inspector must accept it without making it movable.
 CHECK(!transportTargetCandidate(nullptr,f.client,targets,f.actorTransform).entity);
 const char nativeName[]="Prop_Destructible_Villageprops_bottles_12";
 write(f.counterDescriptor,0,f.templateId);write(f.counterDescriptor,0x10,static_cast<const char*>(nativeName));
 write<uint64_t>(f.counterDescriptor,0x18,sizeof(nativeName));write(f.record,0x28,static_cast<Byte*>(f.counterDescriptor));
 const auto inspected=transportInspectTarget(f.client,targets);
 CHECK(std::strcmp(inspected.name.data(),nativeName)==0);
 std::array<char,192> nameTest{};
 write<uint64_t>(f.counterDescriptor,0x18,100000);transportInspectName(f.counterDescriptor,nameTest);CHECK(!nameTest[0]);
 write<uint64_t>(f.counterDescriptor,0x18,7);write<const char*>(f.counterDescriptor,0x10,"<test>");
 transportInspectName(f.counterDescriptor,nameTest);CHECK(!nameTest[0]);
 write(f.counterDescriptor,0x10,static_cast<const char*>(nativeName));write<uint64_t>(f.counterDescriptor,0x18,sizeof(nativeName));
 CHECK(inspected.id==1217&&inspected.kind==0&&inspected.model==f.templateId);
 CHECK(!transportInspectTarget(nullptr,targets).id&&!transportInspectTarget(f.client,nullptr).id);
 write<uint64_t>(targets,0,0);write<uint64_t>(targets,4*16,1217);
 CHECK(transportInspectTarget(f.client,targets).id==1217);
 // Other native target kinds are raw identifiers, never ECS entity pointers.
 write<uint64_t>(targets,0,0x123456789abcdef0ull);targets[8]=3;
 const auto opaque=transportInspectTarget(f.client,targets);
 CHECK(opaque.id==0x123456789abcdef0ull&&opaque.kind==3&&opaque.model==Guid{});
 std::memset(targets,0,sizeof(targets));CHECK(!transportInspectTarget(f.client,targets).id);
 transportInspectedObject=inspected;transportInspectObjects=false;char text[384]{};
 CHECK(!transportInspectorText(text,sizeof(text))&&!text[0]);
 transportInspectObjects=true;transportChests=false;
 CHECK(transportInspectorText(text,sizeof(text))&&std::strstr(text,"1217")&&std::strstr(text,"09000000000000000000000000000000")&&std::strstr(text,nativeName)==text);
 for(uint32_t lang=0;lang<15;++lang) {
  language=lang;CHECK(transportInspectorText(text,sizeof(text))&&std::strstr(text,"1217"));
  CHECK(*transportCaption("setting.inspect_objects")&&*transportCaption("inspect.target")&&*transportCaption("inspect.model"));
 }
 transportActive=false;CHECK(!transportInspectorText(text,sizeof(text))&&!text[0]);
 transportActive=true;transportEligible=false;CHECK(!transportInspectorText(text,sizeof(text)));
 transportEligible=true;transportUpdated=GetTickCount64()-2000;CHECK(!transportInspectorText(text,sizeof(text)));
 transportUpdated=GetTickCount64();transportInspectedObject={};CHECK(!transportInspectorText(text,sizeof(text)));
 CHECK(!f.writes&&!f.moves&&!f.modelMoves);
 transportInspectObjects=false;transportActive=false;transportEligible=false;language=1;
 std::puts("PASS: diagnostic IDs for unlisted objects/opaque targets, no movement authorization, setting off, no target, stale/focus/mode clearing and 15 languages.");
}
static void testTransportRuntime() {
 for(unsigned refusal=0;refusal<7;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);Byte input[0x600]{};
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));runtimeValidatePending(f);
  originalTransportPlacementInput=&runtimeControls;controlsCalls=0;controlsReceivedItem=nullptr;
  if(refusal==1)transportMove.cancel();
  if(refusal==2)transportMove.preview.updatedAt=GetTickCount64()-1000;
  if(refusal==3)f.focused=false;
  if(refusal==4)transportActive=false;
  if(refusal==5)transportMove.preview.actor=nullptr;
  transportPlacementInput(input,reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),f.actor,
   reinterpret_cast<void*>(3),reinterpret_cast<void*>(4),reinterpret_cast<void*>(5),refusal==6?1:0,
   123456,reinterpret_cast<void*>(6),789,f.pincer);
  CHECK(controlsCalls==1&&controlsReceivedItem==(refusal?f.pincer:f.info));
  CHECK(read<int32_t>(input,0x5c4)==(refusal?0:90));
  CHECK(!f.writes&&!f.moves&&!f.modelMoves); // Input processing never commits placement.
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));
  write<uint32_t>(f.bar,0,0);runtimeValidatePending(f);
  CHECK(!transportMove.preview.updatedAt&&!transportPlaceReady.load()&&!f.writes&&!f.moves);
 }
 // Regression: the equipped pincer has no action item. Neither the native
 // ItemInfo pointer nor the native primary/secondary flag identifies our tool.
 for(unsigned nativeConfig=0;nativeConfig<2;++nativeConfig) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);
  write<const Byte*>(f.config,0x60,nativeConfig?f.info:nullptr);f.config[0x70]=0;
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));
  runtimeValidatePending(f);
  CHECK(transportMove.preview.updatedAt&&transportMove.preview.sample.calculated&&transportMove.preview.frame==1);
  CHECK(transportPlaceReady.load()&&!f.writes&&!f.moves);
  runtimeValidatePending(f);CHECK(transportMove.preview.frame==1); // No duplicate secondary query.
  transportRows.fetch_add(1);runtimeValidatePending(f);CHECK(transportMove.preview.frame==2);
  runtimeQueue();write(f.descriptor,0x30,static_cast<Byte*>(f.world));
  runtimeValidatePending(f);CHECK(transportMove.validation.ticket);
  transportMovePostStep(f.server,f.fullWorld());CHECK(f.writes==1&&f.moves==1);
 }
 {
  MoveFixture f;runtimeSetup(f);transportWorld=reinterpret_cast<uintptr_t>(f.client);transportInsideZone=true;
  transport::Frame frame{};frame.world=transportWorld;frame.pincer=true;frame.gameplay=true;frame.focused=true;
  frame.enabled=true;frame.reach=3;frame.candidate={1217,true,true,1};
  transportSelection={};transportSelection.update(frame);frame.tab=true;transportSelection.update(frame);
  Byte row[8]{};write(row,0,&f.iterator);
  transportMoveInput(row,true,false);CHECK(!transportMove.input.grab); // Held on mode entry is not a click.
  transportMoveInput(row,false,false);transportMoveInput(row,true,false);CHECK(transportMove.input.grab);
  transportMovePostStep(f.server,f.fullWorld());CHECK(transportHolding.load());
  transportMoveInput(row,true,false);CHECK(!transportMove.input.grab&&!transportMove.input.place);
  transportMoveInput(row,false,true);CHECK(transportMove.input.place); // Right button requests placement.
  transportActive=false;transportMoveInput(row,false,true);
  CHECK(!transportHolding.load()&&transportMove.transaction.stage()==transport::MoveStage::Idle&&!f.writes&&!f.moves);
 }
 {
  MoveFixture f;runtimeSetup(f);
  std::array<int64_t,6> bounds{};transport::Placement pose{};
  CHECK(transportBounds(f.info,pose,bounds));CHECK(bounds[0]==0&&bounds[3]==int64_t(1)<<32);
  pose.rotation={0,0,1,0};CHECK(transportBounds(f.info,pose,bounds));
  CHECK(bounds[0]==-(int64_t(1)<<32)&&bounds[3]==0&&bounds[1]==-(int64_t(1)<<32));
  pose.rotation={0,0,0,1};pose.position[0]=int64_t(100)<<32;
  CHECK(transportBounds(f.info,pose,bounds));
  CHECK(!transportBoxInClaim(f.zones,TransportClaim{f.claimId,1027},bounds.data()));
  auto opposite=pose;opposite.rotation={0,0,0,-1};CHECK(transportReplicatedPlacement(pose,opposite));
  opposite.rotation[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!transportReplicatedPlacement(pose,opposite));
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);f.rotated=true;const auto storage=read<std::array<Byte,84>>(f.inventory,0);
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));transportMove.input.place=true;
  runtimeValidatePending(f);CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued);
  CHECK(!f.writes&&!f.moves&&read<int64_t>(f.transform,0)==0); // Preview/query did not touch the source.
  write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);transportMovePostStep(f.server,f.fullWorld());
  CHECK(f.writes==1&&f.moves==1&&read<int64_t>(f.transform,0)==int64_t(8)<<32);
  CHECK(read<float>(f.transform,28)==0.70710677f&&read<float>(f.transform,36)==0.70710677f);
  CHECK((read<std::array<Byte,84>>(f.inventory,0)==storage));CHECK(read<uint64_t>(f.collider,8)==163&&read<uint32_t>(f.item,0)==42);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Idle&&!transportHolding.load());
  CHECK(transportMove.clientCollisions.size()==1);
  transportMoveRender(f.client,nullptr);CHECK(f.moves==1&&transportMove.clientCollisions.size()==1&&!f.renderWrites);
  f.badWrite=true;transportMoveRender(f.client,f.client);CHECK(f.moves==1&&!f.modelMoves&&transportMove.clientCollisions.size()==1);
  f.badWrite=false;transportMoveRender(f.client,f.client);
  CHECK(f.moves==2&&f.modelMoves==1&&transportMove.clientCollisions.size()==0);
  for(size_t offset:{size_t(0),size_t(56),size_t(112)})CHECK(std::memcmp(f.render+offset,f.transform,56)==0);
  CHECK(read<uint32_t>(f.render,168)==123&&read<uint32_t>(f.render,172)==456);
  transportMoveRender(f.client,f.client);CHECK(f.modelMoves==1&&f.moves==2);
  transportMovePostStep(f.server,f.fullWorld());CHECK(f.writes==1); // Exactly once.
 }
 // Server and client replication are separate; never validate using an alias.
 {
  MoveFixture f;runtimeSetup(f);f.separateClientTransform=true;
  std::memcpy(f.clientTransform,f.transform,56);
  runtimeGrab(f);runtimeQueue();runtimeValidatePending(f);
  transportMovePostStep(f.server,f.fullWorld());CHECK(f.writes==1);
  transportMoveRender(f.client,f.client);CHECK(f.modelMoves==0&&transportMove.clientCollisions.size()==1);
  std::memcpy(f.clientTransform,f.transform,56);
  transportMoveRender(f.client,f.client);CHECK(f.modelMoves==1&&transportMove.clientCollisions.empty());
 }
 for(unsigned refusal=0;refusal<9;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeQueue();
  if(refusal==0)f.refusal=1u<<8; // Occupied destination.
  if(refusal==1)f.refusal=1u<<14; // No native permission.
  runtimeValidatePending(f);
  if(refusal==2)f.lock[0]=1; // Opened after native validation.
  if(refusal==3)f.focused=false;
  if(refusal==4)write<uint32_t>(f.bar,0,0); // Tool changed.
  if(refusal==5)f.templateId[0]=10; // Replaced source.
  if(refusal==6)write<uint32_t>(f.inventory,4,1219); // Storage link changed.
  if(refusal==7)transportMove.validation.epoch+=1; // Different server step.
  if(refusal==8)transportChests=false;
  transportMovePostStep(f.server,f.fullWorld());CHECK(!f.writes&&!f.moves&&read<int64_t>(f.transform,0)==0);
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeQueue();runtimeValidatePending(f);f.badWrite=true;
  transportMovePostStep(f.server,f.fullWorld());CHECK(!f.moves&&read<int64_t>(f.transform,0)==0);
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeQueue();runtimeValidatePending(f);
  CHECK(transportMove.validation.ticket);f.refusal=1u<<14;runtimeValidatePending(f);
  CHECK(!transportMove.validation.ticket);transportMovePostStep(f.server,f.fullWorld());CHECK(!f.writes&&!f.moves);
 }
 {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeQueue();
  transportMove.input.requestedAt=GetTickCount64()-1500;transportMovePostStep(f.server,f.fullWorld());
  CHECK(!f.writes&&!f.moves&&transportMove.transaction.stage()==transport::MoveStage::Idle);
 }
 // Delayed render work beyond the old 16-entry limit cannot block a commit.
 for(bool duplicate:{false,true}) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);runtimeQueue();runtimeValidatePending(f);
  f.manySources=true;
  for(unsigned i=0;i<32;++i)transportMove.clientCollisions.push_back(
   {reinterpret_cast<uintptr_t>(f.client),i+2000,42,{},GetTickCount64(),false,read<transport::Identity>(f.templateId.data(),0)});
  if(duplicate)transportMove.clientCollisions[0].entity=1217;
  transportMovePostStep(f.server,f.fullWorld());CHECK(f.writes==1&&f.moves==1);
  CHECK(transportMove.clientCollisions.size()==(duplicate?32u:33u));
 }
 { // A nested native callback cannot recursively enter the transaction.
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);
  std::lock_guard<std::mutex> held(transportMoveMutex);TransportMoveEntry entry;
  transportMovePostStep(f.server,f.fullWorld());transportMoveRender(f.client,f.client);
  transportMoveInput(nullptr,true,true);CHECK(!f.writes&&!f.moves);
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportPlaceReady=false;
 std::puts("PASS: input edges, grab/validate/commit, unchanged inventories, exact-once move, client collision sync and refusal paths.");
}
