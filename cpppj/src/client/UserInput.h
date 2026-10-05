// Userinput.cpp의 선택/이동/카메라 입력을 현재 월드에 연결하는 부분 구현.
#pragma once
#include "client/GameWorld.h"
#include "client/InputEvent.h"

namespace netstorm::client {
class Client;
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
    Client& client_;
    GameWorld& world_;
    std::array<bool,256> held_{};
    double lastWall_{};
};
}
