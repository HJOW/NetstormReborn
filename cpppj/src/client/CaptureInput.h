// Capture의 일반 명령과 원문 현재 active/두번째 DWORD를 같은 Client에 전달한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string_view>
namespace netstorm::client { //사제 포획과 별개인 화면 연속 저장 입력이다.
inline constexpr std::uint32_t kCaptureCommand=0x0200009dU; //543070의 이름→542968 전역 초기값이다.
struct CaptureInputHooks { //실제로 선택한 시작/종료/안내만 필수다.
    std::function<void()> start,stop; //4db210/4db2a0의 같은 장기 Client 저장소를 사용한다.
    std::function<void(std::string_view)> tell; //시작 확인은 기존 BeginCapture 섹션으로 연다.
};
bool DispatchCaptureInput(InputEvent event,const std::uint32_t& active,const CaptureInputHooks& hooks); //뗌은 효과 없이 소비하며 mode/첫번째 인자를 조회하지 않는다.
}
