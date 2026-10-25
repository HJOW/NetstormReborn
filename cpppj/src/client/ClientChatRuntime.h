// 기존 Chat 본체를 실제 Client의 할당/설정/명령표와 현재 Core/아이콘 자산에 연결한다.
#pragma once

namespace netstorm::client { // 동일 Client의 현재 슬롯을 빌리며 ChatWindow 입력 접두와 구별한다.
class Client; // 실제 장기 Chat/Core/Subscriber/명령표의 소유자다.
class GameAssets; // 원문 icon 타입의 B/C 프레임 개수를 제공한다.
class RawSurfaceStorage; // 실제 공통 월드 모드의 현재 metamap을 빌린다.
void InitializeClientChatManager(Client& client); // 공통 시작의0042e050을 실제 명령 등록/Config/할당에 실행한다. 재호출도 원문 재할당 순서를 따른다.
void RefreshClientChatRankPrefix(Client& client,const GameAssets& assets,const RawSurfaceStorage& surface); //0042f540의 현재 manager/active 관문과 실제 Core+4·아이콘 getter를 실행한다.
}
