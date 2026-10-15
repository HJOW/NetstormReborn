// 원본은 Player의 AI DWORD를 읽은 뒤에만 __thiscall 피해 반응을 요청한다.
#include "o/RawPlayerDamageNotice.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 실제9기록의 폭을 확인하며 합성 AI 목록을 만들지 않는다.
RawPlayerDamageNotice::RawPlayerDamageNotice(std::span<const std::uint8_t> players,const SquidOwnerMode& mode,PlayerAiDamage ai)
    :players_(players),mode_(mode),ai_(std::move(ai)) {
    if (players_.size()!=9*0xb4) throw std::invalid_argument("피해 통지 Player9 바이트 크기 오류");
}
// 실제 래퍼의 모드→owner→현재 AI 조회→두 SID 통지 순서다.
void RawPlayerDamageNotice::Notice(std::int32_t owner,Sid victim,Sid attacker) const {
    if (mode_.challenge || owner<1) return;
    if (owner>=9) throw std::out_of_range("피해 통지 Player 번호가 실제9기록 범위 밖임");
    const auto offset=static_cast<std::size_t>(owner)*0xb4+0xb0; // 원본 현재 Player의 AI 포인터 필드다.
    std::uint32_t token=0; // 비정렬 opaque32 AI 주소의 little endian 비트다.
    // 현재 네 바이트를 읽는다. 주변 상태나 저장소를 바꾸지 않는다.
    for (std::size_t i=0;i<4;++i) token|=std::uint32_t(players_[offset+i])<<(8*i);
    if (!token) return;
    if (!ai_) throw std::logic_error("실제 Player AI 피해 반응00415460 미연결");
    ai_(token,victim,attacker);
}
}
