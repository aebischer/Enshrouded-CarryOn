static const void* routingOriginal=nullptr;
static const Byte* routingExpectedItem=nullptr;
static unsigned routingCalls=0;
static bool routedAllowed[256]{};
static bool __fastcall routedDown(void* mapping,Byte action) {
 return read<Byte>(mapping,(size_t(action)+1)*0x118)!=0;
}
static void __fastcall routedAllow(void*,Byte action,bool allowed){routedAllowed[action]=allowed;}
static void __fastcall routedContext(void* mapping,void*,const void*) {
 transportActionAllowed(mapping,TransportModeAction,false);
 transportActionAllowed(mapping,TransportCancelAction,false);
 transportActionAllowed(mapping,10,false);
}
static void __fastcall routingEvaluate(void* mapping,void* state,const void* context) {
 CHECK(mapping==reinterpret_cast<void*>(3)&&state==reinterpret_cast<void*>(4));
 ++routingCalls;
 CHECK(read<const Byte*>(context,0x58)==routingExpectedItem);
 // Preserve every other pointer and all three native UI/blocking flags.
 for(size_t i=0;i<0x63;++i)if(i<0x58||i>=0x60)
  CHECK(static_cast<const Byte*>(context)[i]==static_cast<const Byte*>(routingOriginal)[i]);
 CHECK(!read<uint32_t>(moveFixture->actor,0xc00));
}
static void testTransportInputRouting() {
 originalTransportActionContext=&routingEvaluate;
 for(unsigned refusal=0;refusal<9;++refusal) {
  MoveFixture f;runtimeSetup(f);runtimeGrab(f);
  write(f.descriptor,0x30,static_cast<Byte*>(f.client));runtimeValidatePending(f);
  std::array<Byte,0x63> context{};context.fill(0xa5);
  write(context.data(),8,static_cast<const void*>(f.actor));
  write(context.data(),0x58,static_cast<const Byte*>(f.pincer));
  const auto before=context;routingOriginal=context.data();routingCalls=0;
  if(refusal==1)transportMove.cancel();
  if(refusal==2)transportMove.preview.updatedAt=GetTickCount64()-1000;
  if(refusal==3)f.focused=false;
  if(refusal==4)transportActive=false;
  if(refusal==5)transportMove.preview.actor=nullptr;
  if(refusal==6)transportEligible=false;
  if(refusal==7)transportChests=false;
  if(refusal==8)transportMove.preview.item=nullptr;
  routingExpectedItem=refusal?f.pincer:f.info;
  transportActionContext(reinterpret_cast<void*>(3),reinterpret_cast<void*>(4),context.data());
  CHECK(routingCalls==1&&context==before);
  CHECK(!f.writes&&!f.moves&&!f.modelMoves);
 }
 transportMove.cancel();transportActive=false;transportMoveFocused=&gameHasFocus;
 {
  MoveFixture f;runtimeSetup(f);
  const auto savedDown=originalActionDown;originalActionDown=&routedDown;
  originalTransportActionContext=&routedContext;originalTransportActionAllowed=&routedAllow;
  Bytes mapping(0x118*144);Byte context[0x63]{};context[0x60]=TransportInGame;
  transportUpdated=GetTickCount64();transportEligible=true;transportActive=false;
  transportTabKey.reset();transportCancelKey.reset();transportTabEvents=0;transportCancelEvents=0;
  auto tick=[&](bool mode,bool cancel) {
   write<Byte>(mapping.data(),(size_t(TransportModeAction)+1)*0x118,mode?1:0);
   write<Byte>(mapping.data(),(size_t(TransportCancelAction)+1)*0x118,cancel?1:0);
   transportActionContext(mapping.data(),nullptr,context);
  };
  tick(false,false);CHECK(routedAllowed[TransportModeAction]&&!routedAllowed[TransportCancelAction]&&!routedAllowed[10]);
  tick(true,false);CHECK(transportTabEvents==1&&!routedDown(mapping.data(),TransportModeAction));
  tick(true,false);CHECK(transportTabEvents==1); // A held mapped button never repeats.
  tick(false,false);tick(true,false);CHECK(transportTabEvents==2);
  transportActive=true;tick(false,true);
  CHECK(transportCancelEvents==1&&routedAllowed[TransportCancelAction]&&!routedDown(mapping.data(),TransportCancelAction));
  tick(false,false);context[0x60]=5;tick(true,false);
  CHECK(transportTabEvents==2&&!routedAllowed[TransportModeAction]&&routedDown(mapping.data(),TransportModeAction));
  context[0x60]=1;tick(true,false);CHECK(transportTabEvents==2); // Don't steal a button held in a menu.
  tick(false,false);context[0x62]=1;tick(true,false);CHECK(transportTabEvents==2&&!routedAllowed[TransportModeAction]);
  CHECK(!transportOwnedMapping&&!transportOwnCancel&&!f.writes&&!f.moves);
  originalActionDown=savedDown;originalTransportActionAllowed=nullptr;originalTransportActionContext=nullptr;
  transportActive=false;transportMoveFocused=&gameHasFocus;
 }
 std::puts("PASS: native action context substitution, immutable caller/actor, UI flags, stale/focus/actor/settings isolation; no world writes.");
}
