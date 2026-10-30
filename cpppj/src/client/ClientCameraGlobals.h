// Client 수명의 전역 snapshot과 논리 Renderer의 현재 view/목표를 별개 주소에서 보관한다.
#pragma once
#include "client/RawCameraView.h"

namespace netstorm::client { // 호스트8비트 Renderer 장치의 생성/소멸과 게임 카메라 수명을 구별한다.
// Renderer 앞64BYTE 밖의 목표/한계다. 목표 쓰기만으로 현재 view를 바꾸지 않는다.
struct RawCameraTargetState {
    std::array<std::int32_t,2> limits{}; // 원문+48/+4c: 칸 크기×세계 크기−화면 크기의 ftol 하위값이다.
    std::array<std::int32_t,2> pixels{}; // 원문+50/+54: 다음 적용할 signed 픽셀 원점이다.
    std::uint32_t zoom{}; // 원문+58: 다음 Q16 배율이며 현재 view.zoom과 구별한다.
};
// 00540ce0/ce4는 view 내부 zoom과 관계없는 별도 배경 분모 두 DWORD다.
struct RawRendererDenominatorState {
    std::int32_t defaultValue{15}; //10.78 PE의 초기값15이며 인자0일 때 선택한다.
    std::int32_t current{15}; // 4978d0이 원문 store 위치에서 바꾸는 현재 분모다.
};
// 모든16DWORD 참조보다 먼저 만들어 Session/World/호스트 화면 장치 교체 뒤에도 남는다.
class ClientCameraGlobals {
public:
    ClientCameraGlobals()=default; // 전역snapshot은 BSS0, 논리Renderer/목표의 예약0은0049a1d0 선행clear와 대응하며 명시NativeInit와 구별한다.
    ClientCameraGlobals(const ClientCameraGlobals&)=delete;ClientCameraGlobals& operator=(const ClientCameraGlobals&)=delete; // 실제 장기 주소를 복제하지 않는다.
    RawCameraViewState& Snapshot() { return snapshot_; } // 0059a900의 현재16DWORD이며 임의 픽셀에서 재계산하지 않는다.
    const RawCameraViewState& Snapshot() const { return snapshot_; } // 음향·커서·미니맵·월드가 같은 현재 값을 읽는다.
    RawCameraViewState& RendererView() { return rendererView_; } // 논리 Native Renderer의 별도 앞64BYTE다.
    const RawCameraViewState& RendererView() const { return rendererView_; } // snapshot과 주소가 같지 않다.
    RawCameraTargetState& Target() { return target_; } // 원문 목표·한계를 명시 호출 위치에서 읽고 쓴다.
    const RawCameraTargetState& Target() const { return target_; } // 목표를 현재값처럼 소비하지 않는다.
    RawRendererDenominatorState& Denominator() { return denominator_; } // 두 별도 분모의 같은 장기 owner다.
    const RawRendererDenominatorState& Denominator() const { return denominator_; } // Q16이나 카메라 원점과 합치지 않는다.
    void InitializeNativeRenderer(std::int32_t width,std::int32_t height,std::int32_t worldCells,std::uint32_t& worldRedraw,std::uint32_t& mapRevision); // 실제4993c0의 명시 초기화·목표·복사·두 마지막store를 수행한다.
    void UpdateTargetLimits(std::int32_t width,std::int32_t height,float worldWidth,float worldHeight); //497f80의 실제 계산만 적용하며 view/snapshot을 초기화하지 않는다.
    void RequestPixels(std::int32_t x,std::int32_t y); // 목표만 쓴다. 497e00/497f20의 좌표/전역 카운터 처리는 상위 caller가 소유한다.
    void RequestCenter(float x,float y,std::int32_t width,std::int32_t height,std::uint32_t& revision); //10.78의497f20: 목표X/Y와005c85dc만 쓰며 현재 view/snapshot·표시 장치는 프레임 말미까지 보존한다.
    void ApplyPixelsToRendererAndSnapshot(std::int32_t x,std::int32_t y); //497960의두497490 호출 조각이다. scroll/dirty/커서 통지 전체 부모 완료로 세지 않는다.
    void CopyRendererSnapshot(); //436450 프레임 말미의16DWORD 복사이며 target/viewport를 복사하지 않는다.
    void SelectDenominator(std::int32_t value,std::uint32_t& worldRedraw); //4978d0:0이면default 선택→변경시현재값store→redraw1이다.
private:
    RawCameraViewState snapshot_{}; // 실제 전역은 NativeInit/명시setter/프레임복사만 바꾼다.
    RawCameraViewState rendererView_{}; // 논리 Native Renderer는 호스트Screen/Renderer 재생성으로 교체하지 않는다.
    RawCameraTargetState target_{}; // 현재 view와 독립인 원문 목표/한계다.
    RawRendererDenominatorState denominator_{}; // PE 초기15/15의 별도 두 DWORD다.
};
}
