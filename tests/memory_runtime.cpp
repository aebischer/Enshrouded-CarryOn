#include <cstdlib>
#include <new>
static bool auditFailAllocation=false;
static int auditAllocationCountdown=-1;
void* operator new(std::size_t size) {
 if(auditFailAllocation){auditFailAllocation=false;throw std::bad_alloc();}
 if(auditAllocationCountdown>=0&&auditAllocationCountdown--==0)throw std::bad_alloc();
 if(auto p=std::malloc(size?size:1))return p;throw std::bad_alloc();
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
#define main existing_test_main
#include "native_content.cpp"
#undef main
#include <chrono>
#include <thread>
namespace carryon::native {
static unsigned auditLookupCalls=0;
static bool auditProbeCacheLock=false;
static const Byte* __fastcall auditLookup(uint32_t id,void*) {
 CHECK(id);++auditLookupCalls;
 if(auditProbeCacheLock) {
  bool available=false;
  std::thread probe([&]{available=transportCatalogMutex.try_lock();if(available)transportCatalogMutex.unlock();});probe.join();
  CHECK(available);auditProbeCacheLock=false;
 }
 static Byte info[0x400]{};write(info,0,id);Guid model{};write(model.data(),0,id);write(info,0x3c0,model);return info;
}
static unsigned auditReads=0,auditIndices=0,auditSlots=0;
static ComponentView* __fastcall auditRead(ComponentView* out,void* world,const uint16_t* index,uint32_t entity) {
 ++auditReads;return runtimeRead(out,world,index,entity);
}
static const uint16_t* __fastcall auditIndex(void* registry,const uint32_t* type) {
 ++auditIndices;return runtimeIndex(registry,type);
}
static size_t __fastcall auditSlot(void* table,const uint32_t* entity) {
 ++auditSlots;return runtimeEntitySlot(table,entity);
}
static Bytes auditLoad(const char* path) {
 std::ifstream f(path,std::ios::binary);CHECK(f.good());return {std::istreambuf_iterator<char>(f),{}};
}
static void auditMemory(const char* profilePath,const char* corpusPath) {
 const auto profile=auditLoad(profilePath),corpus=auditLoad(corpusPath);
 size_t at=0,count=0,totalSize=0,totalCapacity=0,maxSize=0,modelTotal=0;
 while(at<corpus.size()) {
  const auto model=read<Guid>(corpus.data(),at);const auto size=u32(corpus,at+16);at+=20;
  const auto bytes=slice(corpus,at,size);at+=size;
  try {const auto out=makeFloorPlacement(profile,bytes,model);++count;totalSize+=out.size();totalCapacity+=out.capacity();maxSize=(std::max)(maxSize,out.size());modelTotal+=size;}
  catch(const std::runtime_error&){}
 }
 std::printf("floor_corpus accepted=%zu retained_bytes=%zu vector_capacity=%zu largest=%zu original_model_bytes=%zu donor_each=%zu\n",count,totalSize,totalCapacity,maxSize,modelTotal,profile.size());
 CHECK(count==5769&&totalCapacity==totalSize);
 std::printf("sizeof source=%zu transaction=%zu runtime=%zu pending=%zu texture_state=%zu private_completion=%zu\n",
  sizeof(TransportSourceSnapshot),sizeof(transport::MoveTransaction),sizeof(TransportMoveRuntime),sizeof(TransportClientCollisionMove),sizeof(PincerTextureMemory),sizeof(PrivateCompletion));
 loadPincerAssets();size_t embeddedCopies=0,embeddedPayload=0;
 for(const auto& asset:pincerAssets){if(asset.key.type)embeddedCopies+=asset.size;else embeddedPayload+=asset.size;}
 std::printf("embedded_resource_heap_copies=%zu zero_copy_payload=%zu ui_source=%u\n",embeddedCopies,embeddedPayload,244792u);
 {
  MoveFixture f;runtimeSetup(f);runtimeDoor(f);transportDoorTemplates.clear();transportResourceDb=f.registry;
  originalOpen=&floorOpen;floorUnavailable=true;floorTestReads=0;
  for(unsigned i=0;i<100;++i)CHECK(!transportIsDoor(f.counterDescriptor));
  std::printf("missing_door_resource attempts_per_100_queries=%u\n",floorTestReads);
  CHECK(floorTestReads==1);
  transportTemplateRetry[read<Guid>(f.counterDescriptor,0)]=GetTickCount64()-1001;
  CHECK(!transportIsDoor(f.counterDescriptor)&&floorTestReads==2);
  floorUnavailable=false;
 }
 {
  MoveFixture f;runtimeSetup(f);transportRead=&auditRead;transportTypeIndex=&auditIndex;transportEntitySlot=&auditSlot;
  auditReads=auditIndices=auditSlots=0;TransportSourceSnapshot out{};
  CHECK(transportSnapshotSource(f.world,f.zones,1,1217,out));
  std::printf("chest_snapshot native_reads=%u type_lookups=%u template_lookups=%u\n",auditReads,auditIndices,auditSlots);
  f.manySources=true;
  for(uint32_t id=2000;id<2032;++id)transportMove.clientCollisions.push_back({reinterpret_cast<uintptr_t>(f.client),id,42,{},1,false,read<transport::Identity>(f.templateId.data(),0)});
  auditReads=auditIndices=auditSlots=0;
  for(unsigned i=0;i<100;++i)transportPruneSynchronization(transportMove,f.world);
  std::printf("32_pending_100_server_steps reads=%u type_lookups=%u template_lookups=%u\n",auditReads,auditIndices,auditSlots);
  CHECK(auditReads==32&&auditSlots==32);
  transportPruneSynchronization(transportMove,f.world,GetTickCount64()+1001);CHECK(auditReads==64);
 }
 {
  MoveFixture f;runtimeSetup(f);bool escaped=false;auditFailAllocation=true;
  try{transportRefreshPlacementCatalog(f.registry);}catch(const std::bad_alloc&){escaped=true;}
  auditFailAllocation=false;std::printf("catalog_allocation_escapes_native_boundary=%d\n",escaped);CHECK(!escaped);
  runtimeCounter(f);transportPlacementCatalog.clear();transportCatalogRegistries.insert(f.registry);transportResourceDb=f.registry;
  escaped=false;auditFailAllocation=true;
  try{transportDecorPlacement(f.counterDescriptor,1217);}catch(const std::bad_alloc&){escaped=true;}
  auditFailAllocation=false;std::printf("floor_retry_allocation_escapes_native_boundary=%d\n",escaped);CHECK(!escaped);
 }
 {
  entries[6].bytes.clear();entries[6].failed=false;auditFailAllocation=true;
  CHECK(!build(6,nullptr));auditFailAllocation=false;
  const bool poisoned=build(6,nullptr)==nullptr&&entries[6].failed.load();
  std::printf("transient_embedded_allocation_failure_permanently_cached=%d\n",poisoned);CHECK(!poisoned);
 }
 {
  // Partial publication must be restartable after failure at any early allocation.
  auto saved=std::move(entries[0].bytes);const auto lookup=transportItemLookup;
  entries[0].bytes=Bytes(8+65*16);put32(entries[0].bytes,0,8);put32(entries[0].bytes,4,65);
  for(uint32_t i=0;i<65;++i){Guid id{};write(id.data(),0,i+1);putGuid(entries[0].bytes,8+i*16,id);}
  transportItemLookup=&auditLookup;auto registry=reinterpret_cast<void*>(123);
  for(int failure=-1;failure<8;++failure) {
   transportCatalogRegistries.clear();transportCatalogProgress.clear();transportPlacementCatalog.clear();auditLookupCalls=0;
   auditProbeCacheLock=failure==-1;auditAllocationCountdown=failure;
   transportRefreshPlacementCatalog(registry);auditAllocationCountdown=-1;
   CHECK(auditLookupCalls<=64&&!transportCatalogRegistries.count(registry));
   for(unsigned tries=0;tries<10&&!transportCatalogRegistries.count(registry);++tries) {
    const auto before=auditLookupCalls;transportRefreshPlacementCatalog(registry);CHECK(auditLookupCalls-before<=64);
   }
   CHECK(transportCatalogRegistries.count(registry)&&transportPlacementCatalog.size()==65);
  }
  entries[0].bytes=std::move(saved);transportItemLookup=lookup;
  std::puts("PASS: 64-entry catalogue budget, native lookup outside cache lock, allocation failure at eight publication points and complete recovery.");
 }
 {
  // More entries than the server budget: every entry receives a turn, even
  // when callbacks are separated by more than the retry delay.
  MoveFixture f;runtimeSetup(f);
  for(unsigned i=0;i<65;++i)transportMove.clientCollisions.push_back({reinterpret_cast<uintptr_t>(f.client),1217,42,{},1,false,read<transport::Identity>(f.templateId.data(),0)});
  const auto start=GetTickCount64();
  for(unsigned i=0;i<3;++i)transportPruneSynchronization(transportMove,f.world,start+i*1001);
  for(const auto& pending:transportMove.clientCollisions)CHECK(pending.lastServerCheck);
 }
 {
  // Failure before publication leaves the caller buffer untouched and sends
  // the original descriptor to native fallback, never a false completion.
  Byte requester[0x188]{},row[0x48]{},output=0xcd;Requests batch{row,1};
  std::memcpy(row+0x10,pixelHash.data(),16);write(row,0,static_cast<void*>(&output));write<uint64_t>(row,8,1);
  originalRequest=&fallback;passed=0;freeCompletions.clear();auditFailAllocation=true;
  request(requester,&batch);auditFailAllocation=false;
  CHECK(passed==1&&output==0xcd&&privateCompletions.empty());
  originalPollCompletion=&emptyCompletion;
  write<LONG>(requester,0x180,1);request(requester,&batch);Byte record[24]{};CHECK(pollCompletion(record,requester));
  CHECK(freeCompletions.size()==1&&privateCompletions.empty());
  // Existing queue plus recycled node needs no new allocation.
  write<LONG>(requester,0x180,2);request(requester,&batch);request(requester,&batch);
  CHECK(pollCompletion(record,requester));
  InterlockedIncrement(reinterpret_cast<volatile LONG*>(requester+0x180));
  auditFailAllocation=true;request(requester,&batch);CHECK(auditFailAllocation);auditFailAllocation=false;
  CHECK(pollCompletion(record,requester));CHECK(pollCompletion(record,requester));
  CHECK(read<LONG>(requester,0x180)==0&&passed==1);
  CHECK(privateCompletions.empty()&&freeCompletions.size()<=CompletionPoolLimit);
  std::puts("PASS: private read allocation refusal, native fallback, requester queue cleanup and bounded node reuse.");
 }
 {
  Byte requester[0x188]{};std::array<Byte,2000> output{};
  write<LONG>(requester,0x180,static_cast<LONG>(output.size()));
  std::thread producer([&]{
   for(size_t i=0;i<output.size();++i) {
    Byte row[0x48]{};std::memcpy(row+0x10,pixelHash.data(),16);
    write(row,0,static_cast<void*>(&output[i]));write<uint64_t>(row,8,1);write<uint64_t>(row,0x30,i);
    Requests batch{row,1};request(requester,&batch);
   }
  });
  const auto deadline=GetTickCount64()+10000;size_t received=0;
  while(received<output.size()) {
   Byte record[24]{};
   if(pollCompletion(record,requester)) {
    CHECK(read<uint64_t>(record,8)==received&&output[received]==PincerPixels[0]);++received;
   } else {CHECK(GetTickCount64()<deadline);std::this_thread::yield();}
  }
  producer.join();CHECK(read<LONG>(requester,0x180)==0&&privateCompletions.empty()&&freeCompletions.size()<=CompletionPoolLimit);
  std::puts("PASS: concurrent producer/consumer, 2000 private completions ordered exactly once, buffers visible before publication.");
 }
}
}
int main(int argc,char** argv){CHECK(argc==3);carryon::native::auditMemory(argv[1],argv[2]);}
