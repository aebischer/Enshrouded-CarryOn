#include "../src/transport_move.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using namespace carryon::transport;
static MoveSource source() {
    MoveSource s{};s.session=7;s.entity=0x100001;s.templateId={13,14};s.claim={15,16};
    s.claimEntity=1027;
    s.movable=true;s.available=true;return s;
}
int main() {
    MoveTransaction move;const auto chest=source();MoveRequest request{};
    auto target=chest.original;target.position[0]=int64_t(8)<<32;
    CHECK(move.begin(chest));CHECK(!move.begin(chest));
    CHECK(!move.queue(1,request)); // No placement sample yet.
    move.preview(target,1,true,true,chest.claim,chest.claimEntity);CHECK(!move.queue(2,request)); // Stale frame.
    move.preview(target,2,false,true,chest.claim,chest.claimEntity);CHECK(!move.queue(2,request));
    move.preview(target,3,true,false,chest.claim,chest.claimEntity);CHECK(!move.queue(3,request)); // Other altar.
    move.preview(target,4,true,true,chest.claim,chest.claimEntity);CHECK(move.queue(4,request));
    CHECK(!move.queue(4,request));CHECK(move.owns(request)); // No duplicate click.
    MoveValidation validation{chest,true,true,true};CHECK(canCommit(request,validation));
    validation.placementAllowed=false;CHECK(!canCommit(request,validation));
    validation.placementAllowed=true;validation.destinationAllowed=false;CHECK(!canCommit(request,validation));
    validation.destinationAllowed=true;validation.actorEligible=false;CHECK(!canCommit(request,validation));
    validation.actorEligible=true;validation.current.entity+=0x100000;
    CHECK(!canCommit(request,validation)); // Recycled entity slot.
    validation.current=chest;validation.current.original.position[2]+=1;
    CHECK(!canCommit(request,validation)); // Source moved after preview.
    validation.current=chest;validation.current.templateId.low+=1;CHECK(!canCommit(request,validation));
    validation.current=chest;validation.current.claim.high+=1;CHECK(!canCommit(request,validation));
    validation.current=chest;validation.current.claimEntity+=1;CHECK(!canCommit(request,validation));
    validation.current=chest;validation.current.session+=1;CHECK(!canCommit(request,validation));
    validation.current=chest;validation.current.available=false;CHECK(!canCommit(request,validation));
    validation.current=chest;
    auto altered=request;altered.destination.scale[0]=2;CHECK(!canCommit(altered,validation));
    altered=request;altered.destination.rotation[0]=std::numeric_limits<float>::quiet_NaN();
    CHECK(!canCommit(altered,validation));
    altered=request;altered.destination.rotation={0,0,0,0};CHECK(!canCommit(altered,validation));
    altered=request;altered.destination.scale[0]=std::numeric_limits<float>::infinity();
    CHECK(!canCommit(altered,validation));
    const auto first=request;
    move.cancel();CHECK(!move.owns(first));CHECK(!move.complete(first.ticket));
    CHECK(move.begin(chest));move.preview(target,5,true,true,chest.claim,chest.claimEntity);CHECK(move.queue(5,request));
    CHECK(request.ticket!=first.ticket);CHECK(!move.complete(first.ticket));CHECK(move.owns(request));
    CHECK(move.complete(request.ticket));CHECK(move.stage()==MoveStage::Idle);
    CHECK(move.begin(chest));CHECK(!move.observe(chest,false)); // Menu/tool/death/cancel.
    CHECK(move.stage()==MoveStage::Idle);
    CHECK(move.begin(chest));auto changed=chest;changed.session+=1;
    CHECK(!move.observe(changed,true)); // Reopening same save is a new session.
    CHECK(move.begin(chest));move.preview(target,9,true,true,chest.claim,chest.claimEntity);move.preview(target,9,true,true,chest.claim,chest.claimEntity);
    CHECK(!move.queue(9,request)); // Replayed frame cannot refresh permission.
    move.preview(target,3,true,true,chest.claim,chest.claimEntity);move.preview(target,4,true,true,chest.claim,chest.claimEntity);
    CHECK(!move.queue(4,request)); // Old samples cannot lower the high-water mark.
    move.cancel();changed=chest;changed.claim={};CHECK(!move.begin(changed));
    changed=chest;changed.claimEntity=0;CHECK(!move.begin(changed));
    changed=chest;changed.movable=false;CHECK(!move.begin(changed));
    move.cancel();CHECK(move.begin(chest));Identity arrival{80,81};
    move.preview(target,100,true,true,arrival,2048);CHECK(move.queue(100,request));
    validation={chest,true,true,true};CHECK(canCommit(request,validation));
    altered=request;altered.destinationClaimEntity=2049;CHECK(!move.owns(altered));
    altered=request;altered.destinationClaim={};CHECK(!canCommit(altered,validation));
    move.suspend();CHECK(move.stage()==MoveStage::Preview&&!move.queue(100,request));
    move.preview(target,101,true,true,arrival,2048);CHECK(move.queue(101,request));
    CHECK(canCommit(request,validation));
    std::puts("Transport move transaction policy tests passed (no native movement performed).");
}

