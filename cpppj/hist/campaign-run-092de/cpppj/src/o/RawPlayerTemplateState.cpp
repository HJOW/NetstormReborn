// existing Stategetter의pos0/최소28과 Notify의isMine재조회/선택저장거부를보존한다.
#include "o/RawPlayerTemplateState.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 현재9개Template/State와선택진단·CD깊이효과를확인한다.
RawPlayerTemplateState::RawPlayerTemplateState(OriginalEdition edition,std::span<std::uint8_t> headers,
    std::array<PlayerTemplateStateStream,9>& states,std::array<std::string,9>& fileNames,
    std::uint32_t& loadingDepth,const bool& inMission,std::function<void(PlayerTemplateStateAssertion)> assertion,
    std::function<void(std::uint32_t)> depthChanged)
    :edition_(edition),headers_(headers),states_(states),names_(fileNames),depth_(loadingDepth),mission_(inMission),assertion_(std::move(assertion)),depthChanged_(std::move(depthChanged)) {
    if (headers.size()!=180 || !assertion_ || !depthChanged_) throw std::invalid_argument("Player Template실제9기록/State/진단/깊이효과누락");
}
// 호스트포인터로reinterpret하지않고매조회현재DWORD를읽는다.
std::uint32_t RawPlayerTemplateState::Header(std::uint32_t slot,std::size_t offset) const {
    if (slot>=9 || offset>16) throw std::out_of_range("Player Template실제9개/20BYTE기록범위");std::uint32_t value=0;
    // 실제네BYTE필드만낮은비트부터결합한다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(headers_[slot*20+offset+index])<<(8*index);
    return value;
}
// 실제State이름선택/조회는이9개전용뷰의동일Statecursor를사용하며데이터사본을만들지않는다.
std::span<std::uint8_t> RawPlayerTemplateState::Get(std::uint32_t slot) const {
    if (edition_==OriginalEdition::Cd1072) { ++depth_;depthChanged_(depth_);--depth_;depthChanged_(depth_); }
    if (!Header(slot,0)) assertion_(PlayerTemplateStateAssertion::MissingStateTemplate);
    if (!Header(slot,0)) throw std::logic_error("Player Stategetter실제manager0메모리경계");
    auto& stream=states_.at(slot);stream.position=0; // 실제lookupState(writable1)의현재MemStream pos0쓰기다.
    if (!stream.present || !stream.end) throw std::logic_error("Player 없는State초기화/stack미정BYTE/힙생성미복원");
    if (std::bit_cast<std::int32_t>(stream.end)<28) throw std::logic_error("Player shortState실제DataManager힙확장미복원");
    if (stream.bytes.size()<28 || stream.end>stream.bytes.size()) throw std::out_of_range("Player 현재State물리BYTE범위");return stream.bytes;
}
// 실제isMine후재검사에의한earlyreturn은빈callback대체가아니다.
void RawPlayerTemplateState::Changed(std::uint32_t slot) const {
    if (slot>=9 || names_.at(slot).empty()) throw std::out_of_range("Player Notify현재manager.fileName물리BYTE누락");
    if (names_[slot][0]=='.') assertion_(PlayerTemplateStateAssertion::DotFileName);
    if (Header(slot,8)) {
        if (Header(slot,12)) assertion_(PlayerTemplateStateAssertion::MineTransientFlag);
        if (Header(slot,8)) return;
    }
    if (mission_) assertion_(PlayerTemplateStateAssertion::InMissionSave);
    if (!Header(slot,0)) assertion_(PlayerTemplateStateAssertion::MissingNotifyTemplate);
    if (Header(slot,16)) return;
    throw std::logic_error("Player Template실제DataManager파일저장44d8d0/CD44cc40미복원");
}
}
