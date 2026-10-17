// 연결선 효과 뒤의 현재 좌표/소유자, 갱신 뒤의 현재 dead/타입을 원본처럼 다시 읽는다.
#include "o/RawFenceLifetime.h"
#include <cstring>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 실제 선택된 효과가 빠졌을 때 빈 처리로 넘어가지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("실제 펜스 수명 효과 미연결");return function; }
// 비정렬 float 좌표의 비트를 그대로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) { float value{};std::memcpy(&value,raw.data()+offset,4);return value; }
}
// 다른 판본의 raw 오프셋을 적용하지 않는다.
RawFenceLifetime::RawFenceLifetime(SidPool& pool,FenceLifetimeHooks hooks):pool_(pool),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("펜스 수명10.78 풀 계약 오류"); }
// Refresh의 포인터 유효성 검사에서 SID1도 허용하던 원본 범위를 유지한다.
std::span<const std::uint8_t> RawFenceLifetime::Raw(Sid sid) const { if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("펜스 수명 SID 범위 오류");return pool_.Slot(sid); }
// 앞선 표식 삭제/재생성이 좌표를 바꿀 수 있으므로 호출마다 현재 슬롯을 읽는다.
std::array<float,4> RawFenceLifetime::Endpoints(Sid first,Sid second) const { const auto a=Raw(first),b=Raw(second);return {Coordinate(a,14),Coordinate(a,18),Coordinate(b,14),Coordinate(b,18)}; }
// 최초 extra 검사는 이 함수 한 번의 진입에서만 수행한다.
void RawFenceLifetime::Clear(Sid parent) const {
    if (Raw(parent)[40]&9) return;
    // 현재 동맹 이웃을0/2/4/6 순서로 찾고 효과 후 현재 인수를 다시 읽는다.
    for (std::uint32_t direction=0;direction<8;direction+=2) {
        const auto other=Need(hooks_.neighbor)(parent,direction,true);if (!other.value) continue;Need(hooks_.clearMarkers)(parent,other);
        const auto points=Endpoints(parent,other);const auto a=Raw(parent),b=Raw(other);Need(hooks_.clearLine)(points,a[34],b[34],parent,other);
    }
}
// 네 이웃 스냅샷은 자기 Unpop 전에 만들고 각 이웃의 유효성/dead/타입은 처리 시 다시 읽는다.
void RawFenceLifetime::Refresh(Sid parent) const {
    Raw(parent);std::array<Sid,4> neighbors{};
    // 원본은 네 방향을 모두 저장한 후에야 정리 효과를 시작한다.
    for (std::uint32_t direction=0;direction<8;direction+=2) neighbors[direction/2]=Need(hooks_.neighbor)(parent,direction,true);
    Clear(parent);if (!(Raw(parent)[11]&2)) Need(hooks_.unpop)(parent,0);
    // 재생성이 다음 이웃 또는 부모의 타입/상태를 바꾸면 원본처럼 다음 처리에 반영한다.
    for (const auto other:neighbors) {
        if (!other.value || other.value>=pool_.Capacity()) continue;const auto raw=Raw(other);if (!(raw[11]&2) && raw[10]==Raw(parent)[10]) { Clear(other);Need(hooks_.rebuild)(other); }
    }
    if (!(Raw(parent)[11]&2)) Need(hooks_.repop)(parent,0x40);
}
// 마지막 일반 이웃은 네 개를 읽되 원본은 첫 두 칸에만 지연 Regular를 붙인다.
void RawFenceLifetime::PreDestroy(Sid parent,std::uint32_t flags) const {
    Refresh(parent);std::array<Sid,4> neighbors{};
    // 실제4556220의 비동맹 제한 없는 현재 이웃을 따로 저장한다.
    for (std::uint32_t direction=0;direction<8;direction+=2) neighbors[direction/2]=Need(hooks_.neighbor)(parent,direction,false);
    // 이 원본의2회 조건을 네 방향으로 고쳐 쓰지 않는다.
    for (std::size_t index=0;index<2;++index) if (neighbors[index].value && Need(hooks_.reserveRegular)()) Need(hooks_.addRegular)(neighbors[index],2,0.10000000149011612f);
    Need(hooks_.damageable)(parent,flags);
}
// 실제 단일 풀을 연결 단계에서 확인한다.
const SidPool& RawFenceLifetime::Pool() const { return pool_; }
}
