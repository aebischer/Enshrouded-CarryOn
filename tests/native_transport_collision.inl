static size_t collisionLookupResult=0;
static unsigned collisionMoves=0;
static void* collisionExpectedManager=nullptr;
static void* collisionExpectedGrid=nullptr;
static const Byte* collisionExpectedResource=nullptr;
static size_t __fastcall collisionLookup(void*,const uint64_t* handle) {CHECK(*handle==163);return collisionLookupResult;}
static void __fastcall collisionMove(void* manager,void* grid,bool replay,uint64_t handle,const float* transform,const void* resource) {
 CHECK(manager==collisionExpectedManager&&grid==collisionExpectedGrid&&!replay&&handle==163);
 CHECK(resource==collisionExpectedResource&&transform[0]==8.0f&&transform[1]==-2.0f&&transform[2]==0.0f);
 CHECK(transform[3]==1.0f&&transform[4]==0&&transform[5]==0&&transform[6]==0&&transform[7]==1.0f);
 ++collisionMoves;
}
static void testTransportCollision() {
 Byte system[0x1a8]{},manager[0x90]{},record[0x80]{},resource[0x38]{},grid{};
 const Byte* records[]{record};
 write(system,0x1a0,static_cast<Byte*>(manager));write<size_t>(manager,0x58,1);write(manager,0x88,records+0);
 write<uint32_t>(record,0x54,1217);write<uint64_t>(record,0x58,163);
 write(record,0x68,static_cast<Byte*>(resource));record[0x78]=1;
 transportColliderSlot=&collisionLookup;transportColliderMove=&collisionMove;
 collisionExpectedManager=manager;collisionExpectedGrid=&grid;collisionExpectedResource=resource;
 TransportCollisionTarget target{};
 CHECK(transportResolveCollision(system,&grid,1217,163,resource,target));
 transport::Placement placement{};placement.position={int64_t(8)<<32,-(int64_t(2)<<32),0};
 CHECK(transportMoveCollision(target,placement)&&collisionMoves==1);
 placement.scale[1]=2;CHECK(!transportMoveCollision(target,placement)&&collisionMoves==1);
 record[0x78]=0;CHECK(!transportResolveCollision(system,&grid,1217,163,resource,target));record[0x78]=1;
 CHECK(!transportResolveCollision(system,&grid,1218,163,resource,target));
 CHECK(!transportResolveCollision(system,&grid,1217,163,resource+1,target));
 write<uint64_t>(record,0x58,164);CHECK(!transportResolveCollision(system,&grid,1217,163,resource,target));
 write<uint64_t>(record,0x58,163);collisionLookupResult=1;
 CHECK(!transportResolveCollision(system,&grid,1217,163,resource,target)&&!target.manager);
 CHECK(collisionMoves==1);
 std::puts("PASS: native static collision ownership, recycled handles, resource matching and transform ABI.");
}
