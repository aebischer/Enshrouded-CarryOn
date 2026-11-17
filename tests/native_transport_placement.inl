// These tests exercise ownership and bounds, not an emulation of the renderer.
static unsigned previewCreates=0,previewRemoves=0,previewMoves=0,previewFades=0,previewPoses=0;
static bool previewCreateFails=false;
static Byte previewColor=0;
static uint32_t __fastcall previewCreate(void*,const Guid*,const void*) {
 ++previewCreates;return previewCreateFails?0:42;
}
static void __fastcall previewRemove(void*,const RenderSpan* handles) {
 CHECK(handles->count==1&&*static_cast<const uint32_t*>(handles->data)==42);++previewRemoves;
}
static void __fastcall previewMove(void*,const RenderSpan* handles,const RenderSpan* transforms) {
 CHECK(handles->count==1&&transforms->count==1);++previewMoves;
}
static void __fastcall previewFade(void*,uint32_t handle,float amount) {
 CHECK(handle==42&&amount>0.0f&&amount<1.0f);++previewFades;
}
static void __fastcall previewStaticPose(void*,uint32_t handle,bool enabled) {
 CHECK(handle==42&&enabled);++previewPoses;
}
static void __fastcall previewHighlight(void*,const RenderSpan*,const RenderSpan* colors) {
 previewColor=*static_cast<const Byte*>(colors->data);
}
static bool __fastcall previewContains(const void* zone,const void* point,float) {
 for(size_t axis=0;axis<3;++axis) {
  const auto value=read<int64_t>(point,axis*8);
  if(value<read<int64_t>(zone,axis*8)||value>read<int64_t>(zone,(axis+3)*8))return false;
 }
 return true;
}
static void testTransportPlacement() {
 transportModelCreate=&previewCreate;transportModelRemove=&previewRemove;
 transportModelMove=&previewMove;transportModelOpacity=&previewFade;
 transportModelStaticPose=&previewStaticPose;
 transportRenderHighlight=&previewHighlight;transportPointInZone=&previewContains;
 Byte sceneA{},sceneB{},transform[56]{};Guid model{};model[0]=1;
 TransportPreview preview;
 CHECK(preview.draw(1,&sceneA,model,transform,true));
 CHECK(previewCreates==1&&previewMoves==1&&previewFades==1&&previewPoses==1&&previewColor==2);
 CHECK(preview.draw(1,&sceneA,model,transform,false));
 CHECK(previewCreates==1&&previewMoves==2&&previewPoses==1&&previewColor==1);
 preview.clear(1,&sceneA);preview.clear(1,&sceneA);CHECK(previewRemoves==1);
 CHECK(preview.draw(1,&sceneA,model,transform,true));
 // The old scene is already dead: never remove its handle from the new scene.
 CHECK(preview.draw(2,&sceneB,model,transform,true));CHECK(previewRemoves==1);
 model[0]=2;CHECK(preview.draw(2,&sceneB,model,transform,true));CHECK(previewRemoves==2);
 CHECK(!preview.draw(2,&sceneB,Guid{},transform,true));CHECK(previewRemoves==3&&!preview.handle);
 previewCreateFails=true;CHECK(!preview.draw(2,&sceneB,model,transform,true));
 CHECK(!preview.handle&&!preview.world&&!preview.scene);previewCreateFails=false;
 Byte zones[0x20]{},zoneEntries[0x90]{};
 write<const Byte*>(zones,0x10,zoneEntries);write<size_t>(zones,0x18,2);
 Guid id{};id[0]=7;
 for(size_t i=0;i<2;++i) {
  for(size_t axis=0;axis<3;++axis)write<int64_t>(zoneEntries+i*0x48,(axis+3)*8,100);
  write<Guid>(zoneEntries+i*0x48,0x30,id);write<uint32_t>(zoneEntries+i*0x48,0x40,static_cast<uint32_t>(i+1));
 }
 const TransportClaim claim{id,2};int64_t bounds[6]{10,10,10,90,90,90};
 CHECK(transportClaimEntry(zones,claim)==zoneEntries+0x48);
 CHECK(transportFindClaim(zones,bounds).altar==1); // Overlapping claims must retain source identity.
 CHECK(transportBoxInClaim(zones,claim,bounds));
 bounds[5]=101;CHECK(!transportBoxInClaim(zones,claim,bounds));bounds[5]=90;
 bounds[0]=91;CHECK(!transportBoxInClaim(zones,claim,bounds));bounds[0]=10;
 zoneEntries[0x48+0x46]=1;CHECK(!transportBoxInClaim(zones,claim,bounds));
 zoneEntries[0x48+0x46]=0;write<uint32_t>(zoneEntries+0x48,0x40,3);
 CHECK(!transportBoxInClaim(zones,claim,bounds)); // Same GUID with recycled/changed altar ID.
 write<size_t>(zones,0x18,1025);CHECK(!transportClaimEntry(zones,claim));
 transportRenderHighlight=nullptr;
 std::puts("PASS: preview ownership, failed creation, cancellation, colors and exact claim bounds.");
}
