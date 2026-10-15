// 원본의 단락 평가·중복 Advance·SP 비교 순서를 유지한다.
#include "o/RawTutorialOne.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kCommandMask=0x03ffffff; // 원본이 이벤트의 modifier/release 상위 비트를 제거하는 마스크다.
constexpr std::uint32_t kBridgeGenus=4; // 원본 제작 수를 합산할 다리 부류다.
constexpr std::size_t kPlayerBytes=0xb4,kTempleOffset=0x7c; // 실제 Player 기록 폭과 현재 신전SID DWORD 위치다.
// 비정렬 Player DWORD를 little endian으로 읽는다.
std::uint32_t ReadPlayer(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result=0;
    // 신전 필드의 네 바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) result|=std::uint32_t(bytes[offset+i])<<(8*i);
    return result;
}
}
// 실제 저장소의 폭과 필수 효과를 확인하며 단계/목록/게임 상태를 변경하지 않는다.
RawTutorialOne::RawTutorialOne(TutorialOneState state,std::span<const std::uint8_t> players,const SquidPostPopList& geysers,TutorialOneHooks hooks)
    :state_(state),players_(players),geysers_(geysers),hooks_(std::move(hooks)) {
    if (players.size()!=9*kPlayerBytes || !hooks_.builtForGenus || !hooks_.graphFor || !hooks_.money || !hooks_.advance || !hooks_.spawnGeysers)
        throw std::invalid_argument("튜토리얼1 실제 Player/목록/하위 연결 오류");
}
// 같은 프레임에서 단계가 바뀌더라도 원본 switch가 선택한 한 분기만 실행한다.
std::uint32_t RawTutorialOne::Handle(std::uint32_t event) const {
    const auto command=event&kCommandMask; // A분기의 두 비교가 사용하는 같은 입력이다.
    switch (state_.stage) {
    case 'A':
        if (command==state_.homeCommand || hooks_.builtForGenus(kBridgeGenus)>0) hooks_.advance();
        if (command==state_.reclaimCommand) return 1;
        break;
    case 'B':
        if (hooks_.builtForGenus(kBridgeGenus)>7) hooks_.advance();
        break;
    case 'C':
        if (hooks_.builtForGenus(kBridgeGenus)>18) { hooks_.advance();hooks_.spawnGeysers(); }
        break;
    case 'D': {
        if (state_.localPlayer>=9) throw std::out_of_range("튜토리얼1 현재 Player 범위");
        const auto temple=ReadPlayer(players_,state_.localPlayer*kPlayerBytes+kTempleOffset); // 간헐천 순회 전에 신전Graph를 한 번만 구한다.
        const auto graph=hooks_.graphFor(temple); // Graph0/254도 원본은 그대로 비교한다.
        // 매 조건에서 현재 signed count를 확인하고 모든 일치 항목에 Advance를 요청한다.
        for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(geysers_.count);++index) {
            if (static_cast<std::size_t>(index)>=geysers_.entries.size()) throw std::out_of_range("튜토리얼1 간헐천 목록 물리 범위");
            if (hooks_.graphFor(geysers_.entries[static_cast<std::size_t>(index)])==graph) hooks_.advance();
        }
        break;
    }
    case 'E':
        if (state_.battleShown && hooks_.money(state_.localPlayer)>=200.0F) hooks_.advance();
        break;
    case 'F':
        if (state_.battleShown && hooks_.money(state_.localPlayer)>=600.0F) hooks_.advance();
        break;
    default: break;
    }
    return 0;
}
}
