static unsigned collisionOriginalCalls=0,collisionRows=0;
static void __fastcall collisionOriginal(void*) {++collisionOriginalCalls;}
static void __fastcall collisionRow(void*,void* row,size_t size) {
 CHECK(collisionOriginalCalls&&(size==0x68||size==0x60));++collisionRows;
 if(size==0x68){write<uintptr_t>(row,0x58,0x1234);write<uintptr_t>(row,0x50,0x5678);}
 else {write<uintptr_t>(row,0x50,0x9876);write<uintptr_t>(row,0x58,0xdeadbeef);}
}
static void testTransportCollisionProbe() {
 originalTransportStaticServer=&collisionOriginal;originalTransportStaticClient=&collisionOriginal;
 transportInitRow=&collisionRow;
 Byte descriptor[0x38]{};void* context=descriptor;
 // Regression: creation precedes the post-step and client selection callbacks.
 transportObservedServerWorld=0;transportWorld=0;write<uintptr_t>(descriptor,0x30,0x10000000+0xe179c0);
 transportStaticServer(&context);
 CHECK(collisionOriginalCalls==1&&collisionRows==1);
 CHECK(transportServerCollision.system.load()==0x1234&&transportServerCollision.pathGrid.load()==0x5678);
 CHECK(transportServerCollision.sequence.load()==2);
 write<uintptr_t>(descriptor,0x30,0x20000000);
 transportStaticClient(&context);CHECK(collisionRows==2&&transportClientCollision.world.load()==0x20000000);
 CHECK(transportClientCollision.system.load()==0x9876&&!transportClientCollision.pathGrid.load());
 write<uintptr_t>(descriptor,0x30,0x30000000);transportStaticServer(&context);
 CHECK(collisionRows==3&&transportServerCollision.world.load()==0x30000000);
 CHECK(transportServerCollision.sequence.load()==4);
 transportStaticServer(nullptr);CHECK(collisionOriginalCalls==4&&collisionRows==3);
 for(size_t i=0;i<0x30;++i)CHECK(descriptor[i]==0);
 std::puts("PASS: static collision probes before world discovery, both native row layouts, world replacement and null context.");
}
