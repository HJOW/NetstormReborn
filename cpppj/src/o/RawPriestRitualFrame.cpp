// 의식 반복의 처음4프레임 생략과605 유무에 따른 소리 요청을 원본 순서로 유지한다.
#include "o/RawPriestRitualFrame.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr std::uint32_t kRestartOffset=4,kSoundOffset=14,kSuppressEvent=605; // 원본 반복 시작/소리 프레임과 소리를 억제하는 현재 사건이다.
constexpr float kFrameDelay=0.08F; // PE00507828의 실제 단정도 비트3da3d70a다.
// 비정렬 DWORD를 원본 폭대로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) { return std::uint32_t(raw[offset])|(std::uint32_t(raw[offset+1])<<8)|(std::uint32_t(raw[offset+2])<<16)|(std::uint32_t(raw[offset+3])<<24); }
}
// 같은10.78 TYPE 범위와 필수 효과를 첫 상태 변경 전에 확인한다.
RawPriestRitualFrame::RawPriestRitualFrame(const SidPool& pool,std::span<const RiftTypeFrames> frames,PriestRitualFrameHooks hooks):pool_(pool),frames_(frames),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188 || !hooks_.setFrame || !hooks_.eventExists || !hooks_.playAt) throw std::invalid_argument("사제604 판본/현재표/필수 효과 오류");
}
// 다음 프레임은 효과 전에 계산하고605 조회 뒤의 실제 좌표로 소리를 요청한다.
float RawPriestRitualFrame::Tick(Sid priest) const {
    const auto raw=pool_.Slot(priest);const auto type=raw[10];if (type>=frames_.size()) throw std::out_of_range("사제604 현재 TYPE 범위");const auto current=Read(raw,36); // 원본 current frame DWORD다.
    const auto direction=RawPathAnimation::Direction(frames_[type],std::bit_cast<std::int32_t>(current));if (direction<=9 || direction>=15) return 0;
    const auto run=frames_[type].Run(static_cast<std::uint8_t>('A'+direction));const auto first=static_cast<std::uint32_t>(run.first);auto next=current+1U; // 원본 signed 비교 전에 DWORD 감김을 보존한다.
    if (std::bit_cast<std::int32_t>(next-first)>=run.count) next=first+kRestartOffset;
    if (next==first+kSoundOffset && !hooks_.eventExists(priest,kSuppressEvent)) { const auto after=pool_.Slot(priest);hooks_.playAt(std::bit_cast<float>(Read(after,14)),std::bit_cast<float>(Read(after,18)),"beamOver03.wav",0); }
    hooks_.setFrame(priest,std::bit_cast<std::int32_t>(next),0);return kFrameDelay;
}
}
