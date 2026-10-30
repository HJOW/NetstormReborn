// 원본 도움말 링크의 토큰/내부 주제·명령/외부 실행 정책을 같은 Client에서 사용한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <string>
#include <string_view>
namespace netstorm::client { // UI와 원문 관측이 공유하는 순수 링크 본문이다.
struct HelpLinkAddress { // 첫NUL까지의 원문에서 파싱한 실제80/256BYTE 역할이다.
    std::string protocol; // 인용 제거/뒤공백 처리 뒤 ':'를 붙인 접두다. 콜론이 없을 때만 빈 값이다.
    std::string body; // 원래 토큰 포인터가 소비한 뒤의 본문이다. 단순 첫 콜론 위치와 다를 수 있다.
};
struct HelpLinkHooks { // 원문의 내부 Script/Help와 선택된 외부 분기만 제공한다.
    std::function<void(std::string_view)> script; //004c9090의 같은 명령 문자열이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> help; //004c63d0의 주제/모드1/학습0이다.
    std::function<void(std::string_view,std::string_view,std::string_view)> external; //원래 전체 입력/파싱 접두/본문을 동기 전달한다.
};
struct ExternalHelpHooks { // 큰 설정/OS 장치는 원래 선택 위치에서만 요구한다.
    std::function<std::string(std::string_view)> pathSpec; //004410f0의 autoOpen이며 다른 세 인자는 원래0이다.
    std::function<bool(std::string_view)> shellOpen; //004dd640의 원래 전체 URL과 ShellExecute 성공 여부다.
    std::function<bool(std::string_view,std::string&)> readString; //00441300의 현재 값/변경 반환이다. 두 번째 조회도 이전 값을 받는다.
    std::function<void(std::string_view)> launch; //004dd6b0(0,command,0)의 실제 비대기 실행이다.
};
HelpLinkAddress ParseHelpLinkAddress(std::string_view value); //기존487620의78입력 바이트/인용/연속 구분자 소비를 공유한다.
void DispatchHelpLink(o::OriginalEdition edition,std::string_view value,const HelpLinkHooks& hooks); //4a00c0의 cmd 첫4비교/내부 주제/외부 분배다.
bool ReadHelpProtocolCommand(std::string_view protocol,std::string& output,const ExternalHelpHooks& hooks); //4a0030의 protocol→protocol2와 '{' 재검사를 보존한다.
void OpenExternalHelpLink(o::OriginalEdition edition,std::string_view original,std::string_view protocol,std::string_view body,const ExternalHelpHooks& hooks); //autoOpen/실패 fallback/첫% 두공백/본문 연결 순서다.
}
