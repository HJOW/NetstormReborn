// Userinput.cpp의 선택/이동/카메라 입력을 현재 월드에 연결하는 부분 구현.
#pragma once
#include "client/GameWorld.h"
#include "client/InputEvent.h"

namespace netstorm::client {
class Client;
// 플레이 입력을 선택·카메라·명령 동작으로 전달하는 처리기이다.
class UserInput {
public:
    // 월드보다 먼저 해제하며 메뉴·브리핑의 입력은 UberGump가 계속 처리한다.
    UserInput(Client& client,GameWorld& world);
    // 활성 커서는 원본 Drop/회전/ESC로 보내며 그 외 입력은 현재 선택/이동 경로다.
    std::optional<o::SquidId> Event(InputEvent event);
    // 사건 큐 처리 전에 현재 폴링 좌표/카메라와 원본 커서 옵션을 반영한다.
    void UpdateCursor(InputEvent mouse);
private:
    void ReadCursorOptions(); // 현재 설정의 snap/cannotPlace를 같은 커서 전역에 공급한다.
    // client_: 원본 전역 변수로 흩어져 있던 클라이언트 상태를 객체 하나로 묶는다. 한 프로세스에 하나만 만든다 (Client).
    Client& client_;
    // world_: 미션 지도와 객체 상태를 갱신하고 월드 그림을 수집하는 프로세스 (GameWorld).
    GameWorld& world_;
};
}
