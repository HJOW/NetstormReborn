// PATCH00416760의 현재 Player AI 조회와 피해 통지 래퍼를 복원한다.
#pragma once
#include "o/SquidOwner.h"
#include <functional>

namespace netstorm::o {
// 원본 AI 주소는 호스트 포인터가 아닌 opaque32 식별자로 실제 AI 몸체에 전달한다.
using PlayerAiDamage=std::function<void(std::uint32_t,Sid,Sid)>;
// CD의 AI 피해 몸체와 함수 경계가 다르므로 이 래퍼는 PATCH10.78 전용이다.
class RawPlayerDamageNotice {
public:
    // 같은 월드의 실제9개 Player 바이트와 현재 모드를 빌린다. AI 소비자는 비영 포인터에서 필수다.
    RawPlayerDamageNotice(std::span<const std::uint8_t> players,const SquidOwnerMode& mode,PlayerAiDamage ai={});
    // challenge 또는 owner<1 또는 현재 AI가0이면 원본대로 반환한다.
    void Notice(std::int32_t owner,Sid victim,Sid attacker) const;
private:
    std::span<const std::uint8_t> players_; // 원본0xb4폭 Player9의 공유 현재 저장소다.
    const SquidOwnerMode& mode_; // 각 호출 시 다시 읽는 원본 challenge 모드다.
    PlayerAiDamage ai_; // 실제00415460의 후속 소비자이며 NULL 경로에서 요구하지 않는다.
};
}
