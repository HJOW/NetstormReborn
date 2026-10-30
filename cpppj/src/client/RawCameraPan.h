// 원본4d6b54의 Alt/가운데 이동과 두 입력 큐 경로 선택을 분리한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { // 현재 UI/창 입력을 소비하며 카메라 현재값은 직접 쓰지 않는다.
struct CameraPanGlobals {
    std::int32_t scrollWhileMouseOut{},dialog{},active{}; //5318d0/565dd4/54dc50의 현재 비영 관문이다.
    std::int32_t clientWidth{},clientHeight{}; //5c78fc/7900의 현재 Win32 client 크기다.
    std::int32_t viewX{},viewY{},viewWidth{},viewHeight{}; //5ca99c/a0/b4/b8의 현재 지도 원점/크기다.
};
struct CameraPanHooks {
    std::function<InputEvent(std::uint32_t)> poll; //가운데/Shift를 각각 실제 도달 위치에서 조회한다.
    std::function<CameraPanGlobals()> globals; //폴링/팝업 callback 뒤 현재 전역을 다시 읽는다.
    std::function<void()> closePopups; //466710의 실제 팝업 Kill 요청이다.
    std::function<void(std::int32_t,std::int32_t)> request; //497e00의 현재 view 기준 목표 쓰기이며 즉시 Scroll과 다르다.
};
// true이면 원문 이동 전용 큐 경로다. 좌표-1/비활성에서도 팝업 정리 뒤 true이며 일반 입력을 재분배하지 않는다.
bool ProcessCameraPan(InputEvent pointer,const CameraPanHooks& hooks);
}
