// State.cpp의67h 전체 부모를 일반 수신과 메타 수신이 같은 순서로 사용한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // 전역 옵션과 선택된 현재 CS/파생값·UI를 사본 없이 빌린다.
// typed의 bool과 Native의 DWORD를 같은 실제 수명으로 빌려 선택 시점의 값을 읽는다.
class StateBattleOptionFlag final {
public:
    StateBattleOptionFlag(const bool& value):boolean_(&value) {} //기존 typed 전역의 같은 bool 주소를 보관한다.
    StateBattleOptionFlag(const std::uint32_t& value):dword_(&value) {} //Native 전역의 전체 DWORD를 임시 bool로 바꾸지 않는다.
    StateBattleOptionFlag(const bool&&)=delete; //동기 수신 전에 사라지는 임시 bool의 주소는 보관하지 않는다.
    StateBattleOptionFlag(const std::uint32_t&&)=delete; //임시 DWORD를 현재 전역 주소로 가장하지 않는다.
    explicit operator bool() const { return dword_ ? *dword_!=0 : *boolean_; } //실제 UI 선택 조건에 도달할 때만 같은 현재 값을 읽는다.
private:
    const bool* boolean_{}; //독립 typed 입력의 실제 bool 주소이며 값 복사는 없다.
    const std::uint32_t* dword_{}; //Native DWORD 주소이며 상위 비트가 비영이어도 참으로 판단한다.
};

struct StateBattleOptionGlobals { //004b4900이 조회하거나 쓰는 같은 실제 전역 뷰다.
    std::span<std::uint8_t> options; //0052f7e0의 실제87BYTE이며 미복사 꼬리는 보존한다.
    const std::uint32_t& challenge; //005467d8의 현재 CS 존재 DWORD다.
    std::uint32_t& value; //005424bc의 현재 환불 비율 DWORD다.
    StateBattleOptionFlag battle; //00594fbc의 현재 bool/DWORD를 후속 효과 뒤 UI 선택 시점에 다시 읽는다.
    const bool& mission;const bool& saved; //594fa4/5c85d4의 기존 bool 전역 참조이며 변환 사본이 없다.
};
struct StateBattleOptionHooks { // 동기 호출보다 오래 사는 기존 함수 장치를 복사 없이 빌리며 선택한 효과만 요구한다.
    const std::function<void(std::string_view)>& assertion; //State.cppd51의 길이 진단이며 복귀 뒤 현재 길이를 다시 읽는다.
    const std::function<void(std::uint32_t,std::span<const std::uint8_t>)>& challengeOptions; //CS 비영에서만0042c920 전체 자식을 호출한다.
    const std::function<void()>& range; //004b4860의 현재 공급 반경/제곱 쓰기다.
    const std::function<std::int32_t()>& cost; //0041c8b0의 현재 옵션 비용 조회다.
    const std::function<void(float)>& storeCost; //현재00541240 TYPE+c4의 실제 float 쓰기다.
    const std::function<std::uint32_t()>& currentValue; //0041ca70의 현재 환불 비율 조회다.
    const std::function<void()>& reconnect; //선택 관문 뒤004c7880의 실제 전체 Uber 재생성이다.
};
void ReceiveStateBattleOptions(std::uint32_t sender,std::span<const std::uint8_t> packet,StateBattleOptionGlobals globals,const StateBattleOptionHooks& hooks); //004b4900 전체의 진단/복사→선택CS→범위/비용/비율→선택UI 부모다.
}
