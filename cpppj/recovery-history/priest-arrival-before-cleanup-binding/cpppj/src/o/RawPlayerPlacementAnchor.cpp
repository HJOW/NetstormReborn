// 임시 목록은 중복을 지우지 않으며 가득 찬 뒤에도 원본의 종속/그래프 조회 순서를 유지한다.
#include "o/RawPlayerPlacementAnchor.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 임시 목록 상한과 거리 선택의 단정도 초기 최소값이다.
constexpr std::size_t kLimit=40;
constexpr float kInitialDistance=9999.0F;
// 디컴파일에 생략된 fadd DWORD 상수(00500edc / CD 00502418)다.
constexpr float kCoordinateBias=0.9999F;
// 공통 raw 타입/좌표·종속 kind/mask의 오프셋이다.
constexpr std::size_t kType=10,kX=14,kY=18,kKind=18,kMask=20;
// 비정렬 raw 좌표의 little endian 비트값을 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 낮은 바이트부터 조립하고 NaN/부호 있는 0은 그대로 전달한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(value);
}
// 종속 조회가 쓰는 비정렬 WORD를 읽는다.
std::uint16_t Word(std::span<const std::uint8_t> raw,std::size_t offset) {
    return static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8));
}
// x87의 좌표+float 상수는 double에서 계산한 뒤 float으로 좁히지 않고 CRT 절삭한다.
int Truncate(float value) {
    const double result=std::trunc(static_cast<double>(value)+static_cast<double>(kCoordinateBias));
    if (!std::isfinite(result) || result<std::numeric_limits<int>::min() || result>std::numeric_limits<int>::max())
        throw std::out_of_range("Player 그래프 좌표 절삭 오류");
    return static_cast<int>(result);
}
}
// 현재 타입과 surface 지도는 복사하지 않으며 공통 장부의 소유자별 작업장 목록을 공유한다.
RawPlayerPlacementAnchor::RawPlayerPlacementAnchor(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const std::uint16_t> surfaces,const SquidPostPopState& bookkeeping,const SquidPostPopList& additional,
    const ContainedFinderState& contained,const PlayerPlacementAnchorState& state)
    :pool_(pool),types_(types),surfaces_(surfaces),bookkeeping_(bookkeeping),additional_(additional),contained_(contained),state_(state) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || surfaces.size()!=65536)
        throw std::invalid_argument("Player 배치 기준점 자료 크기 오류");
}
// 손상된 DWORD 목록 번호를 임의의 다른 WORD SID로 감지 않는다.
std::span<const std::uint8_t> RawPlayerPlacementAnchor::Raw(std::uint32_t sid) const {
    if (sid>std::numeric_limits<std::uint16_t>::max() || sid>=pool_.Capacity()) throw std::out_of_range("Player 기준점 SID 오류");
    return pool_.Slot(Sid{static_cast<std::uint16_t>(sid)});
}
// CD getter는 타입 assert 없이 raw graph를 읽으며 패치만 현재 타입 조건을 확인한다.
std::uint32_t RawPlayerPlacementAnchor::RawGraphByte(std::uint32_t sid) const {
    const auto raw=Raw(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch && (raw[kType]>=types_.size() || (!(types_[raw[kType]].flags1&0x800) && raw[kType]!=state_.fenceType && raw[kType]!=state_.windArcherType)))
        throw std::invalid_argument("Player 표면 graph getter 타입 오류");
    return raw[patch ? 30 : 28];
}
// 그래프 비활성은 좌표를 읽기 전에 고정 무효 값 254로 반환한다.
std::uint32_t RawPlayerPlacementAnchor::GraphAt(float x,float y) const {
    if (!state_.graphReady) return 254;
    const int px=Truncate(x),py=Truncate(y);if (px<0 || py<0 || px>255 || py>255) return 254;
    const auto sid=surfaces_[static_cast<std::size_t>(py)*256+static_cast<std::size_t>(px)];return sid ? RawGraphByte(sid) : 254;
}
// surface 타입은 그래프 활성 여부와 무관하게 자신의 raw 번호를 사용한다.
std::uint32_t RawPlayerPlacementAnchor::GraphFor(std::uint32_t sid) const {
    if (!sid) {
        if (pool_.Edition()==OriginalEdition::Patch1078 && contained_.checkingDead) throw std::invalid_argument("Player graph SID 0 assert");
        return 0;
    }
    const auto raw=Raw(sid);if (raw[kType]>=types_.size()) throw std::out_of_range("Player graph 타입 오류");
    return types_[raw[kType]].flags1&0x800 ? RawGraphByte(sid) : GraphAt(Coordinate(raw,kX),Coordinate(raw,kY));
}
// 기존 contained 커서를 사용하여 저장된 다음 링크와 현재 타입 전역을 유지한다.
bool RawPlayerPlacementAnchor::HasContained(std::uint32_t parent,std::uint32_t kind) const {
    static_cast<void>(Raw(parent));RawContainedFinder finder(pool_,contained_);
    // 일치 타입 후보만 요청 kind와 mask로 검사하며 인덱스는 항상 0이다.
    for (Sid sid=finder.Begin(Sid{static_cast<std::uint16_t>(parent)});sid.value;sid=finder.Next()) {
        const auto raw=pool_.Slot(sid);if ((!kind || Word(raw,kKind)==kind) && (Word(raw,kMask)&0x10)) return true;
    }
    return false;
}
// 거리 중간값은 double로 유지하고 채택한 최소값만 원본처럼 float으로 저장한다.
std::uint32_t RawPlayerPlacementAnchor::Nearest(std::span<const std::uint32_t> candidates,std::uint32_t owner,float x,float y) const {
    float best=kInitialDistance;std::uint32_t selected=0;
    // 현재 raw 소유자와 좌표를 목록 순서대로 읽는다. 동률도 float 최소값과 비교한다.
    for (const auto sid:candidates) {
        const auto raw=Raw(sid);if (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]!=owner) continue;
        const double dx=static_cast<double>(x)-Coordinate(raw,kX),dy=static_cast<double>(y)-Coordinate(raw,kY);
        const double distance=std::sqrt(dx*dx+dy*dy);
        // CD는 C0만 검사하여 unordered(NaN)도 채택한다. 패치는 C0/C2를 함께 검사해 제외한다.
        const bool unordered=std::isnan(distance) || std::isnan(best);
        if (distance<static_cast<double>(best) || (pool_.Edition()==OriginalEdition::Cd1072 && unordered)) {
            best=static_cast<float>(distance);selected=sid;
        }
    }
    return selected;
}
// type와 owner는 실제로 읽는 시점에 검사하여 무효 그래프의 조기 반환을 보존한다.
PlayerAnchorSelection RawPlayerPlacementAnchor::Inspect(std::uint32_t owner,std::uint32_t type,float x,float y) const {
    PlayerAnchorSelection result;const auto graph=GraphAt(x,y);if (graph==state_.invalidGraph) return result;
    if (owner>=bookkeeping_.ownerFactories.size()) throw std::out_of_range("Player 기준점 소유자 오류");
    const auto& workshops=bookkeeping_.ownerFactories[owner];bool linked=false;
    // 목록을 매번 다시 읽고 중복·원래 순서·상한 이후의 조회까지 유지한다.
    for (std::size_t i=0;i<workshops.Items().size();++i) {
        const auto sid=workshops.Items()[i];static_cast<void>(Raw(sid));
        if (type>=types_.size()) throw std::out_of_range("Player 기준점 요청 타입 오류");
        bool allowed=(types_[type].flags2&6)!=0;
        if (!allowed) { allowed=HasContained(sid,type);linked=linked || allowed;if (!allowed) allowed=state_.ignoreRestrictions!=0; }
        if (allowed && GraphFor(sid)==graph && result.candidates.size()<kLimit) result.candidates.push_back(sid);
    }
    if (!result.candidates.empty() || linked) {
        // 추가 목록은 현재 raw owner가 같은 것만 받고 그래프 조회 뒤 40개 상한을 적용한다.
        for (std::size_t i=0;i<additional_.Items().size();++i) {
            const auto sid=additional_.Items()[i];const auto raw=Raw(sid);
            if (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]==owner && GraphFor(sid)==graph && result.candidates.size()<kLimit)
                result.candidates.push_back(sid);
        }
    }
    if (result.candidates.size()==1) result.sid=result.candidates.front();
    else if (!result.candidates.empty()) result.sid=Nearest(result.candidates,owner,x,y);
    return result;
}
// 임시 목록은 호출마다 새로 만들고 원본 DWORD 반환만 권한 함수에 넘긴다.
std::uint32_t RawPlayerPlacementAnchor::Locate(std::uint32_t owner,std::uint32_t type,float x,float y) const { return Inspect(owner,type,x,y).sid; }
// 연결할 권한 모듈의 실제 raw 풀을 제공한다.
const SidPool& RawPlayerPlacementAnchor::Pool() const { return pool_; }
// 범위와 필수 경계를 확인한 실제 조회 함수를 수명 참조로 연결한다.
std::function<std::uint32_t(std::uint32_t,std::uint32_t,float,float)> MakePlayerAnchorQuery(const SidPool& pool,const RawPlayerPlacementAnchor& anchor) {
    if (&pool!=&anchor.Pool()) throw std::invalid_argument("Player 기준점/권한 SID 풀 불일치");
    return [&anchor](std::uint32_t owner,std::uint32_t type,float x,float y) { return anchor.Locate(owner,type,x,y); };
}
}
