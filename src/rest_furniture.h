#pragma once
#include "floor_placement.h"
namespace carryon::content {
enum RestFurniture : uint8_t { RestNone=0,RestSeat=1,RestBed=2,RestNpcTarget=4,RestLock=8,RestAssignment=16,RestCompound=32,RestNoOffer=64,RestSavedAssignments=128 };
struct RestChildDefinition {
 Guid model{};std::array<float,3> offset{};std::array<float,4> rotation{0,0,0,1};uint8_t flags=0;
};
struct RestFurnitureDefinition {
 uint8_t flags=0,count=0;std::array<RestChildDefinition,16> children{};
};
// Classify native interaction sequences and validate bounded child definitions.
// Invisible helpers are accepted only as members of a validated assembly.
inline uint8_t restFurnitureTemplate(const Bytes& model,RestFurnitureDefinition* definition=nullptr,bool helper=false) {
 if(definition)*definition={};
 range(model,0,24);if(model[8]!=(helper?0:8))return RestNone;
 const auto components=array(model,16,12);unsigned traits=0;uint8_t result=0;bool noSave=false;
 for(uint32_t i=0;i<components.count;++i) {
  const auto row=components.at+size_t(i)*12;const auto type=u32(model,row);
  if(type==typeHash("keen::ecs::DoNotSave")){if(!helper)return RestNone;noSave=true;}
  if(type==typeHash("keen::ecs::SubEntities")) {
   if(!definition||helper||definition->count)return RestNone;
   const auto body=row+4+u32(model,row+4);range(model,body,u32(model,row+8));
   if(u32(model,row+8)<12||model[body+8])return RestNone;
   const auto children=array(model,body,216);
   if(!children.count||children.count>definition->children.size())return RestNone;
   definition->count=static_cast<uint8_t>(children.count);result|=RestCompound;
   for(uint32_t j=0;j<children.count;++j) {
    const auto child=children.at+size_t(j)*216;
    // Unconditional, independently positioned helpers owned by this root.
    if(model[child+56]||model[child+200]||!model[child+206]||!model[child+207])return RestNone;
    auto& out=definition->children[j];std::memcpy(out.model.data(),model.data()+child,16);
    if(out.model==Guid{})return RestNone;
    double norm=0;
    for(size_t a=0;a<3;++a){out.offset[a]=blobFloat(model,child+16+a*4);if(std::abs(out.offset[a])>100||blobFloat(model,child+28+a*4)!=0)return RestNone;}
    for(size_t a=0;a<4;++a){out.rotation[a]=blobFloat(model,child+40+a*4);norm+=double(out.rotation[a])*out.rotation[a];}
    if(std::abs(norm-1)>0.001)return RestNone;
   }
  }
  if(type==typeHash("keen::ecs::Collider")||type==typeHash("keen::ecs::InventorySetup"))return RestNone;
  if(type==typeHash("keen::ecs::StaticCollider"))traits|=1;
  if(type==typeHash("keen::ecs::StaticClientCollider"))traits|=2;
  if(type==typeHash("keen::ecs::ModelComponent")){if(helper)return RestNone;traits|=4;}
  if(type==typeHash("keen::ecs::StoreAssociatedNpcIds"))result|=RestSavedAssignments;
  if(type==typeHash("keen::ecs::InteractionAttachment")&&u32(model,row+8)==48)traits|=8;
  if(type==typeHash("keen::ecs::ActiveNpcTarget"))result|=RestNpcTarget;
  if(type==typeHash("keen::ecs::InteractionLock"))result|=RestLock;
  if(type==typeHash("keen::ecs::NpcBed"))result|=RestAssignment;
  if(type==typeHash("keen::ecs::InteractionOfferRuntime")) {
   const auto size=u32(model,row+8);const auto body=row+4+u32(model,row+4);range(model,body,size);
   if(size!=56)return RestNone;
   const auto sequence=u32(model,body+8);
   if(sequence==0x2804c2f7u)result|=RestSeat;
   else if(sequence==0xc2ee9b13u)result|=RestBed;
   else return RestNone;
   traits|=16;
  }
 }
 if(helper){if(!noSave)return RestNone;traits|=4;}
 if((result&RestCompound)&&!(traits&16)){traits|=16;result|=RestNoOffer;}
 const bool valid=traits==31&&((result&(RestSeat|RestBed))||(result&RestCompound));
 if(definition&&valid)definition->flags=result;
 return valid?result:RestNone;
}
}
