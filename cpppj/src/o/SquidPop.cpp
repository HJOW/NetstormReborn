// 공간 쓰기 순서와 판본별 좌표/충돌 차이를 보존한다. 미복원 효과는 호출하지 않는다.
#include "o/SquidPop.h"
#include "o/SquidFactory.h"
#include "o/RawSquidActivate.h"
#include "o/RawMetadisplayDirty.h"
#include "o/SquidUnpop.h"
#include "o/Squid.h"
#include "o/SquidDisplay.h"
#include "o/SquidPostPop.h"
#include "o/RawBuildingSpace.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>

namespace netstorm::o {
// 미니맵 교체 중 이전 C++ 처리기의 소멸은 이후 연결한 실제 관측 대여를 덮지 않는다.
bool SquidPop::ReleaseMetadisplayDirty(const RawMetadisplayDirty* expected) { if (!expected || metadisplay_!=expected) return false;metadisplay_=nullptr;return true; }
// 다른 월드의 SID 풀을 사용하는 큐는 구성 단계에서 거부한다.
void SquidPop::SetMetadisplayDirty(const RawMetadisplayDirty* dirty) {
    if (dirty && &dirty->Pool()!=&pool_) throw std::invalid_argument("Pop/미니맵 큐 SID 풀 불일치");
    metadisplay_=dirty;
}
namespace {
// 정렬되지 않은 raw 필드의 원본 위치다.
constexpr std::size_t kNext=4,kIsland=8,kType=10,kState=11,kX=14,kY=18,kScreenX=22,kScreenY=24;
// 상태와 extra 비트다. 아래 CD 전용 경계는 free/transmitting 입력을 사전에 거부한다.
constexpr std::uint8_t kFree=1,kVoid=4,kContained=8,kTransmitting=16,kBuried=8,kFirstPop=128;
// 영역 갱신/부착 효과를 요구하는 타입군과 spot 선택 마스크다.
constexpr std::uint32_t kBuildingMask=0x50444200,kSpotMask=kBuildingMask|0x400ff;
// SetIsland의 무효 번호와 화면 좌표 배율/오프셋이다.
constexpr std::uint16_t kInvalidIsland=127;
constexpr double kScaleX=16,kScaleY=11,kBias=0.5;
#include "PopVtables.inc"
// little endian 필드를 정렬/호스트 엔디언에 의존하지 않고 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 필요한 바이트만 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(i*8);
    return value;
}
// 폭 밖의 기존 payload를 보존하면서 raw 필드를 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 원본 2/4바이트 쓰기를 그대로 옮긴다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
}
// 공유 spot의 크기와 풀 소유자를 확인한다. 표시와 공통 postPop 일부 효과를 선택 연결한다.
SquidPop::SquidPop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,SquidDisplay* display,SquidPostPop* postPop)
    :pool_(pool),hash_(hash),spots_(spots),display_(display),postPop_(postPop) {
    if (spots_.size()!=kWorldCells*kWorldCells) throw std::invalid_argument("Pop spot 지도 크기 오류");
    if (postPop_ && &postPop_->Pool()!=&pool_) throw std::invalid_argument("Pop/postPop SID 풀이 다릅니다");
    if (postPop_) postPop_->ValidateSpace(hash_,spots_);
}
// 표시를 생략한 Pop으로 프레임을 재등록하면 새 영역이 누락되므로 미연결 구성도 거부한다.
void SquidPop::ValidateFrameBinding(const SidPool& pool,const SquidDisplay& display,const SquidUnpop& unpop) const {
    if (&pool!=&pool_ || &display!=display_) throw std::invalid_argument("프레임 Pop 풀/표시 연결 불일치");
    unpop.ValidateFrameBinding(pool,display,hash_,spots_);
}
// 옆면 조회가 원본 공간과 다른 지도에 기록되는 연결을 막는다.
void SquidPop::SetBuildingSpace(const RawBuildingSpace* space) {
    if (space) space->ValidateSpace(pool_,hash_);
    building_=space;
}
// 실제 PE에서 공통 firstPop/postPop을 확인한 vtable만 허용한다.
bool SquidPop::Supports(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags) {
    // 마나는 공통 공간 Pop을 사용한다.134의4429f0 후처리 접두는 SquidPostPop이12 빛 꼬리 생성 뒤 공통 장부로 연결한다.
    if (edition==OriginalEdition::Patch1078 && (vtable==0x50c3b0 || vtable==0x509810 || vtable==0x5070c0 || vtable==0x50c9f0 || vtable==0x50cc08)) return SquidUnpop::SupportsDisplay(edition,vtable,flags); // 1267: 메타/일반 Sprite 두 VT도 PE의 +20=004b0d30, +7c/80/84=41c0b0/4ad0f0/4ad110 공통 쌍이라 공통 후처리를 허용한다(+88만 SupportsDisplay가 구별). // 134도 실제7c/80/84가41c0b0/4ad0f0/4ad110인 정확한 공통 쌍이다.
    const auto values=edition==OriginalEdition::Patch1078 ? std::span<const std::uint32_t>(kPatchPopVtables) :
        std::span<const std::uint32_t>(kCdPopVtables);
    // 선택적으로 켠 표시도 공통 가상 경로만 지원한다.
    for (auto value:values) if (value==vtable) return SquidUnpop::SupportsDisplay(edition,vtable,flags);
    return false;
}
// 전송을생략하는flags에서는미연결writer를검사하지않는다.
void SquidPop::BindTransmission(std::function<void(Sid,std::uint32_t)> transmission) { transmission_=std::move(transmission); }
// 실제 현재 TYPE는 같은 길이의 표를 빌리며 호출 중에는 선택된 장치만 요구한다.
void SquidPop::BindDiagnostics(SquidPopDiagnostics diagnostics) {
    if (!diagnostics.types.empty() && diagnostics.types.size()!=(pool_.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("Pop 현재 TYPE의 판본/길이 오류");
    diagnostics_=std::move(diagnostics);
}
// 실제 World는 호환 인자의 임의 TYPE/SHP 대신 선택 시점의 전체 현재 표/표시기를 사용한다.
RawPopResult SquidPop::PopCurrent(Sid sid,float x,float y,std::uint32_t flags) {
    if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("지연 현재 Pop은10.78 전용");
    return PopPatch(sid,x,y,flags,nullptr,{});
}
// 기존 독립 fixture의 명시 TYPE/치수와 실제 World의 현재 조회를 같은 물리 부모에 연결한다.
RawPopResult SquidPop::PopPatch(Sid sid,float x,float y,std::uint32_t flags,const RiftTypeRecord* fallback,std::array<float,2> size) {
    SquidPopParentHooks effects;
    effects.type=[this,fallback](std::uint8_t number)->const RiftTypeRecord& {
        if (diagnostics_.types.empty()) { if (!fallback) throw std::logic_error("Pop 현재 전체 TYPE 미연결");return *fallback; }
        if (number>=diagnostics_.types.size()) throw std::out_of_range("Pop 현재 TYPE 번호 범위");return diagnostics_.types[number];
    };
    effects.frameSize=[this,fallback,size](Sid object) {
        const auto current=diagnostics_.frameSize;if (current) return current(object);
        if (display_) { const auto raw=pool_.Slot(object);return display_->FrameSize(pool_.Edition(),raw,std::bit_cast<std::int32_t>(Read(raw,36))); }
        if (!fallback) throw std::logic_error("Pop 현재 SHP 조회 미연결");return size; // 독립 입력의 치수는 실제 SHP 장치가 없는 경우에만 사용한다.
    };
    effects.worldCells=[this] { const auto current=diagnostics_.worldCells;return current ? current() : 256; }; // 실제 장기 Client getter 또는 독립 원문 입력만 읽는다.
    effects.metamap=[this] { const auto current=diagnostics_.metamap;return current ? current() : true; }; // 기존 독립 원문 fixture는00594fc8=1의 무로그 입력이다.
    effects.trace=[this](auto level,auto text) { const auto current=diagnostics_.trace;if (!current) throw std::logic_error("Pop 실제 원문 로그 미연결");current(level,text); };
    effects.assertion=[this](auto expression,auto file,auto line) { const auto current=diagnostics_.assertion;if (!current) throw std::logic_error("Pop 실제 원문 진단 미연결 "+std::to_string(line));current(expression,file,line); };
    effects.miniMap=[this](Sid object,std::uint32_t) { if (metadisplay_) metadisplay_->Enqueue(object); }; // 실제 통지 helper도 활성 큐가 없으면 자연 반환한다.
    effects.side=[this](int cx,int cy,float bx,float by) { if (!building_) throw std::logic_error("Pop 실제 건물 옆면 미연결");building_->PopSide(cx,cy,bx,by); };
    effects.update=[this](Sid object,std::uint32_t requested) {
        const auto raw=pool_.Slot(object);if (!SquidUnpop::SupportsDisplay(pool_.Edition(),Read(raw,0),requested)) throw std::logic_error("Pop 현재 파생 표시 갱신 미복원");
        if (display_) display_->Update(raw,requested);
    };
    effects.firstPop=[this](Sid object) {
        const auto raw=pool_.Slot(object);if (!SquidFactory::SupportsConstructedVtable(pool_.Edition(),Read(raw,0))) throw std::logic_error("Pop 현재 가상 firstPop 미복원");
        const auto extra=raw[40];return (extra&9U)==0 ? 1U : ((extra&1U) ? 2U : 0U)|((extra&8U) ? 4U : 0U); // 보호83VT의+30은 모두004ad470이다. TYPE 생성자 쌍을 강제하지 않는다.
    };
    effects.activate=[this,&effects](Sid object,std::uint32_t requested) {
        std::uint32_t isolatedDepth{};auto& depth=postPop_ ? postPop_->ActivationDepth() : isolatedDepth; //독립fixture의명시억제모드만지역깊이를쓴다.
        SquidActivateHooks current;current.server=[this] { return pool_.IsServer(); };current.assertion=effects.assertion;
        current.transmit=[this](Sid sid,std::uint32_t flags) { const auto send=transmission_;if (!send) throw std::logic_error("현재Squid Pop의실제전송미연결");send(sid,flags); };
        current.postPop=[this,&depth](Sid sid,std::uint32_t flags) {
            if (postPop_) postPop_->PostPopVirtual(sid,flags);
            else { if (!Supports(pool_.Edition(),Read(pool_.Slot(sid),0),flags)) throw std::logic_error("Pop현재가상후처리미복원");--depth; } //표시/장부를생략한기존독립구성의억제계약이다.
        };
        RunSquidActivate(pool_,object,requested,depth,current);
    };
    return RunSquidPopParent(pool_,hash_,spots_,sid,x,y,flags,diagnostics_.checking,effects);
}
// PATCH는 현재 원문 부모에 위임하고 CD는 기존 등록/지원 경계를 유지한다.
RawPopResult SquidPop::Pop(Sid sid,const RiftTypeRecord& type,float frameWidth,float frameHeight,
    float x,float y,std::uint32_t flags) {
    if (pool_.Edition()==OriginalEdition::Patch1078) return PopPatch(sid,x,y,flags,&type,{frameWidth,frameHeight});
    const auto old=pool_.Slot(sid);
    const bool patch=false; // 아래는 기존 CD 전용 구현이며 PATCH 부모의 현재 진단을 섞지 않는다.
    if (!sid.value || (old[kState]&kVoid)==0 || (old[kState]&kContained)!=0) {
        if (patch) return RawPopResult::Unchanged;
        throw std::logic_error("CD Pop은 유효한 void 객체가 필요합니다");
    }
    if ((old[kState]&(kFree|kTransmitting)) || old[kType]<kFirstAssetTypeNumber)
        throw std::logic_error("Pop raw 자산 상태 오류");
    if (!std::isfinite(x) || !std::isfinite(y)) throw std::invalid_argument("Pop 유한 좌표가 필요합니다");
    // CD 디컴파일의 정수 비교는 float 비트값의 signed 비교다. 양수 소수 좌표도 그대로 보존한다.
    if (x<=0 || y<=0 || x>=kWorldCells || y>=kWorldCells) x=y=10;
    const auto extraOffset=patch ? 40U : 35U,levelOffset=patch ? 33U : 31U;
    const auto extra=old[extraOffset]; const bool buried=(extra&kBuried)!=0;
    // 비전투 null dirty 경로와 별개로 건물 옆면은 실제 공유 표면 몸체를 요구한다.
    if (!buried && (type.flags2&kBuildingMask) && !building_)
        throw std::logic_error("Pop 건물 부착 공간이 연결되지 않았습니다");
    // 다리와 섬 받침은 전용 postPop의 접두 동작과 연결 효과를 공급한 경우에만 허용한다. 기존 공통 가상 표 목록은 바꾸지 않는다.
    if (!Supports(pool_.Edition(),Read(old,0),flags) &&
        !(postPop_ && postPop_->HandlesDerived(sid) && SquidUnpop::SupportsDisplay(pool_.Edition(),Read(old,0),flags)))
        // 미복원 분배를 실제 저장 객체와 연결해 분석할 수 있도록 현재 SID/타입/가상 표를 남긴다.
        throw std::logic_error("Pop 파생 가상 후처리는 아직 지원하지 않습니다: SID"+std::to_string(sid.value)+
            " TYPE"+std::to_string(old[kType])+" VT"+std::to_string(Read(old,0)));
    const auto level=SquidHash::ObjectLevel(type.flags2,frameWidth,frameHeight);
    auto& head=hash_.Bucket(level,x,y);
    if (head==sid.value || head>=pool_.Capacity()) throw std::logic_error("Pop 해시 머리 오류");
    const bool writesSpots=!buried && ((type.flags2&kSpotMask)!=0 || (type.flags1&TypeFlag1::kSurface)!=0);
    const int right=static_cast<int>(x),bottom=static_cast<int>(y);
    int left=right,top=bottom;
    if (writesSpots) {
        // 실제 Dude의 발자국0은 left/top이 끝 칸을 넘어 아래 y/x 루프를 빈 순회로 만든다.
        if (type.footX<0 || type.footY<0 || type.footX>kWorldCells || type.footY>kWorldCells)
            throw std::logic_error("Pop 발자국 크기 오류");
        left=right-type.footX+1; top=bottom-type.footY+1;
        if (left<1 || top<1) throw std::out_of_range("Pop 지도 밖 발자국");
    }
    if (display_) display_->Validate(pool_.Edition(),old,flags);
    // 최초 등록/비전투 Activate가 요구할 후처리를 공간 쓰기 전에 검사한다.
    if (postPop_) {
        auto normalized=flags;
        if ((extra&kFirstPop)==0) normalized|=(extra&9)==0 ? 1U : ((extra&1)!=0 ? 2U : 0U)|((extra&8)!=0 ? 4U : 0U);
        if ((normalized&8)==0) normalized|=0x10;
        const RawGraphPop projected{x,y,level,&type};
        postPop_->Validate(sid,normalized,&projected);
    }
    auto bytes=pool_.AllocatedBytes(sid);
    Put(bytes,kX,std::bit_cast<std::uint32_t>(x),4); Put(bytes,kY,std::bit_cast<std::uint32_t>(y),4);
    Put(bytes,kScreenX,static_cast<std::uint32_t>(static_cast<double>(x)*kScaleX+kBias),2);
    Put(bytes,kScreenY,static_cast<std::uint32_t>(static_cast<double>(y)*kScaleY+kBias),2);
    // 원본 00481510/CD00442f30은 spot 겹침으로 반환하기 전의 새 좌표도 고유 큐에 남긴다.
    if (!buried && (type.flags2&6) && metadisplay_) metadisplay_->Enqueue(sid);
    if (writesSpots) {
        // 패치 overlap의 조기 반환 앞에 기록한 좌표와 앞선 칸은 되돌리지 않는다.
        for (int cy=top;cy<=bottom;++cy) {
            // 원본의 y/x 순서로 effective genus의 low byte를 OR한다.
            for (int cx=left;cx<=right;++cx) {
                const auto genus=static_cast<std::uint8_t>(Squid::EffectiveGenus(type.flags2,x,y,type.footX,type.footY,cx,cy));
                auto& spot=spots_[static_cast<std::size_t>(cy*kWorldCells+cx)];
                if (genus) {
                    if (patch && (spot&genus)) return RawPopResult::Overlap;
                    spot=static_cast<std::uint8_t>(spot|genus);
                }
                // 겹침에서 앞서 반환한 칸은 통지하지 않는다. 양 옆면의 모서리는 제외한다.
                if ((type.flags2&kBuildingMask) && (cx==left || cx==right) && cy!=top && cy!=bottom)
                    building_->PopSide(cx,cy,x,y);
            }
        }
    }
    bytes[levelOffset]=static_cast<std::uint8_t>(level); Put(bytes,kNext,head,2); head=sid.value;
    if (level==0 && (type.flags2&TypeFlag2::kBridge)==0) Put(bytes,kIsland,kInvalidIsland,2);
    // 원본은 공간 머리 등록 뒤, firstPop/void 해제 전에 공통 표시 갱신을 호출한다.
    if (display_) display_->Update(bytes,flags);
    if ((extra&kFirstPop)==0) {
        flags|=(extra&9)==0 ? 1U : ((extra&1)!=0 ? 2U : 0U)|((extra&8)!=0 ? 4U : 0U);
        bytes[extraOffset]|=kFirstPop;
    }
    bytes[kState]=static_cast<std::uint8_t>(bytes[kState]&~kVoid);
    // 비전투 Activate가 전달하는 플래그다. 연결된 공통 postPop의 깊이/효과를 갱신한다.
    if ((flags&8)==0) flags|=0x10;
    if (postPop_) postPop_->Activate(sid,flags);
    return RawPopResult::Registered;
}
}
