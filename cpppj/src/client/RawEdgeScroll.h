// 원본004d65ac..004d67ad의 가장자리 입력과497e00의 현재 카메라 목표 쓰기다.
#pragma once
#include "client/InputEvent.h"
#include "client/ClientCameraGlobals.h"
#include <functional>
namespace netstorm::client { // 실제 GUI/OS 조회는 현재 소유자가 같은 호출 위치에서 공급한다.
struct RawEdgeScrollState {
    std::uint32_t& initializationFlags; // 기존 GumpPointerBoundaryState의005cafe8을 공유하며 비트4만 초기화한다.
    double& since; // 원문005cafc8의 최초 가장자리 시각이다. 화면/미션마다 임의 초기화하지 않는다.
};
struct RawEdgeScrollInput {
    std::uint32_t enabled{},dialog{},menuSerial{}; //005318f8/00565dd4/005cad24의 현재 값이다.
    std::int32_t borderX{},borderY{},width{},height{},x{},y{},speed{}; // 실제 창 테두리·논리 화면/현재 포인터·005318f0이다.
    double wall{}; // 원문0055b4d8의 현재 프레임 시각이며 프레임 간격이 아니다.
};
struct RawEdgeScrollHooks {
    std::function<InputEvent(std::uint32_t)> poll; // 현재 좌버튼/Shift의 원문452e00 결과를 새로 읽는다.
    std::function<std::uint32_t(std::uint32_t)> menu; //464020의 현재 serial 조회이며 top edge만 이 관문을 사용한다.
    std::function<bool()> blockingPopup; //4641d0의 직계4000h 자식 조회다.
    std::function<void()> closePopups; //466710의 Kill 요청만 실행하며 즉시 삭제하지 않는다.
    std::function<void(std::int32_t,std::int32_t)> request; //497e00의 실제 X/Y 이동 요청이다. 현재 시야 즉시 변경으로 대체하지 않는다.
};
std::array<float,2> ProcessEdgeScroll(RawEdgeScrollState state,const RawEdgeScrollInput& input,const RawEdgeScrollHooks& hooks); // 초기화→현재 관문→가속/팝업/Shift→목표 요청 순서이며 최종 방향X/Y를 반환한다.
void RequestCameraDelta(const RawCameraViewState& view,RawCameraTargetState& target,std::int32_t height,std::int32_t dx,std::int32_t dy,std::uint32_t& metaRebuild); //497e00: 현재 view+이동량을 목표에만 쓰고005c85dc를 한 번 증가시킨다.
}
