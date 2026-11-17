static const Bytes* floorTestProfile=nullptr;
static const Bytes* floorTestModel=nullptr;
static unsigned floorTestReads=0;
static bool floorUnavailable=false;
static uint32_t* __fastcall floorOpen(uint32_t* out,void*,const Key* key) {
 ++floorTestReads;*out=floorUnavailable?0:key->type==ItemType?1:2;return out;
}
static uint64_t* __fastcall floorResolve(uint64_t* out,void*,uint32_t handle) {
 const auto& bytes=handle==1?*floorTestProfile:*floorTestModel;
 out[0]=reinterpret_cast<uint64_t>(bytes.data());out[1]=bytes.size();out[2]=0x100000000ull;return out;
}
static void testDoorTemplate(const Bytes& profile,const Bytes& door) {
 const auto savedOpen=originalOpen;const auto savedResolve=originalResolve;
 floorTestProfile=&profile;floorTestModel=&door;originalOpen=&floorOpen;originalResolve=&floorResolve;
 MoveFixture f;runtimeSetup(f);runtimeDoor(f);transportDoorTemplates.clear();transportResourceDb=f.registry;
 const auto model=guid("550d7a506569c348bc33efdf9fb894d4");write(f.counterDescriptor,0,model);
 CHECK(transportIsDoor(transportTemplate(f.world,1217))&&transportIsDoor(transportTemplate(f.client,1217)));
 FloorBounds bounds;const auto descriptor=makeFloorPlacement(profile,door,model,&bounds);
 CHECK(equal(descriptor,0x3c0,model));for(size_t a=0;a<3;++a)CHECK(bounds.low[a]<bounds.high[a]);
 auto unsupported=door;const auto components=array(unsupported,16,12);bool changed=false;
 for(uint32_t i=0;i<components.count;++i) {
  const auto row=components.at+size_t(i)*12;
  if(u32(unsupported,row)==typeHash("keen::ecs::InteractionToggle")){put32(unsupported,row,0);changed=true;}
 }
 CHECK(changed);transportDoorTemplates.clear();floorTestModel=&unsupported;
 CHECK(!transportIsDoor(transportTemplate(f.world,1217)));
 originalOpen=savedOpen;originalResolve=savedResolve;transportResourceDb=nullptr;
 std::puts("PASS: actual castle-door template recognized from native components; missing interaction contract refused; complete placement descriptor built.");
}
static void testFloorPlacement(const Bytes& profile,const Bytes& bottles) {
 const auto model=guid("ee386f839f792f4fa64f4a31593a2a1a");
 const auto beforeProfile=profile,beforeModel=bottles;
 FloorBounds bounds;const auto built=makeFloorPlacement(profile,bottles,model,&bounds);
 CHECK(equal(built,0x48,DecorPlacementName)&&equal(built,0x58,Guid{})&&equal(built,0x68,Guid{}));
 CHECK(profile==beforeProfile&&bottles==beforeModel);
 CHECK(equal(built,0x3c0,model)&&u32(built,0)==TransportCounterItem);
 CHECK(std::abs(bounds.low[0]+0.15f)<0.00001f&&std::abs(bounds.low[1])<0.00001f);
 CHECK(std::abs(bounds.high[1]-0.66f)<0.00001f&&std::abs(bounds.high[2]-0.15f)<0.00001f);
 CHECK(std::memcmp(built.data()+0x3d0,built.data()+0x3e8,24)==0);
 CHECK(!built[0x43c]&&!u32(built,0x42c));
 CHECK(built[0x19d]==0&&built[0x19e]==0&&built[0x19f]==1); // Native floor-only attachment.
 const auto resources=array(built,0x400,56);CHECK(resources.count==1);
 const auto shapes=array(built,resources.at,12);CHECK(shapes.count==1);
 CHECK(u32(built,shapes.at)==typeHash("keen::ecs::BoxColliderData"));
 const size_t body=shapes.at+4+u32(built,shapes.at+4);
 CHECK(std::abs(blobFloat(built,body+80)-0.15f)<0.00001f);
 CHECK(makeFloorPlacement(profile,bottles,model)==built);
 size_t sourceShape=0,sourceBody=0;
 const auto components=array(bottles,16,12);
 for(uint32_t i=0;i<components.count;++i) {
  const auto row=components.at+size_t(i)*12;
  if(u32(bottles,row)!=typeHash("keen::ecs::ColliderResourceComponent"))continue;
  const auto colliders=array(bottles,row+4+u32(bottles,row+4),56);
  sourceShape=array(bottles,colliders.at,12).at;
  sourceBody=sourceShape+4+u32(bottles,sourceShape+4);
 }
 CHECK(sourceBody);
 // Shape dimensions and rotation must affect the bounds without replacing
 // native collision geometry with a donor box.
 for(const char* type:{"Sphere","Capsule","Cylinder","TaperedCapsule"}) {
  auto shaped=bottles;const auto full=std::string("keen::ecs::")+type+"ColliderData";
  put32(shaped,sourceShape,typeHash(full.c_str()));
  for(size_t a=0;a<3;++a)write<float>(shaped.data(),sourceBody+8+a*4,0);
  write<float>(shaped.data(),sourceBody+80,2);
  write<float>(shaped.data(),sourceBody+84,6);
  write<float>(shaped.data(),sourceBody+88,8);
  FloorBounds native;auto output=makeFloorPlacement(profile,shaped,model,&native);
  const bool sphere=std::strcmp(type,"Sphere")==0,capsule=std::strcmp(type,"Capsule")==0;
  const bool tapered=std::strcmp(type,"TaperedCapsule")==0;
  CHECK(std::abs(native.high[0]-(tapered?6.f:2.f))<0.00001f);
  CHECK(std::abs(native.high[1]-(sphere?2.f:capsule?5.f:tapered?10.f:3.f))<0.00001f);
  CHECK(std::abs(native.low[1]+(sphere?2.f:capsule?5.f:tapered?6.f:3.f))<0.00001f);
  const auto resourcesOut=array(output,0x400,56),shapesOut=array(output,resourcesOut.at,12);
  CHECK(u32(output,shapesOut.at)==typeHash(full.c_str()));
  write<float>(shaped.data(),sourceBody+28,0.38268343f);write<float>(shaped.data(),sourceBody+32,0.92387953f);
  makeFloorPlacement(profile,shaped,model,&native);
  if(!tapered) {
   const float expected=sphere?2.f:capsule?4.12132034f:3.5355339f;
   CHECK(std::abs(native.high[0]-expected)<0.00001f&&std::abs(native.high[1]-expected)<0.00001f);
  }
  // Signed authored dimensions have the same enclosing bounds.
  const auto positive=native;write<float>(shaped.data(),sourceBody+80,-2);
  makeFloorPlacement(profile,shaped,model,&native);CHECK(native.low==positive.low&&native.high==positive.high);
  put32(shaped,sourceShape+8,80);bool truncated=false;
  try{makeFloorPlacement(profile,shaped,model);}catch(const std::exception&){truncated=true;}CHECK(truncated);
 }
 auto rotated=bottles;
 write<float>(rotated.data(),sourceBody+28,0.70710677f);write<float>(rotated.data(),sourceBody+32,0.70710677f);
 FloorBounds turned;makeFloorPlacement(profile,rotated,model,&turned);
 CHECK(std::abs(turned.high[0]-0.33f)<0.00001f&&std::abs(turned.low[1]-0.18f)<0.00001f);
 for(unsigned invalid=0;invalid<7;++invalid) {
  auto bad=bottles;
  if(invalid==0)bad[8]=3;
  if(invalid==1)put32(bad,sourceShape,0);
  if(invalid==2)put32(bad,sourceShape+4,UINT32_MAX);
  if(invalid==3)put32(bad,sourceBody+80,0x7fc00000);
  if(invalid==4)write<float>(bad.data(),sourceBody+80,0);
  if(invalid==5)write<float>(bad.data(),sourceBody+32,2);
  if(invalid==6)bad.resize(20);
  bool rejected=false;try{makeFloorPlacement(profile,bad,model);}catch(const std::exception&){rejected=true;}CHECK(rejected);
 }
 const auto savedOpen=originalOpen;const auto savedResolve=originalResolve;
 floorTestProfile=&profile;floorTestModel=&bottles;originalOpen=&floorOpen;originalResolve=&floorResolve;
 for(unsigned refusal=0;refusal<4;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeCounter(f);f.missingComfort=true;
  // Exact server state captured from the user's bottle (entity 2853).
  write<int64_t>(f.attachment,0,INT64_MAX);f.attachment[12]=1;
  transportPlacementCatalog.clear();transportCatalogRegistries.insert(f.registry);transportResourceDb=f.registry;floorTestReads=0;
  write(f.counterDescriptor,0,model);
  static const char name[]="Prop_Destructible_Villageprops_bottles_12";
  write(f.counterDescriptor,0x10,static_cast<const char*>(name));write<uint64_t>(f.counterDescriptor,0x18,sizeof(name));
  floorUnavailable=true;CHECK(!transportPlacementItem(f.world,1217));
  CHECK(!transportUnsupportedFloorModels.count(model)&&floorTestReads==1);
  floorUnavailable=false;CHECK(!transportPlacementItem(f.world,1217)&&floorTestReads==1);
  transportFloorRetry[model]=GetTickCount64()-1001;floorTestReads=0;
  CHECK(transportPlacementItem(f.world,1217)==TransportCounterItem&&floorTestReads==2);
  CHECK(transportPlacementItem(f.client,1217)==TransportCounterItem&&floorTestReads==2);
  const auto identity=read<transport::Identity>(model.data(),0);
  f.placementOverride=transportPlacementInfo(TransportCounterItem,f.registry,&identity);
  CHECK(f.placementOverride&&f.placementOverride!=f.info&&equal(transportFloorPlacements.at(model),0x3c0,model));
  CHECK(!transportPlacementInfo(42,f.registry,&identity));
  // A different allowed model uses the same adapter without a name exception.
  auto other=model;other[0]^=1;write(f.counterDescriptor,0,other);
  CHECK(transportPlacementItem(f.world,1217)==TransportCounterItem&&floorTestReads==3);
  CHECK(equal(transportFloorPlacements.at(other),0x3c0,other));
  CHECK(transportPlacementInfo(TransportCounterItem,f.registry,&identity)==f.placementOverride);
  write(f.counterDescriptor,0,model);
  for(unsigned invalid=0;invalid<6;++invalid) {
   const auto stable=read<std::array<Byte,16>>(f.attachment,0);
   if(invalid==0)f.attachment[12]=0;
   if(invalid==1)write<int64_t>(f.attachment,0,123);
   if(invalid==2)write<uint32_t>(f.attachment,8,123);
   if(invalid>=3)f.attachment[10+invalid]=1;
   TransportSourceSnapshot rejected{};CHECK(!transportSnapshotSource(f.world,f.zones,1,1217,rejected));
   write(f.attachment,0,stable);
  }
  runtimeGrab(f);write(f.descriptor,0x30,static_cast<Byte*>(f.client));
  transportMove.input.place=true;runtimeValidatePending(f);
  CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued&&!f.writes&&!f.moves);
  CHECK(transportMove.preview.item==f.placementOverride);
  if(refusal==1)f.refusal=1u<<8; // Native placement refusal must survive.
  if(refusal==2)write(f.counterDescriptor,0,other); // Same donor ID cannot replace source identity.
  write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);
  if(refusal==3)f.attachment[12]=2; // A transition during preview invalidates the snapshot.
  transportMovePostStep(f.server,f.fullWorld());
  if(refusal){CHECK(!f.writes&&!f.moves);continue;}
  CHECK(f.writes==1&&f.moves==1);
  CHECK(f.attachment[12]==1&&read<int64_t>(f.attachment,0)==INT64_MAX);
  CHECK(read<int64_t>(f.transform,0)==int64_t(8)<<32&&read<Guid>(f.counterDescriptor,0)==model);
  transportMoveRender(f.client,f.client);
  CHECK(f.moves==2&&f.modelMoves==1&&transportMove.clientCollisions.empty());
  for(size_t at:{size_t(0),size_t(56),size_t(112)})CHECK(std::memcmp(f.render+at,f.transform,56)==0);
  TransportSourceSnapshot after{};CHECK(transportSnapshotSource(f.world,f.zones,2,1217,after));
  CHECK(after.source.original.position[0]==int64_t(8)<<32&&after.source.templateId==identity);
 }
 originalOpen=savedOpen;originalResolve=savedResolve;transportResourceDb=nullptr;
 std::puts("PASS: bottle floor descriptor, bounds, relative collisions, malformed geometry refusal, generic cache, preview/server commit/render sync and identity preservation.");
}
