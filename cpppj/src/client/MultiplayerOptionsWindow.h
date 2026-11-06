// 원문 Multiplayer Options의 네 정보 행과 실제 장식/돌 버튼을 같은 공통 트리에 보유한다.
#pragma once
#include "client/GumpCreate.h"
#include "client/RawTextEntryContext.h"
#include <string>

namespace netstorm::client { // 게임 접속은 외곽의 실제 입력/State 장치가 수행하고 창은 원문 표시/선택만 처리한다.
enum class MultiplayerOptionsAction {
    ChooseFort,ChooseName,ChooseServer,ChooseProtocol,Connect,Cancel // 네 행 선택과 Connect!/Cancel의 서로 다른 실제 효과다.
};
struct MultiplayerOptionsWindowNode: GumpCreateNode {
    std::uint32_t decoration{},title{},entry{}; // 원문+a0/+a4/+b8의 실제 자식이며 entry는 원문에서 부착하지 않는다.
    std::array<std::uint32_t,4> labels{},arrows{}; // +a8..b4와+c0..cc의 현재 네 정보/선택 자식이다.
    std::uint32_t connect{},cancel{},subscriber{}; // +d0/+d4/+dc의 Connect!/Cancel/Sub ID 돌 버튼이다.
};
struct MultiplayerOptionsWindowContext {
    RawTextEntryContext editor; // 공통 문자 버퍼의 실제 현재 시계/클립보드 장치다.
    std::function<std::string(std::string_view)> translate; // 현재 언어의 실제 글꼴 바이트를 반환한다.
    std::function<std::string()> fortName,playerName,serverName; // 매 Attach/갱신 때 실제 현재 문자열을 다시 읽는다.
    std::function<void(std::string_view)> rememberFort; // 원문 lastMultiplayerFort 설정 쓰기이며 파일/모드는 바꾸지 않는다.
    std::function<void(MultiplayerOptionsAction)> action; // 실제 목록/이름/서버/입력 큐·LAN 시작/취소 장치다. 미연결은 도달 위치에서 거부한다.
    std::function<bool()> special; // 원문 special 비영일 때만 Sub ID 버튼을 활성화한다.
};
}
