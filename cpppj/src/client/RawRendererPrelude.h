// 원문004994b0..00499758의 화면 flags/override/구름 행 스캔과 전체 dirty 준비 순서다.
#pragma once
#include "client/Screen.h"
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::client { // 프레임 앞단의 게임 계산과 실제 화면/구름 장치를 분리한다.
// 현재 인접 전역과 Renderer 카운터를 빌려 원문과 같은 저장소에 부분 효과를 남긴다.
struct RendererPreludeState {
    std::span<std::uint32_t,19> scan; //0059a984의 index·16행·초기화·이전 flags가 같은 인접 물리다. index-1/16/17의 인접 읽기도 보존한다.
    std::uint32_t& backgroundRedraw; //같은 Renderer+60이며 비DIB 전체화면 시차 분기만1을 쓴다.
    std::uint32_t& dirtyCount; //같은 Renderer+64이며 force redraw가 현재 표를 비운 뒤 새 전체 사각형을 요청한다.
    std::uint32_t& drawCounter; //0059a8d4의 현재 DWORD이며 모든 정상 경로가 마지막에0을 쓴다.
};
// 화면·카메라·지도 clip은 호출자가 소유하며 각 장치 효과 뒤에도 같은 참조로 읽는다.
struct RendererPreludeGlobals {
    const std::uint32_t& mode; //005c78d4의 현재 화면 flags이며 효과 뒤 재조회한다.
    const std::uint32_t& force; //0059a8b0의 같은 세계 redraw 요청이며 이 앞단은 지우지 않는다.
    const std::uint32_t& width;const std::uint32_t& height; //005c78f0/f4의 현재 논리 화면 크기다.
    const std::uint32_t& initialHeight; //00531864의 별도 초기 화면 높이이며 행 표 최초 구성에만 쓴다.
    const std::uint32_t& cameraY; //0059a920의 현재 카메라 Y 픽셀이며 매 행 조회 직전에 읽는다.
    const std::uint32_t& left;const std::uint32_t& top;const std::uint32_t& right;const std::uint32_t& bottom; //005ca9cc..d8의 현재 지도 clip이다.
};
// 실제 구름/dirty/override 장치는 선택된 위치에서만 요구하므로 생략 경로에 불필요한 의존을 만들지 않는다.
struct RendererPreludeHooks {
    std::function<void()> markCloudDirty; //00470c80의 같은 실제 all/partial DWORD 쓰기다.
    std::function<void(ScreenRect)> fullInvalidate; //004977f0의 현재 전체 화면 dirty 요청이며 rect는 효과 뒤 크기로 만든다.
    std::function<std::uint32_t()> overrideActive; //00463fe0의 실제 overrideHead 조회다. 일반 모달 개수와 구별한다.
    std::function<void()> updateClouds; //00470c50의 같은 난수/캐시/옵션 갱신이며 선택한 경우에만 실행한다.
    std::function<std::uint32_t(std::int32_t,std::int32_t)> rowDirty; //00470ce0의 현재 행/현재 cameraY 조회다.
    std::function<void(ScreenRect,std::uint32_t)> invalidate; //00497580의 clip된 행 또는 force 전체 dirty 추가다.
    std::function<void()> invalidateBackground; //00497860의 별도 시차 배경 dirty 처리다.
};
// Draw의 월드 좌표/수집/표시 앞에서 한 번 호출한다. 실제 Frame 연결/표시·정리는 호출자가 별도로 소유한다.
void PrepareRendererFrame(RendererPreludeState state,const RendererPreludeGlobals& globals,const RendererPreludeHooks& hooks);
}
