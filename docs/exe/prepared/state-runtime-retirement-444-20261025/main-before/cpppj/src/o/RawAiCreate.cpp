// 생성 실패/누락 경계를 가짜 brain으로 우회하지 않고 실제 원본 정상 경로의 순서를 보존한다.
#include "o/RawAiCreate.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제600바이트 설정의 little endian 필드를 읽는다.
std::uint32_t Field(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 현재 입력 네 바이트만 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
// 선택된 원본 효과의 실제 공급자가 없으면 즉시 경계를 보고한다.
template<class Function> const Function& Need(const Function& function,const char* name) { if (!function) throw std::logic_error(name);return function; }
}
// 이 wrapper는 PATCH전용이며 Player 저장소를 소유하지 않는다.
RawAiCreate::RawAiCreate(OriginalEdition edition,RawPlayerParticipation& players,AiCreateHooks hooks):players_(players),hooks_(std::move(hooks)) { if (edition!=OriginalEdition::Patch1078) throw std::invalid_argument("AiCreate는PATCH전용입니다"); }

// 기존 brain/NoTemple은 생성자 경계를 필요로 하지 않는다. 정상 생성만 실제0xb08 소유자를 요청한다.
std::uint32_t RawAiCreate::Create(std::uint32_t owner,std::span<const std::uint8_t> settings) const {
    if (settings.size()!=RawMissionAiSettings::kSize) throw std::invalid_argument("AiCreate실제600바이트설정누락");
    const auto color=Field(settings,0x250);if (color!=0) Need(hooks_.changeColor,"AiCreate실제색변경누락")(owner,color);
    const auto old=players_.Brain(owner);if (old!=0) return old;
    if (Field(settings,0x244)!=0) { const auto region=Need(hooks_.firstRegion,"AiCreate실제첫지역조회누락")(owner);const auto temple=Need(hooks_.islandTemple,"AiCreate실제지역신전조회누락")(region);if (temple!=0) Need(hooks_.destroyTemple,"AiCreate실제신전삭제누락")(temple,0x400000);return 0; }
    // 공급 누락으로 부착만 성공하지 않게 두 큰 몸체를 생성 전에 확인한다.
    Need(hooks_.construct,"AI실제생성자00416860미복원");Need(hooks_.initialUpdate,"AI실제초기갱신00414710미복원");
    const auto brain=hooks_.construct(owner,settings,0xb08);if (brain==0) throw std::runtime_error("AI실제생성소유자가NULL을반환했습니다");players_.AttachBrain(owner,brain);hooks_.initialUpdate(brain,settings);return brain;
}
}
