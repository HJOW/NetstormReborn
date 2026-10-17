// Dude task의 WORD 쓰기와 구별하며 Carrier의 기존 Regular 자체를 갱신한다.
#include "o/RawCarrierTask.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 실제 선택된 하위만 요구해 조기 경로에 불필요한 효과를 강제하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("캐리어 task 실제 하위 미연결");return function; }
// 실제 FLD(float delay)/FADD(double now)의 NaN payload/부호 선택과 quiet 변환을 보존한다.
double ScheduleTime(float delay,double now) {
    constexpr std::uint64_t kExponent=0x7ff0000000000000ULL,kFraction=0x000fffffffffffffULL,kQuiet=0x0008000000000000ULL; // double NaN 판정/유효숫자/quiet 비트다.
    const auto current=std::bit_cast<std::uint64_t>(now);const auto rawDelay=std::bit_cast<std::uint32_t>(delay);
    const bool currentNaN=(current&kExponent)==kExponent && (current&kFraction)!=0,delayNaN=(rawDelay&0x7f800000U)==0x7f800000U && (rawDelay&0x007fffffU)!=0;
    if (!delayNaN) { if (currentNaN) return std::bit_cast<double>(current|kQuiet);return static_cast<double>(delay)+now; }
    const auto expanded=(std::uint64_t(rawDelay&0x80000000U)<<32)|kExponent|kQuiet|(std::uint64_t(rawDelay&0x007fffffU)<<29); // FLD가 먼저 float signaling NaN을 quiet으로 바꾼다.
    if (!currentNaN) return std::bit_cast<double>(expanded);
    const auto left=expanded&kFraction,right=current&kFraction;
    if (left>right) return std::bit_cast<double>(expanded);if (right>left) return std::bit_cast<double>(current);
    return std::bit_cast<double>(expanded&current); // 유효숫자가 같으면 양의 부호를 우선하며 둘 다 음수일 때만 음수다.
}
}
RawCarrierTask::RawCarrierTask(const SidPool& pool,const double& now,CarrierTaskHooks hooks):pool_(pool),now_(now),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("캐리어 task는 PATCH 전용입니다");
}
void RawCarrierTask::Set(Sid parent,std::uint32_t task,float delay) const {
    auto* process=Need(hooks_.find)(parent);auto effective=task;
    if (!process) {
        if (!Need(hooks_.allocate)()) throw std::bad_alloc(); // 원본의 NULL+10 쓰기를 정상 예약으로 바꾸지 않는다.
        process=Need(hooks_.construct)(parent,task,task!=0x5105U);if (!process) throw std::logic_error("캐리어 task 생성자의 NULL 반환");
        process->SetNextTime(ScheduleTime(delay,now_));return; // 생성/부착 효과가 바꾼 event/count도 원본처럼 보존한다.
    } else if (process->Event()==0x5105U) {
        if (task!=0x5105U) Need(hooks_.assertion)();effective=0x5105U;
    }
    process->ResetTask(effective,ScheduleTime(delay,now_));
}
const SidPool& RawCarrierTask::Pool() const { return pool_; }
}
