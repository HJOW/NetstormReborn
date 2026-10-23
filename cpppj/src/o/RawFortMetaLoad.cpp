// CD Badges의entry즉시depth복구와Technology의끝depth복구를구분한다.
#include "o/RawFortMetaLoad.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제MetaDisplay기록수/stride와countDWORD위치다.
constexpr std::size_t kBadgeRecords=28,kBadgeStride=220,kBadgeCount=216;
}
// 부모/stream원본몸체보다앞서자료크기와필수하위효과를확인한다.
RawFortMetaLoad::RawFortMetaLoad(const SidPool& pool,std::span<std::uint8_t> badges,std::uint32_t& loadingDepth,
    FortMetaLoadState& state,FortMetaLoadHooks hooks)
    :pool_(pool),badges_(badges),loadingDepth_(loadingDepth),state_(state),hooks_(std::move(hooks)) {
    if (badges.size()!=kBadgeRecords*kBadgeStride || !hooks_.assertion || !hooks_.loadingDepthChanged ||
        !hooks_.technologyParent || !hooks_.prepareTypeNames || !hooks_.contents || !hooks_.releaseTypeConversion || !hooks_.refreshTechnologyUi)
        throw std::invalid_argument("Fort meta 실제badge기록/하위효과누락");
}
// 비정렬little endianDWORD의현재바이트를조합한다.
std::uint32_t RawFortMetaLoad::ReadBadge(std::size_t offset) const {
    std::uint32_t value=0;
    // 원본DWORD폭의각BYTE만읽는다.
    for (std::size_t byte=0;byte<4;++byte) value|=static_cast<std::uint32_t>(badges_[offset+byte])<<(byte*8);
    return value;
}
// 원본count필드의네BYTE만대입한다.
void RawFortMetaLoad::WriteBadge(std::size_t offset,std::uint32_t value) const {
    // type/padding/이웃기록은쓰지않는다.
    for (std::size_t byte=0;byte<4;++byte) badges_[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 실제linear순서에서처음일치한typeDWORD를사용한다.
std::int32_t RawFortMetaLoad::FindBadge(std::uint32_t type) const {
    // 종류중복도원본처럼첫기록만반환한다.
    for (std::size_t index=0;index<kBadgeRecords;++index) if (ReadBadge(index*kBadgeStride)==type) return static_cast<std::int32_t>(index);
    return -1;
}
// 진단후계속할때는callback이바꾼현재표를다시검색한다.
void RawFortMetaLoad::SetBadge(std::uint32_t type,std::int32_t count) const {
    if (count<0 || count>255) hooks_.assertion(FortMetaAssertion::BadgeCountRange);
    const auto index=FindBadge(type);if (index!=-1) WriteBadge(static_cast<std::size_t>(index)*kBadgeStride+kBadgeCount,std::bit_cast<std::uint32_t>(count));
}
// Badges의rawpointer직접읽기는호스트물리범위밖이면기존효과를보존한채거부한다.
std::uint8_t RawFortMetaLoad::ReadByte(FortMetaStream& stream) const {
    if (stream.position>=stream.bytes.size()) throw std::out_of_range("Fort Badges 실제stream메모리소진");
    const auto value=stream.bytes[stream.position];++stream.position;return value;
}
// 원본현재로딩깊이쓰기와같은소비자의관찰을함께실행한다.
void RawFortMetaLoad::ChangeDepth(std::uint32_t value) const { loadingDepth_=value;hooks_.loadingDepthChanged(value); }
// patch는깊이를쓰지않고CD는entry의두helper에서바로증감한다.
void RawFortMetaLoad::LoadBadges(FortMetaStream& stream) const {
    LoadBadgesSelected([&stream]() -> FortMetaStream& { stream.position=0;return stream; }); // 기존 독립 원문 입력의 선택 효과도 같은 시점에 수행한다.
}
// 실제 Badges 선택은 CD의 즉시 깊이 증감과 현재 Template 진단 뒤에 수행한다.
void RawFortMetaLoad::LoadBadgesSelected(const std::function<FortMetaStream&()>& select) const {
    if (!select) throw std::invalid_argument("Fort Badges 실제 섹션 선택 누락");
    if (pool_.Edition()==OriginalEdition::Cd1072) { ChangeDepth(loadingDepth_+1U);ChangeDepth(loadingDepth_-1U); }
    if (!state_.templatePresent) hooks_.assertion(FortMetaAssertion::MissingBadgesTemplate);
    auto& stream=select();if (!stream.present || !stream.end) return;
    const auto count=ReadByte(stream);
    // 저장BYTE순서를그대로따르며앞setter효과를새종류/개수로정렬하지않는다.
    for (std::uint32_t index=0;index<count;++index) { const auto type=ReadByte(stream);const auto quantity=ReadByte(stream);SetBadge(type,quantity); }
}
// 현재부모/변환표/깊이를다시읽고항상UI를호출한뒤원래깊이만복구한다.
void RawFortMetaLoad::LoadTechnology(FortMetaStream& stream) const {
    LoadTechnologySelected([&stream]() -> FortMetaStream& { stream.position=0;return stream; }); // 독립 원문의 cursor 초기화도 실제 기술 부모 조회 뒤에 한다.
}
// 실제 Technology 선택을 깊이/Template/isMine 진단과 기술 부모 조회 뒤의 원본 위치에서 수행한다.
void RawFortMetaLoad::LoadTechnologySelected(const std::function<FortMetaStream&()>& select) const {
    if (!select) throw std::invalid_argument("Fort Technology 실제 섹션 선택 누락");
    ChangeDepth(loadingDepth_+1U);
    try {
        if (!state_.templatePresent) hooks_.assertion(FortMetaAssertion::MissingTechnologyTemplate);
        if (state_.templateSlot!=state_.localOwner) hooks_.assertion(FortMetaAssertion::TechnologyNotMine);
        const auto parent=hooks_.technologyParent();auto& stream=select();
        if (stream.present && stream.end) {
            hooks_.prepareTypeNames();hooks_.contents(parent,stream,0x11, true);
            if (state_.hasTypeConversion) hooks_.releaseTypeConversion();state_.hasTypeConversion=false;
        }
        hooks_.refreshTechnologyUi();
    } catch (...) {
        ChangeDepth(loadingDepth_-1U);throw; // 호스트실패에서깊이를복구하지만앞의실제효과는되돌리지않는다.
    }
    ChangeDepth(loadingDepth_-1U);
}
// 실제Contents연결이같은풀인지확인한다.
const SidPool& RawFortMetaLoad::Pool() const { return pool_; }
// 완료된parsed목록의생성은다시구현하지않고같은객체/ContainedCreate사슬로전달한다.
FortMetaLoadHooks MakeFortMetaContentsHooks(const SidPool& pool,const RawFortObjectLoad& objects,const FortTemplate& fort,FortMetaLoadHooks hooks) {
    if (&pool!=&objects.Pool()) throw std::invalid_argument("Fort meta Contents SID풀불일치");
    hooks.contents=[&objects,&fort](Sid parent,FortMetaStream& stream,std::uint32_t flags,bool relaxedConversion) {
        if (!relaxedConversion) throw std::logic_error("Fort Technology 변환0디버그진단모드미복원");
        const auto bytes=fort.Section("Technology");if (fort.technologyTrailingBytes>bytes.size() || bytes.size()-fort.technologyTrailingBytes>std::numeric_limits<std::uint32_t>::max())
            throw std::out_of_range("Fort Technology parsed원본소비폭오류");
        if (stream.bytes.data()!=bytes.data() || stream.bytes.size()!=bytes.size())
            throw std::invalid_argument("Fort Technology parsed목록과현재원본섹션불일치");
        objects.Contents(parent,fort.technology,flags);
        stream.position=static_cast<std::uint32_t>(bytes.size()-fort.technologyTrailingBytes);
    };
    return hooks;
}
}
