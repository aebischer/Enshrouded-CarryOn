// Exercise the real adapter against native ABI substitutes. No game or hooks.
static Byte testArchetype[0x50]{},testRecord[0x38]{},testTransform[176]{},testTemplate[0x28]{};
static Byte testColor=0;
static bool testChest=true,testColorAvailable=true;
static bool testUpgradeable=true;
static unsigned testWrites=0;
static unsigned testTargetReads=0;
static constexpr uint32_t TestEntity=0x10203040,TestTransformHash=12345;
static constexpr uint32_t TestInventoryA=0x203f,TestInventoryB=0x2040;
static Byte testInventoryA[96]{},testInventoryB[96]{};
static size_t testInventorySize=96;
static constexpr uint32_t TestModelHash=54321;
static constexpr uint32_t TestVoxelHash=54322;
static bool testVoxelAvailable=false;
static uint32_t testVoxel[2]{88,1};
static uint32_t testModelHandle=77,testDrawHandle=0;
static Byte testDrawColor=0;
static unsigned testDrawCalls=0;
static std::string testSelectionLog;
static void __cdecl captureSelectionLog(const char* message){testSelectionLog+=message;testSelectionLog+='\n';}
static void* testRenderScene=nullptr;
static unsigned testRenderSystems=0;
static void __fastcall testRenderSystem(void*) {++testRenderSystems;testDrawColor=0;}
static void __fastcall testRenderInit(void*,void* row,size_t size) {
 CHECK(size==0xc0||size==0x60);
 if(size==0xc0) {
  static Byte effects[16]{};write(effects,8,testRenderScene);
  write(row,0x70,static_cast<Byte*>(effects));
 } else write(row,size-8,testRenderScene);
}
static void __fastcall testDraw(void* scene,const RenderSpan* ids,const RenderSpan* colors) {
 CHECK(scene&&ids->count==1&&colors->count==1);
 testDrawHandle=read<uint32_t>(ids->data,0);testDrawColor=read<Byte>(colors->data,0);++testDrawCalls;
}
static const uint16_t* __fastcall testIndex(void*,const uint32_t* type) {
 static const uint16_t color=1,chest=2,transform=3,inventory=4,model=5,voxel=6;
 if(*type==TestVoxelHash)return &voxel;
 if(*type==TestModelHash)return &model;
 if(*type==InventoryType)return &inventory;
 if(*type==HighlightType)return &color;
 if(*type==typeHash("keen::ecs::UpgradeStorage"))return &chest;
 if(*type==TestTransformHash)return &transform;
 return nullptr;
}
static ComponentView* __fastcall testRead(ComponentView* out,void*,const uint16_t* type,uint32_t entity) {
 *out={};
 if(*type==4) {
  if(entity==TestInventoryA)*out={testInventoryA,testInventorySize};
  if(entity==TestInventoryB)*out={testInventoryB,testInventorySize};
  return out;
 }
 if(entity!=TestEntity)return out;
 ++testTargetReads;
 if(*type==6&&testVoxelAvailable)*out={reinterpret_cast<Byte*>(testVoxel),8};
 if(*type==5)*out={reinterpret_cast<Byte*>(&testModelHandle),4};
 if(*type==1 && testColorAvailable)*out={&testColor,1};
 if(*type==3)*out={testTransform,sizeof(testTransform)};
 return out;
}
static void* __fastcall testEntity(void*,uint32_t entity) {
 if(entity!=TestEntity)return nullptr;
 testArchetype[0]=testChest&&testUpgradeable?4:0;
 testTemplate[0x20]=testChest?3:8;write(testRecord,0x28,static_cast<Byte*>(testTemplate));
 write(testRecord,0x18,static_cast<Byte*>(testArchetype));return testRecord;
}
static ComponentView* __fastcall testMutable(ComponentView* out,void* tracker,void* stamps,void* record,const uint16_t* type,bool replay) {
 CHECK(tracker && stamps && record==testRecord && *type==1 && !replay);
 ++testWrites;*out={&testColor,1};return out;
}
static void testTransport(const char* fixtureFile) {
 transportTypeIndex=&testIndex;transportRead=&testRead;
 transportEntity=&testEntity;transportMutable=&testMutable;
 transportRenderType=TestTransformHash;
 transportModelType=TestModelHash;transportRenderHighlight=&testDraw;
 transportVoxelType=TestVoxelHash;
 Byte registry[0x100]{},descriptor[0x1c8]{},row[0x218]{},info[0x100]{};
 Byte slots[216]{},targets[264]{},ui[0x1cd9]{},cursor[4328]{},playerTransform[176]{};
 void* world=registry;void* context=descriptor;
 write(descriptor,0x30,&world);write(descriptor,0,static_cast<void*>(registry));
 write(descriptor,0x1c0,static_cast<void*>(registry));
 write(row,0,&context);write(row,0x40,static_cast<Byte*>(slots));write(row,0x28,static_cast<Byte*>(targets));
 write(row,0x38,static_cast<Byte*>(playerTransform));write(info,0x90,static_cast<Byte*>(ui));write(info,0x58,static_cast<Byte*>(cursor));
 write<uint32_t>(slots,160,TestInventoryA);write<uint32_t>(slots,164,TestInventoryB);
 slots[192]=5;write<uint32_t>(testInventoryA,5*12,hash(Item));
 // Real failed case: selected pincer with actionItem empty.
 CHECK(read<uint32_t>(slots,176)==0);
 CHECK(transportHeldItem(&world,slots)==hash(Item));
 // Verify all 16 selectable positions, both banks, invalid IDs and sizes.
 for(unsigned slot=0;slot<16;++slot) {
  auto bank=slot<8?testInventoryA:testInventoryB;
  write<uint32_t>(bank,(slot%8)*12,1000+slot);slots[192]=static_cast<Byte>(slot);
  CHECK(transportHeldItem(&world,slots)==1000+slot);
 }
 slots[192]=16;CHECK(!transportHeldItem(&world,slots));
 slots[192]=255;CHECK(!transportHeldItem(&world,slots));
 slots[192]=5;testInventorySize=95;CHECK(!transportHeldItem(&world,slots));testInventorySize=96;
 write<uint32_t>(slots,160,0);CHECK(!transportHeldItem(&world,slots));
 write<uint32_t>(slots,160,TestInventoryA);write<uint32_t>(testInventoryA,5*12,hash(Item));
 CHECK(!transportHeldItem(&world,nullptr));
 write<uint64_t>(targets,32,TestEntity);
 write<int64_t>(testTransform,0,int64_t(2)<<32);cursor[200]=0;
 write<float>(testTransform,36,1);write(testTransform,40,std::array<float,3>{1,1,1});
 ui[0x1cd8]=1; // Real PE enum value, not a zero-initialized fake gameplay state.
 transportSelection={};transportWorld=0;transportPainted=0;transportUpdated=0;
 transportChests=true;testColor=0;testWrites=0;
 // Use actual native template bytes for all eleven player-placeable chests.
 const std::string fixturePath=fixtureFile;
 const auto fixtureDirectory=fixturePath.substr(0,fixturePath.find_last_of("/\\")+1);
 const char* chestTemplates[]={
  "85a49dbc6516fa46aa45c40911719925", // Prop_Storage_56_T4_Magic
  "2b8d25353e39044d9392bd990be5ae61", // Prop_Storage_32_T2_Box_Magic
  "a9d6ebbd599e654cb056ad0070db97dc", // Prop_Storage_40_T3_Box
  "8a7dddb0691e1845b52fe94cfa769ebc", // Prop_Storage_32_T2_Box
  "2ea6e7fa70bc644db55a8d222c152f3a", // Prop_Storage_56_T4_Box
  "2fa35b95ea084d41bd98ac75990aef04", // Prop_Storage_24_T1_Box_Magic
  "90a8ba652a024b459cb7b92cb8c06cd4", // Prop_Storage_24_T1_Box
  "f12592431d195c41a335013419b202e7", // Prop_Storage_40_T3_Magic
  "35b87dd454fefd44b5b3d6c340673c41", // Prop_Storage_32_T2_Chest_Wood
  "7e7e660cbd0fb94cb58dc3a20df7eaec", // Prop_Storage_40_T5_Chest_Silver
  "bd4c83bcf1584a4997fb0dc0248eb738", // Prop_Storage_72_T6_Magic
 };
 unsigned upgradeable=0;
 for(const auto templateGuid:chestTemplates) {
  std::ifstream input(fixtureDirectory+"resource-"+templateGuid+"-0x39768775-0.bin",std::ios::binary);
  const Bytes native{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
  CHECK(native.size()>28&&native[8]==3);
  const auto components=array(native,16,12);testUpgradeable=false;
  for(uint32_t i=0;i<components.count;++i)
   if(u32(native,components.at+size_t(i)*12)==typeHash("keen::ecs::UpgradeStorage"))testUpgradeable=true;
  if(testUpgradeable)++upgradeable;
  std::memcpy(testTemplate,native.data()+0,16);
  CHECK(transportTargetCandidate(&context,&world,targets,playerTransform).entity==TestEntity);
  transportChests=false;CHECK(!transportTargetCandidate(&context,&world,targets,playerTransform).entity);
  transportChests=true;
 }
 CHECK(upgradeable==8);testUpgradeable=true;
 std::puts("PASS: eleven native chest templates, including three without UpgradeStorage; chest setting respected.");
 auto now=GetTickCount64();
 auto frame=[&](bool tab,bool cancel=false,bool focus=true) {
  transportFrame(row,info,tab,cancel,focus,now);
 };
 testTargetReads=0;frame(false);CHECK(!transportSelection.active() && !testWrites&&!testTargetReads);
 frame(true);CHECK(transportSelection.active());
 CHECK(transportSelection.selected()==TestEntity && testColor==2 && testWrites==1);
 // Inspection must not authorize selection or write to game components.
 const auto savedLogger=logger;logger=&captureSelectionLog;testSelectionLog.clear();
 for(bool debug:{true,false,true,false}) {
  transportInspectObjects=debug;frame(true);
  CHECK(transportSelection.selected()==TestEntity&&testColor==2&&testWrites==1);
 }
 CHECK(testSelectionLog.find("selection diagnostic")!=std::string::npos);
 CHECK(testSelectionLog.find("distance=2.000 reach=3.000")!=std::string::npos);
 logger=savedLogger;
 CHECK(std::strcmp(text(hash(TransportEnter),5),"Quitter le mode transport")==0);
 frame(true);CHECK(testWrites==1 && transportSelection.active());
 testColor=0;frame(true);CHECK(testColor==2 && testWrites==2);
 write<uint64_t>(targets,32,0);frame(true);CHECK(testColor==0 && !transportPainted);
 write<uint64_t>(targets,32,TestEntity);frame(false);CHECK(testColor==2);
 frame(true);CHECK(!transportSelection.active() && testColor==0);
 frame(false);frame(true);CHECK(testColor==2);
 testColor=7;frame(false,true);CHECK(testColor==7); // Preserve another effect.
 frame(false);frame(true);CHECK(testColor==2);
 ui[0x1cd8]=4;frame(false);CHECK(!transportSelection.active() && testColor==7);
 ui[0x1cd8]=1;frame(true);CHECK(!transportSelection.active());
 frame(false);frame(true);CHECK(transportSelection.active());
 frame(false,false,false);CHECK(!transportSelection.active() && testColor==7);
 frame(false);frame(true);
 configureTransport(false);frame(false);CHECK(!transportPainted && testColor==7);
 configureTransport(true);testChest=false;frame(false);CHECK(!transportSelection.selected());
 testChest=true;targets[40]=1;frame(false);CHECK(!transportSelection.selected());
 targets[40]=0;write<int64_t>(testTransform,0,int64_t(4)<<32);
 frame(false);CHECK(!transportSelection.selected());
 write<int64_t>(testTransform,0,int64_t(2)<<32);frame(false);CHECK(testColor==2);
 write<uint32_t>(slots,176,hash(Item)); // stale action must not mask switching away
 write<uint32_t>(testInventoryA,5*12,hash(Hammer));frame(false);
 CHECK(!transportSelection.active() && !transportPainted && testColor==7);
 write<uint32_t>(slots,176,0);write<uint32_t>(testInventoryA,5*12,hash(Item));
 frame(false);testColorAvailable=false;frame(true);
 CHECK(transportSelection.active() && !transportPainted && testColor==7);
 testColorAvailable=true;frame(true);CHECK(transportPainted==TestEntity && testColor==2);
 write<uint64_t>(targets,32,0x200000001ull);frame(false);
 CHECK(!transportPainted && !transportSelection.selected() && testColor==7);
 write<uint64_t>(targets,32,TestEntity);frame(false);
 CHECK(transportPainted==TestEntity);now+=2000;frame(true);
 CHECK(!transportSelection.active() && !transportPainted && testColor==7);
 // Regression: only the actual InGame value accepts Tab. Invalid and all
 // other reflected screens must release gameplay eligibility and selection.
 bool screenDown=true;
 for(unsigned screen=0;screen<=53;++screen) {
  if(screen==1)continue;
  ui[0x1cd8]=1;frame(false);frame(true);
  CHECK(transportSelection.active() && transportEligible);
  ui[0x1cd8]=static_cast<Byte>(screen);frame(false);
  CHECK(!transportSelection.active() && !transportEligible && !transportPainted);
  transportTabKey.reset();CHECK(!transportCommand(TransportModeAction,screenDown,true,now));
  screenDown=false;CHECK(!transportCommand(TransportModeAction,screenDown,true,now));
  screenDown=true;
 }
 // Map and teleport loading suspend only an already-held transaction. Long
 // gaps must not reset its selection mode; unrelated menus still cancel it.
 ui[0x1cd8]=1;frame(false);frame(true);transportHolding=true;
 now+=5000;ui[0x1cd8]=5;frame(false);
 CHECK(transportCachedTravelState().screen==5);
 CHECK(transportSelection.active()&&!transportActive&&!transportEligible&&transportTravelPaused);
 now+=5000;ui[0x1cd8]=51;frame(false);CHECK(transportSelection.active()&&transportTravelPaused);
 now+=5000;ui[0x1cd8]=1;frame(false);
 CHECK(transportSelection.active()&&transportActive&&transportEligible&&!transportTravelPaused);
 ui[0x1cd8]=5;frame(false);ui[0x1cd8]=2;frame(false);CHECK(!transportSelection.active());
 transportHolding=false;transportTravelPaused=false;
 // Exercise queued keyboard events through the frame adapter with InGame=1.
 ui[0x1cd8]=1;now=GetTickCount64();frame(false);
 transportTabEvents=0;transportCancelEvents=0;
 CHECK(transportCommand(TransportModeAction,screenDown,true,now));
 screenDown=false;CHECK(transportCommand(TransportModeAction,screenDown,true,now));
 CHECK(transportTabEvents==1);
 CHECK(transportFrame(row,info,false,false,true,now));
 frame((transportTabEvents.exchange(0)&1)!=0);
 CHECK(transportSelection.active() && transportPainted==TestEntity);
 frame(false,true);
 // Native action ownership: no native menu on Tab/escape, no repeated toggle,
 // no lost short tap, and every captured key-up stays paired with its key-down.
 now=GetTickCount64();transportUpdated=now;transportEligible=true;transportActive=false;
 transportTabKey.reset();transportCancelKey.reset();transportTabEvents=0;transportCancelEvents=0;
 Byte action=TransportModeAction;bool down=true;
 CHECK(transportCommand(action,down,true,now));CHECK(transportTabEvents==1);
 CHECK(transportCommand(action,down,true,now));CHECK(transportTabEvents==1);
 down=false;CHECK(transportCommand(action,down,true,now));
 CHECK(transportTabEvents.exchange(0)==1); // survives press + release between frames
 down=true;CHECK(!transportCommand(action,down,false,now));
 CHECK(!transportCommand(action,down,true,now)); // native press must retain native release
 down=false;CHECK(!transportCommand(action,down,true,now));
 action=TransportCancelAction;down=true;
 CHECK(!transportCommand(action,down,true,now));transportActive=true;
 CHECK(!transportCommand(action,down,true,now));down=false;
 CHECK(!transportCommand(action,down,true,now));down=true;
 CHECK(transportCommand(action,down,true,now) && transportCancelEvents==1);
 down=false;transportEligible=false;
 CHECK(transportCommand(action,down,true,now+2000));
 down=true;CHECK(!transportCommand(action,down,true,now+2000));
 action=0;CHECK(!transportCommand(action,down,true,now));
 // Placed chest with no HighlightColor: render command only, no ECS writes.
 now=GetTickCount64();ui[0x1cd8]=1;testColorAvailable=false;
 transportSelection={};transportUpdated=0;transportRenderWorld=0;
 transportRenderEntity=0;transportRenderHandle=0;testDrawCalls=0;
 frame(false);frame(true);CHECK(transportSelection.selected()==TestEntity);
 const auto writesBeforeRender=testWrites;
 transportRenderFrame(&world,registry,now);
 CHECK(testDrawCalls==1&&testDrawHandle==77&&testDrawColor==2);
 transportRenderFrame(&world,registry,now);CHECK(testDrawCalls==2);
 frame(false,true);transportRenderFrame(&world,registry,now);
 CHECK(testDrawCalls==3&&testDrawColor==0&&!transportRenderEntity);
 frame(false);frame(true);transportRenderFrame(&world,registry,now);
 testModelHandle=0;frame(false,true);transportRenderFrame(&world,registry,now);
 CHECK(testDrawCalls==4); // destroyed model: never restore a stale handle
 testModelHandle=78;frame(false);frame(true);transportRenderFrame(&world,registry,now);
 CHECK(testDrawCalls==5&&testDrawHandle==78);
 transportRenderFrame(&world,registry,now+2001);
 CHECK(testDrawCalls==6&&testDrawColor==0); // stale session snapshot expires
 transportRenderFrame(&world,registry,now);CHECK(testDrawCalls==7);
 transportRenderFrame(registry,registry,now);
 CHECK(testDrawCalls==7&&!transportRenderEntity); // world change: drop old model
 // Voxel-backed chest uses renderModelHandle, not the brick handle at +4.
 testModelHandle=0;testVoxelAvailable=true;
 transportRenderFrame(&world,registry,now);
 CHECK(testDrawCalls==8&&testDrawHandle==88&&testDrawColor==2);
 frame(false,true);transportRenderFrame(&world,registry,now);
 CHECK(testDrawCalls==9&&testDrawHandle==88&&testDrawColor==0);
 now=GetTickCount64();frame(false);frame(true);testRenderScene=registry;
 CHECK(!transportSelection.selected()); // Voxel-only visuals cannot be repositioned by this adapter.
 testModelHandle=77;frame(false);
 originalTransportRender=&testRenderSystem;originalTransportVoxelRender=&testRenderSystem;
 transportInitRow=&testRenderInit;testRenderSystems=0;
 transportRender(&context);CHECK(testRenderSystems==1&&testDrawColor==2&&testDrawHandle==77);
 transportVoxelRender(&context);CHECK(testRenderSystems==2&&testDrawColor==2&&testDrawHandle==77);
 frame(false,true);transportVoxelRender(&context);CHECK(testDrawColor==0&&!transportRenderEntity);
 originalTransportRender=nullptr;originalTransportVoxelRender=nullptr;transportInitRow=nullptr;
 testVoxelAvailable=false;
 CHECK(testWrites==writesBeforeRender); // no component or inventory mutation
 testColorAvailable=true;transportRenderHighlight=nullptr;transportRenderWorld=0;
 transportTypeIndex=nullptr;transportRead=nullptr;transportEntity=nullptr;
 transportMutable=nullptr;transportSelection={};transportWorld=0;
 transportUpdated=0;transportActive=false;transportEligible=false;
 transportTabKey.reset();transportCancelKey.reset();transportTabEvents=0;transportCancelEvents=0;
 std::puts("PASS: native transport adapter, target filtering, native write marking, color restoration, menus, focus and localized command.");
}
