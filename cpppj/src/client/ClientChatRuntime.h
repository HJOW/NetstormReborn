// 기존 Chat 본체를 실제 Client의 할당/설정/명령표와 현재 Core/아이콘 자산에 연결한다.
#pragma once
#include "o/RawPlayerIdentity.h"
#include <functional>
#include <memory>
#include <string_view>

namespace netstorm::client { // 동일 Client의 현재 슬롯을 빌리며 ChatWindow 입력 접두와 구별한다.
class Client; // 실제 장기 Chat/Core/Subscriber/명령표의 소유자다.
class GameAssets; // 원문 icon 타입의 B/C 프레임 개수를 제공한다.
class RawSurfaceStorage; // 실제 공통 월드 모드의 현재 metamap을 빌린다.
class RawMenuControls; // 같은 실제 Chat/Console과 clear/quit·입력 표시의 소유자다.
struct RawChatWindowContext; //실제 Chat/Console 공장이 같은 현재 대상 검사를 빌린다.
struct ChatMessageHooks; // 일반 본문 조합의 실제 물리/이름 장치이며 패킷 전달은 별도 연결한다.
void InitializeClientChatManager(Client& client); // 공통 시작의0042e050을 실제 명령 등록/Config/할당에 실행한다. 재호출도 원문 재할당 순서를 따른다.
void RefreshClientChatRankPrefix(Client& client,const GameAssets& assets,const RawSurfaceStorage& surface); //0042f540의 현재 manager/active 관문과 실제 Core+4·아이콘 getter를 실행한다.
bool ResolveClientChatIdentity(Client& client,const RawSurfaceStorage& surface,std::string_view currentName,o::PlayerIdentityRecord& record); //004841c0을 현재 Playerbase/Chat100/Client 전역과 실제 이름으로 실행한다. 호출동안 이름/표면뷰가 살아 있어야 한다.
void BindClientChatRecipients(RawChatWindowContext& context,Client& client,std::shared_ptr<RawSurfaceStorage> surface,std::function<std::string_view()> currentName); //같은 현재 Identity/번역/manager를 ChatWindow와Console Draw에 연결한다. 송신/자동완성은 별도 장치다.
void BindClientChatSubmit(RawChatWindowContext& context,Client& client,RawMenuControls& menus); // 현재 대상 해석/명령표·빈 입력·clear/quit와 format/back/front를 같은 Console 제출에 잇는다. 일반 본문 서식·송신은 미연결이다.
std::uint32_t ApplyClientChatFormat(Client& client,bool front,std::string_view input); // 동일 현재 manager/ID와 공통 캡처로 서식 명령을 실행하고 실제 Config 변경 알림·후속 저장에 연결한다.
ChatMessageHooks MakeClientChatMessageHooks(Client& client,std::shared_ptr<RawSurfaceStorage> surface,std::function<std::string_view()> currentName); // 현재 원문 이름/Identity와 실제 manager 배열을 일반 조합 부모에 연결한다. deliver/inputVisible은 실제 호출자가 공급한다.
}
