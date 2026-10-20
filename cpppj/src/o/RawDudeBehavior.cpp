// 거의 올림·현재 상태 재조회·삭제 뒤 반환과 목록의 stale 꼬리를 원본대로 유지한다.
#include "o/RawDudeBehavior.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kCellBias=0.9999F; // 실제500edc를 float 입력에 double 정밀도로 더한 뒤 절삭한다.
constexpr std::uint32_t kIdle=0x5101,kWalk=0x5102,kReturn=0x5103,kDrop=0x5104,kFall=0x5105,kBlocked=0x5106,kNotice=0x5107; // 원본 task WORD 값이다.
// 지정 폭의 little endian 필드만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 원본 비정렬 BYTE/WORD/DWORD를 낮은 바이트부터 합친다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
// 원본 지도 접근의 정상 범위만 허용하며 거의 올림 이전의 float 저장 비트를 사용한다.
std::int32_t Cell(std::uint32_t bits) {
    const double value=static_cast<double>(std::bit_cast<float>(bits))+kCellBias;
    if (!(value>=0 && value<256)) throw std::out_of_range("Dude 지면 판정 지도 범위 오류");return static_cast<std::int32_t>(value);
}
// 실제 선택된 효과가 미연결이면 그 지점에서 중단한다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("Dude 부모의 실제 하위 미연결");return hook;
}
}
// 일반 Dude의 현재 저장소를 공유하고 복원하지 않은 CD Session은 명시 거부한다.
RawDudeBehavior::RawDudeBehavior(const SidPool& pool,std::span<const std::uint8_t> spots,DudeBehaviorState state,DudeBehaviorHooks hooks)
    :pool_(pool),spots_(spots),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || spots.size()!=65536) throw std::invalid_argument("Dude 부모 판본/지도 연결 오류");
}
// 같은 이름의 다른 파생 자산은 포함하지 않는다.
bool RawDudeBehavior::Handles(Sid sid) const { const auto raw=pool_.Slot(sid);return raw[10]==70 && Read(raw,0)==0x508cc8U; }
// 원본 도착은 free/dead에서도 반환하므로 이 getter에 생존 관문을 추가하지 않는다.
std::span<const std::uint8_t> RawDudeBehavior::Raw(Sid sid) const { if (!Handles(sid)) throw std::invalid_argument("Dude 부모 타입/가상 표 오류");return pool_.Slot(sid); }
// 원본은 task 설정 이후의 현재 타입으로 이동 종류를 다시 선택한다.
void RawDudeBehavior::StopMovement(Sid sid) const { const auto type=Need(hooks_.movementType)(Raw(sid)[10]);static_cast<void>(Need(hooks_.find)(sid,type,0,true)); }
// 450f50의 반환1은 실제 이동 객체가 있었는지와 무관하다.
bool RawDudeBehavior::Fall(Sid sid) const { Raw(sid);Need(hooks_.task)(sid,kFall,0);StopMovement(sid);return true; }
// bit6은 표면, bit8은 건물 점유다. y/x 모두 원본 almostceil을 거친다.
bool RawDudeBehavior::CheckSpace(Sid sid) const {
    const auto raw=Raw(sid);if (raw[11]&2U) return false;if (Read(raw,12,2)==kFall) return true;
    const auto x=Cell(Read(raw,14)),y=Cell(Read(raw,18));const auto spot=spots_[static_cast<std::size_t>(y)*256+x];
    if (!(spot&6U) && Fall(sid)) return true;
    if (spot&8U) { Need(hooks_.task)(sid,kBlocked,0);StopMovement(sid);return true; }
    return false;
}
// 공간 효과가 현재 Path를 지웠을 때도 원본12를 그대로 반환한다.
std::uint32_t RawDudeBehavior::Halt(Sid sid) const { return (Raw(sid)[11]&2U) || CheckSpace(sid) ? 12U : 10U; }
// 원본의 진단 후 fallthrough와 삭제 뒤 고정 반환을 보존한다.
std::uint32_t RawDudeBehavior::Arrival(Sid sid) const {
    const auto raw=Raw(sid);if (raw[11]&3U) return 1001;
    switch (Read(raw,12,2)) {
    case kIdle:if (state_.checking) Need(hooks_.assertion)(0x153);[[fallthrough]];
    case kDrop:case kNotice:Need(hooks_.destroy)(sid,0);return 1001;
    case kWalk:return 1000;
    case kFall:if (state_.checking) Need(hooks_.assertion)(0x157);[[fallthrough]];
    case kReturn:Need(hooks_.destroy)(sid,0);return 1001;
    default:Need(hooks_.assertion)(0x15b);return 1000;
    }
}
// 원본40ea00은 모든 일치 항목을 제거하고 남은 항목 순서와 물리 꼬리 바이트를 보존한다.
void RawDudeBehavior::PostDestroy(Sid sid,std::uint32_t flags) const {
    if (!(Raw(sid)[40]&9U)) {
        auto& list=state_.population;static_cast<void>(list.Items());std::uint32_t removed{}; // 원본 count와 같은 DWORD 폭이며 count 이하만 증가한다.
        // 앞의 제거 수만큼 당겨 쓰며 count 뒤의 stale 영역은 지우지 않는다.
        for (std::size_t index=0;index<list.count;++index) { const auto value=list.entries[index];if (value==sid.value) ++removed;else list.entries[index-removed]=value; }
        list.count-=removed;
    }
    Need(hooks_.basePostDestroy)(sid,flags);
}
// 기존 월드/경로/후처리의 같은 저장소 검증에 사용한다.
const SidPool& RawDudeBehavior::Pool() const { return pool_; }
}
