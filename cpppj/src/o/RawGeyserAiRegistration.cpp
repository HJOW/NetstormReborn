// 같은 Player 원본 BYTE를 직접 읽으며 AI가 없는 최초 로드의 실제 순회를 실행한다.
#include "o/RawGeyserAiRegistration.h"
#include <utility>

namespace netstorm::o {
// 실제 첫 하위 요청의 네 DWORD를 그대로 유지하고 완료하지 않은 AI 몸체를 알린다.
GeyserAiRegistrationBoundary::GeyserAiRegistrationBoundary(std::uint32_t player,std::uint32_t pointer,
    std::uint32_t geyser,std::uint32_t operation)
    :std::runtime_error("간헐천 등록의 nonnull AI 하위 몸체는 아직 복원되지 않았습니다"),
     owner(player),aiPointer(pointer),sid(geyser),state(operation) {}

// 현재 판본의 동일한 9기록 BYTE 저장소를 받아 별도 Player 복사본을 만들지 않는다.
RawGeyserAiRegistration::RawGeyserAiRegistration(OriginalEdition edition,
    std::span<const std::uint8_t> playerRecords)
    :players_(playerRecords),stride_(edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU),
     aiOffset_(edition==OriginalEdition::Patch1078 ? 0xb0U : 0xa8U) {
    if (players_.size()<9*stride_) throw std::invalid_argument("간헐천 등록에는 실제 Player 9기록이 필요합니다");
}

// 실제 번호순 null 검사를 실행하며 각 하위 뒤 다음 Player의 현재 포인터를 다시 읽는다.
void RawGeyserAiRegistration::Register(std::uint32_t sid,std::uint32_t state) const {
    // Player0은 원본 순회 대상이 아니다. 1..8의 현재 포인터를 매 호출 다시 읽는다.
    for (std::uint32_t owner=1;owner<9;++owner) {
        const auto field=owner*stride_+aiOffset_; // 해당 Player의 실제 32비트 AI 위치다.
        const auto ai=static_cast<std::uint32_t>(players_[field]) |
            (static_cast<std::uint32_t>(players_[field+1])<<8) |
            (static_cast<std::uint32_t>(players_[field+2])<<16) |
            (static_cast<std::uint32_t>(players_[field+3])<<24); // 정렬되지 않아도 원본 little endian DWORD를 보존한다.
        if (ai) { if (!update_) throw GeyserAiRegistrationBoundary(owner,ai,sid,state);update_(ai,sid,state); }
    }
}
// 빈 성공을 허용하지 않고 실제 PATCH 하위만 바인딩한다.
void RawGeyserAiRegistration::BindPatch(std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> update) { if (stride_!=0xb4 || !update) throw std::invalid_argument("간헐천 AI의 실제 PATCH 하위 누락/판본 오류");update_=std::move(update); }
}
