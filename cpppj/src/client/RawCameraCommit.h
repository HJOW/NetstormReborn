// 497960의 현재 view/snapshot·dirty·배율 적용을 외부 화면/커서 장치와 구별한다.
#pragma once
#include "client/ClientCameraGlobals.h"
#include "client/Renderer.h"
#include <vector>

namespace netstorm::client { // 현재 논리 Renderer 자료에 원문 적용 순서를 수행한다.
// 원본 viewport의 시작점과 폭/높이다. right/bottom 좌표로 해석하지 않는다.
struct RawCameraCommitViewport {
    std::int32_t x{},y{},width{},height{}; // 005ca99c/a0/b4/b8의 네 현재 signed DWORD다.
};
// 같은 Renderer·snapshot과 변경 표를 값 복사 없이 빌린다.
struct RawCameraCommitState {
    RawCameraViewState& renderer; // Renderer 앞64BYTE의 현재 view다.
    RawCameraViewState& snapshot; // 전역0059a900의 별도 현재64BYTE다.
    RawCameraTargetState& target; // 같은 Renderer의48..58 한계/목표/배율이다.
    std::uint32_t& redraw; // Renderer+5c의 현재 전체 다시 그리기 DWORD다.
    std::uint32_t& worldRedraw; // 0059a8b0의 별도 현재 전역이다.
    std::vector<DirtyRegion>& dirty; // +64 count와+68/6a8의 같은 현재 표이며 장치 효과 뒤에도 다시 읽는다.
};
// callback 뒤 원문이 재조회하는 장치/viewport 전역의 현재 주소다.
struct RawCameraCommitGlobals {
    const std::uint32_t& screenFlags; // 005c78d4의 시차16/DIB2 비트를 읽는다.
    const std::int32_t& denominator; // 00540ce4의 현재 signed 시차 분모다.
    const std::int32_t& width; // 005c78f0의 현재 논리 폭이다.
    const std::int32_t& height; // 005c78f4의 현재 논리 높이다.
    const RawCameraCommitViewport& viewport; // 네 viewport DWORD의 현재 저장소다.
    const ScreenRect& fullViewport; // 005ca9cc의 별도 전체 viewport 사각형이다.
    const ScreenPoint& mouse; // 005cae94/98의 현재 실제 마우스 정수 좌표다.
};
// 복원된 커서/Gump/dirty 몸체와 화면 이동을 실제 호스트가 연결한다. 선택된 경계만 요구한다.
struct RawCameraCommitHooks {
    std::function<void(std::uint32_t)> lockCursor; // 446a10(0)의 현재 잠금/커서 효과다.
    std::function<void()> unlockCursor,redrawRoots; // 446a40 해제와4660f0 현재 Gump 재그리기다.
    std::function<void(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t)> scroll,scrollBackground; // 4a1ad0/4a1e00의 x/y/폭/높이/dx/dy다.
    std::function<void(ScreenRect,std::uint32_t)> addDirty; // 기존497580 변경 영역 추가 몸체다.
    std::function<void(ScreenRect)> addBackgroundDirty; // 4977f0 배경 변경 영역 추가 몸체다.
    std::function<void(ScreenPoint)> updateMouse; // 4a41c0의 현재 커서 장치 좌표 갱신이다.
    CameraViewAssertion assertion; // 두497490의 원문 진단을 같은 caller에 전달한다.
};
void CommitCameraTarget(RawCameraCommitState state,const RawCameraCommitGlobals& globals,const RawCameraCommitHooks& hooks); //497960 본체이며 Present 제한과 실제 Frame 호출 위치는 caller가 소유한다.
}
