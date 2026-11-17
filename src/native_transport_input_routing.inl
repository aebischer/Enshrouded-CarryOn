// Native action context (3ab140 -> 3c47b0), before binding evaluation.
// The caller initializes exactly 0x63 bytes: eleven pointers then three flags.
// Only replace the selected ItemInfo; native modifier/axis/menu policy remains
// authoritative. Never mutate the real equipped item or actor construction state.
using TransportActionContext=void(__fastcall*)(void*,void*,const void*);
TransportActionContext originalTransportActionContext=nullptr;
using TransportActionAllowed=void(__fastcall*)(void*,Byte,bool);
TransportActionAllowed originalTransportActionAllowed=nullptr;
// e85800 toggles action eligibility; it does not remap or evaluate bindings.
// Keep the configured mode/cancel actions enabled only in this local context.
thread_local void* transportOwnedMapping=nullptr;
thread_local bool transportOwnCancel=false;
void __fastcall transportActionAllowed(void* mapping,Byte action,bool allowed) {
 if(mapping==transportOwnedMapping&&mapping&&
    (action==TransportModeAction||(action==TransportCancelAction&&transportOwnCancel)))allowed=true;
 originalTransportActionAllowed(mapping,action,allowed);
}
struct TransportInputScope {
 void* previous;bool previousCancel;
 TransportInputScope(void* mapping,bool cancel):previous(transportOwnedMapping),previousCancel(transportOwnCancel) {
  transportOwnedMapping=mapping;transportOwnCancel=cancel;
 }
 ~TransportInputScope(){transportOwnedMapping=previous;transportOwnCancel=previousCancel;}
};
void transportCaptureActions(void* mapping,bool eligible,ULONGLONG now) {
 if(!mapping||!originalActionDown)return;
 for(const Byte action:{TransportModeAction,TransportCancelAction}) {
  const bool down=originalActionDown(mapping,action);
  if(transportCommand(action,down,eligible,now)) {
   // ea2990 reads this evaluated byte. Leave bindings, physical input,
   // modifiers and previous state intact so held buttons cannot re-trigger.
   write<Byte>(mapping,(size_t(action)+1)*0x118,0);
  }
 }
}
void __fastcall transportActionContext(void* mapping,void* state,const void* context) {
 if(transportMoveEntered){originalTransportActionContext(mapping,state,context);return;}
 const auto now=GetTickCount64();
 const bool owns=mapping&&context&&read<Byte>(context,0x60)==TransportInGame&&
  !read<Byte>(context,0x62)&&transportMoveFocused()&&transportEligible.load()&&
  transport_time::fresh(now,transportUpdated.load(),transport_time::SelectionFresh);
 transportCaptureActions(mapping,owns,now);
 TransportInputScope scope(owns?mapping:nullptr,owns&&transportActive.load());
 const Byte* item=nullptr;
 if(mapping&&context&&transportHolding.load()) {
  std::lock_guard<std::mutex> lock(transportMoveMutex);
  item=transportPlacementItemFor(transportMove,read<const void*>(context,8),GetTickCount64());
 }
 if(!item){originalTransportActionContext(mapping,state,context);return;}
 alignas(16) std::array<Byte,0x63> local{};
 std::memcpy(local.data(),context,local.size());
 write(local.data(),0x58,item);
 // No lock held across native input processing; it may call other hooks.
 originalTransportActionContext(mapping,state,local.data());
}
