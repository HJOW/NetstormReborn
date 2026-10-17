// 실제 Client 장치를 동일 월드의 원본 선택/강조/이동 입력에 연결한다.
#pragma once
#include <cstdint>
#include <functional>
namespace netstorm::client {
class Client; //현재 게임 시계/정지/장치의 실제 소유자다.
class GameWorld; //같은 Session과 UI 선택 저장소를 소유한다.
// 실제 하드웨어 커서 선택기를 공유하며 별도 선택/경로 사본을 만들지 않는다.
void BindCampaignUserInputServices(Client& client,GameWorld& world,std::function<void(std::uint32_t)> selectCursor);
}
