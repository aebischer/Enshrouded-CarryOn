// Observe the local server's post-step boundary, then run the movement adapter
// on that owning thread. The observed address alone never authorizes a write.
using TransportServerStep=void(__fastcall*)(void*);
TransportServerStep originalTransportServerStep=nullptr;
std::atomic<uintptr_t> transportObservedServerWorld{0};
void (*transportMoveServerCallback)(void*,uintptr_t)=nullptr;
void __fastcall transportServerStep(void* server) {
 originalTransportServerStep(server);
 const auto world=server?read<uintptr_t>(server,0x1d0):0;
 const auto previous=transportObservedServerWorld.exchange(world,std::memory_order_relaxed);
 if(world&&world!=previous)log("CarryOn: local server post-step observed");
 if(transportMoveServerCallback)transportMoveServerCallback(server,world);
}
