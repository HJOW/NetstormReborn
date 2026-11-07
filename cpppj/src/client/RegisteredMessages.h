// 설정 도구와 LAN 서버의 같은 등록 메시지를 Client 수명 전역으로 처리한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::client { // 원문435ad0/435ae0/436550의 등록 메시지 계약이다.
struct RegisteredMessageState { // 원문 BSS 두 DWORD이며 월드/메타 상태 교체로 지우지 않는다.
    std::uint32_t message{}; //0054da94의 RegisterWindowMessage 반환 ID다.
    std::uint32_t toolActive{}; //0054da98의 설정 도구 활성 표식이며 게임 루프 정지 요청이 아니다.
};
struct RegisteredMessageHooks { // 선택된 메시지의 실제 장치만 원문 순서대로 요청한다.
    std::function<void()> reload; //441790의 NULL 명령줄 설정 재적재다.
    std::function<void()> interpret; //435220의 명시 옵션 재해석이다.
    std::function<void()> subscriber; //4b3410의 등록 ID 읽기/현재 Fort 동기화다.
    std::function<bool()> fullscreen; // 현재 장치005c78d4의 bit0을 읽는다.
    std::function<void()> restore; //500h에서만 같은 수신 창에 SW_RESTORE를 요청한다.
};
void DispatchRegisteredMessage(RegisteredMessageState& state,std::uint32_t message,std::uint32_t parameter,const RegisteredMessageHooks& hooks); // lParam 하위 DWORD의 ff00h만 분류하며 일반 WndProc 처리는 호출자가 계속한다.
}
