// Compile the adapter in the test translation unit to exercise the same hooks
// against fake native functions, without patching any executable.
#include "../src/native_content.cpp"
#include "../sdk/tests/catalog_checks.h"
#include <fstream>
#include <iterator>
#include <filesystem>
#include <cstdlib>
#include "../src/transport_move.h"
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
namespace carryon::native {
const Guid TransportCounterTemplate=guid("423c01619630d241b98a591808d9ea21");
constexpr uint32_t TransportCounterItem=0xf60b84c2;
static bool __fastcall mappedAction(void* mapping,Byte action){return static_cast<bool*>(mapping)[action];}
static bool __fastcall emptyCompletion(void*,void*){return false;}
// Emulate the native consumer, retaining the historical test record inspection.
static void testRequest(void* requester,Requests* requests) {
 originalPollCompletion=&emptyCompletion;
 request(requester,requests);
 Byte record[24]{};
 while(pollCompletion(record,requester)) {
  const auto head=read<uint32_t>(requester,0x80),last=read<uint32_t>(requester,0x50);
  std::memcpy(read<Byte*>(requester,0x48)+size_t(head)*24,record,24);
  write<uint32_t>(requester,0x80,head==last?0:head+1);
 }
}
#include "native_transport.inl"
#include "native_transport_hud.inl"
#include "native_transport_server.inl"
#include "native_transport_collision_probe.inl"
#include "native_transport_placement.inl"
#include "native_transport_snapshot.inl"
#include "native_transport_collision.inl"
#include "native_transport_cursor.inl"
#include "native_transport_runtime.inl"
#include "native_floor_placement.inl"
#include "native_transport_input_routing.inl"
#include "native_pincer_model.inl"
#include "native_pincer_textures.inl"
static size_t passed=0;
static void __fastcall fallback(void*,Requests* r){passed+=r->count;}
static std::vector<Requests> forwardedBatches;
static void* expectedRequester=nullptr;
static Requests* expectedWholeBatch=nullptr;
static void __fastcall captureBatch(void* requester,Requests* batch) {
 CHECK(requester==expectedRequester);
 if(expectedWholeBatch)CHECK(batch==expectedWholeBatch);
 forwardedBatches.push_back(*batch);
}
static void testRequestBatching() {
 originalRequest=&captureBatch;originalPollCompletion=&emptyCompletion;
 pixelHash={160000,1,2,3};
 const auto asset=std::find_if(pincerAssets.begin(),pincerAssets.end(),[](const auto& a){return a.key.type==0;});
 CHECK(asset!=pincerAssets.end());
 Byte requester[0x188]{};expectedRequester=requester;
 for(unsigned mask=0;mask<256;++mask) {
  Byte rows[8*0x48]{},outputs[8]{};
  unsigned privateCount=0;std::vector<Requests> expected;
  size_t start=0;
  for(size_t i=0;i<8;++i) {
   auto row=rows+i*0x48;row[0x30]=static_cast<Byte>(i);
   if(mask&(1u<<i)) {
    if(i>start)expected.push_back({rows+start*0x48,i-start});start=i+1;
    ++privateCount;
    std::memcpy(row+0x10,i%2?static_cast<const void*>(pixelHash.data()):static_cast<const void*>(asset->key.guid.data()),16);
    write(row,0,static_cast<void*>(outputs+i));write<uint64_t>(row,8,1);
   }
  }
  if(start<8)expected.push_back({rows+start*0x48,8-start});
  const Bytes before(rows,rows+sizeof(rows));
  Requests batch{rows,8};forwardedBatches.clear();
  expectedWholeBatch=mask==0?&batch:nullptr;
  write<LONG>(requester,0x180,privateCount);request(requester,&batch);
  CHECK(forwardedBatches.size()==expected.size());
  for(size_t i=0;i<expected.size();++i)
   CHECK(forwardedBatches[i].data==expected[i].data&&forwardedBatches[i].count==expected[i].count);
  CHECK(std::memcmp(rows,before.data(),sizeof(rows))==0);
  for(unsigned i=0;i<8;++i)if(mask&(1u<<i)) {
   Byte record[24]{};CHECK(pollCompletion(record,requester));
   CHECK(record[0]==0&&record[8]==i);
   CHECK(outputs[i]==(i%2?PincerPixels[0]:asset->data[0]));
  }
  Byte record[24]{};CHECK(!pollCompletion(record,requester));
  CHECK(read<LONG>(requester,0x180)==0&&privateCompletions.empty());
 }
 // Large native batch stays a single original call, not 128 notifications.
 Byte rows[128*0x48]{};Requests batch{rows,128};expectedWholeBatch=&batch;
 forwardedBatches.clear();request(requester,&batch);
 CHECK(forwardedBatches.size()==1&&forwardedBatches[0].count==128);
 Requests empty{nullptr,0};forwardedBatches.clear();expectedWholeBatch=nullptr;
 request(requester,&empty);CHECK(forwardedBatches.empty());
 originalRequest=&fallback;
 std::puts("PASS: 256 mixed batch patterns, native runs and order preserved, descriptor identity, icon/model completions exactly once, 128 native reads in one call.");
}
static uint64_t* __fastcall fallbackText(uint64_t* out,uint32_t id){out[0]=id;return out;}
static Bytes nativeIcon;
static uint32_t* __fastcall iconOpen(uint32_t* out,void*,const Key* key){CHECK(key->guid==RgbaIconBase.guid);*out=123;return out;}
static uint64_t* __fastcall iconResolve(uint64_t* out,void*,uint32_t handle){CHECK(handle==123);out[0]=reinterpret_cast<uint64_t>(nativeIcon.data());out[1]=nativeIcon.size();out[2]=0x100000000ull;return out;}
static void testWellTemplates(const Bytes& profile,const char* fixture) {
 const char* ids[]={
 "aa74e3024574c146b1c6d868b60b6802","c9295a17d533da4b95793b9ceb61dfb3","b0a4130c7c82ac41be48be2ecfef4326",
 "abc315119baca14c9a4bf92d9bfa04c2","4eb48327c3cc4645a51933c42bf99f8a","73602f249483e941bded2856d2eff2c6",
 "dc88bb73c435474f88760864655b8c2e","8408997f01e1db4990181d979f573c31","d9faf0ae739c894bbb862ca53a194f4d",
 "a19ff4e6ec76a843815e4c40567c9570","7204d7cb70d63743bb5a0ee18e120dd6"};
 for(const auto id:ids) {
  const auto path=std::filesystem::path(fixture).parent_path()/(std::string("resource-")+id+"-0x39768775-0.bin");
  std::ifstream stream(path,std::ios::binary);CHECK(stream.good());Bytes well{std::istreambuf_iterator<char>(stream),{}};
  CHECK(isWaterWellTemplate(well));const auto placed=makeFloorPlacement(profile,well,guid(id));CHECK(equal(placed,0x3c0,guid(id)));
  auto invalid=well;invalid[8]=8;CHECK(!isWaterWellTemplate(invalid));
  const auto components=array(well,16,12);
  for(uint32_t i=0;i<components.count;++i) {
   const size_t row=components.at+size_t(i)*12;
   if(u32(well,row)==typeHash("keen::ecs::Fuel")){invalid=well;put32(invalid,row,0);CHECK(!isWaterWellTemplate(invalid));}
   if(u32(well,row)==typeHash("keen::ecs::InteractionOfferRuntime")){
    invalid=well;put32(invalid,row+4+u32(well,row+4)+8,0);CHECK(!isWaterWellTemplate(invalid));
    invalid=well;put32(invalid,row,typeHash("keen::ecs::DoNotSave"));CHECK(!isWaterWellTemplate(invalid));
   }
  }
 }
 std::puts("PASS: eleven native water well appearances, placement geometry and semantic contract; unrelated interaction/missing fuel/DoNotSave refused.");
}
int run(const char* hammerFile,const char* menuFile,const char* iconFile,const char* counterFile,const char* bottlesFile,const char* doorFile) {
 std::ifstream counterStream(counterFile,std::ios::binary);
 Bytes counter{std::istreambuf_iterator<char>(counterStream),std::istreambuf_iterator<char>()};
 CHECK(counter.size()==2826&&u32(counter,0)==TransportCounterItem);
 CHECK(read<Guid>(counter.data(),0x3c0)==TransportCounterTemplate);
 CHECK(read<Guid>(counter.data(),4)==guid("e3a56abbb97f3946ad1fbd266c5ccace"));
 for(size_t axis=0;axis<3;++axis)CHECK(read<float>(counter.data(),0x3d0+axis*4)<read<float>(counter.data(),0x3dc+axis*4));
 testPincerModel();
 testPincerTextures();
 testTransport();
 testTransportHud();
 testTransportServer();
 testTransportCollisionProbe();
 testTransportPlacement();
 testTransportSnapshot();
 testTransportCollision();
 testTransportCursor();
 testTransportRuntime();
 testCounterRuntime();
 testSupportedDecorRuntime();
 testDoorRuntime();
 testWellRuntime();
 testTeleportRuntime();
 testTransportInterruptions();
 testClaimAndSyncRecovery();
 testPlacementHud();
 std::ifstream bottlesStream(bottlesFile,std::ios::binary);
 Bytes bottles{std::istreambuf_iterator<char>(bottlesStream),std::istreambuf_iterator<char>()};
 testFloorPlacement(counter,bottles);
 std::ifstream doorStream(doorFile,std::ios::binary);
 Bytes door{std::istreambuf_iterator<char>(doorStream),std::istreambuf_iterator<char>()};
 testDoorTemplate(counter,door);
 testWellTemplates(counter,doorFile);
 testQuantizedReplication();
 testPlacementCatalog();
 testObjectSettingsPolicy();
 testObjectInspector();
 testTransportInputRouting();
 { bool actions[256]{};originalActionDown=&mappedAction;
   transportPublishCommands(actions,GetTickCount64());
   actions[10]=true;transportPublishCommands(actions,GetTickCount64());CHECK(transportBoundAction(10)&&!transportBoundAction(14));
   actions[10]=false;actions[14]=true;transportPublishCommands(actions,GetTickCount64());CHECK(!transportBoundAction(10)&&transportBoundAction(14));
   CHECK(originalActionDown(actions,14)); // Unrelated action is untouched.
   transportPublishCommands(actions,GetTickCount64()-1000);CHECK(!transportBoundAction(14));
   originalActionDown=nullptr;transportCommands={};
 }
 std::ifstream iconStream(iconFile,std::ios::binary);
 nativeIcon={std::istreambuf_iterator<char>(iconStream),std::istreambuf_iterator<char>()};
 CHECK(nativeIcon.size()>=56);originalOpen=&iconOpen;originalResolve=&iconResolve;
 std::ifstream stream(hammerFile,std::ios::binary);
 Bytes hammer{std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
  std::ifstream menuStream(menuFile,std::ios::binary);
 Bytes menu{std::istreambuf_iterator<char>(menuStream),std::istreambuf_iterator<char>()};
 CHECK(menu.size()==244792);const Bytes originalMenu=menu;
 prepareMenu(menu.data());CHECK(writeMenu(menuReplacement));
 const auto relative=read<uint32_t>(menuField,0);const auto count=read<uint32_t>(menuField,4);
 const auto listPointer=static_cast<Byte*>(menuField)+relative;
 CHECK(listPointer==menuAllocation);bool found=false;
 for(uint32_t i=0;i<count;++i)if(read<uint32_t>(listPointer,i*4)==hash(Recipe)) {
  CHECK(i>=2);CHECK(read<uint32_t>(listPointer,(i-1)*4)==hash(WandRecipe));found=true;
 }
 CHECK(found);CHECK(writeMenu(menuOriginal));CHECK(menu==originalMenu);
 VirtualFree(menuAllocation,0,MEM_RELEASE);menuAllocation=nullptr;
 CHECK(hammer.size()==2232);auto pincer=makeItem(hammer);
 CHECK(pincer[item_layout::PlacementConsume]==0);
 CHECK(equal(pincer,item_layout::CursorPlacementSound,Guid{}));
 { auto invalid=hammer;invalid.pop_back();bool rejected=false;
   try{makeItem(invalid);}catch(const std::exception&){rejected=true;}CHECK(rejected); }
 CHECK(u32(pincer,0)==hash(Item));CHECK(equal(pincer,4,Item));CHECK(equal(pincer,1388,Icon));
 CHECK(equal(pincer,0x148,PincerVisualId)&&equal(pincer,0x4f8,PincerModelId));
 CHECK(!equal(hammer,0x148,PincerVisualId)&&!equal(hammer,0x4f8,PincerModelId));
 CHECK(equal(pincer,72,Name));CHECK(equal(pincer,104,Description));
 CHECK(hammer[204]==10 && pincer[204]==3);
 CHECK(equal(pincer,152,Guid{}) && equal(pincer,168,Guid{}));
 CHECK(equal(pincer,184,TransportEnter) && !equal(hammer,184,TransportEnter));
 CHECK(pincer[200]==129 && pincer[201]==1);
 CHECK(equal(pincer,1404,Guid{}));CHECK(equal(pincer,1420,Guid{}));
 CHECK(!equal(hammer,1420,Guid{}));
 // Native display dispatch tests bit 3 and substitutes the hammer when set.
 CHECK((u32(hammer,1496)&8)!=0);CHECK((u32(pincer,1496)&8)==0);
 CHECK((u32(pincer,1496)^u32(hammer,1496))==8);
 auto otherFlags=hammer;put32(otherFlags,1496,0x30009);
 CHECK(u32(makeItem(otherFlags),1496)==0x30001);
 CHECK(equal(pincer,312,Guid{}));CHECK(u32(pincer,1352)==0);
 CHECK(u32(hammer,0)==hash(Hammer));
  uint32_t explicitHandle=0,autoHandle=0;
 open(&explicitHandle,nullptr,&PincerIcon);open(&autoHandle,nullptr,&PincerAutoIcon);
 CHECK(explicitHandle!=0&&autoHandle!=0&&explicitHandle!=autoHandle);
 uint64_t iconView[3]{};resolve(iconView,nullptr,autoHandle);
 CHECK(iconView[0]&&iconView[1]>=56&&iconView[2]==0x100000000ull);
 Byte keyBytes[40]{};keyOf(keyBytes,nullptr,autoHandle);
 CHECK(std::memcmp(keyBytes+8,&PincerAutoIcon,32)==0);
 auto icon=makeIcon(nativeIcon);CHECK(u32(icon,40)==200&&u32(icon,44)==200&&icon[49]==0x25);
 CHECK(std::memcmp(icon.data()+16,nativeIcon.data()+16,16)==0);
 CHECK(std::memcmp(icon.data()+40,nativeIcon.data()+40,16)==0);
 auto invalidIcon=nativeIcon;invalidIcon[49]=0x91;bool rejected=false;
 try{makeIcon(invalidIcon);}catch(const std::exception&){rejected=true;}CHECK(rejected);
 static_assert(sizeof(PincerPixels)==160000);
  originalFormat=&fallbackText;uint64_t view[3]{};
 Byte pool[0xdc]{};
 const uint32_t offered[]={1,3,5,6,7,9,10,11,13,16,17,18,20,21,34};
 for(auto lang:offered) {
  write<uint32_t>(pool,0xd8,lang);
  for(const auto tag:{hash(Name),hash(Caption),hash(Description),hash(TransportEnter)}) {
   lookup(view,pool,tag);CHECK(view[0]==0&&view[1]&&view[2]);
   auto expected=text(tag,lang);CHECK(std::strcmp(reinterpret_cast<const char*>(view[1]),expected)==0);
   format(view,tag);CHECK(std::strcmp(reinterpret_cast<const char*>(view[0]),expected)==0);
  }
 }
 CHECK(std::strcmp(text(hash(Name),22),text(hash(Name),5))==0);
 CHECK(std::strcmp(text(hash(Name),8),text(hash(Name),7))==0);
 CHECK(std::strcmp(text(hash(Name),9999),text(hash(Name),1))==0);
 CHECK(text(0,5)==nullptr);
 language.store(5);
 format(view,hash(Name));CHECK(std::strcmp(reinterpret_cast<const char*>(view[0]),"Pince de transport")==0);
 format(view,123);CHECK(view[0]==123);
 originalRequest=&fallback;pixelHash={160000,1,2,3};
 Byte requester[0x188]{};Byte records[24*4]{};
 write(requester,0x48,static_cast<Byte*>(records));write<uint32_t>(requester,0x50,3);write<uint32_t>(requester,0xc0,3);
 Byte nativeRequests[0x48*3]{};Byte output[20]{};
 std::memcpy(nativeRequests+0x48+0x10,pixelHash.data(),16);
 write(nativeRequests+0x48,0,static_cast<void*>(output));write<uint64_t>(nativeRequests+0x48,8,20);
 write<uint32_t>(nativeRequests+0x48,0x40,12);nativeRequests[0x48+0x30]=42;
 Requests r{nativeRequests,3};testRequest(requester,&r);
 CHECK(passed==2);CHECK(std::memcmp(output,PincerPixels+12,20)==0);
 CHECK(read<uint32_t>(requester,0x80)==1&&records[0]==0&&records[8]==42);
 write<uint64_t>(nativeRequests+0x48,8,163841);Requests invalid{nativeRequests+0x48,1};testRequest(requester,&invalid);
 CHECK(read<uint32_t>(requester,0x80)==2&&records[24]==0x1f);
 // Full top mip: verify every pixel and both buffer canaries, not just 20 bytes.
 Bytes full(sizeof(PincerPixels)+2,0xcd);
 write(nativeRequests+0x48,0,static_cast<void*>(full.data()+1));
 write<uint64_t>(nativeRequests+0x48,8,sizeof(PincerPixels));
 write<uint32_t>(nativeRequests+0x48,0x40,0);
 write<uint32_t>(requester,0xc0,0);testRequest(requester,&invalid);
 CHECK(records[48]==0);CHECK(full.front()==0xcd&&full.back()==0xcd);
 CHECK(std::memcmp(full.data()+1,PincerPixels,sizeof(PincerPixels))==0);
 // Regression from the real game log: disk-aligned request, NOT image size.
 Bytes aligned(163840+2,0xcd);write(nativeRequests+0x48,0,static_cast<void*>(aligned.data()+1));
 write<uint64_t>(nativeRequests+0x48,8,163840);write<uint32_t>(requester,0xc0,1);
 testRequest(requester,&invalid);CHECK(records[72]==0);
 CHECK(aligned.front()==0xcd&&aligned.back()==0xcd);
 CHECK(std::memcmp(aligned.data()+1,PincerPixels,sizeof(PincerPixels))==0);
 for(size_t i=160000;i<163840;++i)CHECK(aligned[i+1]==0);
 // A partial read spanning the end of the pixels must zero only its tail.
 Byte tail[10];std::memset(tail,0xcd,sizeof(tail));
 write(nativeRequests+0x48,0,static_cast<void*>(tail+1));write<uint64_t>(nativeRequests+0x48,8,8);
 write<uint32_t>(nativeRequests+0x48,0x40,159996);write<uint32_t>(requester,0xc0,2);
 testRequest(requester,&invalid);CHECK(records[0]==0&&tail[0]==0xcd&&tail[9]==0xcd);
 CHECK(std::memcmp(tail+1,PincerPixels+159996,4)==0);
 for(size_t i=5;i<9;++i)CHECK(tail[i]==0);
 // Private completions do not depend on producer ring availability and keep
 // requester identities isolated. Every accepted read decrements exactly once.
 Byte second[0x188]{};write<LONG>(requester,0x180,64);write<LONG>(second,0x180,1);
 for(unsigned i=0;i<64;++i){nativeRequests[0x48+0x30]=static_cast<Byte>(i);request(requester,&invalid);}
 request(second,&invalid);CHECK(privateCompletions.size()==2&&privateCompletions.at(requester).ready.size()==64);
 Byte completion[24]{};
 CHECK(pollCompletion(completion,second)&&read<LONG>(second,0x180)==0);
 for(unsigned i=0;i<64;++i){CHECK(pollCompletion(completion,requester));CHECK(completion[8]==i);}
 CHECK(read<LONG>(requester,0x180)==0&&privateCompletions.empty());
 CHECK(!pollCompletion(completion,requester)&&read<LONG>(requester,0x180)==0);
 testRequestBatching();
 std::puts("PASS: item identity, icon format, localized name, texture range bounds, mixed native requests and completion records.");return 0;
}
}
int main(int argc,char**argv){CHECK(argc==7);return carryon::native::run(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6]);}




