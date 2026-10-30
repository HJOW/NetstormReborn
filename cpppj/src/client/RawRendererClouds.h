// 원문4977f0/497860과499bfd..499cfe의 구름 사각형 저장·시차 갱신·배경 선택이다.
#pragma once
#include "client/Screen.h"
#include <functional>
#include <span>

namespace netstorm::client { // 일반 화면 dirty 표와 별도인 원문 구름 처리 본체다.
// 실제4칸과0059a8d4의 같은 count를 빌리며 함수가 임의로 초기화하지 않는다.
struct RendererCloudQueue {
    std::span<ScreenRect,4> rectangles; //0059a940..97f의 네 사각형이며 유효 범위 밖 쓰기는 호스트 오류로 구별한다.
    std::uint32_t& count; //0059a8d4의 현재 signed 비교 대상 DWORD다.
};
// 실제 진단/색 지우기/구름 픽셀은 선택한 경로에서만 요구한다.
struct RendererCloudHooks {
    std::function<void(std::uint32_t)> assertion; //4977f0의 cloudDirtyRectCount < NUM_CLOUD_DIRTY_RECT 진단 행을 받는다.
    std::function<void(ScreenRect,std::uint32_t)> clear; //4a2060의 같은 현재 네 좌표/전체 색 DWORD다.
    std::function<void(ScreenRect,std::int32_t,std::int32_t,bool)> draw; //false=4711d0 일반, true=471410 별도 시차 표면이며 같은 장치로 속이지 않는다.
};
// 배경 선택 효과 뒤에도 같은 현재 화면·색·카메라를 참조한다.
struct RendererBackdropGlobals {
    const std::uint32_t& mode;const std::uint32_t& hideGroups; //005c78d4/0059a8b4이며 F2의 다른0059a8ac와 합치지 않는다.
    const std::uint32_t& tint;const std::uint32_t& black; //00540cdc의 현재 날씨색과005b5e58의 기본 검정색이다.
    const std::int32_t& cameraX;const std::int32_t& cameraY; //0059a91c/920의 현재 snapshot 픽셀 원점이다.
    const std::int32_t& denominator; //00540ce4의 실제 signed 분모이며 Q16 배율과 구별한다.
};
void AppendRendererCloudDirty(RendererCloudQueue queue,ScreenRect rect,const std::uint32_t& checking,const RendererCloudHooks& hooks); //4977f0:4칸 한계/진단 뒤 count 재조회·부분 store를 보존한다.
void RefreshRendererCloudBackground(RendererCloudQueue queue,const std::int32_t& cameraX,const std::int32_t& cameraY,const std::int32_t& denominator,const RendererCloudHooks& hooks); //497860:현재 count/카메라 재조회와 Y→X IDIV 뒤 별도 표면을 요청하고 정상 종료에만 count0을 쓴다.
void DrawRendererBackdrop(ScreenRect& clipped,const std::uint32_t& dirtyFlags,const std::uint32_t& covered,const RendererBackdropGlobals& globals,const RendererCloudHooks& hooks); //499bfd..cfe:구름/날씨색/별도 표면 표식을 원문처럼 선택하며 수집/스프라이트/정리는 호출자가 소유한다.
}
