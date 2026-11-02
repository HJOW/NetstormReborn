// 원문00413c60/004143b0의 직접 AI 본체 writer를 기존 가산/전략 프레임과 구별한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o { // 실제 Player9와 생존0xb08 본체의 현재 조회를 빌린다.
struct AiCommandStateHooks { // 값형 지갑/AI 사본이나 새 생성 성공을 제공하지 않는다.
    std::function<std::span<const std::uint8_t>()> players; // 각 원문 Player base 재조회에서 같은 현재1620BYTE를 읽는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> body; // 비영 opaque32 토큰을 같은 실제 생존 본체로 해석한다.
};
void CrankAiMoney(const AiCommandStateHooks& hooks); // 현재 owner1..8의 비영 brain+2d8을30000으로 직접 쓰며 mode/demo/SP는 읽지 않는다.
void SetAiParameter(std::uint32_t packed,const AiCommandStateHooks& hooks); // low3 owner/8bit selector/signed high16의 다섯 writer이며 selector0은 두 현재 포인터를 재조회한다.
}
