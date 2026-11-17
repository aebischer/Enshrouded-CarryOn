#pragma once
#include "content_catalog.h"
#include <cmath>
#include <algorithm>
namespace carryon::content {
inline uint32_t typeHash(const char* name) {
 uint32_t h=2166136261u;for(;*name;++name)h=(h^static_cast<uint8_t>(*name))*16777619u;return h;
}
// Native water suppliers (family 13), identified by their interaction contract,
// not by model names. Fuel alone also occurs on unrelated usable objects.
inline bool isWaterWellTemplate(const Bytes& model) {
 range(model,0,24);if(model[8]!=13)return false;
 const auto components=array(model,16,12);unsigned traits=0;
 for(uint32_t i=0;i<components.count;++i) {
  const size_t row=components.at+size_t(i)*12;const auto type=u32(model,row);
  if(type==typeHash("keen::ecs::DoNotSave")||type==typeHash("keen::ecs::SubEntities")||type==typeHash("keen::ecs::Collider"))return false;
  if(type==typeHash("keen::ecs::Fuel"))traits|=1;
  if(type==typeHash("keen::ecs::FuelSetup"))traits|=2;
  if(type==typeHash("keen::ecs::FuelCraftingStock"))traits|=4;
  if(type==typeHash("keen::ecs::NetworkFuel"))traits|=8;
  if(type==typeHash("keen::ecs::StaticCollider"))traits|=16;
  if(type==typeHash("keen::ecs::StaticClientCollider"))traits|=32;
  if(type==typeHash("keen::ecs::InteractionOfferRuntime")) {
   const size_t body=row+4+u32(model,row+4);range(model,body,u32(model,row+8));
   // Native water drawing sequence, shared by all eleven current variants.
   if(u32(model,row+8)!=56||u32(model,body+8)!=0xf5024957u)return false;
   traits|=64;
  }
 }
 return traits==127;
}
inline float blobFloat(const Bytes& b,size_t at) {
 range(b,at,4);float value;std::memcpy(&value,b.data()+at,4);
 if(!std::isfinite(value))throw std::runtime_error("non-finite collider geometry");return value;
}
struct FloorBounds {std::array<float,3> low{},high{};};
// Copy whole resources: nested blob-relative offsets remain valid. Never clone
// only the fixed ItemInfo or only a collider header with borrowed children.
inline Bytes makeFloorPlacement(const Bytes& profile,const Bytes& model,const Guid& modelId,FloorBounds* result=nullptr) {
 const auto profileId=guid("e3a56abbb97f3946ad1fbd266c5ccace");
 if(profile.size()!=2826||!equal(profile,4,profileId)||u32(profile,0)!=hash(profileId))
  throw std::runtime_error("unsupported native floor placement profile");
 range(model,0,24);if(model[8]!=8&&model[8]!=10&&!isWaterWellTemplate(model))throw std::runtime_error("unsupported floor placement family");
 const auto components=array(model,16,12);size_t colliderField=SIZE_MAX;
 for(uint32_t i=0;i<components.count;++i) {
  const size_t row=components.at+size_t(i)*12;
  if(u32(model,row)==typeHash("keen::ecs::DoNotSave"))throw std::runtime_error("native template disables persistence");
  if(u32(model,row)==typeHash("keen::ecs::ColliderResourceComponent")) {
   if(colliderField!=SIZE_MAX||u32(model,row+8)!=8)throw std::runtime_error("ambiguous collider component");
   colliderField=row+4+u32(model,row+4);range(model,colliderField,8);
  }
 }
 if(colliderField==SIZE_MAX)throw std::runtime_error("no placement colliders");
 const auto resources=array(model,colliderField,56);
 if(!resources.count||resources.count>64)throw std::runtime_error("unsupported collider count");
 FloorBounds bounds{};bool haveBounds=false;
 for(uint32_t r=0;r<resources.count;++r) {
  const auto shapes=array(model,resources.at+size_t(r)*56,12);
  if(shapes.count>128)throw std::runtime_error("unsupported collider shapes");
  for(uint32_t i=0;i<shapes.count;++i) {
   const size_t row=shapes.at+size_t(i)*12;const auto type=u32(model,row);
   const size_t body=row+4+u32(model,row+4),size=u32(model,row+8);range(model,body,size);
   // Bounds use support functions of the native shapes. The original shapes
   // themselves remain intact in the placement collider array below.
   if(size<80)throw std::runtime_error("truncated collider header");
   range(model,body,80);
   std::array<float,3> center{},extent{};std::array<double,4> q{};double norm=0;
   for(size_t a=0;a<3;++a){center[a]=blobFloat(model,body+8+a*4);
    if(std::abs(center[a])>1000)throw std::runtime_error("invalid collider offset");}
   for(size_t a=0;a<4;++a){q[a]=blobFloat(model,body+20+a*4);norm+=q[a]*q[a];}
   if(std::abs(norm-1)>0.001)throw std::runtime_error("invalid collider orientation");
   for(auto& value:q)value/=std::sqrt(norm);
   const auto x=q[0],y=q[1],z=q[2],w=q[3];
   const double rotation[3][3]={{1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)},
    {2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)},
    {2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)}};
   auto dimension=[&](size_t index,bool allowZero=false) {
    if(size<84+index*4)throw std::runtime_error("truncated collider dimensions");
    // Authored native resources contain signed dimensions (including the
    // counter). Bounds enclose either sign; retain the original collider bytes.
    const auto value=std::abs(blobFloat(model,body+80+index*4));
    if((!allowZero&&value==0)||value>1000)throw std::runtime_error("invalid collider extent");
    return double(value);
   };
   std::array<float,3> lower{},upper{};
   if(type==typeHash("keen::ecs::BoxColliderData")) {
    const double half[3]={dimension(0),dimension(1),dimension(2)};
    for(size_t a=0;a<3;++a)for(size_t j=0;j<3;++j)extent[a]+=static_cast<float>(std::abs(rotation[a][j])*half[j]);
   } else if(type==typeHash("keen::ecs::SphereColliderData")) {
    extent.fill(static_cast<float>(dimension(0)));
   } else if(type==typeHash("keen::ecs::CapsuleColliderData")||type==typeHash("keen::ecs::CylinderColliderData")) {
    const auto radius=dimension(0),halfLength=dimension(1,true)*0.5;
    const bool capsule=type==typeHash("keen::ecs::CapsuleColliderData");
    for(size_t a=0;a<3;++a) {
     const auto axis=(std::min)(1.0,std::abs(rotation[a][1]));
     extent[a]=static_cast<float>(axis*halfLength+radius*(capsule?1.0:std::sqrt((std::max)(0.0,1-axis*axis))));
    }
   } else if(type==typeHash("keen::ecs::TaperedCapsuleColliderData")) {
    const auto bottom=dimension(0),top=dimension(1),halfLength=dimension(2,true)*0.5;
    for(size_t a=0;a<3;++a) {
     const auto shift=rotation[a][1]*halfLength;
     lower[a]=static_cast<float>((std::min)(-shift-bottom,shift-top));
     upper[a]=static_cast<float>((std::max)(-shift+bottom,shift+top));
    }
   } else throw std::runtime_error("unknown native placement collider shape");
   for(size_t a=0;a<3;++a) {
    const auto low=center[a]+lower[a]-extent[a],high=center[a]+upper[a]+extent[a];
    bounds.low[a]=haveBounds?(std::min)(bounds.low[a],low):low;
    bounds.high[a]=haveBounds?(std::max)(bounds.high[a],high):high;
   }
   haveBounds=true;
  }
 }
 if(!haveBounds)throw std::runtime_error("no placement collider geometry");
 const size_t aligned=(profile.size()+3)&~size_t(3);
 constexpr size_t limit=16*1024*1024;
 if(aligned>limit||model.size()>limit-aligned)throw std::runtime_error("native resource size limit");
 Bytes out;out.reserve(aligned+model.size());out.assign(profile.begin(),profile.end());
 // Geometry comes from the donor, presentation must not name that donor.
 putGuid(out,0x48,DecorPlacementName);
 putGuid(out,0x58,Guid{});putGuid(out,0x68,Guid{});
 const size_t appended=append(out,model);point(out,0x400,appended+resources.at,resources.count);
 putGuid(out,0x3c0,modelId);
 std::memcpy(out.data()+0x3d0,bounds.low.data(),12);std::memcpy(out.data()+0x3dc,bounds.high.data(),12);
 // Use the same footprint for snapping. No donor-sized snapping box survives.
 std::memcpy(out.data()+0x3e8,bounds.low.data(),12);std::memcpy(out.data()+0x3f4,bounds.high.data(),12);
 out[0x43c]=0;put32(out,0x428,0);put32(out,0x42c,0); // No item/material consumption.
 if(result)*result=bounds;
 return out;
}
}
