#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace carryon::transport {
// Values only: no component pointers may cross from the client to the server.
struct Identity {
    uint64_t low=0,high=0;
    explicit operator bool() const noexcept {return low||high;}
    bool operator==(const Identity& b) const noexcept {return low==b.low&&high==b.high;}
};
struct Placement {
    std::array<int64_t,3> position{}; // Native 32.32 world coordinates.
    std::array<float,4> rotation{0,0,0,1};
    std::array<float,3> scale{1,1,1};
    bool valid() const noexcept {
        double length=0;
        for(auto f:rotation){if(!std::isfinite(f))return false;length+=double(f)*f;}
        if(std::abs(length-1.0)>0.001)return false;
        for(auto f:scale)if(!std::isfinite(f)||f<=0)return false;
        return true;
    }
    bool operator==(const Placement& b) const noexcept {
        return position==b.position&&rotation==b.rotation&&scale==b.scale;
    }
    bool movable() const noexcept {
        return valid()&&scale[0]==scale[1]&&scale[0]==scale[2];
    }
};
struct MoveSource {
    uint64_t session=0;
    uint32_t entity=0; // Native ID including generation, on authoritative world.
    Identity templateId{},claim{}; // Template + session/entity identify this source.
    uint32_t claimEntity=0; // Keep the selected altar, even if claims overlap.
    Placement original{};
    bool movable=false,available=false;
};
struct MoveRequest {
    uint64_t ticket=0;
    MoveSource source{};
    Placement destination{};
    Identity destinationClaim{};
    uint32_t destinationClaimEntity=0;
};
// These facts must be freshly resolved on the server thread, immediately before
// changing the existing entity. Client flags are not authority to commit.
struct MoveValidation {
    MoveSource current{};
    bool destinationAllowed=false; // Full bounds in the exact requested destination claim.
    bool placementAllowed=false; // Native collision/support/permission checks.
    bool actorEligible=false;
};
inline bool sameSource(const MoveSource& a,const MoveSource& b) noexcept {
    return a.session==b.session&&a.entity==b.entity&&a.templateId==b.templateId&&
        a.claim==b.claim&&a.claimEntity==b.claimEntity&&a.original==b.original&&b.movable&&b.available;
}
inline bool canCommit(const MoveRequest& r,const MoveValidation& v) noexcept {
    return r.ticket&&r.source.session&&r.source.entity&&r.source.templateId&&r.source.claim&&r.source.claimEntity&&
        r.source.movable&&r.source.available&&r.source.original.movable()&&
        r.destination.valid()&&r.destination.scale==r.source.original.scale&&
        r.destinationClaim&&r.destinationClaimEntity&&sameSource(r.source,v.current)&&v.destinationAllowed&&v.placementAllowed&&v.actorEligible;
}
enum class MoveStage {Idle,Preview,Queued};
class MoveTransaction {
    MoveStage stage_=MoveStage::Idle;
    uint64_t serial_=0,previewFrame_=0;
    MoveRequest request_{};
    bool previewValid_=false;
public:
    MoveStage stage() const noexcept {return stage_;}
    const MoveRequest& request() const noexcept {return request_;}
    void cancel() noexcept {
        stage_=MoveStage::Idle;request_={};previewFrame_=0;previewValid_=false;
        // Never reset serial: an acknowledgement from an old operation must
        // not complete a new operation, including after a world transition.
    }
    bool begin(const MoveSource& source) noexcept {
        if(stage_!=MoveStage::Idle||!source.session||!source.entity||!source.templateId||
           !source.claim||!source.claimEntity||!source.movable||!source.available||!source.original.movable()||
           serial_==(std::numeric_limits<uint64_t>::max)())return false;
        request_={++serial_,source,source.original,source.claim,source.claimEntity};stage_=MoveStage::Preview;return true;
    }
    bool observe(const MoveSource& current,bool eligible) noexcept {
        if(stage_==MoveStage::Idle)return false;
        if(!eligible||!sameSource(request_.source,current)){cancel();return false;}
        return true;
    }
    void preview(const Placement& destination,uint64_t frame,bool nativeValid,bool allowed,Identity claim,uint32_t altar) noexcept {
        if(stage_!=MoveStage::Preview)return;
        previewValid_=frame&&frame>previewFrame_&&nativeValid&&allowed&&claim&&altar&&destination.valid()&&
            destination.scale==request_.source.original.scale;
        // An invalid sample invalidates the previous preview; never fall back
        // to the last green position when the cursor now points somewhere red.
        if(frame>previewFrame_)previewFrame_=frame;
        request_.destination=destination;request_.destinationClaim=claim;request_.destinationClaimEntity=altar;
    }
    void suspend() noexcept {
        if(stage_!=MoveStage::Idle){stage_=MoveStage::Preview;previewValid_=false;}
    }
    bool queue(uint64_t currentFrame,MoveRequest& out) noexcept {
        if(stage_!=MoveStage::Preview||!previewValid_||currentFrame!=previewFrame_)return false;
        stage_=MoveStage::Queued;previewValid_=false;out=request_;return true;
    }
    bool owns(const MoveRequest& r) const noexcept {
        return stage_==MoveStage::Queued&&r.ticket==request_.ticket&&
            sameSource(r.source,request_.source)&&r.destination==request_.destination&&r.destinationClaim==request_.destinationClaim&&
            r.destinationClaimEntity==request_.destinationClaimEntity;
    }
    bool complete(uint64_t ticket) noexcept {
        if(stage_!=MoveStage::Queued||ticket!=request_.ticket)return false;
        cancel();return true;
    }
};
// The adapter must serialize cancel/owns/revalidation/commit under one lock.
// No native write is implemented here: a request is not a completed move.
}

