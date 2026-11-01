// 시나리오의 일반 연기·화면 번개·명시 무동작 명령을 같은 일반 입력에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <array>
#include <functional>
#include <string_view>

namespace netstorm::client { // 실제 현재 월드와 표시 생성 부모 사이의 원문 관문이다.
// 표시 생성 경계만 빌리며 새 입자/기술/화면 상태를 보관하지 않는다.
struct ScenarioEffectInputHooks {
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> smoke; //004a8660의 전체 부모 인자와 두0 오프셋이다.
    std::function<std::int32_t()> width; // 기술 조회 전에 읽는 현재00531860 화면 폭이다.
    std::function<std::uint32_t()> technology; //0048fc60의 현재 기술 부모 조회이며 새 객체 생성으로 대체하지 않는다.
    std::function<void(std::uint32_t,std::array<std::int32_t,2>,std::array<std::int32_t,2>,float)> lightning; //00472770의 부모/시작/종료/수명이다.
};
// 원본 네 이름만 대소문자 무시로 조회하고 나머지는 ffffffff다.
std::uint32_t ScenarioEffectScriptCode(std::string_view name);
// 실제 복원된 네 일반 입력 명령인지 플래그를 제외해 확인한다.
bool IsScenarioEffectCommand(std::uint32_t command);
// 키 뗌/명시 무동작에는 장치를 요구하지 않고 나머지는 원문 순서로 부모를 호출한다.
bool DispatchScenarioEffectInput(const InputEvent& event,const ScenarioEffectInputHooks& hooks);
}
