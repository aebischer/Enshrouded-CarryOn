static void runtimeRest(MoveFixture& f,bool bed) {
 runtimeCounter(f);f.rest=true;
 const uint8_t kind=static_cast<uint8_t>((bed?RestBed|RestAssignment:RestSeat)|RestNpcTarget|RestLock);
 transportRestTemplates[TransportCounterTemplate]=kind;
 write<uint32_t>(f.restOffer,8,bed?0xc2ee9b13u:0x2804c2f7u);
 // Historical user who has already stood up must not permanently lock a seat.
 write<uint32_t>(f.restOffer,24,77);write<uint32_t>(f.bedAssignment,0,1234);
 f.npcTarget[0]=1;write<float>(f.restAttachment,4,0.8f);
}
static void testRestFurnitureRuntime() {
 for(bool bed:{false,true}) {
  for(bool decor:{false,true})for(bool usable:{false,true}) {
   MoveFixture f;runtimeSetup(f);runtimeRest(f,bed);transportObjects=decor;transportInteractive=usable;
   TransportSourceSnapshot snapshot{};
   CHECK(bool(transportClassifyProp(f.client,1217).item)==usable);
   CHECK(transportSnapshotSource(f.world,f.zones,1,1217,snapshot)==usable);
  }
  for(unsigned refusal=0;refusal<12;++refusal) {
   MoveFixture f;runtimeSetup(f);runtimeRest(f,bed);transportObjects=false;
   const auto attachment=read<std::array<Byte,48>>(f.restAttachment,0);
   const auto offer=read<std::array<Byte,56>>(f.restOffer,0);
   const auto comfort=read<std::array<Byte,16>>(f.comfort,0);
   runtimeGrab(f);CHECK(transportMove.source.rest&&transportMove.source.decor);
   CHECK(!transportMove.source.door&&!transportMove.source.well);
   write(f.descriptor,0x30,static_cast<Byte*>(f.client));transportMove.input.place=true;runtimeValidatePending(f);
   CHECK(transportMove.transaction.stage()==transport::MoveStage::Queued);
   if(refusal==1)f.lock[0]=1;
   if(refusal==2)f.lock[1]=1;
   if(refusal==3)write<uint32_t>(f.npcTarget,24,77);
   if(refusal==4)write<uint32_t>(f.guestLink,0,1217);
   if(refusal==5)write<uint32_t>(f.guestLink,4,1217);
   if(refusal==6)f.missingRest=true;
   if(refusal==7)f.restAttachment[40]=1;
   if(refusal==8)f.restOffer[0]=1;
   if(refusal==9)transportInteractive=false;
   if(refusal==10)write<uint32_t>(f.restOffer,12,1);
   if(refusal==11){if(bed)write<uint32_t>(f.bedAssignment,0,5678);else write<uint32_t>(f.restOffer,24,78);}
   write(f.descriptor,0x30,static_cast<Byte*>(f.world));runtimeValidatePending(f);
   transportMovePostStep(f.server,f.fullWorld());
   if(refusal){CHECK(!f.writes&&!f.moves);continue;}
   CHECK(f.writes==1&&f.moves==1);
   CHECK((read<std::array<Byte,48>>(f.restAttachment,0)==attachment));
   CHECK((read<std::array<Byte,56>>(f.restOffer,0)==offer));
   CHECK((read<std::array<Byte,16>>(f.comfort,0)==comfort));
   CHECK(read<uint32_t>(f.bedAssignment,0)==1234);
   transportMoveRender(f.client,f.client);CHECK(f.moves==2&&f.modelMoves==1&&transportMove.clientCollisions.empty());
   TransportSourceSnapshot after{};CHECK(transportSnapshotSource(f.world,f.zones,2,1217,after));
   CHECK(after.source.original.position[0]==int64_t(8)<<32);
  }
  MoveFixture f;runtimeSetup(f);runtimeRest(f,bed);
  TransportSourceSnapshot rejected{};
  f.lock[0]=1;CHECK(!transportSnapshotSource(f.world,f.zones,1,1217,rejected));f.lock[0]=0;
  write<uint32_t>(f.guestLink,0,1217);CHECK(!transportSnapshotSource(f.world,f.zones,1,1217,rejected));
  write<uint32_t>(f.guestLink,0,0);CHECK(transportSnapshotSource(f.world,f.zones,1,1217,rejected));
 }
 transportMove=TransportMoveRuntime{};transportHolding=false;transportPlaceReady=false;
 std::puts("PASS: bed/chair usable gate, live occupancy and interaction changes refuse without writes, vacated furniture accepted, comfort/NPC assignment preserved, static collision/render synchronization.");
}
static void testRestFurnitureTemplate(const Bytes& profile,const Bytes& bed,const Bytes& chair) {
 const auto savedOpen=originalOpen;const auto savedResolve=originalResolve;
 originalOpen=&floorOpen;originalResolve=&floorResolve;floorTestProfile=&profile;
 for(const auto* fixture:{&bed,&chair}) {
  const auto expected=restFurnitureTemplate(*fixture);CHECK(expected&(fixture==&bed?RestBed:RestSeat));
  MoveFixture f;runtimeSetup(f);runtimeRest(f,fixture==&bed);transportRestTemplates.clear();
  transportResourceDb=f.registry;floorTestModel=fixture;
  floorUnavailable=true;CHECK(!transportRestTemplate(f.counterDescriptor));CHECK(transportRestTemplates.empty());
  floorUnavailable=false;transportTemplateRetry[TransportCounterTemplate]=GetTickCount64()-1001;
  CHECK(transportRestTemplate(f.counterDescriptor)==expected);
  CHECK(transportClassifyProp(f.client,1217).rest==expected);
  const auto count=floorTestReads;CHECK(transportRestTemplate(f.counterDescriptor)==expected&&floorTestReads==count);
  for(const char* excluded:{"keen::ecs::DoNotSave","keen::ecs::SubEntities","keen::ecs::Collider","keen::ecs::InventorySetup"}) {
   auto bad=*fixture;const auto components=array(bad,16,12);bool replaced=false;
   for(uint32_t i=0;i<components.count;++i) {
    const auto row=components.at+size_t(i)*12;
    if(u32(bad,row)==typeHash("keen::ecs::Comfort")){put32(bad,row,typeHash(excluded));replaced=true;break;}
   }
   CHECK(replaced&&!restFurnitureTemplate(bad));
  }
  auto bad=*fixture;const auto components=array(bad,16,12);
  for(uint32_t i=0;i<components.count;++i) {
   const auto row=components.at+size_t(i)*12;
   if(u32(bad,row)==typeHash("keen::ecs::InteractionOfferRuntime"))put32(bad,row+4+u32(bad,row+4)+8,0xf5024957u);
  }
  CHECK(!restFurnitureTemplate(bad));
 }
 originalOpen=savedOpen;originalResolve=savedResolve;transportResourceDb=nullptr;
 std::puts("PASS: actual desert bed/Cairn chair contracts, transient resource retry, cached classification, compound/unsafe/unrelated interaction rejection.");
}
