static unsigned testServerCalls=0;
static void __fastcall testServerOriginal(void* server) {
 ++testServerCalls;
 // The original may change the current world during its step.
 if(server)write<uintptr_t>(server,0x1d0,0x12345678);
}
static void testTransportServer() {
 originalTransportServerStep=&testServerOriginal;
 Byte server[0x1d8]{};write<uintptr_t>(server,0x1d0,0x11111111);
 transportServerStep(server);
 CHECK(testServerCalls==1&&transportObservedServerWorld.load()==0x12345678);
 // The observer does not change any server bytes. Only our fake original did.
 for(size_t i=0;i<0x1d0;++i)CHECK(server[i]==0);
 CHECK(read<uintptr_t>(server,0x1d0)==0x12345678);
 transportServerStep(nullptr);
 CHECK(testServerCalls==2&&!transportObservedServerWorld.load());
 std::puts("PASS: read-only server observer, original callback ordering and null world.");
}
