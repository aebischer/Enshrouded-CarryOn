static Byte snapshotTransform[56]{},snapshotInventory[84]{},snapshotItem[4]{},snapshotCollider[16]{};
static bool snapshotMissing=false;
static size_t __fastcall snapshotSlot(void*,const uint32_t* entity) {return *entity==1205?0:1;}
static const uint16_t* __fastcall snapshotIndex(void*,const uint32_t* type) {
 static const uint16_t indices[]{0,1,2,3,4};return *type>=1&&*type<=4?indices+*type:nullptr;
}
static ComponentView* __fastcall snapshotRead(ComponentView* out,void*,const uint16_t* index,uint32_t) {
 *out={};if(snapshotMissing)return out;
 switch(*index) {
 case 1:*out={snapshotTransform,sizeof(snapshotTransform)};break;
 case 2:*out={snapshotInventory,sizeof(snapshotInventory)};break;
 case 3:*out={snapshotItem,sizeof(snapshotItem)};break;
 case 4:*out={snapshotCollider,sizeof(snapshotCollider)};break;
 }
 return out;
}
static void testTransportSnapshot() {
 transportEntitySlot=&snapshotSlot;transportTypeIndex=&snapshotIndex;transportRead=&snapshotRead;
 transportCurrentTransformType=1;transportInventorySetupType=2;transportUsedItemType=3;transportStaticColliderType=4;
 Byte world[0x190]{},registry[0x100]{},record[0x38]{},zone[0x48]{},zones[0x20]{};
 const Byte* records[]{record};std::array<Byte,0x28> templateId{};templateId[0]=9;Guid claimId{};claimId[0]=7;
 write(world,0,static_cast<Byte*>(registry));write<size_t>(world,0x158,1);write(world,0x188,records+0);
 write(record,0x28,templateId.data());
 write(zones,0x10,static_cast<Byte*>(zone));write<size_t>(zones,0x18,1);
 write<Guid>(zone,0x30,claimId);write<uint32_t>(zone,0x40,1027);
 for(size_t i=0;i<3;++i)write<int64_t>(zone,(i+3)*8,100);
 write<float>(snapshotTransform,36,1.0f);
 for(size_t i=0;i<3;++i)write<float>(snapshotTransform,40+i*4,1.0f);
 write<uint32_t>(snapshotInventory,0,1205);write<uint32_t>(snapshotInventory,4,1206);
 snapshotInventory[0x53]=1;write<uint32_t>(snapshotItem,0,0xcc076a8);write<uint64_t>(snapshotCollider,8,151);
 TransportSourceSnapshot first{},next{};
 CHECK(transportSnapshotSource(world,zones,5,1205,first));
 CHECK(first.source.entity==1205&&first.source.claimEntity==1027&&first.source.original.valid());
 CHECK(!first.source.movable&&!first.source.available); // Snapshot alone is never permission to move.
 CHECK(read<uint32_t>(first.inventorySetup.data(),4)==1206);
 CHECK(transportSnapshotSource(world,zones,5,1205,next)&&transportSameSourceState(first,next));
 write<uint32_t>(snapshotInventory,4,1207);
 CHECK(transportSnapshotSource(world,zones,5,1205,next)&&!transportSameSourceState(first,next));
 CHECK(read<uint32_t>(first.inventorySetup.data(),4)==1206); // Value copy, no borrowed inventory view.
 CHECK(!transportSnapshotSource(world,zones,5,1206,next)&&!next.source.entity);
 snapshotMissing=true;CHECK(!transportSnapshotSource(world,zones,5,1205,next));snapshotMissing=false;
 snapshotInventory[0x53]=0;CHECK(!transportSnapshotSource(world,zones,5,1205,next));snapshotInventory[0x53]=1;
 write<float>(snapshotTransform,36,0.0f);CHECK(!transportSnapshotSource(world,zones,5,1205,next)&&!next.source.entity);
 std::puts("PASS: authoritative source value snapshot, storage identity, missing components and no implicit move permission.");
}
