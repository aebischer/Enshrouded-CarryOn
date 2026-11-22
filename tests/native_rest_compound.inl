struct RestAssemblyFixture {
 struct Child {
  Byte record[0x38]{},descriptor[0x28]{},pose[56]{},render[176]{},collider[16]{},collision[0x80]{};
  Byte attachment[48]{},offer[56]{},owner[4]{},link[8]{},npc[32]{},lock[2]{},bed[4]{};
 } child[3];
 const Byte* records[4]{};const Byte* collisions[4]{};
 Byte saved[12]{};unsigned count=0,writes=0,moves=0,renders=0;int failWrite=-1;
 bool noOffer=false;
};
static RestAssemblyFixture* restAssembly=nullptr;
static std::map<Guid,Bytes> assemblyResources;
static const Bytes* assemblyResource=nullptr;
static unsigned assemblyReads=0;
static uint32_t* __fastcall assemblyOpen(uint32_t* out,void*,const Key* key) {
 ++assemblyReads;const auto found=assemblyResources.find(key->guid);
 assemblyResource=found==assemblyResources.end()?nullptr:&found->second;*out=assemblyResource?1:0;return out;
}
static uint64_t* __fastcall assemblyResolve(uint64_t* out,void*,uint32_t) {
 CHECK(assemblyResource);out[0]=reinterpret_cast<uint64_t>(assemblyResource->data());out[1]=assemblyResource->size();out[2]=0x100000000ull;return out;
}
static const uint16_t* __fastcall assemblyIndex(void* registry,const uint32_t* hash) {
 static const uint16_t owner=30,link=31,saved=32;
 if(*hash==typeHash("keen::ecs::OwnerRelationship"))return &owner;
 if(*hash==typeHash("keen::ecs::DespawnLink"))return &link;
 if(*hash==typeHash("keen::ecs::StoreAssociatedNpcIds"))return &saved;
 return runtimeIndex(registry,hash);
}
static size_t __fastcall assemblySlot(void*,const uint32_t* entity) {
 return *entity>=1217&&*entity<=1217+restAssembly->count?*entity-1217:999;
}
static ComponentView* __fastcall assemblyRead(ComponentView* out,void* world,const uint16_t* index,uint32_t entity) {
 auto& a=*restAssembly;*out={};
 if(entity==1217) {
  if(*index==18){*out={moveFixture->children,64};return out;}
  if(*index==32){*out={a.saved,12};return out;}
  if(*index==27&&a.noOffer)return out;
 }
 if(entity>=1218&&entity<1218+a.count) {
  auto& c=a.child[entity-1218];
  switch(*index) {
   case 1:*out={c.pose,56};break;case 4:case 10:*out={c.collider,16};break;
   case 11:*out={c.render,176};break;case 26:*out={c.attachment,48};break;
   case 27:*out={c.offer,56};break;case 30:*out={c.owner,4};break;case 31:*out={c.link,8};break;
   case 23:*out={c.npc,32};break;case 7:*out={c.lock,2};break;case 28:*out={c.bed,4};break;
  }return out;
 }
 return runtimeRead(out,world,index,entity);
}
static const Byte* __fastcall assemblyStatic(void* world,const uint16_t* index,uint32_t entity) {
 if(entity>=1218&&entity<1218+restAssembly->count&&*index==8)return moveFixture->resource;
 return runtimeStatic(world,index,entity);
}
static ComponentView* __fastcall assemblyWrite(ComponentView* out,void* world,void* tracker,const uint16_t* index,uint32_t entity,Byte flags) {
 if(entity>=1218&&entity<1218+restAssembly->count) {
  auto& a=*restAssembly;const auto i=entity-1218;auto& c=a.child[i];*out={};
  CHECK(!flags);
  if(world==moveFixture->world){CHECK(*index==1);++a.writes;if(int(i)!=a.failWrite)*out={c.pose,56};}
  else {CHECK(world==moveFixture->client&&*index==11);++a.renders;*out={c.render,176};}
  return out;
 }
 return runtimeWrite(out,world,tracker,index,entity,flags);
}
static size_t __fastcall assemblyCollisionSlot(void*,const uint64_t* handle) {return *handle>=163&&*handle<=163+restAssembly->count?size_t(*handle-163):999;}
static void __fastcall assemblyCollisionMove(void* manager,void* grid,bool replay,uint64_t handle,const float* pose,const void* resource) {
 if(handle==163){runtimeCollisionMove(manager,grid,replay,handle,pose,resource);return;}
 CHECK(handle>163&&handle<=163+restAssembly->count&&manager==moveFixture->manager&&!replay&&resource==moveFixture->resource+8);
 for(size_t i=0;i<8;++i)CHECK(std::isfinite(pose[i]));++restAssembly->moves;
}
static void assemblySetup(MoveFixture& f,RestAssemblyFixture& a,unsigned count,bool bed) {
 runtimeSetup(f);runtimeRest(f,bed);restAssembly=&a;a.count=count;a.noOffer=bed;
 RestFurnitureDefinition definition{};definition.count=static_cast<uint8_t>(count);
 definition.flags=uint8_t(bed?(RestBed|RestCompound|RestNoOffer|RestSavedAssignments):(RestSeat|RestCompound));
 transportRestTemplates[TransportCounterTemplate]=definition.flags;
 a.records[0]=f.record;a.collisions[0]=f.collision;
 for(unsigned i=0;i<count;++i) {
  auto& d=definition.children[i];d.model[0]=70;d.offset={float(i)-0.5f,0,0};
  d.flags=uint8_t((bed?RestBed|RestAssignment:RestSeat)|RestNpcTarget|RestLock);
  auto& c=a.child[i];write(c.descriptor,0,d.model);write(c.record,0x28,static_cast<Byte*>(c.descriptor));
  a.records[i+1]=c.record;a.collisions[i+1]=c.collision;
  write<uint32_t>(f.children,i*4,1218+i);write<uint32_t>(c.owner,0,1217);write<uint32_t>(c.link,0,1217);
  write<uint32_t>(c.offer,8,bed?0xc2ee9b13u:0x2804c2f7u);write<uint32_t>(c.bed,0,800+i);
  transport::Placement pose{};CHECK(transport::restChildPlacement(transportReadPlacement(f.transform),d.offset,d.rotation,pose));
  const auto bytes=transportNativePlacement(pose);std::memcpy(c.pose,bytes.data(),56);
  write<uint64_t>(c.collider,8,164+i);write<uint32_t>(c.collision,0x54,1218+i);write<uint64_t>(c.collision,0x58,164+i);
  write(c.collision,0x68,static_cast<Byte*>(f.resource+8));c.collision[0x78]=1;
 }
 a.saved[0]=1;write<uint32_t>(a.saved,4,800);write<uint32_t>(a.saved,8,801);
 transportRestAssemblies[TransportCounterTemplate]=definition;
 for(auto world:{f.world,f.client}){write<size_t>(world,0x158,count+1);write(world,0x188,a.records+0);}
 write<size_t>(f.manager,0x58,count+1);write(f.manager,0x88,a.collisions+0);
 transportTypeIndex=&assemblyIndex;transportEntitySlot=&assemblySlot;transportRead=&assemblyRead;
 transportStaticRead=&assemblyStatic;transportWorldWrite=&assemblyWrite;transportColliderSlot=&assemblyCollisionSlot;transportColliderMove=&assemblyCollisionMove;
}
static void testRestCompoundRuntime() {
 for(bool bed:{false,true})for(bool rotated:{false,true})for(unsigned refusal=0;refusal<10;++refusal) {
  MoveFixture f;RestAssemblyFixture a;assemblySetup(f,a,2,bed);
  CHECK(transportRestRoot(f.client,1219)==1217);
  runtimeGrab(f);
  if(rotated) {
   f.rotated=true;auto destination=transportMove.source.source.original;
   destination.position={int64_t(8)<<32,-(int64_t(2)<<32),0};destination.rotation={0,0.70710677f,0,0.70710677f};
   const auto frame=++transportMove.preview.frame;
   transportMove.transaction.preview(destination,frame,true,true,transportMove.source.source.claim,transportMove.source.source.claimEntity);
   transport::MoveRequest request{};CHECK(transportMove.transaction.queue(frame,request));transportMove.input.requestedAt=GetTickCount64();
  }else runtimeQueue();
  transportMove.validation.ticket=transportMove.transaction.request().ticket;
  transportMove.validation.epoch=transportMove.world.epoch;
  if(refusal==1)write<uint32_t>(a.child[1].npc,24,9);
  if(refusal==2)a.child[1].lock[0]=1;
  if(refusal==3)write<uint32_t>(a.child[1].owner,0,99);
  if(refusal==4)write<uint32_t>(f.children,4,1218);
  if(refusal==5)write<uint32_t>(a.child[1].link,0,99);
  if(refusal==6)write<int64_t>(a.child[1].pose,0,99LL<<32);
  if(refusal==7)write<uint32_t>(a.child[1].collision,0x54,99);
  if(refusal==8)a.failWrite=1;
  if(refusal==9)write<uint32_t>(a.child[1].offer,24,77),write<uint32_t>(f.guestLink,0,1219);
  const auto before=read<std::array<Byte,56>>(a.child[0].pose,0);
  TransportSourceSnapshot snapshot{};
  if(transportObserveMove(transportMove,f.world,GetTickCount64(),snapshot,f.zones))
   transportCommitMove(transportMove,f.world,f.zones,GetTickCount64(),snapshot);
  if(refusal){CHECK(!f.moves&&!a.moves&&read<int64_t>(f.transform,0)==0);CHECK((read<std::array<Byte,56>>(a.child[0].pose,0)==before));continue;}
  CHECK(f.moves==1&&a.moves==2&&transportMove.clientCollisions.size()==3);
  CHECK(transportSnapshotSource(f.world,f.zones,2,1217,snapshot));
  CHECK(read<uint32_t>(a.saved,4)==800&&read<uint32_t>(a.child[1].bed,0)==801);
  transportMoveRender(f.client,f.client);CHECK(f.modelMoves==1&&a.renders==2&&a.moves==4&&transportMove.clientCollisions.empty());
  // A fresh source snapshot after placement uses the same authored offsets as native respawn.
  for(size_t i=0;i<2;++i){transport::Placement expected{};CHECK(transport::restChildPlacement(snapshot.source.original,snapshot.childOffsets[i],snapshot.childRotations[i],expected));CHECK(transport::restChildMatches(expected,transportReadPlacement(a.child[i].pose)));}
 }
 transportMove=TransportMoveRuntime{};
 std::puts("PASS: compound seats/beds, occupancy and links, atomic refusal, invisible child render/collision, native respawn offsets and NPC assignments.");
}
