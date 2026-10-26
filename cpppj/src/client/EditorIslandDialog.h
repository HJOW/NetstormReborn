// 실제 Add Island 메뉴와 아직 미복원인 전용 GUI 생성 경계를 구분한다.
#pragma once
#include "client/InputEvent.h"
#include <cstdint>
#include <functional>

namespace netstorm::client { //편집 창 명령만 소유하며 기존 NewFort의 부모를 호출하지 않는다.
inline constexpr std::uint32_t kAddIslandCommand=0x02000022U; //00542780의 실제 보호PE 초기 명령이다.
// release는 소비만 한다. 실제 factory→Attach→controlled는 필수 장치가 수행한다.
bool DispatchEditorIslandDialog(const InputEvent& event,const std::function<void()>& createAttachControlled);
}
