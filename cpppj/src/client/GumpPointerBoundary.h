// 원본 UserInput의 OS 폴링 결과와 같은 입력 큐를 연결하는 안팎 전환 앞단이다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client {
// Client 수명으로 유지하는 원본 정적 기록이다. 페이지·월드 교체 때 임의 초기화하지 않는다.
struct GumpPointerBoundaryState {
    std::uint32_t initializationFlags{}; //005cafe8의 전체 DWORD이며 이 앞단은 비트1/2만 초기화한다. 나머지 비트는 보존한다.
    std::uint32_t wasInside{1}; //0054354c의 PE 초기값1이다. 현재 포인터를 기록한 뒤에는0/1만 쓴다.
    InputEvent leftOutside; //005cafdc/e0/e4의 마지막 inside→outside 좌버튼12BYTE 기록이다.
    InputEvent rightOutside; //005cafd0/d4/d8의 마지막 inside→outside 우버튼12BYTE 기록이다.
};
// 폴링은 원본452e00의 사건을 공급하고 queue는 원본52f90의 raw code/x/y를 그대로 예약한다.
struct GumpPointerBoundaryHooks {
    std::function<InputEvent(std::uint32_t)> poll; // 요청 code마다 현재 OS 상태를 새로 읽는다. 콜백은 같은 state를 바꿀 수 있다.
    std::function<void(InputEvent)> queue; // 재진입 release만 좌→우로 예약한다. PushCommand의 코드 변환을 넣지 않는다.
};
// 10.78의004d6379..004d65a0이다. 현재 language00542384와 별도 정적 기록으로 정규화한 pointer를 반환한다.
// 직후 Guide4d65a7→카메라/Overlay/Broadcast/일반 사건 드레인은 호출자가 같은 현재 pointer로 이어 간다.
InputEvent PrepareGumpPointerBoundary(GumpPointerBoundaryState& state,std::uint32_t currentLanguage,
    const GumpPointerBoundaryHooks& hooks);
// 실제40칸 InputQueue::Push에 합성 release를 연결한다. 큐 포화·기존 사건 순서는 기존 큐가 소유한다.
InputEvent PrepareGumpPointerBoundary(GumpPointerBoundaryState& state,std::uint32_t currentLanguage,
    std::function<InputEvent(std::uint32_t)> poll,InputQueue& queue);
}
