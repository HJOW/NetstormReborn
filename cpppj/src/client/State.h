// 원본 State.cpp 004b88c0의 다음 루프 상태 전환 자리. 현재는 단일 플레이 UI 전환만 연결한다.
#pragma once
#include "client/DialogScript.h"
#include <deque>
#include <optional>

namespace netstorm::client {
enum class ClientPhase { MainMenu, Dialog, LoadingMission, Briefing, Mission };
// 메뉴·미션 전환에 사용하는 상태와 현재 월드이다.
class State {
public:
    // 입력 중에는 현재 화면을 삭제하지 않고 다음 루프에 명령을 보낸다.
    void Post(DialogAction action);
    // 루프 시작 시 하나 꺼낸다. 빈 큐는 아무 전환도 만들지 않는다.
    std::optional<DialogAction> Take();
    // 현재 UI/미션 단계. 실제 미션 프로세스는 다음 월드 단계에서 연결한다.
    ClientPhase phase{ClientPhase::MainMenu};
private:
    // pending_: 후속 처리 전까지 보류하는 요청이나 상태.
    std::deque<DialogAction> pending_;
};
}
