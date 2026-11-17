// Pinned manager path confirmed on the live magical chest. Calling this helper
// requires the owning world's thread; a diagnostic address is not a capability.
using TransportColliderSlot=size_t(__fastcall*)(void*,const uint64_t*);
using TransportColliderMove=void(__fastcall*)(void*,void*,bool,uint64_t,const float*,const void*);
TransportColliderSlot transportColliderSlot=nullptr;
TransportColliderMove transportColliderMove=nullptr;
struct TransportCollisionTarget {
 void* manager=nullptr;
 void* pathGrid=nullptr;
 const Byte* resource=nullptr;
 uint64_t handle=0;
 uint32_t entity=0;
};
bool transportResolveCollision(void* system,void* grid,uint32_t entity,uint64_t handle,
                               const Byte* resource,TransportCollisionTarget& out,bool dynamic=false) {
 out={};
 if(!system||!entity||!handle||!resource||!transportColliderSlot||!transportColliderMove)return false;
 auto manager=read<Byte*>(system,0x1a0);if(!manager)return false;
 const auto slot=transportColliderSlot(manager+0x48,&handle);
 if(slot>=read<size_t>(manager,0x58))return false;
 const auto values=read<const Byte*>(manager,0x88);if(!values)return false;
 const auto record=read<const Byte*>(values,slot*sizeof(void*));if(!record)return false;
 if(read<uint32_t>(record,0x54)!=entity||read<uint64_t>(record,0x58)!=handle||
    read<const Byte*>(record,0x68)!=resource||bool(read<Byte>(record,0x78)&1)==dynamic)return false;
 out={manager,grid,resource,handle,entity};return true;
}
bool transportMoveCollision(const TransportCollisionTarget& target,const transport::Placement& placement) {
 if(!target.manager||!target.resource||!target.handle||!target.entity||!transportColliderMove||!placement.movable())return false;
 alignas(16) float transform[8]{};
 for(size_t axis=0;axis<3;++axis)transform[axis]=static_cast<float>(placement.position[axis])*0x1p-32f;
 transform[3]=placement.scale[0];
 std::memcpy(transform+4,placement.rotation.data(),16);
 transportColliderMove(target.manager,target.pathGrid,false,target.handle,transform,target.resource);
 return true; // Native routine has no error result; never claim a saved move here.
}
