constexpr uint32_t TransportCreationOnlyRefusals=(1u<<12)|(1u<<13);
using TransportItemLookup=const Byte*(__fastcall*)(uint32_t,void*);
using TransportCursorConfig=void(__fastcall*)(void*,void*,const void*,const void*,bool,bool);
using TransportCastConfig=void(__fastcall*)(void*,void*,const void*);
using TransportCast=void(__fastcall*)(void*,void*,const void*,const void*);
using TransportSolve=void(__fastcall*)(uint32_t*,void*,bool*,bool*,float*,void*,const void*,const void*);
// Fifth argument is the native permission MASK, not bool. Truncating it would
// silently drop the build permission (0x10) and other native restrictions.
using TransportValidate=void(__fastcall*)(uint32_t*,void*,const void*,const void*,Byte,Byte,float);
TransportItemLookup transportItemLookup=nullptr;
const Byte* transportPlacementInfo(uint32_t item,void* registry,const transport::Identity* model=nullptr) {
 if(model) {
  Guid key{};std::memcpy(key.data(),model,16);
  std::lock_guard<std::mutex> lock(transportCatalogMutex);
  const auto found=transportFloorPlacements.find(key);
  if(found!=transportFloorPlacements.end())return u32(found->second,0)==item?found->second.data():nullptr;
 }
 return transportItemLookup?transportItemLookup(item,registry):nullptr;
}
constexpr size_t TransportCatalogBatch=64;
void transportRefreshPlacementCatalog(void* registry) noexcept {
 if(!registry||!transportItemLookup)return;
 try {
  {std::lock_guard<std::mutex> lock(transportCatalogMutex);if(transportCatalogRegistries.count(registry))return;}
  std::unique_lock<std::mutex> work(transportCatalogWorkMutex,std::try_to_lock);if(!work.owns_lock())return;
  const auto generation=transportSessionGeneration.load(),now=GetTickCount64();
  std::array<uint32_t,TransportCatalogBatch> ids{};size_t count=0,start=0;
  {
   std::lock_guard<std::mutex> lock(transportCatalogMutex);
   if(transportCatalogRegistries.count(registry))return;
   auto& progress=transportCatalogProgress[registry];
   if(progress.attemptedAt&&now-progress.attemptedAt<1000)return;
   if(progress.pending.empty()) {
    auto& entry=entries[0];std::lock_guard<std::mutex> resourceLock(entry.mutex);
    if(entry.bytes.empty())return;
    const auto refs=array(entry.bytes,0,16);
    // Build separately: allocation failure cannot publish a truncated catalogue.
    std::vector<uint32_t> pending;pending.reserve(refs.count);
    for(uint32_t i=0;i<refs.count;++i)pending.push_back(hash(read<Guid>(entry.bytes.data(),refs.at+size_t(i)*16)));
    progress.pending=std::move(pending);progress.cursor=0;
   }
   start=progress.cursor;count=(std::min)(TransportCatalogBatch,progress.pending.size()-start);
   std::copy_n(progress.pending.begin()+start,count,ids.begin());
  }
  std::array<Guid,TransportCatalogBatch> models{};std::array<bool,TransportCatalogBatch> resolved{};
  // Native lookups stay on the caller's thread, outside the shared cache lock.
  for(size_t i=0;i<count;++i) {
   if(!ids[i]){resolved[i]=true;continue;} // A prior publication may have failed midway.
   const auto info=transportItemLookup(ids[i],registry);
   if(info&&read<uint32_t>(info,0)==ids[i]){resolved[i]=true;models[i]=read<Guid>(info,0x3c0);}
  }
  std::lock_guard<std::mutex> lock(transportCatalogMutex);
  if(generation!=transportSessionGeneration.load())return;
  const auto found=transportCatalogProgress.find(registry);if(found==transportCatalogProgress.end())return;
  auto& progress=found->second;
  for(size_t i=0;i<count;++i)if(resolved[i]) {
   if(models[i]!=Guid{})transportPlacementCatalog.emplace(models[i],ids[i]);
   progress.pending[start+i]=0;
  }
  progress.cursor=start+count;
  if(progress.cursor<progress.pending.size())return;
  progress.pending.erase(std::remove(progress.pending.begin(),progress.pending.end(),0u),progress.pending.end());
  progress.cursor=0;progress.attemptedAt=now;
  if(!progress.pending.empty())return;
  transportCatalogRegistries.insert(registry);transportCatalogProgress.erase(found);
  log("CarryOn: native placement catalog indexed for decor movement");
 }catch(const std::exception&){/* Retry later; never throw through a native callback. */}
}
TransportCursorConfig transportCursorConfig=nullptr;
TransportCastConfig transportCastConfig=nullptr;
TransportCast transportCast=nullptr;
TransportSolve transportSolve=nullptr;
TransportValidate originalTransportValidate=nullptr;
struct TransportPlacementSample {
 transport::Placement placement{};
 uint32_t flags=1;
 Byte special=0;
 bool calculated=false;
};
std::array<Byte,56> transportNativePlacement(const transport::Placement& placement) {
 std::array<Byte,56> bytes{};
 write(bytes.data(),0,placement.position);write(bytes.data(),24,placement.rotation);write(bytes.data(),40,placement.scale);
 return bytes;
}
transport::Placement transportReadPlacement(const void* bytes) {
 transport::Placement placement{};
 placement.position=read<std::array<int64_t,3>>(bytes,0);
 placement.rotation=read<std::array<float,4>>(bytes,24);
 placement.scale=read<std::array<float,3>>(bytes,40);return placement;
}
bool transportPrepareCursor(void* context,uint32_t item,Byte* config,const transport::Identity* model=nullptr) {
 if(!context||!item||!config||!transportItemLookup||!transportCursorConfig)return false;
 const auto registry=read<void*>(context,0x1b0);if(!registry)return false;
 const auto info=transportPlacementInfo(item,registry,model);if(!info||read<uint32_t>(info,0)!=item)return false;
 std::memset(config,0,0xa0);config[0x20]=0x1f;write<uint64_t>(config,0x40,0xff0201);config[0x70]=1;
 // Registry-owned ItemInfo or a process-long complete fallback blob. Both
 // preserve their nested relative references; never use a fixed-header clone.
 transportCursorConfig(config,context,info,info,false,true);
 config[0x47]=0; // Existing chest: no materials are consumed by transport.
 return read<const Byte*>(config,0x60)==info&&config[0x70]==1;
}
TransportPlacementSample transportCalculatePlacement(void* nativeContext,uint32_t item,
                       const transport::Placement& original,Byte permissions,float delta,const transport::Identity* model=nullptr) {
 TransportPlacementSample sample{};
 if(!nativeContext||!original.valid()||!transportCastConfig||!transportCast||!transportSolve||!originalTransportValidate)return sample;
 const auto actor=read<const Byte*>(nativeContext,0x108);if(!actor)return sample;
 // Query-local copies only. The native solver requires building-state bit 24,
 // but transport must not activate construction/destruction on the real actor.
 alignas(16) Byte context[0x6a0]{},actorCopy[0xc38]{},config[0xa0]{};
 std::memcpy(context,nativeContext,sizeof(context));std::memcpy(actorCopy,actor,sizeof(actorCopy));
 for(const size_t offset:{size_t(0xbc8),size_t(0xc00)})write<uint32_t>(actorCopy,offset,read<uint32_t>(actorCopy,offset)|(1u<<24));
 write(context,0x108,static_cast<Byte*>(actorCopy));
 if(!transportPrepareCursor(context,item,config,model))return sample;
 alignas(16) Byte castConfig[0xe08]{},hit[0xa0]{};
 auto transform=transportNativePlacement(original);bool attached=false,snapped=false;float offset[3]{};
 transportCastConfig(castConfig,context,config);
 transportCast(hit,context,config,castConfig);
 sample.special=read<Byte>(hit,0x28)&&config[3]!=2?read<Byte>(hit,0x24):0;
 transportSolve(&sample.flags,transform.data(),&attached,&snapped,offset,context,hit,config);
 sample.placement=transportReadPlacement(transform.data());
 sample.placement.scale=original.scale;transform=transportNativePlacement(sample.placement);
 originalTransportValidate(&sample.flags,context,config,transform.data(),permissions,sample.special,delta);
 // Materials/unlock checks govern item creation. Permission, collision,
 // terrain, reach, zone and all remaining native refusal bits remain intact.
 sample.flags&=~TransportCreationOnlyRefusals;
 sample.calculated=sample.placement.valid();return sample;
}
