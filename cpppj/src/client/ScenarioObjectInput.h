// 시나리오와 객체 메뉴의 공통 명령을 현재 월드의 가상6c에 전달한다.
#pragma once
#include "client/InputEvent.h"
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // 원문 일반 입력의 메뉴와 독립적인 객체 관문이다.
// 원본 이름표의 다섯 이름만 조회하고 미등록 이름에는 ffffffff를 반환한다.
std::uint32_t ScenarioObjectScriptCode(std::string_view name);
// 입력 플래그를 제외한 명령이 복원된 공통 객체 입력인지 확인한다.
bool IsScenarioObjectCommand(std::uint32_t command);
// 키 뗌/낮은 SID 0은 소비하고, 가상6c의 반환은 다음 일반 입력의 처리에 사용하지 않는다.
bool DispatchScenarioObjectInput(const InputEvent& event,const std::function<std::uint32_t(o::Sid,std::uint32_t)>& objectInput);
}
