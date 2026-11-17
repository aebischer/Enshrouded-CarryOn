// ItemInfo schema of the pinned client. Kept separate from resource hooks.
// Runtime donor fields are restricted to the exact supported baseline layout.
namespace item_layout {
constexpr size_t FixedSize=0x828,PlacementConsume=0x43c,CursorPlacementSound=0x1d4;
constexpr size_t Id=0x0;
constexpr size_t Guid=0x4;
constexpr size_t Name=0x48;
constexpr size_t Description=0x68;
constexpr size_t Caption=0x58;
constexpr size_t Lore=0x78;
constexpr size_t AcquisitionHint=0x88;
constexpr size_t PrimaryHint=0x98;
constexpr size_t SecondaryHint=0xa8;
constexpr size_t ModeHint=0xb8;
constexpr size_t Category=0xcc;
constexpr size_t ModeAction=0xc8;
constexpr size_t ModeEnabled=0xc9;
constexpr size_t Icon=0x56c;
constexpr size_t DebugName=0x820;
constexpr size_t IconModel=0x57c;
constexpr size_t IconScene=0x58c;
constexpr size_t Flags=0x5d8;
constexpr size_t SequenceTag=0x108;
constexpr size_t ImpactEntity=0x128;
constexpr size_t Program=0x138;
constexpr size_t PlacedEntity=0x3c0;
constexpr size_t BlueprintConfig=0x494;
constexpr size_t Sequences=0x544;
constexpr size_t SequenceCount=0x548;
constexpr size_t BuildingCamera=0x39e,VisualTemplate=0x148,Model=0x4f8;
}
Bytes makeItem(const Bytes& input) {
 if(input.size()!=2232||u32(input,0)!=hash(Hammer)||!equal(input,4,Hammer))
  throw std::runtime_error("unsupported tool baseline");
 range(input,0,item_layout::FixedSize);
 Bytes out=input; // Relative child arrays remain valid in this complete copy.
 put32(out,item_layout::Id,hash(Item));putGuid(out,item_layout::Guid,Item);putGuid(out,item_layout::Name,Name);putGuid(out,item_layout::Description,Description);
 putGuid(out,item_layout::Caption,Caption);putGuid(out,item_layout::Lore,Guid{});putGuid(out,item_layout::AcquisitionHint,Guid{});
 // The hammer's category is Blueprints (10), independently of IsBlueprint.
 // Replace its action hints only on the cloned pincer resource. The binding
 // is the native UiBuildingMenu action (129, Tab by default); the transport
 // adapter must consume it before native building navigation is dispatched.
 out[item_layout::Category]=3; // ItemCategory::Tools
 putGuid(out,item_layout::PrimaryHint,Guid{});putGuid(out,item_layout::SecondaryHint,Guid{});
 putGuid(out,item_layout::ModeHint,TransportEnter);out[item_layout::ModeAction]=129;out[item_layout::ModeEnabled]=1;
 putGuid(out,item_layout::Icon,Icon);string(out,item_layout::DebugName,"CarryOn_TransportPincer");
 // Do not let inventory/detail renderers select the hammer's icon scene.
 putGuid(out,item_layout::IconModel,Guid{});putGuid(out,item_layout::IconScene,Guid{});
 // ItemFlag::IsBlueprint (bit 3) routes both native detail panels to the
 // hardcoded hammer ID 0x2950da8d (client RVAs b1a48b and b211cc).
 // A transport tool is not a building plan. Preserve all unrelated flags.
 put32(out,item_layout::Flags,u32(out,item_layout::Flags)&~uint32_t(1u<<3));
 // Transport is supplied by its own adapter; no construction action program.
 const Guid zero{};putGuid(out,item_layout::SequenceTag,zero);putGuid(out,item_layout::ImpactEntity,zero);putGuid(out,item_layout::Program,zero);
 putGuid(out,item_layout::PlacedEntity,zero);putGuid(out,item_layout::BlueprintConfig,zero);put32(out,item_layout::Sequences,0);put32(out,item_layout::SequenceCount,0);
 putGuid(out,item_layout::VisualTemplate,PincerVisualId);putGuid(out,item_layout::Model,PincerModelId);
 out[item_layout::BuildingCamera]=0; // buildingCameraMode in EquipmentSetup
 // Transport does not create/consume a placed item and has no voxel blueprint.
 out[item_layout::PlacementConsume]=0;
 putGuid(out,item_layout::CursorPlacementSound,Guid{});
 return out;
}
