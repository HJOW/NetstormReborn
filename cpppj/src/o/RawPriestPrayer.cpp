// 원본의 초기 쓰기/단락 평가/일치 비교와 큰 효과 뒤 현재 필드 조회를 유지한다.
#include "o/RawPriestPrayer.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kPrayer=605,kDevastation=168,kPrayCommand=0x030000de,kPanicCommand=0x03000103; // 실제605/5412f8/542a6c/542b00 초기값이다.
// 선택되지 않은 경계는 요구하지 않으며 선택한 누락만 오류로 남긴다.
template<class Function> const Function& Need(const Function& hook) { if (!hook) throw std::logic_error("사제 기도의 실제 하위 미연결");return hook; }
}
RawPriestPrayer::RawPriestPrayer(SidPool& pool,std::span<const PriestPrayerType> types,const bool& boss,PriestPrayerHooks hooks)
    :pool_(pool),types_(types),boss_(boss),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("사제 기도 PATCH 풀/188TYPE 필요");
}
std::uint32_t RawPriestPrayer::Read(Sid parent,std::size_t offset,std::size_t width) const {
    const auto raw=pool_.Slot(parent);std::uint32_t result{};
    for (std::size_t index=0;index<width;++index) result|=std::uint32_t(raw[offset+index])<<(index*8); // 요청한 원문 폭만 읽는다.
    return result;
}
void RawPriestPrayer::Write(Sid parent,std::size_t offset,std::uint32_t value) const {
    const auto raw=pool_.Slot(parent);auto* bytes=const_cast<std::uint8_t*>(raw.data()); // 풀은 실제 mutable 저장소이며 Native의 초기 쓰기에서 할당 필터를 추가하지 않는다.
    for (std::size_t index=0;index<4;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); // 정확히 현재 DWORD만 쓴다.
}
const PriestPrayerType& RawPriestPrayer::Type(std::uint32_t kind) const { if (kind>=types_.size()) throw std::out_of_range("사제 기도 현재 TYPE 범위");return types_[kind]; }
std::uint32_t RawPriestPrayer::Command(Sid parent,std::uint32_t code,std::uint32_t parameter) const {
    const auto command=code&0x03ffffffU; // 원본 입력의 상위6개 비트는 명령 비교에서 제외한다.
    if (command==kPrayCommand) {
        const auto kind=static_cast<std::uint16_t>(parameter>>16); // 외부 조회 전에 원본이 읽는 사건+10 WORD를 보존한다.
        if (!Need(hooks_.hasEvent)(parent,kPrayer)) {
            const auto direction=Need(hooks_.direction)(parent);if (direction<10 || direction>14) {
                std::array<std::uint8_t,8> packet{1,0x6d,8,0,static_cast<std::uint8_t>(parent.value),static_cast<std::uint8_t>(parent.value>>8),static_cast<std::uint8_t>(kind),static_cast<std::uint8_t>(kind>>8)};
                Need(hooks_.request)(9,packet);
            }
        }
        return 1;
    }
    if (command==kPanicCommand) Need(hooks_.panic)(parent);
    return Need(hooks_.parent)(parent,code,parameter);
}
void RawPriestPrayer::Receive(std::span<const std::uint8_t> packet) const {
    if (packet.size()<8) throw std::invalid_argument("기도6d 부모/종류 WORD 부족");const Sid parent{static_cast<std::uint16_t>(packet[4]|(std::uint16_t(packet[5])<<8))};const auto kind=std::uint32_t(packet[6])|(std::uint32_t(packet[7])<<8); // 8바이트 뒤 추가 자료는 읽지 않는다.
    Write(parent,42,kind);if (Read(parent,11,1)&3U) return;
    if (Need(hooks_.hasEvent)(parent,kPrayer)) return;const auto direction=Need(hooks_.direction)(parent);if (direction>=10 && direction<=14) return;
    if (boss_) { const auto movement=Need(hooks_.movementType)(Read(parent,10,1));static_cast<void>(Need(hooks_.movement)(parent,movement,0,true));Need(hooks_.broadcast)(packet); }
    if (Need(hooks_.reserveShared)()) Need(hooks_.addShared)(parent,kPrayer,1.0F);
    Need(hooks_.ritual)(parent,4);
}
float RawPriestPrayer::Tick(Sid parent,std::uint32_t count,float payload) const {
    Write(parent,46,count);const auto movementType=Need(hooks_.movementType)(Read(parent,10,1));const auto movement=Need(hooks_.movement)(parent,movementType,0,false);
    if (Need(hooks_.immobile)(parent) || movement.value) return 0;
    if (std::bit_cast<std::int32_t>(count)!=Type(Read(parent,42)).prayTime) return payload;
    if (boss_ && !Need(hooks_.containedType)(parent,1,kDevastation,0)) { Need(hooks_.discard)(parent);Need(hooks_.create)(parent,Read(parent,42),0,1,0,0); }
    Need(hooks_.setFrame)(parent,Type(Read(parent,10,1)).defaultFrame,0);return 0;
}
}
