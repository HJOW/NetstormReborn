// 원본00435ba0의 모드 복구·현재 설정 저장·메타 갱신 순서를 실제 Client에 연결한다.
#pragma once
#include <cstdint>
#include <functional>
#include <string_view>

namespace netstorm::client { // OS 장치와 원문 부모의 상태 전환을 구별한다.
struct ScreenModeTransitionHooks { // 선택한 경계에서만 실제 장치를 요구한다.
    std::function<std::uint32_t()> current; //005c78d4의 현재 DWORD를 각 원문 소비 위치에서 읽는다.
    std::function<void(std::uint32_t)> change; //004a4fe0의 실제 화면 장치이며 실패하면 예외를 발생시킨다.
    std::function<void(std::uint32_t,std::string_view)> diagnostic; //004c2630의 실패 진단이며 복구 전에 실행한다.
    std::function<bool()> initialized; //0054dbb4의 현재 화면 모드 초기화 완료 관문이다. HWND 존재와 구별한다.
    std::function<void(std::string_view,std::uint32_t)> remember; //00441520의 현재 startInFullScreen 설정 쓰기다.
};
// 창 모드의 원문 비트 정리 뒤 필요할 때만 장치를 바꾸고 복구/설정 저장 후 갱신 DWORD를 감김 증가한다.
// Windows10/11 DIB 정규화는 실제 change 장치가 맡으며 원문 부모의 요청값을 미리 대체하지 않는다.
void TransitionScreenMode(std::uint32_t requested,std::uint32_t& refresh,const ScreenModeTransitionHooks& hooks);
}
