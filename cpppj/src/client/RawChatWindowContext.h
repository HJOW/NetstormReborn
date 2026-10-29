// 실제 ChatWindow 공장이 사용하는 Client 전역·문자열·선택 장치 경계다.
#pragma once
#include "client/ClientChatWindowGlobals.h"
#include "client/RawTextEntryContext.h"
#include <memory>
namespace netstorm::client { // 공통 Chat UI를 실제 Client 소유자에 연결한다.
class ClientDotCommandGlobals; // 기존 인접 명령 표를 그대로 빌린다.
struct RawChatWindowContext { // 조회/콜백은 생성한 UI의 모든 입력/소멸보다 오래 유지한다.
    std::shared_ptr<ClientChatWindowGlobals> globals;std::shared_ptr<ClientDotCommandGlobals> commands; // 실제 장기 전역/명령 표다.
    RawTextEntryContext editor; // 같은 실제 Console의 벽시계/clipboard/완성/제출 장치다.
    std::function<std::uint32_t(std::uint32_t)> read; // 실제 현재 모드/Chatmanager/마우스/참조 포인터 전역이다.
    std::function<std::string(std::string_view)> translate; // 현재 언어의 실제 원문 글꼴 바이트다.
    std::function<std::uint32_t(std::string_view)> storeText;std::function<std::string_view(std::uint32_t)> text; // 같은80칸 ScriptStringRing이다.
    std::function<bool(std::string_view)> recipient; //42dc70의 실제 선택 경계다.
    std::function<void(std::uint32_t,std::string_view)> sendChat; //42da00의 같은 현재 manager와 실제 입력이다.
    std::function<std::uint32_t(std::span<const std::uint8_t>)> message; //0042f1e0→조합→실제 패킷/큐에 전달하는 일반 본문 장치다.
    std::function<bool(o::PlayerIdentityRecord&)> resolve; // 실제 Subscriber/identity 조회다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t)> diagnostic; // 이름 실패 진단의 실제 장치다.
    std::function<std::int32_t(std::string_view)> config; // helper 체크의 현재 실제 설정이다.
    std::function<bool(std::uint32_t)> fileValid;std::function<void(std::uint32_t,std::span<const char>)> writeLog; // 실제 파일 토큰이 선택됐을 때만 사용하는 장치다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> quit; //42e520의 현재 원문 사건/0/0 예약이며 즉시 프로세스 강제 종료가 아니다.
};
}
