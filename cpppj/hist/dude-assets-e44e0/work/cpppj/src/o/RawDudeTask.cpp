// 원본 일반 Dude task 설정의 판본별 진단/반환과 Regular 재예약을 복원한다.
#include "o/RawDudeTask.h"
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 낙하 중 작업 교체를 막는 원본 task 번호와 현재 상태 비트다.
constexpr std::uint32_t kFallTask=0x5105;
constexpr std::uint8_t kFreeOrDead=3;
}
// 원본의 현재 시계와 실제 하위 호출을 보존하며 미연결 호출은 도달 시 거부한다.
RawDudeTask::RawDudeTask(SidPool& pool,DudeTaskState state,DudeTaskHooks hooks)
    :pool_(pool),state_(state),hooks_(std::move(hooks)) {}
// float 지연은 먼저 double로 승격하며 원본 x87의 현재 시각 정밀도를 유지한다.
void RawDudeTask::Set(Sid sid,std::uint32_t event,float delay) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!patch && !sid.value) return;
    const auto raw=pool_.Slot(sid);
    if (!patch && (raw[11]&kFreeOrDead)) return;
    // 새 C++의 쓰기 계약은 예약 머리/free SID를 원본의 미정의 쓰기 전에 거부한다.
    static_cast<void>(pool_.AllocatedBytes(sid));
    if (!std::isfinite(delay) || !std::isfinite(state_.clock.now)) throw std::invalid_argument("Dude task 유한 시각 입력 오류");
    const auto previous=static_cast<std::uint16_t>(raw[12]|(raw[13]<<8));
    if (previous==kFallTask && event!=kFallTask) {
        if (patch) throw std::logic_error("Dude task FALL 교체 원본 assert");
        event=kFallTask;
    }
    if (!hooks_.taskRegular) throw std::logic_error("Dude task 현재 Regular 조회 미연결");
    auto* regular=hooks_.taskRegular(sid);
    if (!regular) {
        if (!hooks_.addTaskRegular) throw std::logic_error("Dude task 실제 Regular 생성 미연결");
        regular=hooks_.addTaskRegular(sid,event);
        if (!regular) throw std::logic_error("Dude task 실제 Regular 생성 실패");
    }
    regular->ResetTask(event,state_.clock.now+static_cast<double>(delay));
    auto bytes=pool_.AllocatedBytes(sid);
    bytes[12]=static_cast<std::uint8_t>(event);bytes[13]=static_cast<std::uint8_t>(event>>8);
    if (pool_.Slot(sid)[10]==state_.bridgeType) {
        if (!hooks_.notifySurface) throw std::logic_error("Dude task 실제 표면 통지 미연결");
        hooks_.notifySurface(sid);
    }
}
// 같은 월드 저장소를 사용하는 실제 드롭/factory 연결을 확인한다.
const SidPool& RawDudeTask::Pool() const { return pool_; }
// 기존 Regular 객체와 실제 생성/부착을 재사용하며 payload/flags를 원본값으로 고정한다.
DudeTaskHooks MakeDudeTaskHostHooks(const SidPool& pool,SquidProcessHost& host,DudeTaskHooks hooks) {
    if (&pool!=&host.Pool()) throw std::invalid_argument("Dude task/Host SID 풀 불일치");
    hooks.taskRegular=[&host](Sid sid) { return host.FindEventMasked(sid,0xfff0,0x5100); };
    hooks.addTaskRegular=[&host](Sid sid,std::uint32_t event) { return host.AddRegular(sid,event,5.0f,0x50); };
    return hooks;
}
}
