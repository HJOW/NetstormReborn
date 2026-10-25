// Renderer의 앞64BYTE와 전역 snapshot을 같은 원문 변환 본문으로 읽고 쓴다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <type_traits>

namespace netstorm::client { // 순수 좌표/바이트 처리는 장치나 Session을 요구하지 않는다.
//10.78 PATCH0059a900부터 시작하는 정확16DWORD다. 최초 전역snapshot은 BSS0이며 CD 전체배치는 여기서 완료로 세지 않는다.
struct RawCameraViewState {
    std::array<std::uint32_t,2> worldBits{}; // +00/+04: 4971e0이 읽는 월드 float 비트이며 Q16/픽셀이 아니다.
    std::array<std::int32_t,2> cellPixels{}; // +08/+0c: 현재 zoom을 적용한 정수 칸 크기다.
    std::array<std::int32_t,2> baseCellPixels{}; // +10/+14: 기본 정수16/11이다.
    std::uint32_t zoom{}; // +18: 현재 Q16 배율이며 배경 분모00540ce4와 별개다.
    std::array<std::int32_t,2> pixels{}; // +1c/+20: 현재 signed 픽셀 원점0059a91c/920이다.
    std::array<std::uint32_t,2> cellFloatBits{}; // +24/+28: 현재 칸 크기의 float 비트다.
    std::array<std::uint32_t,2> baseCellFloatBits{}; // +2c/+30: 기본16/11 float 비트다.
    std::uint32_t zoomFloatBits{}; // +34: 현재 Q16을 float로 변환한 비트다.
    std::array<std::uint32_t,2> pixelFloatBits{}; // +38/+3c: 픽셀 원점의 단정도 사본이며 월드 float와 구별한다.
};
static_assert(sizeof(RawCameraViewState)==64 && std::is_trivially_copyable_v<RawCameraViewState>); // Native16DWORD 복사를 그대로 보존한다.
static_assert(offsetof(RawCameraViewState,worldBits)==0 && offsetof(RawCameraViewState,cellPixels)==8 && offsetof(RawCameraViewState,baseCellPixels)==16); // 첫 세 쌍의 원문 위치다.
static_assert(offsetof(RawCameraViewState,zoom)==24 && offsetof(RawCameraViewState,pixels)==28 && offsetof(RawCameraViewState,cellFloatBits)==36); // Q16과 픽셀/float 칸 위치다.
static_assert(offsetof(RawCameraViewState,baseCellFloatBits)==44 && offsetof(RawCameraViewState,zoomFloatBits)==52 && offsetof(RawCameraViewState,pixelFloatBits)==56); // 마지막 다섯 DWORD의 원문 위치다.
using CameraViewAssertion=std::function<void(std::string_view,std::uint32_t)>; // Renderer.cpp의 원래 진단 문자열/행을 소비하며 선택된 미연결은 거부한다.
void InitializeCameraView(RawCameraViewState& view,CameraViewAssertion assertion={}); // 4993c0의 view 초기 쓰기와4973e0(0,0)이며 snapshot 복사는 caller가 소유한다.
void SetCameraViewWorld(RawCameraViewState& view,float x,float y,CameraViewAssertion assertion={}); //4973e0: 입력F32를 보존하고 PC027f의 double곱→ftol 하위값을 쓴다. 극단칸크기의PC037f 차이는 독립probe에 명시한다.
void SetCameraViewPixels(RawCameraViewState& view,std::int32_t x,std::int32_t y,CameraViewAssertion assertion={}); // 497490: Y→X→월드 float→픽셀 float 순서이며 다른10DWORD를 보존한다.
void CopyCameraViewSnapshot(RawCameraViewState& destination,const RawCameraViewState& source); // Native rep movsd10h의 정확64BYTE 값 복사이며 두 주소를 합치지 않는다.
std::array<std::int32_t,2> ProjectCameraCoordinates(const std::int32_t& cameraX,const std::int32_t& cameraY,float x,float y); // 기존 SoundToScreen과497220의 계산을 한 본문으로 공유한다.
std::array<std::int32_t,2> ProjectCameraView(const RawCameraViewState& view,float x,float y); // 현재 snapshot의 픽셀 두 칸을 직접 읽어497220을 실행한다.
std::int32_t CameraFtolLow(double value); // 실제 signed64 ftol의 하위 signed32이며 미정값의 하위DWORD는0이다.
}
