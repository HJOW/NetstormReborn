// 원본0042ed30/0042f0e0의 일반 채팅 수신·필터·로그 문맥과 선택 UI 효과를 복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // 실제 Chat UI와 같은 현재 전역을 빌리는 수신 부모다.
struct ChatReceiveHooks { // 순수 getter 외의 효과는 원문에 도달한 위치에서만 실행한다.
    std::function<std::uint32_t(std::uint32_t)> read; // 현재 모드/ID/채널 마스크/관계/글자색/Chat 토큰의 원문 슬롯이다.
    std::function<bool(std::uint32_t)> ignored; //0042e300의 현재 subscriber 무시 목록 조회다.
    std::function<std::int32_t(std::string_view)> config; // 열 개 옵션을 원문 순서대로 현재 설정에서 읽는다.
    std::function<std::uint32_t(std::uint32_t)> owner; //00484550의 현재 Identity 조회이며 실패도 초기 소유자0을 반환한다.
    std::function<void(std::uint32_t)> wake; //00419d50의 현재 알림 Gump 사건이다.
    std::function<void(std::string_view)> sound; // 실제 상대 메시지의 newChatMessage.wav 재생 요청이다.
    std::function<void(std::uint32_t,std::string_view,std::span<const std::uint32_t>)> append; //42eca0의 실제 Chat/완성 본문/8DWORD 문맥이다.
    std::function<void()> show; //0042e410의 현재 Chat 표시 사건이다. 입력줄 표시와 구별한다.
    std::function<void(std::uint32_t,std::string_view)> log; // 새로운 버전/비ASCII 패킷의 원문 진단이다.
    std::function<void(std::uint32_t)> setNewsTime; //00548748 store와 원문 뉴스 설정 저장 효과를 함께 연결한다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> news; // 시스템 뉴스의400/30 표시 장치다. 일반 수신과 독립이다.
};
// 먼저 마지막 본문 BYTE에NUL을 쓰고, 무시/필터 뒤에도 그 효과를 유지한다. 실제 물리 밖 접근은 명시 오류다.
void ReceiveChatPacket(std::uint32_t chat,std::span<std::uint8_t> packet,const ChatReceiveHooks& hooks);
// 현재 Chat0이면 패킷/다른 장치를 건드리지 않는다. 그 밖에는 같은 실제 수신 부모를 실행한다.
void DispatchChatPacket(std::span<std::uint8_t> packet,const ChatReceiveHooks& hooks);
}
