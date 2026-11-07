// 현재 Client의 기록 재생 전역과 실제 ProcessForm/Kernel·파일/입력 부모를 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <cstdint>
#include <memory>

namespace netstorm::client { // 원본59a7e0/e4의 수명만 장기 보유하며 게임 세션을 별도 생성하지 않는다.
class Client;struct ClientBuildReplayContext; // 현재 Client 장치와 원문 이벤트 배열의 실제 소유자다.
struct ClientBuildReplayGlobals {
    std::uint32_t playing{}; //0059a7e4의 BSS0이며 재생 생성자는1, 정상 Run 완료만0을 쓴다.
    std::weak_ptr<ClientBuildReplayContext> current; //0059a7e0의 현재 객체다. 반납 뒤 원문 dangling 접근은 명시 호스트 오류로 구별한다.
};
//495910: 동일 사용자 홈 경로의 첫 빈 번호 바로 앞 rec를 실제4955b0/Host에 설치한다.
void StartLatestBuildReplay(Client& client);
// 일반 Ctrl+F7의 현재 모드/정지/specialAuxiliary 관문과 Shift의 NextStage 큐 예약이다.
bool DispatchBuildReplayKey(Client& client,InputEvent event);
//495710/495730의 현재 재생 객체를 매번 조회하며 이전 객체를 값으로 고정하지 않는다.
bool CurrentBuildReplayIsLabel(const std::shared_ptr<ClientBuildReplayGlobals>& globals);
void NotifyCurrentBuildReplay(const std::shared_ptr<ClientBuildReplayGlobals>& globals,std::uint32_t sid);
}
