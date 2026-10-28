// 일반 송금 명령의 원본 금액·AI 지갑·로컬 메시지 전송 순서를 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // 메뉴와 HTML 확인 버튼이 같은 송금 부모를 사용한다.
// 원본 일반 입력이 읽는 현재 상태와 이미 복원된 경제/표시 장치다.
struct SendMoneyHooks {
    std::function<std::uint32_t()> local,known,editor,mission,single; // 실제 현재 로컬 번호·PlayerKnown·편집·미션·단독 상태다.
    std::function<float()> money; //0040ee90의 현재 로컬 SP다.
    std::function<bool(float)> canAfford; //00425e90의 현재 SP/Editor 비교다.
    std::function<std::uint32_t(std::uint32_t)> flags; //현재 Player+50 DWORD다.
    std::function<void(float)> addLocal; //0040eec0의 실제 차감과 지갑·표시 갱신이다.
    std::function<void(std::uint32_t,float)> addOwner; //0041dff0의 현재 소유자 차감이다.
    std::function<void(std::uint32_t,std::int32_t)> wallet; //00413bb0의 실제 AI 지갑 가산이다.
    std::function<void(std::span<const std::uint8_t>)> send; //004e2dc0의 대상9이며11BYTE 메시지를 복사한다.
    std::function<void(std::uint32_t)> numberDialog,askAll; //실제 원본 X 입력 창과 asksend 확인 창이다.
    std::function<void()> assertion; //inSingle 진단 뒤 정상 복귀를 보존하는 장치다.
};
// 메뉴/HTML 이름표에서 확인한 여섯 명령만 허용한다.
bool IsSendMoneyCommand(std::uint32_t code);
// 대소문자 무시 이름 조회이며 모르는 이름은ffffffff다.
std::uint32_t SendMoneyScriptCode(std::string_view name);
// 원본 일반 입력의 공통 release 관문과 고정/X/All 명령을 실행한다.
bool DispatchSendMoneyInput(const InputEvent& event,const SendMoneyHooks& hooks);
//004c9350의 숫자 창 확정은 잘못된 금액에서도1을 반환해 창을 닫는다.
std::uint32_t ConfirmSendMoney(std::uint32_t target,std::string_view text,const SendMoneyHooks& hooks);
}
