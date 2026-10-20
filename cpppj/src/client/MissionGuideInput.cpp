// 현재 안내의 원본 문자열 바이트·최종 DWORD 쓰기와 First의 즉시/예약 경계를 보존한다.
#include "client/MissionGuideInput.h"
#include "o/ConfigInterface.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
// 재시작 필드는 원본의 test/set 분기와 같이 비제로 전체를1로 만든다.
MissionReloadRequest CaptureMissionReload(std::string_view name,std::uint32_t mode,std::uint32_t demo) {
    return {std::string(name),mode ? 1U : 0U,demo ? 1U : 0U};
}
// 원본처럼 비어 있는 demo만 이전 직접 쓰기를 보존한다. 공백/잘못된 숫자 문자열은0을 쓴다.
void ApplyMissionDemo(std::string_view text,std::uint32_t& demo) {
    if (!text.empty()) demo=std::bit_cast<std::uint32_t>(o::ConfigParseLong(text));
}
// demo 안내는 단계 글자와 무관하다. char0은 원본 NUL 때문에 빈 섹션이 된다.
void TellMissionStage(const o::MissionFrameGateState& gate,std::uint32_t& stageShown,const MissionStageEffects& effects) {
    if (!effects.tell) throw std::logic_error("현재 미션 안내의 실제 Tell 연결 누락");
    std::string section="Demo"; // 원본00514a18의 NUL 종료 문자열이다.
    if (!gate.demo) {
        const auto stage=static_cast<std::uint8_t>(gate.stage); // 원본은 signed DWORD의 낮은 char만 쓴다.
        section.clear();if (stage) { section+=std::bit_cast<char>(stage);section+='.'; }
    }
    const auto shown=effects.tell(section); // 후속 효과가 현재 단계/결과를 바꾸어도 이 호출의 반환값을 마지막에 쓴다.
    if (shown) {
        if (!effects.closeTransient || !effects.pause) throw std::logic_error("현재 미션 안내의 실제 팝업/정지 연결 누락");
        effects.closeTransient();effects.pause();
    }
    stageShown=shown;
}
// 원본 First는 순번이0일 때 Tell을 호출하지 않으며 다른 flags와 상태 필드를 보존한다.
void MissionFirst(o::MissionStartState& state,const std::function<void()>& tellStage) {
    if (state.sequence==0) { state.flags|=1U;return; }
    if (!tellStage) throw std::logic_error("현재 미션 First의 실제 안내 연결 누락");tellStage();
}
// Guide가 먼저 소비하므로 release 표시가 있는 명시 First 명령도 같은 분기로 들어간다.
bool DispatchMissionFirst(const InputEvent& event,const std::function<void()>& currentFirst) {
    if ((event.code&0x03ffffffU)!=kMissionFirstCommand) return false;
    if (currentFirst) currentFirst();return true;
}
// 실제 F8은 즉시 Tell을 하지 않고 같은 입력 큐의 끝에 First/0/0을 예약한다.
bool QueueMissionFirstKey(const InputEvent& event,InputQueue& queue,bool activeMission) {
    if (!activeMission || (event.code&InputCode::kRelease) || (event.code&0x03ffffffU)!=(0x77U<<InputCode::kVirtualKeyShift) ||
        (event.code&(InputCode::kControl|InputCode::kShift))) return false;
    queue.PushCommand(kMissionFirstCommand,0,0);return true;
}
}
