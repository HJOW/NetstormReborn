// 원문434f40→42f190의 선택 관문/현재 색/인자 없는 서식/현재 Chat 전달을 연결한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // 공통 로그에서 현재 Chat UI로 넘어가는 실제 장치 경계다.
// 현재 색/포인터는 실행 위치에서 다시 읽으며 UI 소유자는 이 콜백보다 오래 살아야 한다.
struct TraceChatHooks {
    std::function<std::uint32_t()> color,currentChat; // 005acd24/00548740의 현재 DWORD 조회다.
    std::function<void(std::uint32_t,std::string_view,std::span<const std::uint32_t>)> append; //42eca0의 현재 root/완성 문자열/8DWORD 문맥 전달이다.
};
// traceToChat0은 아무 장치도 읽지 않는다. 비영이면 원문 1024BYTE 서식 뒤 현재 포인터가 있을 때만 추가한다.
// 인자 없는 서식의 일반 문자/%%를 지원한다. 추가 인자가 필요한 원문 미정의 스택 읽기는 명시 거부한다.
void ForwardTraceToChat(std::uint32_t enabled,std::string_view format,const TraceChatHooks& hooks);
}
