static Key catalogNative[2]{};
static bool catalogFailure=false;
static Keys* __fastcall catalogList(Keys* out,void*,uint32_t type,uint32_t* found) {
 catalogNative[0]={Hammer,type,0,0};catalogNative[1]={ItemName,type,2,0};
 *out={catalogFailure?uint64_t(0x1f):0,catalogNative,2};if(found)*found=1;return out;
}
static void testVisualCatalog() {
 originalList=&catalogList;
 for(const auto type:{uint32_t(0x39768775),uint32_t(0x177394c7),uint32_t(0xaee1894b),IconType,uint32_t(0x12345678)}) {
  Keys out{};uint32_t found=0;list(&out,nullptr,type,&found);
  const auto expected=type==IconType?4u:type==0x12345678?2u:3u;
  CHECK(found==1&&!out.status&&out.count==expected);
  CHECK(out.data[0].guid==Hammer&&out.data[1].guid==ItemName&&out.data[1].part==2);
  if(type==0x39768775) {
   // Native ECS builds its template catalog exclusively from this enumeration.
   CHECK(out.data[2].guid==PincerVisualId);
   uint32_t handle=0;open(&handle,nullptr,out.data+2);CHECK(handle==FirstHandle+8);
   uint64_t view[3]{};resolve(view,nullptr,handle);CHECK(view[0]&&view[1]);
  }
  auto pointer=out.data;list(&out,nullptr,type,nullptr);CHECK(out.data==pointer&&out.count==expected);
  catalogFailure=true;list(&out,nullptr,type,nullptr);CHECK(out.status==0x1f&&out.data==catalogNative&&out.count==2);catalogFailure=false;
 }
 std::puts("PASS: world template catalog includes pincer before spawn; native keys, variants, errors, repeat calls and unrelated catalogs preserved.");
}
static void testPincerModel() {
 loadPincerAssets();CHECK(pincerAssets.size()==11);testVisualCatalog();
 for(size_t i=6;i<std::size(entries);++i) {
  const auto& k=entries[i].key;const auto asset=pincerAsset(k.guid,k.type);CHECK(asset&&asset->size);
  uint32_t handle=0;open(&handle,nullptr,&k);CHECK(handle==FirstHandle+i);
  uint64_t view[3]{};resolve(view,nullptr,handle);
  CHECK(view[1]==asset->size&&std::memcmp(reinterpret_cast<const void*>(view[0]),asset->data,asset->size)==0);
  Byte key[40]{};keyOf(key,nullptr,handle);CHECK(std::memcmp(key+8,&k,32)==0);
 }
 const auto visual=pincerAsset(PincerVisualId,0x39768775);CHECK(visual);
 const auto table=16+read<uint32_t>(visual->data,16),count=read<uint32_t>(visual->data,20);
 bool attachment=false,model=false,orb=false,orbAttachment=false;CHECK(count==9);
 for(uint32_t i=0;i<count;++i) {
  const auto at=table+i*12,type=read<uint32_t>(visual->data,at);
  const auto data=visual->data+at+4+read<uint32_t>(visual->data,at+4);
  if(type==0xb3721934){orb=true;CHECK(read<Guid>(data,0)==guid("1973ad464d14b043a9ee842d1c2355c9"));}
  if(type==0xe0122600) {
   orbAttachment=true;CHECK(read<uint32_t>(visual->data,at+8)==52);
   CHECK(std::abs(read<float>(data,8)+.014f)<.000001f&&std::abs(read<float>(data,12)-.209f)<.000001f&&read<float>(data,16)==0);
   CHECK(read<float>(data,44)==1&&data[48]==0&&data[49]==0&&data[50]==0);
  }
  if(type==0x09561436)attachment=true;
  if(type==0x192b2ed8){model=true;CHECK(read<Guid>(visual->data,at+4+read<uint32_t>(visual->data,at+4))==PincerModelId);}
 }
 CHECK(attachment&&model&&orb&&orbAttachment);
 // Corrupted embedded tables must fail before any native hook can be enabled.
 for(unsigned defect=0;defect<5;++defect) {
  Bytes bad(76,0);std::memcpy(bad.data(),"COP3D001",8);write<uint32_t>(bad.data(),8,4);
  if(defect==1)bad[0]=0;
  if(defect==2)write<uint32_t>(bad.data(),8,0xffffffffu);
  if(defect>=3){bad.resize(12+4*32);write(bad.data(),12,PincerModelId);write<uint32_t>(bad.data(),28,0x177394c7);write<uint32_t>(bad.data(),36,defect==3?0xffffffffu:0);write<uint32_t>(bad.data(),40,100);}
  bool failed=false;try{parsePincerAssets(bad.data(),bad.size());}catch(const std::exception&){failed=true;}CHECK(failed);
 }
 // Serve every model/texture stream at its real size and disk-aligned tail.
 for(const auto& a:pincerAssets)if(!a.key.type) {
  const auto padded=(a.size+4095)&~size_t(4095);Bytes dest(padded+2,0xcd);
  Byte r[0x48]{},requester[0x188]{},records[96]{};
  write(r,0,static_cast<void*>(dest.data()+1));write<uint64_t>(r,8,padded);write(r,0x10,a.key.guid);
  write(requester,0x48,static_cast<Byte*>(records));write<uint32_t>(requester,0x50,3);write<uint32_t>(requester,0xc0,3);
  Requests requests{r,1};testRequest(requester,&requests);
  CHECK(read<uint32_t>(requester,0x80)==1&&records[0]==0&&dest.front()==0xcd&&dest.back()==0xcd);
  CHECK(std::memcmp(dest.data()+1,a.data,a.size)==0);
  for(size_t i=a.size;i<padded;++i)CHECK(dest[i+1]==0);
  write<uint32_t>(r,0x40,static_cast<uint32_t>(padded));write<uint64_t>(r,8,1);testRequest(requester,&requests);
  CHECK(records[24]==0x1f&&dest.front()==0xcd&&dest.back()==0xcd);
 }
 std::puts("PASS: embedded 3D resources, unique visual identity, hand attachment, native orange orb at pincer joint, full payloads/padded tails, malformed bounds.");
}
