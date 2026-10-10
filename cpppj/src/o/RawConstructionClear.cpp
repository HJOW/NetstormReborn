// 로컬 예측 조각 정리의 칸 범위 계산·후보 조건·같은 조각 판정을 원본 순서대로 유지한다.
#include "o/RawConstructionClear.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본 공통 raw 필드: 타입 바이트의 위치다.
constexpr std::size_t kType=10;
// 판본별 abstract 비트 바이트, 소유자 바이트, 프레임 필드(패치 DWORD / CD BYTE)의 위치다.
constexpr std::size_t kPatchExtra=40,kCdExtra=35,kPatchOwner=34,kCdOwner=32,kPatchFrame=36,kCdFrame=34;
// 일반 객체의 첫 번호다. 후보는 이 번호 이상 서버 영역 첫 번호 미만(클라이언트 영역)이어야 한다.
constexpr std::uint16_t kFirstSid=5;
// 좌표를 자르는 범위(패치 005012d0·005022e8, CD 00501764·00501768)와 좌표 유효성의 상한(지도 크기 256)이다.
constexpr float kClampLow=1.0F,kClampHigh=255.0F,kMapLimit=256.0F;
// 큰 쪽 모서리를 올림할 때 더하는 값(패치 00502310, CD 005017a0)이다.
constexpr float kRoundUpBias=0.9999899864196777F;
// 원본 __ftol: 0방향으로 절삭한 64비트 정수의 하위 32비트다. 유한하지 않거나 64비트 범위 밖이면 하위 DWORD가 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 0041d0d0 / CD 0043f950: 좌표 하나를 1~255로 자른다. 먼저 아래를 올리고 그 다음 위를 내린다.
float Clamp(float value) {
    const float raised=kClampLow>value ? kClampLow : value;
    return kClampHigh<raised ? kClampHigh : raised;
}
// 0040e800: 두 좌표가 모두 0 초과 지도 크기 미만인지 본다.
bool Valid(float x,float y) { return 0.0F<x && x<kMapLimit && 0.0F<y && y<kMapLimit; }
// 정렬되지 않은 little endian 필드를 폭만큼 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
}
// 필수 경계는 첫 효과 전에 검사한다.
RawConstructionClear::RawConstructionClear(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,const ConstructionClearState& state,ConstructionClearHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.destroy) throw std::invalid_argument("건설 예측 정리의 삭제 효과 연결 오류");
}
// 칸 좌표가 유효하면 두 모서리를 자른 점과 비교해 넓히고, 유효하지 않으면 두 모서리 모두 자른 점이 된다.
SquidSearchArea RawConstructionClear::CellArea(float x,float y,int footX,int footY) {
    // 발자국만큼 왼쪽/위로 물린 점을 x87처럼 넓은 정밀도로 구한 뒤 단정도로 저장하고 1~255로 자른다.
    const float pointX=Clamp(static_cast<float>(static_cast<double>(x)-(static_cast<double>(footX)-1.0)));
    const float pointY=Clamp(static_cast<float>(static_cast<double>(y)-(static_cast<double>(footY)-1.0)));
    float left=pointX,top=pointY,right=pointX,bottom=pointY;
    if (Valid(x,y)) {
        left=x<pointX ? x : pointX;top=y<pointY ? y : pointY;
        right=x>pointX ? x : pointX;bottom=y>pointY ? y : pointY;
    }
    // 큰 쪽은 0.99999를 더한 값을 단정도로 좁히지 않고 절삭한다.
    return {Ftol(static_cast<double>(left)),Ftol(static_cast<double>(top)),
        Ftol(static_cast<double>(right)+static_cast<double>(kRoundUpBias)),Ftol(static_cast<double>(bottom)+static_cast<double>(kRoundUpBias))};
}
// 원본 순서: decoder 생성 → 칸마다 (범위 계산 → 탐색 → 첫 후보 판정/삭제) → 다음 칸.
void RawConstructionClear::Clear(const ConstructionClearRequest& request) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (request.type>=types_.size() || request.type>=frames_.size() || request.type>0xff) throw std::out_of_range("건설 예측 정리 타입 번호 오류");
    const auto& record=types_[request.type];const auto& meta=frames_[request.type];
    auto decoder=DecodeCanonType(meta.frames,meta.defaultFrame,state_.patternTypes,{request.type,std::bit_cast<int>(request.argument),
        std::bit_cast<int>(request.direction),request.x,request.y,false},pool_.Edition());
    const std::size_t extraOffset=patch ? kPatchExtra : kCdExtra,ownerOffset=patch ? kPatchOwner : kCdOwner;
    const std::uint32_t serverFirst=pool_.Layout().serverFirst;
    const auto codes=meta.frames.Codes();
    // 유효 칸마다 그 칸 범위에서 지울 후보를 하나 찾는다.
    for (;decoder.Valid();decoder.Advance()) {
        const auto area=CellArea(decoder.X(),decoder.Y(),record.footX,record.footY);
        if (hooks_.scan) hooks_.scan(area);
        RawSquidFinder finder(pool_,hash_,types_);
        // 탐색기가 돌려주는 순서대로 후보 조건을 본다. 조건에 맞는 첫 객체를 지우고 이 칸을 끝낸다.
        for (Sid current=finder.Begin(area);current.value!=0;current=finder.Next()) {
            const auto raw=pool_.Slot(current);
            const std::uint32_t type=raw[kType];
            if (!(raw[extraOffset]&1) || current.value<kFirstSid || current.value>=serverFirst || (type!=request.type && state_.authority)) continue;
            std::uint32_t flags=0;
            if (type==request.type) {
                // 프레임이 같거나, 다리 타입이면서 방향 글자가 같으면 통지와 같은 조각이다. 패치는 DWORD, CD는 BYTE 프레임을 비교한다.
                const std::uint32_t frame=Read(raw,patch ? kPatchFrame : kCdFrame,patch ? 4 : 1);
                bool same=frame==std::bit_cast<std::uint32_t>(decoder.Frame());
                if (!same && type==state_.bridgeType) {
                    if (frame>=codes.size()) throw std::out_of_range("건설 예측 정리 후보의 프레임 범위 오류");
                    same=static_cast<std::int8_t>(codes[frame].side)==static_cast<std::int8_t>(decoder.Side());
                }
                // 같은 조각이라도 소유자가 통지의 플레이어와 같을 때만 같은 조각 플래그로 지운다.
                if (same && static_cast<std::uint32_t>(raw[ownerOffset])==request.player) flags=kSamePieceFlag;
            }
            hooks_.destroy(current,flags);
            break;
        }
    }
}
// 호출자가 같은 풀을 쓰는지 확인할 수 있게 실제 풀을 돌려준다.
const SidPool& RawConstructionClear::Pool() const { return pool_; }
}
