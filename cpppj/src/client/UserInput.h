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
    // 좌클릭 선택→땅 좌클릭 이동. 우클릭은 메뉴 대상 번호를 돌려준다.
    std::optional<o::SquidId> Event(InputEvent event);
    // 화살표 또는 Alt/가운데 버튼 방향으로 벽시계 기준 카메라를 움직인다.
    void Tick();
    // 대화상자·창 비활성에서 눌린 키가 남지 않도록 지운다.
    void Cancel();
private:
    // client_: 원본 전역 변수로 흩어져 있던 클라이언트 상태를 객체 하나로 묶는다. 한 프로세스에 하나만 만든다 (Client).
    Client& client_;
    // world_: 미션 지도와 객체 상태를 갱신하고 월드 그림을 수집하는 프로세스 (GameWorld).
    GameWorld& world_;
    // held_: 보유한.
    std::array<bool,256> held_{};
    // lastWall_: 마지막 벽시계.
    double lastWall_{};
};
}
