// 공간 쓰기 순서와 판본별 좌표/충돌 차이를 보존한다. 미복원 효과는 호출하지 않는다.
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"
#include "o/Squid.h"
#include "o/SquidDisplay.h"
#include "o/SquidPostPop.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 정렬되지 않은 raw 필드의 원본 위치다.
constexpr std::size_t kNext=4,kIsland=8,kType=10,kState=11,kX=14,kY=18,kScreenX=22,kScreenY=24;
// 상태와 extra 비트다. free/transmitting 입력은 원본 assert 경로 대신 사전에 거부한다.
constexpr std::uint8_t kFree=1,kVoid=4,kContained=8,kTransmitting=16,kBuried=8,kFirstPop=128;
// 영역 갱신/부착 효과를 요구하는 타입군과 spot 선택 마스크다.
constexpr std::uint32_t kBuildingMask=0x50444200,kIslandBridge=6,kSpotMask=kBuildingMask|0x400ff;
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
// 실제 PE에서 공통 firstPop/postPop을 확인한 vtable만 허용한다.
bool SquidPop::Supports(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags) {
    const auto values=edition==OriginalEdition::Patch1078 ? std::span<const std::uint32_t>(kPatchPopVtables) :
        std::span<const std::uint32_t>(kCdPopVtables);
    // 선택적으로 켠 표시도 공통 가상 경로만 지원한다.
    for (auto value:values) if (value==vtable) return SquidUnpop::SupportsDisplay(edition,vtable,flags);
    return false;
}
// 비전투 좌표→spot→체인→공통 표시→firstPop→Activate와 선택한 postPop 효과를 처리한다.
RawPopResult SquidPop::Pop(Sid sid,const RiftTypeRecord& type,float frameWidth,float frameHeight,
    float x,float y,std::uint32_t flags) {
    const auto old=pool_.Slot(sid);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
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
    if (!buried && (type.flags2&(kIslandBridge|kBuildingMask)))
        throw std::logic_error("Pop 영역/부착 표면 효과는 아직 지원하지 않습니다");
    if (!Supports(pool_.Edition(),Read(old,0),flags))
        throw std::logic_error("Pop 파생 가상 후처리는 아직 지원하지 않습니다");
    const auto level=SquidHash::ObjectLevel(type.flags2,frameWidth,frameHeight);
    auto& head=hash_.Bucket(level,x,y);
    if (head==sid.value || head>=pool_.Capacity()) throw std::logic_error("Pop 해시 머리 오류");
    const bool writesSpots=!buried && ((type.flags2&kSpotMask)!=0 || (type.flags1&TypeFlag1::kSurface)!=0);
    const int right=static_cast<int>(x),bottom=static_cast<int>(y);
    int left=right,top=bottom;
    if (writesSpots) {
        if (type.footX<1 || type.footY<1 || type.footX>kWorldCells || type.footY>kWorldCells)
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
