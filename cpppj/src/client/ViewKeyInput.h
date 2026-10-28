// 원본 일반 입력의 F2/F3/F8 순서와 현재 표시/안내 전역을 같은 Client에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // Guide/Gump/모달 관문 뒤의 확인된 표시/안내 키를 처리한다.
struct ViewKeyInputGlobals { // 원본 DWORD의 주소를 빌려 메뉴·Draw와 같은 값을 사용한다.
    std::uint32_t& hideBuildings; //0059a8ac의 현재 건물 숨김 값이다.
    std::uint32_t& redraw; //0059a8b0의 현재 월드 dirty 값이다.
};
struct ViewKeyInputHooks { // 원문에서 실제 도달한 외부 장치만 요구한다.
    std::function<bool()> paused; //00460de0의 현재 정지 여부이며 Tell 뒤 다시 조회한다.
    std::function<bool(std::string_view,std::int32_t&)> readInt; //00441270의 초기0 지역값을 쓰는 설정 조회다.
    std::function<void(std::string_view)> tell; //004cf960의 SetIslands(0/1) 섹션 호출이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> queue; //00452fe0의 같은 FIFO에 F3의0/2 또는 F8의0/0 인자를 넣는다.
    std::function<std::uint32_t()> fort; //00594fa0의 현재 Fort 값이며 missionPost594fa4나 Guide 포인터와 구별한다.
    std::function<std::uint32_t()> metamap; //00594fc8을 Fort0일 때만 조회한다. 해당 Tell 분기는 정적으로 연결한다.
    std::function<std::uint32_t()> editor; //005c85a4를 First 예약/Tell 뒤 다시 조회한다.
    std::function<void(std::uint32_t)> highlight; //004d5bb0의 원본 mode1 강조이며 선택 해제/건설 취소와 구별한다.
    std::function<std::uint32_t()> useBanners; //005319c0의 현재 DWORD이며 Config 문자열을 대신 읽지 않는다.
    std::function<void(std::uint32_t)> writeBanners; //F7의 useBanners=0/1 Config 쓰기이며 실제 onChanged가43bc40을 호출한다.
};
// 뗌/다른 키에는 장치를 호출하지 않는다. F7은 Shift 없는 배너 전환만 담당하며 Shift의 파일/진단은 별도다.
bool DispatchViewKeyInput(InputEvent event,ViewKeyInputGlobals globals,const ViewKeyInputHooks& hooks);
}
