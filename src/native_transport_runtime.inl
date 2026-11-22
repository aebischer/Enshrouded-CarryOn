using TransportCurrentEntity=uint32_t(__fastcall*)(const void*,const void*,const void*);
using TransportWorldWrite=ComponentView*(__fastcall*)(ComponentView*,void*,void*,const uint16_t*,uint32_t,Byte);
TransportCurrentEntity transportCurrentEntity=nullptr;
TransportWorldWrite transportWorldWrite=nullptr;
uint32_t transportSlotsType=0,transportLockType=0,transportColliderResourceType=0;
std::mutex transportMoveMutex;
// Native engine calls may re-enter our detours on the same thread. Skip nested
// adapter work, while still forwarding native callbacks. Never recursively
// mutate a transaction or release its lock halfway through a commit.
thread_local bool transportMoveEntered=false;
struct TransportMoveEntry {
 bool entered=!transportMoveEntered;
 TransportMoveEntry(){if(entered)transportMoveEntered=true;}
 ~TransportMoveEntry(){if(entered)transportMoveEntered=false;}
};
bool (*transportMoveFocused)()=&gameHasFocus;
// The renderer's global UI context is scoped to native rendering calls and
// can be null on the server thread. Read only the value snapshot published by
// the targeting row and the UI callback, never the transient native pointer.
TransportTravelState (*transportTravelSnapshot)()=&transportCachedTravelState;
struct TransportClientCollisionMove {
 uintptr_t world=0;uint32_t entity=0,item=0;
 transport::Placement destination{};
 ULONGLONG queuedAt=0;
 bool reported=false;
 transport::Identity templateId{};
 ULONGLONG lastAttempt=0;
 ULONGLONG lastServerCheck=0;
 uint32_t restParent=0;
};
struct TransportWorldState {
 uintptr_t client=0,server=0;
 uint64_t session=0,epoch=0;
 uint32_t actor=0;
 ULONGLONG lastStep=0,uiGraceAt=0;
};
struct TransportInputState {
 bool grab=false,place=false,wasActive=false,left=false,right=false;
 uint32_t requestedEntity=0,requestedItem=0;
 ULONGLONG requestedAt=0;
};
struct TransportPreviewState {
 TransportPreview renderer;
 Guid model{};
 const Byte* item=nullptr;
 const void* actor=nullptr;
 TransportPlacementSample sample;
 ULONGLONG updatedAt=0;
 uint64_t frame=0,queryRow=(std::numeric_limits<uint64_t>::max)();
 bool controlsReported=false,failureReported=false;
 bool suspended=false;ULONGLONG resumedAt=0,resumeWaitAt=0;
};
struct TransportValidationState {
 uint64_t ticket=0,epoch=0;
 std::array<int64_t,6> bounds{};
};
struct TransportMoveRuntime {
 // Transaction owns move authorization. Source retains the immutable storage
 // snapshot needed to revalidate identity and inventory links at commit.
 transport::MoveTransaction transaction;
 TransportSourceSnapshot source;
 TransportWorldState world;
 TransportInputState input;
 TransportPreviewState preview;
 TransportValidationState validation;
 std::vector<TransportClientCollisionMove> clientCollisions;
 size_t syncCursor=0;
 void clearMoveData() {
  source={};input.grab=false;input.place=false;validation.ticket=0;
  input.requestedEntity=0;input.requestedItem=0;preview.sample={};preview.updatedAt=0;
  preview.item=nullptr;preview.actor=nullptr;
  preview.controlsReported=false;
  preview.queryRow=(std::numeric_limits<uint64_t>::max)();preview.failureReported=false;
  transportHolding=false;transportPlaceReady=false;transportTravelPaused=false;preview.suspended=false;preview.resumedAt=0;preview.resumeWaitAt=0;
 }
 // Also clear HUD readiness when the transaction itself is already idle.
 void cancel() {transaction.cancel();clearMoveData();}
};
TransportMoveRuntime transportMove;
bool transportReplicatedPlacement(const transport::Placement& a,const transport::Placement& b);
bool transportBounds(const Byte* info,const transport::Placement& placement,std::array<int64_t,6>& bounds) {
 if(!info||!placement.valid())return false;
 const auto low=read<std::array<float,3>>(info,0x3d0),high=read<std::array<float,3>>(info,0x3dc);
 double minimum[3]{},maximum[3]{};
 const auto& q=placement.rotation;
 for(unsigned corner=0;corner<8;++corner) {
  double point[3]{};
  for(unsigned axis=0;axis<3;++axis) {
   if(!std::isfinite(low[axis])||!std::isfinite(high[axis])||low[axis]>high[axis])return false;
   point[axis]=double((corner&(1u<<axis))?high[axis]:low[axis])*placement.scale[axis];
  }
  const double cross[3]={2*(q[1]*point[2]-q[2]*point[1]),2*(q[2]*point[0]-q[0]*point[2]),2*(q[0]*point[1]-q[1]*point[0])};
  const double rotated[3]={point[0]+q[3]*cross[0]+q[1]*cross[2]-q[2]*cross[1],
   point[1]+q[3]*cross[1]+q[2]*cross[0]-q[0]*cross[2],point[2]+q[3]*cross[2]+q[0]*cross[1]-q[1]*cross[0]};
  for(unsigned axis=0;axis<3;++axis) {
   const double fixed=double(placement.position[axis])+rotated[axis]*4294967296.0;
   if(!std::isfinite(fixed)||fixed<=-0x1p63||fixed>=0x1p63)return false;
   if(!corner||fixed<minimum[axis])minimum[axis]=fixed;
   if(!corner||fixed>maximum[axis])maximum[axis]=fixed;
  }
 }
 for(unsigned axis=0;axis<3;++axis){bounds[axis]=static_cast<int64_t>(std::floor(minimum[axis]));bounds[axis+3]=static_cast<int64_t>(std::ceil(maximum[axis]));}
 return true;
}
TransportClaim transportRequestClaim(const transport::MoveRequest& request) {
 TransportClaim claim{};std::memcpy(claim.id.data(),&request.destinationClaim,16);claim.altar=request.destinationClaimEntity;return claim;
}
void transportSuspendMove(TransportMoveRuntime& move) {
 move.transaction.suspend();move.preview.suspended=true;move.preview.updatedAt=0;
 move.preview.resumeWaitAt=0;
 move.preview.item=nullptr;move.preview.actor=nullptr;move.preview.sample={};
 move.preview.queryRow=(std::numeric_limits<uint64_t>::max)();
 move.validation.ticket=0;move.input.place=false;move.input.grab=false;
 move.input.wasActive=false;move.input.left=false;move.input.right=false;transportPlaceReady=false;
}
bool transportActorHoldsPincer(void* world,uint32_t actor) {
 const auto slots=transportComponent(world,actor,transportSlotsType,216);
 return slots.data&&transportHeldItem(world,slots.data)==hash(Item);
}
bool transportActorEligible(void* world,uint32_t actor) {
 // Health is authoritative and server-only. The pinned native attribute layout
 // stores current/max signed values in slots 0/7 after its eight-byte header.
 // Player entities need not have the separate NetworkHealth component.
 const auto health=transportComponent(world,actor,typeHash("keen::ecs::Health"),40);
 return health.data&&read<uint32_t>(health.data,0)==0x8eb84995&&
  read<uint16_t>(health.data,4)==8&&(read<Byte>(health.data,6)&1)&&read<Byte>(health.data,7)==8&&
  read<int32_t>(health.data,8)>0&&read<int32_t>(health.data,36)>0&&transportActorHoldsPincer(world,actor);
}
bool transportStorageAvailable(void* world,uint32_t entity) {
 const auto lock=transportComponent(world,entity,transportLockType,2);
 return lock.data&&read<uint16_t>(lock.data,0)==0;
}
bool transportCollisionFor(void* world,const TransportCollisionObservation& observation,uint32_t entity,
                           uint64_t handle,TransportCollisionTarget& target,bool dynamic=false) {
 const auto sequence=observation.sequence.load();if(sequence&1)return false;
 if(observation.world.load()!=reinterpret_cast<uintptr_t>(world))return false;
 const auto system=observation.system.load(),grid=observation.pathGrid.load();
 if(observation.sequence.load()!=sequence)return false;
 const auto index=transportIndex(world,transportColliderResourceType);if(!index)return false;
 const auto resource=transportStaticRead(world,index,entity);if(!resource)return false;
 const auto relative=read<int32_t>(resource,0);
 if(!relative||read<uint32_t>(resource,4)!=1)return false;
 if(dynamic&&read<Byte>(resource+relative,0x2c)!=0)return false;
 const bool valid=transportResolveCollision(reinterpret_cast<void*>(system),reinterpret_cast<void*>(grid),
  entity,handle,resource+relative,target,dynamic)&&observation.sequence.load()==sequence;
 if(valid&&dynamic) {
  const auto collider=transportComponent(world,entity,typeHash("keen::ecs::Collider"),24);
  if(!collider.data)return false;
  if(!read<Byte>(collider.data,16))target.pathGrid=nullptr;
 }
 return valid;
}
void transportMoveInput(void* row,bool left,bool right) {
 TransportMoveEntry entry;if(!entry.entered)return;
 uintptr_t world=0;uint32_t selected=0,actor=0,item=0;Guid model{};
 const bool active=transportModeActive()&&transportEligible.load()&&transportMovementEnabled();
 {
  std::lock_guard<std::mutex> lock(transportMutex);world=transportWorld;
  selected=static_cast<uint32_t>(transportSelection.selected());
  if(selected&&world) {
   const auto modelView=transportComponent(reinterpret_cast<void*>(world),selected,transportModelType,4);
   if(!modelView.data||!read<uint32_t>(modelView.data,0))selected=0;
  }
  if(active&&world&&row) {
   actor=transportCurrentEntity(read<void*>(row,0),nullptr,nullptr);
   item=transportPlacementItem(reinterpret_cast<void*>(world),selected);
   const auto index=transportIndex(reinterpret_cast<void*>(world),transportModelResourceType);
   const auto resource=index&&selected?transportStaticRead(reinterpret_cast<void*>(world),index,selected):nullptr;
   if(resource)model=read<Guid>(resource,0);
  }
 }
 std::lock_guard<std::mutex> lock(transportMoveMutex);auto& move=transportMove;
 if(!active&&transportTravelPaused.load()&&transportHolding.load()&&world==move.world.client) {
  transportSuspendMove(move);return;
 }
 if(!active||world!=move.world.client||actor!=move.world.actor) {
  move.cancel();move.input.wasActive=false;move.world.client=world;move.world.actor=actor;
 }
 const bool leftEdge=active&&move.input.wasActive&&left&&!move.input.left;
 const bool rightEdge=active&&move.input.wasActive&&right&&!move.input.right;
 move.input.left=left;move.input.right=right;move.input.wasActive=active;
 if(!active)return;
 const auto now=GetTickCount64();
 if(!transport_time::fresh(now,move.preview.updatedAt,transport_time::InputFresh))transportPlaceReady=false;
 if(leftEdge&&move.transaction.stage()==transport::MoveStage::Idle&&!move.input.grab&&
    selected&&item&&model!=Guid{}&&transportInsideZone.load()) {
  move.input.requestedEntity=selected;move.input.requestedItem=item;move.preview.model=model;move.input.grab=true;move.input.requestedAt=GetTickCount64();
 }
 if(rightEdge&&move.transaction.stage()==transport::MoveStage::Preview)move.input.place=true;
}
void __fastcall transportValidateMove(uint32_t* flags,void* context,const void* config,const void* transform,
                                     Byte permissions,Byte special,float delta) {
 originalTransportValidate(flags,context,config,transform,permissions,special,delta);
 TransportMoveEntry entry;if(!entry.entered)return;
 if(context&&(transportObjects.load()||transportInteractive.load()))transportRefreshPlacementCatalog(read<void*>(context,0x1b0));
 if(!context||!config||!transportModeActive()||!transportEligible.load()||!transportMovementEnabled())return;
 const auto iterator=read<void*>(context,0);if(!iterator)return;
 const auto descriptor=read<const Byte*>(iterator,0);if(!descriptor)return;
 const auto world=read<uintptr_t>(descriptor,0x30);
 std::lock_guard<std::mutex> lock(transportMoveMutex);auto& move=transportMove;
 if(world!=move.world.client&&world!=move.world.server)return;
 if(transportCurrentEntity(iterator,nullptr,nullptr)!=move.world.actor)return;
 const auto stage=move.transaction.stage();
 if(stage==transport::MoveStage::Idle)return;
 // Native cursor configuration describes the action item, which is empty for
 // this tool. Resolve the held pincer from SlotSelection on the actual actor.
 // Its configuration's primary flag is likewise not a transport-mode flag.
 // Client preview has no Health component; only the server authorizes moves.
 if(world==move.world.server) {
  if(!transportActorEligible(reinterpret_cast<void*>(world),move.world.actor))return;
 } else if(!transportActorHoldsPincer(reinterpret_cast<void*>(world),move.world.actor))return;
 const auto zones=read<const void*>(context,0x190);
 if(world==move.world.client&&stage==transport::MoveStage::Preview) {
  const auto row=transportRows.load(std::memory_order_relaxed);
  if(move.preview.queryRow==row)return; // Primary and secondary native callbacks share this frame.
  move.preview.queryRow=row;
  const auto info=transportPlacementInfo(move.source.item,read<void*>(context,0x1b0),&move.source.source.templateId);
  if(!info||!(read<transport::Identity>(info,0x3c0)==move.source.source.templateId)) {
   move.cancel();return;
  }
  move.preview.sample=transportCalculatePlacement(context,move.source.item,move.source.source.original,permissions,delta,&move.source.source.templateId);
  move.preview.item=info;move.preview.actor=read<const void*>(context,0x108);
  std::array<int64_t,6> bounds{};
  const auto claim=transportBounds(info,move.preview.sample.placement,bounds)?transportDestinationClaim(zones,bounds.data()):TransportClaim{};
  const bool allowed=bool(claim);const auto claimId=read<transport::Identity>(claim.id.data(),0);
  move.transaction.preview(move.preview.sample.placement,++move.preview.frame,move.preview.sample.calculated&&!move.preview.sample.flags,allowed,claimId,claim.altar);
  if(!move.preview.updatedAt) {
   char message[160];sprintf_s(message,"CarryOn: transport placement query calculated=%u flags=%08x destination_claim=%u",
    unsigned(move.preview.sample.calculated),move.preview.sample.flags,unsigned(allowed));log(message);
  }
  move.preview.updatedAt=GetTickCount64();
  transportPlaceReady=move.preview.sample.calculated&&!move.preview.sample.flags&&allowed;
  if(move.input.place) {
   move.input.place=false;transport::MoveRequest request{};
   if(move.transaction.queue(move.preview.frame,request)){move.input.requestedAt=GetTickCount64();transportPlaceReady=false;}
  }
 } else if(world==move.world.server&&stage==transport::MoveStage::Queued) {
  move.validation.ticket=0;
  alignas(16) Byte ownConfig[0xa0]{};
  if(!transportPrepareCursor(context,move.source.item,ownConfig,&move.source.source.templateId))return;
  const auto info=read<const Byte*>(ownConfig,0x60);
  if(!(read<transport::Identity>(info,0x3c0)==move.source.source.templateId))return;
  const auto& request=move.transaction.request();auto destination=transportNativePlacement(request.destination);
  uint32_t ownFlags=0;
  originalTransportValidate(&ownFlags,context,ownConfig,destination.data(),permissions,move.preview.sample.special,delta);
  ownFlags&=~TransportCreationOnlyRefusals;
  if(ownFlags||!transportBounds(info,request.destination,move.validation.bounds)||
     !transportBoxInClaim(zones,transportRequestClaim(request),move.validation.bounds.data()))return;
  move.validation.ticket=request.ticket;move.validation.epoch=move.world.epoch;
 }
}
// Server helpers execute under the caller's transaction lock, in this order.
void transportProcessGrab(TransportMoveRuntime& move,void* world,const void* zones,ULONGLONG now) {
 if(!move.input.grab)return;

  move.input.grab=false;TransportSourceSnapshot source{};
  const auto actorTransform=transportComponent(world,move.world.actor,transportCurrentTransformType,56);
  if(transport_time::within(now,move.input.requestedAt,transport_time::GrabLifetime)&&transportActorEligible(world,move.world.actor)&&
     transportSnapshotSource(world,zones,move.world.session,move.input.requestedEntity,source)&&source.item==move.input.requestedItem&&
     actorTransform.data&&(source.decor||transportStorageAvailable(world,source.source.entity))) {
   const auto original=transportNativePlacement(source.source.original);
   source.source.movable=true;source.source.available=transportDistance(actorTransform.data,original.data())<=3.0f;
   TransportCollisionTarget collision{};
   if(source.source.available&&transportCollisionFor(world,transportServerCollision,source.source.entity,source.collider,collision,source.door)&&
      move.transaction.begin(source.source)) {
    move.source=source;transportHolding=true;log("CarryOn: object transport preview started; original entity unchanged");
   }
  }
}
bool transportObserveMove(TransportMoveRuntime& move,void* world,ULONGLONG now,TransportSourceSnapshot& current,const void* zones) {
  TransportClaim sourceClaim{};std::memcpy(sourceClaim.id.data(),&move.source.source.claim,16);sourceClaim.altar=move.source.source.claimEntity;
  const bool exists=transportSnapshotSource(world,zones,move.world.session,move.source.source.entity,current,&sourceClaim);
  current.source.movable=exists&&current.item==move.source.item;
  current.source.available=exists&&(current.decor||transportStorageAvailable(world,current.source.entity));
  if(!exists||!transportSameSourceState(move.source,current)||!transportActorEligible(world,move.world.actor)||
     !move.transaction.observe(current.source,true)){move.cancel();return false;}
  else if(!move.preview.suspended&&(!move.preview.resumedAt||now-move.preview.resumedAt>transport_time::PreviewStartup)&&move.transaction.stage()==transport::MoveStage::Preview&&now>=move.input.requestedAt&&
          now-move.input.requestedAt>transport_time::PreviewStartup&&(!move.preview.updatedAt||now-move.preview.updatedAt>transport_time::PreviewLifetime)) {
   move.cancel();log("CarryOn: native placement preview unavailable or stale; original entity unchanged");return false;

  }
 return true;
}
void transportCommitMove(TransportMoveRuntime& move,void* world,const void* zones,ULONGLONG now,const TransportSourceSnapshot& current) {
 if(!transport_time::fresh(now,move.input.requestedAt,transport_time::CommitLifetime)) {
  move.cancel();log("CarryOn: expired placement cancelled before mutation");return;
 }
 const auto request=move.transaction.request();
 const bool nativeValid=move.validation.ticket==request.ticket&&move.validation.epoch==move.world.epoch;
 transport::MoveValidation validation{current.source,
  transportBoxInClaim(zones,transportRequestClaim(request),move.validation.bounds.data()),nativeValid,true};
 TransportCollisionTarget collision{};
 bool synchronizationReserved=false;
 try {move.clientCollisions.reserve(move.clientCollisions.size()+1+current.restChildCount);synchronizationReserved=true;}
 catch(const std::exception&){log("CarryOn: placement refused: synchronization allocation failed");}
 if(nativeValid&&synchronizationReserved&&transport::canCommit(request,validation)&&move.transaction.owns(request)&&
    transportCollisionFor(world,transportServerCollision,current.source.entity,current.collider,collision,current.door)) {
  // Preflight every failure before requesting a writable component. No item
  // spawn, destroy, loot, inventory transfer or entity replacement is used.
  const auto& scale=request.destination.scale;
  const auto index=transportIndex(world,transportCurrentTransformType);
  if(index&&scale[0]==scale[1]&&scale[0]==scale[2]) {
   std::array<TransportCollisionTarget,16> childCollisions{};
   std::array<transport::Placement,16> childDestinations{};
   std::array<ComponentView,16> childWrites{};
   bool childrenReady=true;
   for(size_t i=0;i<current.restChildCount&&childrenReady;++i)
    childrenReady=transport::restChildPlacement(request.destination,current.childOffsets[i],current.childRotations[i],childDestinations[i])&&
     transportCollisionFor(world,transportServerCollision,current.children[i],current.childColliders[i],childCollisions[i]);
   if(!childrenReady)return;
   // Resolve every child before mutating any pose. Marking a component writable
   // may schedule replication but does not change its position or interactions.
   for(size_t i=0;i<current.restChildCount;++i) {
    transportWorldWrite(&childWrites[i],world,static_cast<Byte*>(world)+0x7bc030,index,current.children[i],0);
    if(!childWrites[i].data||childWrites[i].size!=56)return;
   }
   ComponentView attachment{};
   const bool invalidateSupport=current.decor&&read<uint32_t>(current.attachmentState.data(),8)!=0;
   bool attachmentReady=!invalidateSupport;
   if(invalidateSupport) {
    const auto attachmentIndex=transportIndex(world,transportAttachmentType);
    if(attachmentIndex)transportWorldWrite(&attachment,world,static_cast<Byte*>(world)+0x7bc030,attachmentIndex,current.source.entity,0);
    attachmentReady=attachment.data&&attachment.size==16&&std::memcmp(attachment.data,current.attachmentState.data(),16)==0;
   }
   ComponentView writable{};
   if(attachmentReady)transportWorldWrite(&writable,world,static_cast<Byte*>(world)+0x7bc030,index,current.source.entity,0);
   if(writable.data&&writable.size==56&&transportMoveCollision(collision,request.destination)) {
    for(size_t i=0;i<current.restChildCount;++i) {
     // All collision targets and movable poses were validated above; this
     // wrapper calls the native void relocation operation without allocation.
     transportMoveCollision(childCollisions[i],childDestinations[i]);
     const auto childBytes=transportNativePlacement(childDestinations[i]);
     std::memcpy(childWrites[i].data,childBytes.data(),childBytes.size());
    }
    const auto bytes=transportNativePlacement(request.destination);std::memcpy(writable.data,bytes.data(),bytes.size());
    if(invalidateSupport) {
     // Native attach_to_surface caches the support in +8 and skips support
     // discovery while searchStartTime is INT64_MAX (3513f6). Invalidate only
     // after the move succeeds. Keep state/flags and let native wakeup/search
     // re-evaluate at the new pose; never request the destructive detach action.
     write<uint32_t>(attachment.data,8,0);write<int64_t>(attachment.data,0,0);
    }
    const TransportClientCollisionMove pending{move.world.client,current.source.entity,current.item,request.destination,now,false,current.source.templateId};
    auto existing=std::find_if(move.clientCollisions.begin(),move.clientCollisions.end(),[&](const auto& p){return p.world==pending.world&&p.entity==pending.entity;});
    if(existing!=move.clientCollisions.end())*existing=pending;
    else move.clientCollisions.push_back(pending);

    for(size_t i=0;i<current.restChildCount;++i) {
     TransportClientCollisionMove childPending{move.world.client,current.children[i],0,childDestinations[i],now,false,read<transport::Identity>(current.childTemplates[i].data(),0)};
     childPending.restParent=current.source.entity;
     const auto found=std::find_if(move.clientCollisions.begin(),move.clientCollisions.end(),[&](const auto& p){return p.world==childPending.world&&p.entity==childPending.entity;});
     if(found!=move.clientCollisions.end())*found=childPending;else move.clientCollisions.push_back(childPending);
    }
    move.transaction.complete(request.ticket);move.clearMoveData();log("CarryOn: existing object transform committed; components preserved");
   }
  }
 }
}
// Only the server can distinguish a temporarily unreplicated object from a
// destroyed/replaced one. Never expire a valid destination just because far-away
// client streaming is slow. Obsolete acknowledgements must not grow forever.
void transportPruneSynchronization(TransportMoveRuntime& move,void* world,ULONGLONG now=GetTickCount64()) {
 size_t checked=0,visited=0;const auto initial=move.clientCollisions.size();
 while(!move.clientCollisions.empty()&&visited++<initial&&checked<32) {
  const size_t i=move.syncCursor%move.clientCollisions.size();
  auto& pending=move.clientCollisions[i];
  const bool old=now>=pending.queuedAt&&now-pending.queuedAt>transport_time::SyncWarning;
  if(old&&transport_time::fresh(now,pending.lastServerCheck,transport_time::SyncRetry)){move.syncCursor=i+1;continue;}
  pending.lastServerCheck=now;++checked;
  const auto descriptor=transportTemplate(world,pending.entity);
  const auto pose=transportComponent(world,pending.entity,transportCurrentTransformType,56);
  if(!descriptor||!(read<transport::Identity>(descriptor,0)==pending.templateId)) {
   move.clientCollisions[i]=move.clientCollisions.back();move.clientCollisions.pop_back();
   move.syncCursor=i;
   log("CarryOn: obsolete client synchronization discarded after server revalidation");
  } else {
   // Native settling/support updates may adjust the pose after our commit.
   // Follow this same entity; temporary missing components are not destruction.
   if(pose.data) {
    const auto authoritative=transportReadPlacement(pose.data);
    if(authoritative.valid()&&!(authoritative==pending.destination)) {
     pending.destination=authoritative;pending.lastAttempt=0;
    }
   }
   move.syncCursor=i+1;
  }
 }
}
void transportMovePostStep(void* server,uintptr_t fullWorld) {
 TransportMoveEntry entry;if(!entry.entered)return;
 std::lock_guard<std::mutex> lock(transportMoveMutex);auto& move=transportMove;
 const auto identity=fullWorld?fullWorld+0xe179c0:0;
 if(move.world.server!=identity){move.cancel();move.world.server=identity;++move.world.session;move.clientCollisions.clear();}
 if(!identity||!server){++move.world.epoch;return;}
 const auto now=GetTickCount64();
 auto world=reinterpret_cast<void*>(identity);
 // A stalled server (loading) does not impose a time limit on the trip. Give
 // the UI its first frame after the stall before deciding its snapshot is dead.
 if(move.world.lastStep&&!transport_time::within(now,move.world.lastStep,transport_time::UiFresh))move.world.uiGraceAt=now;
 move.world.lastStep=now;
 if(move.clientCollisions.empty()&&move.transaction.stage()==transport::MoveStage::Idle&&!move.input.grab) {
  ++move.world.epoch;return;
 }
 transportPruneSynchronization(move,world,now);
 if(transportHolding.load()) {
  const auto ui=transportTravelSnapshot(); // One coherent screen/row/session snapshot.
  const bool sameSession=ui.generation==transportSessionGeneration.load()&&ui.world==move.world.client;
  const bool fresh=transport_time::fresh(now,ui.updatedAt,transport_time::UiFresh);
  if(!sameSession||!transportMovementEnabled()||!transportMoveFocused()||!transportActorEligible(world,move.world.actor))move.cancel();
  else if(!fresh) {
   if(move.preview.suspended&&transport_time::fresh(now,move.world.uiGraceAt,transport_time::PreviewStartup))transportSuspendMove(move);
   else {move.cancel();log("CarryOn: transport cancelled: UI heartbeat expired");}
  } else if(ui.screen==5||(ui.screen==51&&(move.preview.suspended||transportTravelPaused.load()))) {
   transportTravelPaused=true;transportSuspendMove(move);
  } else if(ui.screen==TransportInGame) {
   const bool ready=ui.active&&ui.eligible&&transport_time::fresh(now,ui.rowAt,transport_time::SelectionFresh);
   if(ready) {
    if(move.preview.suspended){move.preview.suspended=false;move.preview.resumedAt=now;move.preview.resumeWaitAt=0;transportTravelPaused=false;}
   } else if(move.preview.suspended) {
    if(!move.preview.resumeWaitAt)move.preview.resumeWaitAt=now;
    if(!transport_time::within(now,move.preview.resumeWaitAt,transport_time::PreviewStartup)) {
     move.cancel();log("CarryOn: transport cancelled: gameplay cursor did not resume");
    }
   } else move.cancel();
  } else move.cancel();
 } else if(!transportModeActive()||!transportEligible.load()||!transportMovementEnabled()||!transportMoveFocused()) {
  move.cancel();
 }
 const auto zones=read<const void*>(server,0x1790);
 transportProcessGrab(move,world,zones,now);
 if(move.transaction.stage()!=transport::MoveStage::Idle) {
  TransportSourceSnapshot current{};
  if(transportObserveMove(move,world,now,current,zones)&&move.transaction.stage()==transport::MoveStage::Queued)
   transportCommitMove(move,world,zones,now,current);
 }
 ++move.world.epoch;
}
using TransportDestroyWorld=void(__fastcall*)(void*);
TransportDestroyWorld originalTransportDestroyWorld=nullptr;
void __fastcall transportDestroyWorld(void* world) {
 // Native ECS destructor 95d9c0, called for server and client worlds. Forget
 // handles before native teardown; never free a render handle on this thread.
 {
  std::scoped_lock lock(transportMutex,transportMoveMutex,transportTravelMutex,transportKeyboardMutex,transportCollisionProbeMutex,transportCommandsMutex,transportCatalogMutex);
  const auto identity=reinterpret_cast<uintptr_t>(world);
  auto& move=transportMove;
  if(identity&&(identity==transportWorld||identity==move.world.server||identity==move.world.client)) {
   transportSessionGeneration.fetch_add(1);
   transportCommands={};
   transportCatalogRegistries.clear();transportCatalogProgress.clear();transportFloorRetry.clear();transportTemplateRetry.clear();
   for(auto* observation:{&transportServerCollision,&transportClientCollision}) {
    const auto observed=observation->world.load();
    if(observed==move.world.server||observed==move.world.client||observed==identity) {
     observation->sequence.fetch_add(1);observation->world=0;observation->system=0;observation->pathGrid=0;observation->sequence.fetch_add(1);
    }
   }
   move.cancel();move.clientCollisions.clear();++move.world.session;
   move.world.client=0;move.world.server=0;move.world.actor=0;move.world.lastStep=0;move.world.uiGraceAt=0;
   move.input={};move.preview.renderer.forget();
   transportSelection={};transportWorld=0;transportPainted=0;transportInspectedObject={};
   transportRenderWorld=0;transportRenderEntity=0;transportRenderHandle=0;
   transportActive=false;transportEligible=false;transportUpdated=0;transportTravelState={};
   transportZoneKnown=false;transportInsideZone=false;transportHasTarget=false;
   transportTabEvents=0;transportCancelEvents=0;
   transportTabKey.reset();transportCancelKey.reset();
  }
 }
 originalTransportDestroyWorld(world);
}
// Caller holds transportMoveMutex. Return a registry-owned item, never an
// actor mutation; each detour keeps its own domain/context preconditions.
const Byte* transportPlacementItemFor(const TransportMoveRuntime& move,const void* actor,ULONGLONG now) {
 if((move.source.door||move.source.well||move.source.rest)?!transportInteractive.load():(move.source.decor?!transportObjects.load():!transportChests.load()))return nullptr;
 if(!actor||!transportModeActive()||!transportEligible.load()||!transportMovementEnabled()||
    !transportMoveFocused()||move.transaction.stage()!=transport::MoveStage::Preview||
    move.preview.actor!=actor||!move.preview.item||
    !transport_time::fresh(now,move.preview.updatedAt,transport_time::InputFresh))return nullptr;
 return move.preview.item;
}
void __fastcall transportPlacementPanel(void* input,float* bounds,float* adjustmentBounds) {
 const Byte* item=nullptr;
 if(transportHudReady()&&transportHolding.load()) {
  std::lock_guard<std::mutex> lock(transportMoveMutex);
  item=transportPlacementItemFor(transportMove,transportMove.preview.actor,GetTickCount64());
 }
 struct Scope {
  const Byte* previous;
  explicit Scope(const Byte* item):previous(transportPlacementHudItem){transportPlacementHudItem=item;}
  ~Scope(){transportPlacementHudItem=previous;}
 } scope(item);
 // Reuse the entire native parent: visibility settings, bottom-right anchor,
 // item title/rotation/snap panel and adjustment panel above it. Its context
 // constructor/copy/destructor retain native ownership of temporary UI data.
 // The item lookup adapter targets only the parent's equipped-item callsite.
 originalTransportPlacementPanel(input,bounds,adjustmentBounds);
}
// Native ClientPlayerInput processing: the last parameter is the selected
// ItemInfo. Substitution affects placement controls, not the held item or its
// action program. Keep the real registry object (relative blob references).
using TransportPlacementInput=void(__fastcall*)(void*,void*,void*,void*,void*,void*,void*,uint32_t,uint64_t,void*,uint32_t,const Byte*);
TransportPlacementInput originalTransportPlacementInput=nullptr;
void __fastcall transportPlacementInput(void* input,void* states,void* digital,void* actor,void* mapping,
 void* settings,void* camera,uint32_t domain,uint64_t time,void* device,uint32_t tick,const Byte* item) {
 if(transportMoveEntered) {
  originalTransportPlacementInput(input,states,digital,actor,mapping,settings,camera,domain,time,device,tick,item);return;
 }
 if(domain==0&&mapping&&originalActionDown) {
  transportPublishCommands(mapping,GetTickCount64());
 }
 std::unique_lock<std::mutex> lock(transportMoveMutex);
 auto& move=transportMove;
 const auto now=GetTickCount64();
 const auto replacement=input&&domain==0?transportPlacementItemFor(move,actor,now):nullptr;
 const bool active=replacement!=nullptr;
 if(active&&!move.preview.controlsReported) {
  move.preview.controlsReported=true;log("CarryOn: native object placement controls active");
 }
 const auto selected=replacement?replacement:item;
 lock.unlock();
 originalTransportPlacementInput(input,states,digital,actor,mapping,settings,camera,domain,time,device,tick,selected);
}
bool transportReplicatedPlacement(const transport::Placement& a,const transport::Placement& b) {
 if(!a.valid()||!b.valid())return false;
 for(size_t i=0;i<3;++i)if(std::abs(double(a.position[i])-double(b.position[i]))>8388608.0||a.scale[i]!=b.scale[i])return false;
 // Replication quantizes orientation. Compare the normalized angular error,
 // not individual float components (the old 0.0001 threshold rejected a live
 // 60-degree placement with only ~0.07 degrees of network rounding).
 // This is a bounded acknowledgement tolerance, never a placement permission.
 double dot=0,lengthA=0,lengthB=0;
 for(size_t i=0;i<4;++i) {
  dot+=double(a.rotation[i])*b.rotation[i];
  lengthA+=double(a.rotation[i])*a.rotation[i];
  lengthB+=double(b.rotation[i])*b.rotation[i];
 }
 constexpr double CosHalfMaxAngle=0.9999976201773518; // 0.25 degrees, q and -q equivalent.
 return std::abs(dot)/std::sqrt(lengthA*lengthB)>=CosHalfMaxAngle;
}
bool transportSyncObjectRender(void* world,void* scene,uint32_t entity,const Byte* transform,bool helper=false) {
 if(!world||!scene||!transform||!transportModelMove||!transportWorldWrite)return false;
 const auto model=transportComponent(world,entity,transportModelType,4);
 const auto render=transportComponent(world,entity,transportRenderType,176);
 const auto index=transportIndex(world,transportRenderType);
 if(!render.data||!index||(!helper&&(!model.data||!read<uint32_t>(model.data,0)))||(helper&&model.data))return false;
 const auto handle=model.data?read<uint32_t>(model.data,0):0;
 ComponentView writable{};
 transportWorldWrite(&writable,world,static_cast<Byte*>(world)+0x7bc030,index,entity,0);
 if(!writable.data||writable.size!=176)return false;
 // StaticTransform entities do not follow the dynamic interpolation path.
 // Reset all three poses together; retain the native interpolation ticks.
 for(size_t offset:{size_t(0),size_t(56),size_t(112)})std::memcpy(writable.data+offset,transform,56);
 const RenderSpan handles{&handle,1},transforms{transform,1};
 if(!helper)transportModelMove(scene,&handles,&transforms);
 return true;
}
void transportMoveRender(void* world,void* scene) {
 TransportMoveEntry entry;if(!entry.entered)return;
 std::lock_guard<std::mutex> lock(transportMoveMutex);auto& move=transportMove;
 const auto identity=reinterpret_cast<uintptr_t>(world);const auto now=GetTickCount64();
 if(identity==move.world.client&&transportModeActive()&&transportHolding.load()&&move.preview.updatedAt&&
    now>=move.preview.updatedAt&&now-move.preview.updatedAt<transport_time::PreviewLifetime&&move.preview.sample.calculated) {
  const auto transform=transportNativePlacement(move.preview.sample.placement);
  if(!move.preview.renderer.draw(identity,scene,move.preview.model,transform.data(),transportPlaceReady.load())&&!move.preview.failureReported) {
   move.preview.failureReported=true;log("CarryOn: transport preview render model unavailable");
  }
 } else move.preview.renderer.clear(identity,scene);
 for(size_t i=0;i<move.clientCollisions.size();) {
  auto it=&move.clientCollisions[i];
  auto remove=[&](){move.clientCollisions[i]=move.clientCollisions.back();move.clientCollisions.pop_back();};
  if(it->world!=identity){remove();continue;}
  const bool delayed=transport_time::fresh(now,it->queuedAt,(std::numeric_limits<ULONGLONG>::max)())&&now-it->queuedAt>transport_time::SyncWarning;
  if(delayed&&!it->reported) {
   it->reported=true;log("CarryOn: object synchronization delayed; waiting for client replication/render components");
  }
  if(delayed&&transport_time::fresh(now,it->lastAttempt,transport_time::SyncRetry)){++i;continue;}
  it->lastAttempt=now;
  const auto item=it->restParent?0:transportPlacementItem(world,it->entity);
  const auto descriptor=transportTemplate(world,it->entity);
  const auto transform=transportComponent(world,it->entity,transportCurrentTransformType,56);
  static const auto clientColliderType=typeHash("keen::ecs::StaticClientCollider");
  const bool door=transportIsDoor(transportTemplate(world,it->entity));
  const auto collider=transportComponent(world,it->entity,door?typeHash("keen::ecs::ClientCollider"):clientColliderType,16);
  // Distant source was not replicated to the client. Wait for native spawn
  // at the destination; absence is not evidence of an identity mismatch.
  if(!descriptor){++i;continue;}
  if(item!=it->item||!(read<transport::Identity>(descriptor,0)==it->templateId)){remove();continue;}
  if(it->restParent&&transportRestRoot(world,it->entity)!=it->restParent){++i;continue;}
  TransportCollisionTarget target{};
  if(transform.data&&collider.data&&transportReplicatedPlacement(transportReadPlacement(transform.data),it->destination)&&
     transportCollisionFor(world,transportClientCollision,it->entity,read<uint64_t>(collider.data,8),target,door)&&
     transportSyncObjectRender(world,scene,it->entity,transform.data,it->restParent!=0)&&
     transportMoveCollision(target,transportReadPlacement(transform.data))) {
   log("CarryOn: object render pose and client collision synchronized");remove();
  }
  else ++i;
 }
}
void bindTransportMovement() {
 bindTransportPlacement();
 transportComfortProviderType=typeHash("keen::ecs::ComfortProvider");
 transportAttachmentType=typeHash("keen::ecs::DynamicAttachToSurface");
 transportInventorySetupType=typeHash("keen::ecs::InventorySetup");
 transportStaticColliderType=typeHash("keen::ecs::StaticCollider");
 transportSlotsType=typeHash("keen::ecs::SlotSelection");
 transportLockType=typeHash("keen::ecs::DynamicInteractionLock");
 transportColliderResourceType=typeHash("keen::ecs::ColliderResourceComponent");
 transportEntitySlot=reinterpret_cast<TransportEntitySlot>(base+0x9b4dc0);
 transportCurrentEntity=reinterpret_cast<TransportCurrentEntity>(base+0x9c3d70);
 transportWorldWrite=reinterpret_cast<TransportWorldWrite>(base+0x9b2930);
 transportColliderSlot=reinterpret_cast<TransportColliderSlot>(base+0x9b4990);
 transportColliderMove=reinterpret_cast<TransportColliderMove>(base+0xa1fdc0);
 transportItemLookup=reinterpret_cast<TransportItemLookup>(base+0xe94470);
 transportCursorConfig=reinterpret_cast<TransportCursorConfig>(base+0x28c430);
 transportCastConfig=reinterpret_cast<TransportCastConfig>(base+0x28caa0);
 transportCast=reinterpret_cast<TransportCast>(base+0x2905c0);
 transportSolve=reinterpret_cast<TransportSolve>(base+0x2994d0);
 transportMoveFrameCallback=&transportMoveInput;transportMoveRenderCallback=&transportMoveRender;
 transportMoveServerCallback=&transportMovePostStep;
}
