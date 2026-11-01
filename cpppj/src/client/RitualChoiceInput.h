// 의식 보상 선택의 일반 입력은 즉시 기술 지급과 분리해 원문6b 요청으로 전송한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <span>

namespace netstorm::client { //원문 UserInput의 세 선택 분기만 다루며 멀티플레이 신규 검사는 하지 않는다.
inline constexpr std::uint32_t kUpgradeAltarCommand=0x020000d4U,kGetNewTechCommand=0x020000d5U,kRequestStormpowerCommand=0x020000f9U; //00542a44/48/00542ad8의 실제 명령 초기값이다.
struct RitualChoiceInputHooks { //현재 Session의 동일 Carrier.cpp 전역·타입·실제 전송을 빌린다.
    std::function<std::uint32_t()> dais,nuggetType,daisType; //005453dc/005412c0/005412ec의 현재 DWORD 조회다.
    std::function<void(std::uint32_t)> assertion; //대상0 진단 뒤 현재 전역을 다시 읽고 원문처럼 계속한다.
    std::function<void(std::span<std::uint8_t>)> construct; //0040af50의 실제 protocol1/message6b/length10 헤더 작성이다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //004e2dc0(9,packet)의 기존 비동기 송신 경계다.
};
bool IsRitualChoiceCommand(std::uint32_t code); //수정키/뗌을 제외한 실제 세 명령 번호만 판정한다.
void ConstructRitualChoicePacket(std::span<std::uint8_t> packet); //0040af50의 최초 헤더4BYTE만 쓰며 본문/뒤쪽 바이트는 보존한다.
bool DispatchRitualChoiceInput(const InputEvent& event,const RitualChoiceInputHooks& hooks); //뗌은 장치 조회 없이 소비하고 본문은 제단WORD/종류DWORD 폭을 유지한다.
}
