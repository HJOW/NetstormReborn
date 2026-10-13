// 각 판본의 중립 조건과 float/double 거리 저장 경계를 보존한다.
#include "o/RawPriestArrival.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kReplaced=1001,kCaptureMask=32,kPriestNoIsland=127; // 사제 자기삭제/회피 상태와 원본 contained/지역 값이다.
constexpr double kNearLimit=1.0; // 양 PE의005012d0/CD00500398 실제 float1.0 경계다.
// 원본 비정렬 raw DWORD/WORD를 지정 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정 필드 밖의 stale 바이트는 섞지 않는다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(8*index);
    return value;
}
// 현재 plain 좌표를 원본0방향 절삭의 정상 signed32 범위에서 정수로 바꾼다.
int Coordinate(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-2147483648.0 || static_cast<double>(value)>=2147483648.0) throw std::out_of_range("사제 도착 현재 좌표 범위");
    return static_cast<int>(value);
}
}
// 최초 조회 전에 실제 같은 월드/필수 가상 경계를 확인한다.
RawPriestArrival::RawPriestArrival(const SidPool& pool,const RawPriestState& priest,const SquidReward& hitPoints,
    const RawCanonPlacementTerrain& terrain,const RawIslandRuntime& islands,const RawSquidNeighbors& footprint,
    const RawContainedCreate& create,const RawCarrierArrival& carrier,PriestArrivalState& state,PriestArrivalHooks hooks)
    :pool_(pool),priest_(priest),hitPoints_(hitPoints),terrain_(terrain),islands_(islands),footprint_(footprint),create_(create),carrier_(carrier),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&priest.Pool() || &pool!=&hitPoints.Pool() || &pool!=&terrain.Pool() || &pool!=&islands.Pool() || &pool!=&footprint.Pool() || &pool!=&create.Pool() || &pool!=&carrier.Pool() || !hooks_.destroy || !hooks_.neutralReturn)
        throw std::invalid_argument("사제 도착 실제 월드/하위 효과 연결 누락");
}
// 현재 raw float 두 개만 읽는다.
std::array<float,2> RawPriestArrival::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 중립0과 실제 owner BYTE를 효과 뒤 다시 읽는다.
std::uint32_t RawPriestArrival::Owner(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]; }
// 실제 CD0040c3d0은 loading부터 검사하며 패치 wrapper도 같은 세 입력을 읽는다.
bool RawPriestArrival::NeutralEligible(Sid sid) const { return !state_.loading && !Owner(sid) && !state_.suppressNeutralReturn; }
// 현재 지면이 없으면 sentinel127→Temple0이며 실제 섬 getter를 재사용한다.
Sid RawPriestArrival::TempleAt(float x,float y) const {
    const auto island=terrain_.RegionAt(Coordinate(x),Coordinate(y));if (island==kPriestNoIsland) return {};const auto temple=islands_.Temple(std::bit_cast<std::int32_t>(island));if (temple>=pool_.Capacity()) throw std::out_of_range("사제 도착 현재 섬 Temple SID");return Sid{static_cast<std::uint16_t>(temple)};
}
// 원본 거리 helper는 발자국 안에서도 음수거리를 그대로 유지하고 Euclid 길이를 쓰지 않는다.
bool RawPriestArrival::NearTemple(Sid sid,Sid temple) const {
    const auto point=Position(sid);const auto bounds=footprint_.Footprint(temple);const float x=point[0]<bounds.left ? static_cast<float>(static_cast<double>(bounds.left)-point[0]) : static_cast<float>(static_cast<double>(point[0])-bounds.right);double y=point[1]<bounds.top ? static_cast<double>(bounds.top)-point[1] : static_cast<double>(point[1])-bounds.bottom;
    if (pool_.Edition()==OriginalEdition::Cd1072) y=static_cast<float>(y);return (y<=static_cast<double>(x) ? static_cast<double>(x) : y)<=kNearLimit;
}
// 저HP중립의 현재 섬복귀/내용물 생성→자기삭제, 절반이상중립 귀환, 일반 Carrier 위임 순서다.
std::uint32_t RawPriestArrival::Arrival(Sid sid,std::uint32_t targetA,std::uint32_t targetB) const {
    if (state_.boss) {
        if (priest_.CurrentHitPoints(sid)<hitPoints_.MaxHitPoints(sid) && NeutralEligible(sid)) {
            const auto point=Position(sid);const auto temple=TempleAt(point[0],point[1]);
            if (temple.value && Owner(temple)==Owner(sid) && NearTemple(sid,temple)) { const auto raw=pool_.Slot(sid);create_.Create(temple,raw[10],Read(raw,12,2),kCaptureMask,0,0);hooks_.destroy(sid,0);return kReplaced; }
        }
    }
    if (state_.boss && NeutralEligible(sid) && priest_.CurrentHitPoints(sid)>=hitPoints_.MaxHitPoints(sid)/2) { hooks_.neutralReturn(sid);return kReplaced; }
    return carrier_.Arrival(sid,targetA,targetB);
}
// 실제 부모 path가 같은 풀의 사제 wrapper인지 검증할 때 사용한다.
const SidPool& RawPriestArrival::Pool() const { return pool_; }
}
