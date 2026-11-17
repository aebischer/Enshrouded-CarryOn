// Read-only probe of the native collision service used by each ECS world.
// Captured service addresses are validated against the active owning world by
// the movement adapter; they are never used as permission to move a chest.
using TransportColliderStep=void(__fastcall*)(void*);
TransportColliderStep originalTransportStaticServer=nullptr,originalTransportStaticClient=nullptr;
struct TransportCollisionObservation {
 std::atomic<uintptr_t> world{0},system{0},pathGrid{0};
 std::atomic<uint64_t> sequence{0};
};
TransportCollisionObservation transportServerCollision,transportClientCollision;
std::mutex transportCollisionProbeMutex;
void transportObserveStaticCollision(void* context,bool server) {
 if(!context||!transportInitRow)return;
 const auto descriptor=read<const Byte*>(context,0);if(!descriptor)return;
 const auto world=read<uintptr_t>(descriptor,0x30);if(!world)return;
 // These two native systems have explicit sides. Capture during creation,
 // even before the client selector or the server post-step knows the world.
 alignas(16) Byte row[0x68]{};
 transportInitRow(context,row,server?0x68:0x60);
 const auto system=read<uintptr_t>(row,server?0x58:0x50);if(!system)return;
 const auto grid=server?read<uintptr_t>(row,0x50):0;
 auto& observation=server?transportServerCollision:transportClientCollision;
 std::lock_guard<std::mutex> lock(transportCollisionProbeMutex);
 observation.sequence.fetch_add(1,std::memory_order_acq_rel);
 const auto previous=observation.world.exchange(world,std::memory_order_relaxed);
 observation.system.store(system,std::memory_order_relaxed);
 observation.pathGrid.store(grid,std::memory_order_relaxed);
 observation.sequence.fetch_add(1,std::memory_order_release);
 if(world!=previous)log(server?
  "CarryOn: server static collision service captured (read-only)":
  "CarryOn: client static collision service captured (read-only)");
}
void __fastcall transportStaticServer(void* context) {
 originalTransportStaticServer(context);
 transportObserveStaticCollision(context,true);
}
void __fastcall transportStaticClient(void* context) {
 originalTransportStaticClient(context);
 transportObserveStaticCollision(context,false);
}
