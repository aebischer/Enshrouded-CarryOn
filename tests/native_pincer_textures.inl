static unsigned textureAdds=0,textureAllocations=0,textureBindings=0,textureLookups=0;

static bool textureAllocationFails=false;

static void __fastcall fakeTextureRegister(void* manager,const Byte* texture) {

 ++textureAdds;

 auto count=read<uint32_t>(manager,0xfdc);

 auto hash=read<Guid>(texture,8);

 const auto capacity=read<uint64_t>(manager,0xf98);

 auto bits=read<uint64_t*>(manager,0xf90);

 auto keys=read<Guid*>(manager,0xfb0);

 auto values=read<uint16_t*>(manager,0xfc8);

 auto slot=uint64_t(read<uint32_t>(hash.data(),4))&(capacity-1);

 while(bits[slot/64]&(uint64_t(1)<<(slot%64)))slot=(slot+1)&(capacity-1);

 bits[slot/64]|=uint64_t(1)<<(slot%64);keys[slot]=hash;values[slot]=static_cast<uint16_t>(count);

 auto record=read<Byte*>(manager,0xfe8)+size_t(count)*72;

 write(record,0x30,reinterpret_cast<void*>(uintptr_t(0x1000+count)));

 write<uint32_t>(manager,0xfdc,count+1);

}

static void* __fastcall fakeTextureRequirements(void* out,void*,void*) {

 write<uint64_t>(out,0,8192);write<uint64_t>(out,8,4096);

 write<uint64_t>(out,16,0x123456789abcdef0ull); // La structure native fait 24 octets.

 return out;

}

static void* __fastcall fakeTextureAllocate(void*,const void* descriptor) {

 ++textureAllocations;CHECK(read<uint64_t>(descriptor,8)==8192);

 CHECK(read<uint16_t>(descriptor,0x38)==1&&read<Byte>(descriptor,0)==7);

 return textureAllocationFails?nullptr:reinterpret_cast<void*>(uintptr_t(0x2000+textureAllocations));

}

static void __fastcall fakeTextureBind(void*,const void* span) {

 ++textureBindings;CHECK(read<uint64_t>(span,8)==1);

 auto binding=read<const Byte*>(span,0);CHECK(read<void*>(binding,0)&&read<void*>(binding,8));

 CHECK(read<uint64_t>(binding,16)==0&&read<uint64_t>(binding,24)==8192);

}

static uint32_t* __fastcall fakeTextureLookup(uint32_t* out,void* manager,const Byte* texture,uint32_t flags) {

 ++textureLookups;CHECK(flags==3);

 if(privateTexture(read<Guid>(texture,8))) {

  uint16_t index=0;CHECK(textureIndex(manager,read<Guid>(texture,8),index));

  // Aucun accès du moteur avant l'allocation ET la liaison GPU.

  CHECK(textureBindings==textureAdds);

  *out=0x10000u|index;

 }else *out=123;

 return out;

}

static void testPincerTextures() {

 loadPincerAssets();

 registerTexture=&fakeTextureRegister;textureRequirements=&fakeTextureRequirements;

 allocateTextureMemory=&fakeTextureAllocate;bindTextureMemory=&fakeTextureBind;

 originalTextureLookup=&fakeTextureLookup;

 Byte manager[0x1410]{},records[16*72]{};

 uint64_t bits[1]{},pending[1]{};Guid keys[32]{};uint16_t values[32]{};

 write(manager,0xf90,bits+0);write<uint64_t>(manager,0xf98,32);write(manager,0xfb0,keys+0);

 write(manager,0xfc8,values+0);write<uint32_t>(manager,0xfdc,1);

 write<uint32_t>(manager,0xfd8,24);

 write(manager,0xfe8,records+0);write<uint64_t>(manager,0xff0,16);

 write(manager,0xff8,pending+0);write<uint64_t>(manager,0x1000,16);

 write<uint8_t>(manager,0x1020,1);write<uint64_t>(manager,0x1378,16);

 write(manager,0x10,reinterpret_cast<void*>(uintptr_t(1)));write<Byte>(manager,0x178,7);

 pincerTextureMemory.clear();

 const auto material=pincerAsset(PincerMaterialId,0xaee1894b);

 const auto images=material->data+16+read<int32_t>(material->data,16);

 uint32_t handle=0;

 // Cas reproduisant le crash : pince déjà équipée, catalogue natif initialisé,

 // aucune de ses quatre textures encore enregistrée.

 for(unsigned i=0;i<4;++i) {

  const auto texture=images+i*96+8;

  CHECK(privateTexture(read<Guid>(texture,8))==texture);

  pincerTextureLookup(&handle,manager,texture,3);CHECK(handle==(0x10001u+i));

  pincerTextureLookup(&handle,manager,texture,3);CHECK(handle==(0x10001u+i));

 }

 CHECK(textureAdds==4&&textureAllocations==4&&textureBindings==4&&textureLookups==8);

 CHECK(read<uint32_t>(manager,0x1010)==4&&pending[0]==0x1e);

 // A recreated manager re-registers and binds the texture instead of staying poisoned.

 bits[0]=0;pending[0]=0;write<uint32_t>(manager,0xfdc,1);

 pincerTextureLookup(&handle,manager,images+8,3);CHECK(handle==0x10001u&&textureLookups==9);

 CHECK(textureAdds==5&&textureBindings==5);

 // Continue the capacity/failure fixture from its original baseline.

 textureAdds=4;textureBindings=4;textureLookups=8;write<uint32_t>(manager,0xfdc,5);

 Byte unrelated[88]{};pincerTextureLookup(&handle,manager,unrelated,3);CHECK(handle==123);

 // Pas d'assertion ni d'appel d'enregistrement si les réserves sont épuisées.

 pincerTextureMemory.clear();bits[0]=0;write<uint64_t>(manager,0xff0,5);

 pincerTextureLookup(&handle,manager,images+8,3);CHECK(handle==0&&textureAdds==4);

 write<uint64_t>(manager,0xff0,16);write<uint32_t>(manager,0xfd8,5);

 pincerTextureLookup(&handle,manager,images+8,3);CHECK(handle==0&&textureAdds==4);

 write<uint32_t>(manager,0xfd8,24);write<uint64_t>(manager,0x1370,16);

 pincerTextureLookup(&handle,manager,images+8,3);CHECK(handle==0&&textureAdds==4);

 write<uint64_t>(manager,0x1370,0);textureAllocationFails=true;

 pincerTextureLookup(&handle,manager,images+8,3);CHECK(handle==0&&textureAdds==5&&textureBindings==4);

 pincerTextureLookup(&handle,manager,images+8,3);CHECK(handle==0&&textureAdds==5&&textureLookups==9);

 textureAllocationFails=false;

 pincerTextureLookup(&handle,manager,images+8,3);

 CHECK(handle==0x10005u&&textureAdds==5&&textureBindings==5&&textureLookups==10);

 pincerTextureLookup(&handle,manager,images+8,3);

 CHECK(handle==0x10005u&&textureAdds==5&&textureBindings==5&&textureLookups==11);

 pincerTextureMemory.clear();

 std::puts("PASS: equipped-at-entry private textures registered before lookup, GPU bind ordering, repeated use, native passthrough, capacity and allocation failures.");

}

