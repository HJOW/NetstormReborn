// 원문 직접 writer는 지갑 Credit/데모 하한/분산SP 갱신을 호출하지 않는다.
#include "o/RawAiCommandState.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 현재 Player와 실제 AI 생존 저장소만 사용한다.
namespace { // 같은 물리 DWORD 판독/기록과 도달한 실제 경계를 확인한다.
constexpr std::size_t kPlayerBytes=180,kPlayerSlots=9,kBrainOffset=0xb0,kBodyBytes=0xb08; // 10.78 Player 표와 AI 본체의 물리 폭이다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("AI 직접 명령의 실제 현재 저장소 미연결");return hook; } // 선택하지 않은 하위 경계를 미리 요구하지 않는다.
// 현재 Player base를 각 방문/재조회 위치에서 새로 읽는다. 원문 감긴 NULL 주소는 host의 유효 span으로 합성하지 않는다.
std::uint32_t Brain(std::uint32_t owner,const AiCommandStateHooks& hooks) {
    const auto players=Need(hooks.players)();if (players.size()!=kPlayerBytes*kPlayerSlots) throw std::invalid_argument("AI 직접 명령의 현재 Player9 누락");
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(players[owner*kPlayerBytes+kBrainOffset+byte])<<(byte*8);return value; // 해당+b0 네 BYTE만 읽는다.
}
// 같은 생존 본체만 반환하며 해제 포인터/NULL 재조회/잘못된 폭을 성공으로 바꾸지 않는다.
std::span<std::uint8_t> Body(std::uint32_t token,const AiCommandStateHooks& hooks) {
    if (!token) throw std::logic_error("AI 직접 명령의 재조회된 NULL 본체");auto raw=Need(hooks.body)(token);if (raw.size()!=kBodyBytes) throw std::invalid_argument("AI 직접 명령의 실제0xb08 본체 누락");return raw;
}
// 한 DWORD 외의 본체 바이트를 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) raw[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 원문 little endian 물리 네 BYTE만 쓴다.
}
}
// 원문은 중립0을 방문하지 않고 여덟 현재 포인터를 각각 읽어 고정30000을 쓴다.
void CrankAiMoney(const AiCommandStateHooks& hooks) {
    for (std::uint32_t owner=1;owner<kPlayerSlots;++owner) { // 앞선 실제 경계가 후속 Player를 바꾸더라도 다음 방문은 현재값이다.
        const auto token=Brain(owner,hooks);if (token) Put(Body(token,hooks),0x2d8,30000);
    }
}
// owner는 일반1..8이 아니라 packed low3이며 selector는 정확히4..11bit다.
void SetAiParameter(std::uint32_t packed,const AiCommandStateHooks& hooks) {
    const auto owner=packed&7U;if (!owner) return;const auto token=Brain(owner,hooks);if (!token) return;
    const auto selector=(packed>>4)&255U;if (selector>4) return;const auto value=std::bit_cast<std::int32_t>(packed)>>16;
    if (!selector) {
        const auto bits=std::bit_cast<std::uint32_t>(static_cast<float>(value));
        Put(Body(Brain(owner,hooks),hooks),0x220,bits);Put(Body(Brain(owner,hooks),hooks),0x600,bits);return; // 원문 fst 뒤Player+b0를 다시 읽는 두 포인터를 한 저장 포인터로 합치지 않는다.
    }
    const auto bits=selector==1 ? std::bit_cast<std::uint32_t>(static_cast<float>(value)) : std::bit_cast<std::uint32_t>(value);
    constexpr std::size_t offsets[]{0,0x228,0x230,0x234,0x24c}; // selector1..4의 float/DWORD 기록 위치다.
    Put(Body(token,hooks),offsets[selector],bits); // 나머지는 원문ESI에 보관한 첫 brain을 사용한다.
}
}
