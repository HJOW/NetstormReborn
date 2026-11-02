// 숫자 키의 저장 카메라10칸은 Tab의 최근 배치5칸과 별도 Client 전역이다.
#pragma once
#include "client/InputEvent.h"
#include "client/RawCameraView.h"
#include "o/RawConstructionHistory.h"
#include <functional>

namespace netstorm::client { // 원문 저장 Point의 값과 선택된 실제 장치만 빌린다.
struct SavedCameraPositions { // 005caeb0의80BYTE이며 일반 World/Renderer 교체로 초기화하지 않는다.
    std::array<o::ConstructionNoticePoint,10> points{}; // 최초 BSS0과 명시 시작의 무효 Point 초기화를 구별한다.
};
struct SavedCameraKeyHooks { // 정지 뒤 공개 언어와 선택음 이후 현재 지도 중심을 각각 조회한다.
    std::function<bool()> paused; //00460de0의 실제 현재 정지 판정이다.
    std::function<std::int32_t()> language; //00542384를 정지 판정 뒤 소비하며 현재 Config 문자열로 다시 추정하지 않는다.
    std::function<void()> sound; //004a9cb0의 select.wav/나머지0 다섯 인자를 실행한다.
    std::function<o::ConstructionNoticePoint()> capture; //선택음 뒤 실제 Renderer497e50의 지도 중심을 읽는다.
    std::function<void(float,float)> center; //저장된 두 단정도 좌표를433aa0 즉시 카메라에 전달한다.
};
void InitializeSavedCameraPositions(SavedCameraPositions& saved); //00439928의 최초10칸만 무효 Point로 쓴다.
o::ConstructionNoticePoint CaptureSavedCameraCenter(const RawCameraViewState& view,const std::array<std::int32_t,4>& viewport); //497e50의 현재 Renderer/지도LTRB와 signed DWORD 감김을 보존한다.
bool IsSavedCameraPositionValid(o::ConstructionNoticePoint point); //004d5020은 두 축 모두0 이상256 미만이며 WorldCells를 읽지 않는다.
bool DispatchSavedCameraKey(InputEvent event,SavedCameraPositions& saved,const SavedCameraKeyHooks& hooks); //숫자 호출/Shift 실제 기호 저장이며 다른 입력·뗌은 소비하지 않는다.
}
