// Private visual resources, embedded in the DLL; no external paths or archive edits.
const Guid PincerModelId=guid("9016531065fed65fbf82c9096d472e34");
const Guid PincerMaterialId=guid("4b50cec646cf515caec390230074b23b");
const Guid PincerVisualId=guid("5e52f7b0ac078d53b18d237b5de7fe8c");
struct PincerAsset {Key key;const Byte* data;size_t size;};
std::vector<PincerAsset> pincerAssets;
std::vector<PincerAsset> parsePincerAssets(const Byte* data,size_t size) {
 if(!data||size<12||std::memcmp(data,"COP3D001",8))throw std::runtime_error("invalid pincer model pack");
 const auto count=read<uint32_t>(data,8);
 if(count<4||count>32||size<12+size_t(count)*32)throw std::runtime_error("invalid pincer model table");
 std::vector<PincerAsset> result;
 for(uint32_t i=0;i<count;++i) {
  const auto p=data+12+size_t(i)*32;Key key{};
  key.guid=read<Guid>(p,0);key.type=read<uint32_t>(p,16);key.part=read<uint32_t>(p,20);
  const auto offset=read<uint32_t>(p,24),length=read<uint32_t>(p,28);
  if(key.part||key.guid==Guid{}||!length||offset<12+size_t(count)*32||offset>size||length>size-offset||offset%16)
   throw std::runtime_error("pincer model range invalid");
  const bool known=key.type==0||(key.type==0x177394c7&&key.guid==PincerModelId)||
   (key.type==0xaee1894b&&key.guid==PincerMaterialId)||(key.type==0x39768775&&key.guid==PincerVisualId);
  if(!known||(!key.type&&read<uint32_t>(p,0)!=length))throw std::runtime_error("pincer model identity invalid");
  for(const auto& prior:result)if(prior.key.guid==key.guid&&prior.key.type==key.type)
   throw std::runtime_error("duplicate pincer model identity");
  result.push_back({key,data+offset,length});
 }
 for(const auto id:{PincerModelId,PincerMaterialId,PincerVisualId}) {
  bool found=false;for(const auto& asset:result)if(asset.key.guid==id&&asset.key.type)found=true;
  if(!found)throw std::runtime_error("pincer model resource missing");
 }
 return result;
}
void loadPincerAssets() {
 if(!pincerAssets.empty())return;
 HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
     reinterpret_cast<LPCWSTR>(&loadPincerAssets),&module))throw std::runtime_error("pincer model module missing");
 const auto resource=FindResourceW(module,MAKEINTRESOURCEW(401),MAKEINTRESOURCEW(10));
 if(!resource)throw std::runtime_error("embedded pincer model missing");
 const auto loaded=LoadResource(module,resource);
 if(!loaded)throw std::runtime_error("embedded model resource could not be loaded");
 pincerAssets=parsePincerAssets(static_cast<const Byte*>(LockResource(loaded)),SizeofResource(module,resource));
}
const PincerAsset* pincerAsset(const Guid& id,uint32_t type) {
 for(const auto& a:pincerAssets)if(a.key.guid==id&&a.key.type==type)return &a;
 return nullptr;
}
