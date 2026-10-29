// ChatWindow/Console이 공유하는 원본 대상 이름 검사를 복원한다.
#pragma once
#include "o/RawPlayerIdentity.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client { // 같은 현재 Identity/번역을 빌리는 표시 전용 검사이며 패킷을 보내지 않는다.
// 선택한 하위만 도달 시 요구한다. Identity는 실제 같은60BYTE 지역을 수정할 수 있다.
struct ChatRecipientHooks {
    std::function<bool(o::PlayerIdentityRecord&)> resolve; //004841c0의 현재 Player/Chat 이름 조회다.
    std::function<std::string(std::string_view)> translate; //004de9a0의 현재 채널 이름 번역이다. 빈 키도 원문처럼 조회한다.
};
//0042dc70→0042d910: manager0/NULL은false, 나머지는 모든 토큰이 Identity/채널에 있으면true다.
// scratch는 명시 초기60BYTE다. 같은 원문 부분쓰기·40BYTE 이름과 인접 주소 첫BYTE의NUL을 보존한다.
// 고정 Regexp 005049e8만 직역하며 범용 정규식/송수신/재접속은 수행하지 않는다.
bool CheckChatRecipients(std::uint32_t manager,std::optional<std::string_view> text,o::PlayerIdentityRecord& scratch,const ChatRecipientHooks& hooks);
}
