// 원본 송금의 금액 조회·플래그 분기·SP와 지갑을 별도 상태로 보존한다.
#include "client/SendMoneyInput.h"
#include "o/OriginalText.h"
#include "o/ConfigInterface.h"
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>

namespace netstorm::client { // 실제 입력 사건만 처리하며 메뉴 생성 조건을 실행에 덧붙이지 않는다.
namespace { // 도달한 경계와 정수 패킷 작성만 지역화한다.
// 선택된 경계가 없으면 그 자리에서 실패하며 이미 발생한 효과를 되돌리지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Send Money 실제 장치 미연결");return function; }
// 원본 _ftol의64비트 절삭 후 하위DWORD를 쓰며 불확정 정수의 하위DWORD는0이다.
std::int32_t Amount(float value) { const double exact=value;if (!std::isfinite(exact) || exact>=9223372036854775808.0 || exact< -9223372036854775808.0) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(exact))); }
// 같은 고정/All/X의 플래그 분기를 공유하되 All에는 원문에 없는 inSingle 진단을 넣지 않는다.
void Transfer(std::uint32_t target,std::int32_t amount,bool diagnose,const SendMoneyHooks& h) {
    const auto flags=Need(h.flags)(target);const auto negative=static_cast<float>(std::bit_cast<std::int32_t>(0U-std::bit_cast<std::uint32_t>(amount))); // 원본 DWORD neg 뒤 signed fild다.
    if (flags&0x20U) {
        if (diagnose && Need(h.single)() && !Need(h.mission)()) Need(h.assertion)();
        Need(h.addLocal)(negative);Need(h.wallet)(target,amount);
    } else if (flags&1U) {
        Need(h.addOwner)(Need(h.local)(),negative);
        std::array<std::uint8_t,11> packet{1,0x6c,11,0,static_cast<std::uint8_t>(target),static_cast<std::uint8_t>(Need(h.local)()),0}; //0040af90 초기 헤더와 실제 송금 kind0이다.
        const auto value=std::bit_cast<std::uint32_t>(amount);
        for (std::size_t byte=0;byte<4;++byte) packet[7+byte]=static_cast<std::uint8_t>(value>>(8*byte)); //원본 비정렬 signed 금액을 낮은 BYTE부터 쓴다.
        Need(h.send)(packet);
    }
}
}
bool IsSendMoneyCommand(std::uint32_t code) { //실제 입력 코드의 하위26비트만 비교한다.
    code&=0x03ffffffU;return code==0x02000120U || code==0x02000108U || code==0x020000ecU || code==0x02000100U || code==0x0200012bU || code==0x02000118U;
}
std::uint32_t SendMoneyScriptCode(std::string_view name) { // 원본 이름표의 확인 버튼 연결을 복원한다.
    return o::AsciiLower(name)=="sendallbucks" ? 0x02000118U : 0xffffffffU;
}
bool DispatchSendMoneyInput(const InputEvent& event,const SendMoneyHooks& h) {
    const auto code=event.code&0x03ffffffU;if (!IsSendMoneyCommand(code)) return false;if (event.code&0x80000000U) return true;
    const auto target=std::bit_cast<std::uint32_t>(event.y);
    if (code==0x02000100U) { Need(h.numberDialog)(target);return true; }
    if (code==0x0200012bU) { Need(h.askAll)(target);return true; }
    if (code==0x02000118U) {
        if (Need(h.canAfford)(Need(h.money)())) Transfer(target,Amount(Need(h.money)()),false,h);
        return true;
    }
    const auto amount=code==0x02000120U ? 300 : code==0x02000108U ? 700 : 1000;
    if (Need(h.canAfford)(static_cast<float>(amount))) Transfer(target,amount,true,h);return true;
}
std::uint32_t ConfirmSendMoney(std::uint32_t target,std::string_view text,const SendMoneyHooks& h) {
    const auto line=text.substr(0,text.find('\n'));const std::string terminated(line);const auto amount=o::ConfigParseLong(terminated); // 원본 CopyText/첫LF/NUL 뒤 _atol 의미를 기존 CRT 파서로 유지한다.
    if (amount>0) {
        const auto visible=Need(h.known)() ? Need(h.money)() : 0.0F;
        if (static_cast<float>(amount)<=visible && Need(h.canAfford)(static_cast<float>(amount))) Transfer(target,amount,true,h);
    }
    return 1;
}
}
