// 화면 장치/Session 교체보다 오래 살아 있는 실제 Renderer 전역을 같은 주소에서 보관한다.
#pragma once
#include "client/RawRendererClouds.h"
#include <array>

namespace netstorm::client { // Native 전역과 짧은 호스트 출력 장치의 수명을 구분한다.
static_assert(sizeof(ScreenRect)==16); //원문 구름4칸의64BYTE 물리 배치를 유지한다.
// 명시 초기화나 생성 효과 없이 PE/BSS의0 이미지와 실제 인접 배치를 유지한다.
class ClientRendererFrameGlobals {
public:
    ClientRendererFrameGlobals()=default; //최초19DWORD/구름4칸/count/묶음 숨김은 모두 BSS0이다.
    ClientRendererFrameGlobals(const ClientRendererFrameGlobals&)=delete;ClientRendererFrameGlobals& operator=(const ClientRendererFrameGlobals&)=delete; //빌린 원문 전역 주소를 복제하지 않는다.
    std::span<std::uint32_t,19> Scan() { return scan_; } //0059a984의 같은 index/16행/초기화/이전 flags를 빌린다.
    std::span<const std::uint32_t,19> Scan() const { return scan_; } //진단은 상태를 초기화하지 않는다.
    RendererCloudQueue CloudQueue() { return {cloudRectangles_,cloudDirtyCount_}; } //4977f0/497860/4994b0이 같은4칸과count를 공유한다.
    const std::array<ScreenRect,4>& CloudRectangles() const { return cloudRectangles_; } //실제 현재4칸을 표시/검사에서 읽는다.
    std::uint32_t& CloudDirtyCount() { return cloudDirtyCount_; } //0059a8d4는 일반 Renderer dirty 개수와 다르다.
    const std::uint32_t& CloudDirtyCount() const { return cloudDirtyCount_; } //표시 전후의 실제 카운터를 관측한다.
    std::uint32_t& HideGroups() { return hideGroups_; } //0059a8b4의 실제 DWORD이며 건물 숨김0059a8ac와 다르다.
    const std::uint32_t& HideGroups() const { return hideGroups_; } //현재 표시 선택은 값을 복사하거나 bool로 정규화하지 않는다.
private:
    std::array<std::uint32_t,19> scan_{}; //서로 인접한 실제0059a984..9cf의76BYTE다.
    std::array<ScreenRect,4> cloudRectangles_{}; //0059a940..97f의64BYTE이며 화면 dirty100칸에 합치지 않는다.
    std::uint32_t cloudDirtyCount_{},hideGroups_{}; //0059a8d4/0059a8b4의 별도 최초BSS0 DWORD다.
};
}
