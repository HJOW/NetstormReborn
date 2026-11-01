// Tab의 최근 건설 위치 순환과 최초5칸 초기화를 원문의 현재 장부에 적용한다.
#pragma once
#include "client/InputEvent.h"
#include "o/RawConstructionHistory.h"
#include <functional>

namespace netstorm::client { // 입력은 Client 장기의 같은 이력과 실제 즉시 카메라 장치를 빌린다.
struct ConstructionHistoryKeyHooks { // 선택된 실제 정지/카메라만 호출한다.
    std::function<bool()> paused; //00460de0의 현재 정지 판정이다.
    std::function<void(float,float)> center; //00433aa0의 두 단정도 인자를 받는 실제 즉시 중심 이동이다.
};
void InitializeConstructionHistory(o::ConstructionHistoryState& history); //0043994c의40BYTE만 -10000 두 좌표로 쓰며 현재 순번은 보존한다.
bool IsConstructionHistoryPointValid(o::ConstructionNoticePoint point,std::int32_t worldCells); //0040e800의0<각 축<float(signed 세계 크기)를 그대로 판정한다.
bool DispatchConstructionHistoryKey(InputEvent event,o::ConstructionHistoryState& history,const std::int32_t& worldCells,const ConstructionHistoryKeyHooks& hooks); //뗌/다른 키는 미소비, 정지된Tab은 효과 없이 소비한다.
}
