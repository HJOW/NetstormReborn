// 실제 Present 말미의 진단 관문·두 목표 제한과 뒤 적용 호출의 순서를 보존한다.
#pragma once
#include "client/ClientCameraGlobals.h"
#include <functional>

namespace netstorm::client { // 화면 장치 출력과 현재 논리 카메라 데이터를 구별한다.
// 값 사본을 만들지 않으므로 flip 효과가 바꾼 현재 크기/한계도 원문 읽기 위치에 반영한다.
struct RawCameraPresentGlobals {
    const std::uint32_t& diagnostic; // 005318ec의 현재 DWORD이며 정확1에서만 이후 호출을 건너뛴다.
    const std::int32_t& width; // 005c78f0의 현재 논리 화면 폭이다.
    const std::int32_t& height; // 005c78f4의 현재 논리 화면 높이다.
    const std::int32_t& worldCells; // 00531928의 현재 signed 세계 크기이며 Renderer의 두 limit과 별개다.
};
// dirty 출력/clear와4a17e0은 이 본체 앞의 caller가 수행한다.
struct RawCameraPresentHooks {
    std::function<void()> flip; // 실제004a16d0의 장치 경계다. 진단1이면 요구하지 않는다.
    std::function<void()> commit; // 두 제한 뒤 같은 Renderer의00497960 전체를 호출한다. 현재 view를 본체에서 대신 변경하지 않는다.
};
void PresentCameraTarget(RawCameraTargetState& target,const RawCameraPresentGlobals& globals,const RawCameraPresentHooks& hooks); //49a081..49a1c9의 실제 후처리만 실행하며497960 자체와 구별한다.
}
