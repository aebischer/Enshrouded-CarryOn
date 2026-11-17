static Byte cursorItemInfo[4]{},cursorActor[0xc38]{};
static unsigned cursorSequence=0;
static uint32_t cursorRefusal=0;
static const Byte* __fastcall cursorLookup(uint32_t item,void*) {CHECK(item==42);return cursorItemInfo;}
static void __fastcall cursorConfig(void* config,void* context,const void* item,const void* effective,bool alternate,bool primary) {
 CHECK(cursorSequence++==0&&item==cursorItemInfo&&effective==item&&!alternate&&primary);
 const auto actor=read<const Byte*>(context,0x108);CHECK(actor&&actor!=cursorActor);
 CHECK((read<uint32_t>(actor,0xbc8)&(1u<<24))&&(read<uint32_t>(actor,0xc00)&(1u<<24)));
 CHECK(!read<uint32_t>(cursorActor,0xbc8)&&!read<uint32_t>(cursorActor,0xc00));
 CHECK(read<Byte>(config,0x20)==0x1f&&read<uint64_t>(config,0x40)==0xff0201);
 write(config,0x60,item);write<Byte>(config,0x47,1);
}
static void __fastcall cursorCastConfig(void*,void*,const void* config) {CHECK(cursorSequence++==1&&!read<Byte>(config,0x47));}
static void __fastcall cursorCast(void* hit,void*,const void*,const void*) {
 CHECK(cursorSequence++==2);write<Byte>(hit,0x24,0xa7);write<Byte>(hit,0x28,1);
}
static void __fastcall cursorSolve(uint32_t* flags,void* transform,bool*,bool*,float*,void*,const void*,const void*) {
 CHECK(cursorSequence++==3&&*flags==1);*flags=cursorRefusal;
 write<int64_t>(transform,0,int64_t(4)<<32);
 write<float>(transform,40,2.0f); // Adapter must preserve the real chest scale.
}
static void __fastcall cursorValidate(uint32_t* flags,void*,const void*,const void* transform,Byte permissions,Byte special,float delta) {
 CHECK(cursorSequence++==4&&permissions==0x9d&&special==0xa7&&delta==0.25f);
 CHECK(read<float>(transform,40)==1.0f);*flags|=(1u<<12)|(1u<<13);
}
static void testTransportCursor() {
 transportItemLookup=&cursorLookup;transportCursorConfig=&cursorConfig;
 transportCastConfig=&cursorCastConfig;transportCast=&cursorCast;transportSolve=&cursorSolve;
 originalTransportValidate=&cursorValidate;write<uint32_t>(cursorItemInfo,0,42);
 Byte context[0x6a0]{},registry{};
 write(context,0x108,static_cast<Byte*>(cursorActor));write(context,0x1b0,&registry);
 const auto originalContext=read<std::array<Byte,0x6a0>>(context,0);
 transport::Placement original{};
 auto sample=transportCalculatePlacement(context,42,original,0x9d,0.25f);
 CHECK(cursorSequence==5&&sample.calculated&&!sample.flags&&sample.placement.position[0]==int64_t(4)<<32);
 CHECK(sample.placement.scale==original.scale);
 CHECK((read<std::array<Byte,0x6a0>>(context,0)==originalContext));
 for(const auto byte:cursorActor)CHECK(byte==0);
 cursorSequence=0;cursorRefusal=(1u<<8)|(1u<<14);
 sample=transportCalculatePlacement(context,42,original,0x9d,0.25f);
 CHECK(sample.flags==cursorRefusal); // Never bypass occupied area or permission refusal.
 cursorSequence=0;original.rotation={0,0,0,0};
 CHECK(!transportCalculatePlacement(context,42,original,0x9d,0.25f).calculated&&!cursorSequence);
 std::puts("PASS: isolated native placement query, actor preservation, permission mask and refusal flags.");
}
