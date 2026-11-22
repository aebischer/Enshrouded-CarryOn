#include "native_content.h"
#include "content_catalog.h"
#include "MinHook.h"
#include "pincer_pixels.h"
#include "enshrouded_i18n.h"
#include "carryon_texts.h"
#include "transport_selection.h"
#include "transport_move.h"
#include "floor_placement.h"
#include "rest_furniture.h"
#include "rest_placement.h"
#include <fstream>
#include <map>
#include <set>
#include <Windows.h>
#include <intrin.h>
#include <bcrypt.h>
#include <atomic>
#include <mutex>
#include <memory>
#include <algorithm>
#include <list>
#include <cstdio>
#pragma comment(lib,"bcrypt.lib")
#pragma comment(lib,"user32.lib")

namespace carryon::native {
namespace {
using namespace content;
using Byte=unsigned char;
#include "native_helpers.inl"
template<class T> T read(const void* p,size_t at) {T v;std::memcpy(&v,static_cast<const Byte*>(p)+at,sizeof(v));return v;}
template<class T> void write(void* p,size_t at,const T& v) {std::memcpy(static_cast<Byte*>(p)+at,&v,sizeof(v));}
struct Key {Guid guid;uint32_t type,part;uint64_t reserved;};
static_assert(sizeof(Key)==32);
#include "native_pincer_model.inl"
constexpr uint32_t ItemType=0xb5ce8765,IconType=0xf9e86997;
const Guid Name=ItemName;
const Guid Description=guid("32e8a124d1a81740a8420099894dd561");
const Guid Caption=guid("68c7dfacb1ad5143b6e70b3ff53a1ad4");
const Guid TransportEnter=guid("1a5e4704cd9cba4f837d62b12c0e9b65");
std::atomic<uint32_t> language{1};
const Key ItemRegistry{guid("5dd364304273ca40bdc9aad58f83bf45"),0x72ab39b6,0,0};
const Key RecipeRegistry{guid("0fde575e0c2859458191f02c1ba656e2"),0x5f5382cf,0,0};
const Key UiBundle{guid("f64de1468ffb664f9463e23b9e19e48e"),0xfef918b8,0,0};
const Key Pincer{Item,ItemType,0,0},PincerIcon{Icon,IconType,0,0};
// The item grid resolves an automatically generated icon with the item's GUID;
// detail panels may use the explicit ItemInfo icon reference instead.
const Key PincerAutoIcon{Item,IconType,0,0};
const Key HammerKey{Hammer,ItemType,0,0};
// Native summoning-tool image: 200x200 RGBA8, one mip, non-tiled.
const Key RgbaIconBase{guid("ac11e027e3891e4b8aa6d69aedac1a2d"),IconType,0,0};
constexpr uint32_t FirstHandle=0xfca10001;
enum class ResourceKind {ItemRegistry,RecipeRegistry,Menu,Item,Icon,Embedded};
struct Entry {
 Key key,base;ResourceKind kind;Bytes bytes;std::mutex mutex;std::atomic<bool> failed{false};
 Entry(Key own,Key donor,ResourceKind type):key(own),base(donor),kind(type){}
};
// Keep handle order stable; resource construction depends only on its kind.
Entry entries[]={
 {ItemRegistry,ItemRegistry,ResourceKind::ItemRegistry},
 {RecipeRegistry,RecipeRegistry,ResourceKind::RecipeRegistry},
 {UiBundle,UiBundle,ResourceKind::Menu},
 {Pincer,HammerKey,ResourceKind::Item},
 {PincerIcon,RgbaIconBase,ResourceKind::Icon},
 {PincerAutoIcon,RgbaIconBase,ResourceKind::Icon},
 {{PincerModelId,0x177394c7,0,0},{},ResourceKind::Embedded},
 {{PincerMaterialId,0xaee1894b,0,0},{},ResourceKind::Embedded},
 {{PincerVisualId,0x39768775,0,0},{},ResourceKind::Embedded}
};
uintptr_t base=0;
void(__cdecl* logger)(const char*)=nullptr;
std::mutex installMutex;
bool installed=false,restartRequired=false;
void log(const char* s) {if(logger)logger(s);}
// Runtime adapter is defined below, after resource helpers.
bool transportModeActive();
using Open=uint32_t*(__fastcall*)(uint32_t*,void*,const Key*);
using Resolve=uint64_t*(__fastcall*)(uint64_t*,void*,uint32_t);
using KeyOf=Byte*(__fastcall*)(Byte*,void*,uint32_t);
using Lookup=uint64_t*(__fastcall*)(uint64_t*,void*,uint32_t);
using Format=uint64_t*(__fastcall*)(uint64_t*,uint32_t);
struct Keys {uint64_t status;Key* data;uint64_t count;};
using List=Keys*(__fastcall*)(Keys*,void*,uint32_t,uint32_t*);
using Fill=void(__fastcall*)(void*);
struct Requests {Byte* data;size_t count;};
using Request=void(__fastcall*)(void*,Requests*);
Open originalOpen=nullptr;Resolve originalResolve=nullptr;KeyOf originalKey=nullptr;
Lookup originalLookup=nullptr;Format originalFormat=nullptr;
List originalList=nullptr;Fill originalFill=nullptr;Request originalRequest=nullptr;
std::vector<Key> iconKeys;
std::array<std::vector<Key>,3> visualKeys;
std::mutex keysMutex;
std::atomic<bool> relisted{false};
std::atomic<void*> transportResourceDb{nullptr};
std::array<uint32_t,4> pixelHash{};
Bytes cachedUi;
void* menuAllocation=nullptr;
void* menuField=nullptr;
uint64_t menuOriginal=0;
uint64_t menuReplacement=0;
void prepareMenu(Byte* ui) {
 Bytes sourceUi(ui,ui+244792);
 cachedUi=add_crafting_menu(sourceUi);
 size_t field=SIZE_MAX,last=0;
 for(size_t i=0;i<sourceUi.size();++i)if(sourceUi[i]!=cachedUi[i]) {
  if(field==SIZE_MAX)field=i&~size_t(3);last=i;
 }
 if(field==SIZE_MAX||last>=field+8)throw std::runtime_error("unexpected menu patch layout");
 const auto added=array(cachedUi,field,4);
 if(added.at<sourceUi.size())throw std::runtime_error("menu entries were not appended");
 menuField=ui+field;menuOriginal=read<uint64_t>(menuField,0);
 SYSTEM_INFO info{};GetSystemInfo(&info);const uintptr_t gran=info.dwAllocationGranularity;
 const uintptr_t address=reinterpret_cast<uintptr_t>(menuField);
 const size_t bytes=static_cast<size_t>(added.count)*4;
 for(uintptr_t candidate=(address+gran)&~(gran-1);candidate-address<INT32_MAX-gran;candidate+=gran) {
  MEMORY_BASIC_INFORMATION region{};
  if(!VirtualQuery(reinterpret_cast<void*>(candidate),&region,sizeof(region)))break;
  if(region.State!=MEM_FREE) {
   const uintptr_t end=reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize;
   if(end>candidate+gran)candidate=((end+gran-1)&~(gran-1))-gran;
   continue;
  }
  menuAllocation=VirtualAlloc(reinterpret_cast<void*>(candidate),bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
  if(menuAllocation)break;
 }
 if(!menuAllocation)throw std::runtime_error("no nearby memory for crafting entries");
 std::memcpy(menuAllocation,cachedUi.data()+added.at,bytes);
 const uint32_t offset=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(menuAllocation)-address);
 menuReplacement=static_cast<uint64_t>(added.count)<<32|offset;
}
bool writeMenu(uint64_t value) {
 DWORD old=0;
 if(!VirtualProtect(menuField,8,PAGE_READWRITE,&old))return false;
 write(menuField,0,value);DWORD ignored=0;
 return VirtualProtect(menuField,8,old,&ignored)!=0;
}

bool compatible() {
 std::vector<Byte> block(65536);
 std::vector<wchar_t> path(32768);const DWORD n=GetModuleFileNameW(nullptr,path.data(),32768);
 if(!n||n>=32768)return false;
 HANDLE file=CreateFileW(path.data(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
 if(file==INVALID_HANDLE_VALUE)return false;
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE digest=nullptr;
 bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(algorithm,&digest,nullptr,0,nullptr,0,0)>=0;
 DWORD count=0;
 while(ok) {if(!ReadFile(file,block.data(),static_cast<DWORD>(block.size()),&count,nullptr)){ok=false;break;}
  if(!count)break;ok=BCryptHashData(digest,block.data(),count,0)>=0;}
 Byte result[32]{};if(ok)ok=BCryptFinishHash(digest,result,32,0)>=0;
 if(digest)BCryptDestroyHash(digest);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(file);
 constexpr Byte expected[]={0x5d,0x9e,0xf2,0xf1,0x40,0xb8,0xed,0x49,0xe3,0x0d,0xbe,0xe6,0x55,0x84,0x1b,0x89,0xd0,0x8b,0xae,0xc2,0x0f,0x79,0x9a,0x18,0x17,0xe1,0x71,0xa1,0x8a,0x85,0x0c,0x77};
 return ok&&std::memcmp(result,expected,32)==0;
}
struct ResourceUnavailable:std::runtime_error {using std::runtime_error::runtime_error;};
Bytes source(void* db,const Key& key) {
 uint32_t handle=0;originalOpen(&handle,db,&key);
 if(!handle)throw ResourceUnavailable("base resource missing");
 uint64_t view[3]{};originalResolve(view,db,handle);
 if(!view[0]||!view[1]||view[1]>16*1024*1024)throw ResourceUnavailable("invalid base resource");
 auto p=reinterpret_cast<const Byte*>(view[0]);return Bytes(p,p+static_cast<size_t>(view[1]));
}
#include "pincer_item.inl"
Bytes makeIcon(const Bytes& sourceIcon) {
 range(sourceIcon,0,56);
 if(u32(sourceIcon,40)!=200||u32(sourceIcon,44)!=200||sourceIcon[48]!=1||
    sourceIcon[49]!=0x25||sourceIcon[50]!=1||sourceIcon[51]!=0)
  throw std::runtime_error("unexpected native RGBA icon layout");
 // Preserve the native content category, bundle/sort metadata and texture flags.
 Bytes out=sourceIcon;std::memcpy(out.data(),pixelHash.data(),16);
 put32(out,40,200);put32(out,44,200);out[48]=1;out[49]=0x25;out[50]=1;
 string(out,32,"CarryOn_TransportPincer");return out;
}
const Bytes* build(size_t i,void* db) noexcept {
 try {
  auto& e=entries[i];std::lock_guard<std::mutex> lock(e.mutex);
  if(e.failed)return nullptr;if(!e.bytes.empty())return &e.bytes;
  switch(e.kind) {
  case ResourceKind::Embedded: {
   const auto asset=pincerAsset(e.key.guid,e.key.type);
   if(!asset)throw std::runtime_error("pincer visual resource missing");
   e.bytes.assign(asset->data,asset->data+asset->size);break;
  }
  case ResourceKind::Icon:e.bytes=makeIcon(source(db,e.base));break;
  case ResourceKind::Menu:e.bytes=cachedUi;break;
  case ResourceKind::ItemRegistry:e.bytes=add_item_registry(source(db,e.base));break;
  case ResourceKind::RecipeRegistry:e.bytes=add_recipe_registry(source(db,e.base));break;
  case ResourceKind::Item:e.bytes=makeItem(source(db,e.base));break;
  }
  char message[128];sprintf_s(message,"CarryOn: native resource %zu ready (%zu bytes)",i,e.bytes.size());log(message);
  return &e.bytes;
 }catch(const ResourceUnavailable&){return nullptr;}
 catch(const std::bad_alloc&){return nullptr;}
 catch(const std::exception& e){entries[i].failed=true;log(e.what());return nullptr;}
 catch(...){entries[i].failed=true;log("CarryOn: resource construction failed");return nullptr;}
}
uint32_t* __fastcall open(uint32_t* out,void* db,const Key* key) {
 if(key)for(size_t i=0;i<std::size(entries);++i)
  if(key->guid==entries[i].key.guid&&key->type==entries[i].key.type&&key->part==0) {
   if(i==0)transportResourceDb.store(db,std::memory_order_release);
   *out=build(i,db)?FirstHandle+static_cast<uint32_t>(i):0;return out;
  }
 return originalOpen(out,db,key);
}
uint64_t* __fastcall resolve(uint64_t* out,void* db,uint32_t handle) {
 if(handle>=FirstHandle&&handle<FirstHandle+std::size(entries)) {
  const auto b=build(handle-FirstHandle,db);out[0]=b?reinterpret_cast<uint64_t>(b->data()):0;
  out[1]=b?b->size():0;out[2]=b?0x100000000ull:0;return out;
 }
 return originalResolve(out,db,handle);
}
Byte* __fastcall keyOf(Byte* out,void* db,uint32_t handle) {
 if(handle>=FirstHandle&&handle<FirstHandle+std::size(entries)) {
  std::memset(out,0,40);std::memcpy(out+8,&entries[handle-FirstHandle].key,32);return out;
 }
 return originalKey(out,db,handle);
}
const char* text(uint32_t id,uint32_t selectedLanguage) {
 const char* key=nullptr;
 if(id==hash(Name))key="item.name";
 if(id==hash(DecorPlacementName))key="setting.move_objects";
 if(id==hash(Caption))key="item.caption";
 if(id==hash(Description))key="item.description";
 if(id==hash(TransportEnter))key=transportModeActive()?"transport.exit":"transport.enter";
 if(key)return enshrouded::i18n::find(CarryOnTexts,key,selectedLanguage);
 return nullptr;
}
uint64_t* __fastcall lookup(uint64_t* out,void* pool,uint32_t id) {
 // Verified in this client's lookup: current language at pool+0xd8.
 const auto current=pool?read<uint32_t>(pool,0xd8):1u;
 language.store(current,std::memory_order_relaxed);
 if(const char* s=text(id,current)){out[0]=0;out[1]=reinterpret_cast<uint64_t>(s);out[2]=std::strlen(s);return out;}
 return originalLookup(out,pool,id);
}
uint64_t* __fastcall format(uint64_t* out,uint32_t id) {
 if(const char* s=text(id,language.load(std::memory_order_relaxed))) {
  const auto h=nativeTextHash(s);
  out[0]=reinterpret_cast<uint64_t>(s);out[1]=std::strlen(s);out[2]=h;return out;
 }
 return originalFormat(out,id);
}
Keys* __fastcall list(Keys* out,void* db,uint32_t type,uint32_t* found) {
 auto result=originalList(out,db,type,found);
 const uint32_t visualTypes[]={0x177394c7,0xaee1894b,0x39768775};
 size_t visualIndex=std::size(visualTypes);
 for(size_t i=0;i<std::size(visualTypes);++i)if(type==visualTypes[i])visualIndex=i;
 if((type!=IconType&&visualIndex==std::size(visualTypes))||static_cast<Byte>(out->status)!=0)return result;
 try {
  std::lock_guard<std::mutex> lock(keysMutex);
  auto& keys=type==IconType?iconKeys:visualKeys[visualIndex];
  if(keys.empty()) {
   if(out->count>1000000||(out->count&&!out->data))return result;
   std::vector<Key> candidate;
   if(out->count)candidate.assign(out->data,out->data+out->count);
   for(const auto& entry:entries)if(entry.key.type==type) {
    for(const auto& key:candidate)if(key.guid==entry.key.guid&&key.part==entry.key.part)
     throw std::runtime_error("custom resource GUID collision");
    candidate.push_back(entry.key);
   }
   keys=std::move(candidate);
   if(type!=IconType) {
    char message[128];sprintf_s(message,"CarryOn: private visual resource registered in catalog type=%08x",type);log(message);
   }
  }
  out->data=keys.data();out->count=keys.size();
 }catch(...){log("CarryOn: unable to list custom resources");}
 return result;
}
void __fastcall fill(void* registry) {
 if(!relisted.exchange(true)) {write<Byte>(registry,0x36c0,0);log("CarryOn: requesting native icon registry refresh");}
 originalFill(registry);
}
// Private completions use the native poll boundary and reservation counter.
// No writes to the engine's SPSC producer ring from an additional producer.
struct PrivateCompletion {std::array<Byte,24> record;};
struct PrivateCompletionQueue {std::list<PrivateCompletion> ready;size_t preparing=0;};
std::mutex completionMutex;
std::map<void*,PrivateCompletionQueue> privateCompletions;
std::list<PrivateCompletion> freeCompletions;
std::list<PrivateCompletion> preparingCompletions;
constexpr size_t CompletionPoolLimit=64;
using PollCompletion=bool(__fastcall*)(void*,void*);
PollCompletion originalPollCompletion=nullptr;
bool __fastcall pollCompletion(void* out,void* requester) {
 {
  std::lock_guard<std::mutex> lock(completionMutex);
  const auto it=privateCompletions.find(requester);
  if(it!=privateCompletions.end()&&!it->second.ready.empty()) {
   auto& queue=it->second;
   std::memcpy(out,queue.ready.front().record.data(),24);
   if(freeCompletions.size()<CompletionPoolLimit)freeCompletions.splice(freeCompletions.end(),queue.ready,queue.ready.begin());
   else queue.ready.pop_front();
   if(queue.ready.empty()&&!queue.preparing)privateCompletions.erase(it);
   InterlockedDecrement(reinterpret_cast<volatile LONG*>(static_cast<Byte*>(requester)+0x180));
   return true;
  }
 }
 return originalPollCompletion(out,requester);
}
void __fastcall request(void* requester,Requests* requests) {
 // Keep native batches intact. A private payload only splits its surrounding
 // contiguous native runs; no descriptor copies, reordering or allocation.
 // The game serializes producers on this entry point.
 size_t nativeStart=0;
 for(size_t i=0;i<requests->count;++i) {
  Byte* r=requests->data+i*0x48;
  const bool icon=std::memcmp(r+0x10,pixelHash.data(),16)==0;
  const auto asset=icon?nullptr:pincerAsset(read<Guid>(r,0x10),0);
  if(!icon&&!asset)continue;
  if(i>nativeStart) {
   Requests native{requests->data+nativeStart*0x48,i-nativeStart};
   originalRequest(requester,&native);
  }
  nativeStart=i+1;
  const Byte* payload=icon?PincerPixels:asset->data;
  const size_t payloadSize=icon?sizeof(PincerPixels):asset->size;
  bool published=false;
  try {
   // Reserve the result node and requester queue BEFORE writing any payload.
   // Empty queues with a producer in progress cannot be erased by the consumer.
   PrivateCompletionQueue* queue=nullptr;
   std::list<PrivateCompletion>::iterator prepared;
   {
    std::lock_guard<std::mutex> completionLock(completionMutex);
    queue=&privateCompletions[requester];
    try {
     if(freeCompletions.empty())preparingCompletions.push_back({{}});
     else preparingCompletions.splice(preparingCompletions.end(),freeCompletions,freeCompletions.begin());
     prepared=std::prev(preparingCompletions.end());++queue->preparing;
    }catch(...) {
     if(queue->ready.empty()&&!queue->preparing)privateCompletions.erase(requester);
     throw;
    }
   }
   prepared->record={};
   auto wanted=read<uint64_t>(r,8);auto offset=read<uint32_t>(r,0x40);auto buffer=read<void*>(r,0);
   Byte status=0;
   // The archive reader rounds disk transfers to 4 KiB (observed: 163840
   // for 160000 image bytes). Supply zeroed tail padding, never read past PNG.
   const size_t padded=(payloadSize+4095)&~size_t(4095);
   if(!buffer||offset>padded||wanted>padded-offset)status=0x1f;
   else {
    const size_t available=offset<payloadSize?payloadSize-offset:0;
    const size_t copied=static_cast<size_t>(wanted)<available?static_cast<size_t>(wanted):available;
    if(copied)std::memcpy(buffer,payload+offset,copied);
    if(wanted>copied)std::memset(static_cast<Byte*>(buffer)+copied,0,static_cast<size_t>(wanted)-copied);
   }
   auto& completed=*prepared;
   completed.record[0]=status;std::memcpy(completed.record.data()+8,r+0x30,16);
   {
    std::lock_guard<std::mutex> completionLock(completionMutex);
    queue->ready.splice(queue->ready.end(),preparingCompletions,prepared);--queue->preparing;published=true;
   }
   if(auto event=read<void*>(requester,8))reinterpret_cast<void(__fastcall*)(void*)>(base+0xf510d0)(event);
   if constexpr(TransportDiagnostics) {char message[160];sprintf_s(message,"CarryOn: pincer %s offset=%u bytes=%llu available=%zu status=%u",
       icon?"texture":"3D payload",offset,static_cast<unsigned long long>(wanted),payloadSize,static_cast<unsigned>(status));log(message);}
  }catch(const std::exception& e){log(e.what());if(!published){Requests one{r,1};originalRequest(requester,&one);}}
 }
 if(nativeStart<requests->count) {
  if(nativeStart==0)originalRequest(requester,requests); // Common all-native path.
  else {
   Requests native{requests->data+nativeStart*0x48,requests->count-nativeStart};
   originalRequest(requester,&native);
  }
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
#include "native_transport_input_routing.inl"
struct Hook {uintptr_t rva;void* detour;void** original;};
#include "native_pincer_textures.inl"
Hook hooks[]={
 {0x95d9c0,reinterpret_cast<void*>(&transportDestroyWorld),reinterpret_cast<void**>(&originalTransportDestroyWorld)},
 {0xd5bb50,reinterpret_cast<void*>(&transportUiTick),reinterpret_cast<void**>(&originalTransportUiTick)},
 {0x1081020,reinterpret_cast<void*>(&pollCompletion),reinterpret_cast<void**>(&originalPollCompletion)},
 {0xb5580,reinterpret_cast<void*>(&pincerTextureLookup),reinterpret_cast<void**>(&originalTextureLookup)},
 {0x29bfb0,reinterpret_cast<void*>(&transportValidateMove),reinterpret_cast<void**>(&originalTransportValidate)},
 {0x2477d0,reinterpret_cast<void*>(&transportPlacementInput),reinterpret_cast<void**>(&originalTransportPlacementInput)},
 {0x3c47b0,reinterpret_cast<void*>(&transportActionContext),reinterpret_cast<void**>(&originalTransportActionContext)},
 {0xe85800,reinterpret_cast<void*>(&transportActionAllowed),reinterpret_cast<void**>(&originalTransportActionAllowed)},
 {0xa1ebf0,reinterpret_cast<void*>(&transportStaticServer),reinterpret_cast<void**>(&originalTransportStaticServer)},
 {0xa1ee20,reinterpret_cast<void*>(&transportStaticClient),reinterpret_cast<void**>(&originalTransportStaticClient)},
 {0xe66060,reinterpret_cast<void*>(&transportServerStep),reinterpret_cast<void**>(&originalTransportServerStep)},
 {0x3aa980,reinterpret_cast<void*>(&transportZoneVisualState),reinterpret_cast<void**>(&originalTransportZoneVisualState)},
 {0xbe2490,reinterpret_cast<void*>(&transportHints),reinterpret_cast<void**>(&originalTransportHints)},
 {0xb6bb80,reinterpret_cast<void*>(&transportPlacementPanel),reinterpret_cast<void**>(&originalTransportPlacementPanel)},
 {0xe94470,reinterpret_cast<void*>(&transportHudItemLookup),reinterpret_cast<void**>(&originalTransportHudItemLookup)},
 {0xd8e320,reinterpret_cast<void*>(&transportPlacementHudAllowed),reinterpret_cast<void**>(&originalTransportPlacementHudAllowed)},
 {0xbe5150,reinterpret_cast<void*>(&transportCursorHud),reinterpret_cast<void**>(&originalTransportCursorHud)},
 {0x1080ae0,reinterpret_cast<void*>(&open),reinterpret_cast<void**>(&originalOpen)},
 {0x10805d0,reinterpret_cast<void*>(&resolve),reinterpret_cast<void**>(&originalResolve)},
 {0x107ed60,reinterpret_cast<void*>(&keyOf),reinterpret_cast<void**>(&originalKey)},
 {0x12169b0,reinterpret_cast<void*>(&lookup),reinterpret_cast<void**>(&originalLookup)},
 {0xaff9f0,reinterpret_cast<void*>(&format),reinterpret_cast<void**>(&originalFormat)},
 {0x1081730,reinterpret_cast<void*>(&list),reinterpret_cast<void**>(&originalList)},
 {0xa56180,reinterpret_cast<void*>(&fill),reinterpret_cast<void**>(&originalFill)},
 {0x1083210,reinterpret_cast<void*>(&request),reinterpret_cast<void**>(&originalRequest)},
 {0x292c50,reinterpret_cast<void*>(&transportRow),reinterpret_cast<void**>(&originalTransportRow)},
  {0x2963c0,reinterpret_cast<void*>(&transportRender),reinterpret_cast<void**>(&originalTransportRender)},
  {0x315a20,reinterpret_cast<void*>(&transportVoxelRender),reinterpret_cast<void**>(&originalTransportVoxelRender)}
};
void prepareNativeContent() {
 // The UI bundle was already read before world selection. Keep a process-long
 // modified copy so the manual crafting menu sees the new entry as well.
 auto context=read<void*>(reinterpret_cast<void*>(base),0x283f4a8);
 if(!context)throw std::runtime_error("UI context unavailable");
 auto services=read<void*>(context,0xd0);
 if(services)if(auto pool=read<void*>(services,0x33ce8))language.store(read<uint32_t>(pool,0xd8));
 auto ui=read<const Byte*>(context,0xc8);if(!ui)throw std::runtime_error("UI bundle unavailable");
 loadPincerAssets();
 prepareMenu(const_cast<Byte*>(ui));
 pixelHash={static_cast<uint32_t>(sizeof(PincerPixels)),2166136261u,0x9e3779b9u,0x43415259u};
 for(auto p:PincerPixels){pixelHash[1]=(pixelHash[1]^p)*16777619u;pixelHash[2]=(pixelHash[2]^p)*16777619u;}
}
void rollbackNativeHooks(size_t created,bool menuWritten) {
 bool detached=true;
 for(size_t i=0;i<created;++i) {
  const auto target=reinterpret_cast<void*>(base+hooks[i].rva);
  const auto disabled=MH_DisableHook(target);
  if(disabled!=MH_OK&&disabled!=MH_ERROR_DISABLED){detached=false;continue;}
  if(MH_RemoveHook(target)!=MH_OK)detached=false;
 }
 const bool restored=!menuWritten||writeMenu(menuOriginal);
 if(restored&&menuAllocation) {
  if(VirtualFree(menuAllocation,0,MEM_RELEASE))menuAllocation=nullptr;
  else log("CarryOn: failed to release crafting menu allocation during rollback");
 }
 if(!restored||!detached){restartRequired=true;log("CarryOn: rollback incomplete; pinned module retained for safety; restart required");}
}
void activateNativeHooks() {
 auto status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED) {
  if(menuAllocation&&VirtualFree(menuAllocation,0,MEM_RELEASE))menuAllocation=nullptr;
  throw std::runtime_error("MinHook initialization failed");
 }
 size_t created=0;
 bool menuWritten=false;
 try {
  for(const auto& h:hooks) {
   if(MH_CreateHook(reinterpret_cast<void*>(base+h.rva),h.detour,h.original)!=MH_OK)
    throw std::runtime_error("native hook creation failed");
   ++created;
  }
  // ABI 1 hooks and resources have process lifetime, including rollback errors.
  HMODULE owner=nullptr;
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
      reinterpret_cast<LPCWSTR>(&initialize),&owner))throw std::runtime_error("unable to pin native adapter");
  menuWritten=true;
  if(!writeMenu(menuReplacement))throw std::runtime_error("crafting menu write refused");
  for(const auto& h:hooks)
   if(MH_QueueEnableHook(reinterpret_cast<void*>(base+h.rva))!=MH_OK)
    throw std::runtime_error("native hook enable queue failed");
  if(MH_ApplyQueued()!=MH_OK)throw std::runtime_error("native hook activation failed");
 }catch(...) {
  rollbackNativeHooks(created,menuWritten);
  throw;
 }
}

}
void configureTransport(bool moveChests,bool inspectObjects,bool moveObjects,bool moveInteractive) noexcept {
 transportChests.store(moveChests,std::memory_order_relaxed);
 transportObjects.store(moveObjects,std::memory_order_relaxed);
 transportInteractive.store(moveInteractive,std::memory_order_relaxed);
 transportInspectObjects.store(inspectObjects,std::memory_order_relaxed);
}
bool initialize(void(__cdecl* callback)(const char*)) {
 std::lock_guard<std::mutex> lock(installMutex);if(installed)return true;logger=callback;
 if(restartRequired){log("CarryOn: previous rollback incomplete; restart required");return false;}
 if(!compatible()){log("CarryOn: unsupported executable; native integration refused");return false;}
 base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 bindTransport();
 bindTransportMovement();
 bindPincerTextures();
 try {
  prepareNativeContent();
  activateNativeHooks();
  installed=true;
  log("CarryOn: pincer content hooks enabled; manual recipe costs 1 metal scrap");return true;
 }catch(const std::exception& e){log(e.what());return false;}
}
}


