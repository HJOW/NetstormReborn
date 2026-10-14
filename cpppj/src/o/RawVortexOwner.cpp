// 외부 섬/도장 효과 뒤에 현재 raw 소유자·좌표·목록을 다시 읽는다.
#include "o/RawVortexOwner.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 같은 원본 Owner 몸체를 가리키는 패치의 네 신전 가상 표다.
constexpr std::array<std::uint32_t,4> kPatchTables{0x5196f0,0x519860,0x5197a8,0x519918};
// CD는 세 원소 신전의 같은 Owner 재정의를 사용한다.
constexpr std::array<std::uint32_t,3> kCdTables{0x501d28,0x501de0,0x501e98};
// 정렬되지 않은 원본 little endian DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0; // 현재 네BYTE의 원래 비트다.
    // 이웃 필드를 섞지 않고 정확DWORD만 조합한다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 원본 Player DWORD만 저장하며 주변 바이트를 유지한다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 지정한 네BYTE만 낮은 비트부터 기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 외부 효과 뒤의 현재 단정도 좌표를 읽는다.
float Position(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(Read(raw,offset)); }
}
// 같은 풀/실제9기록/필수 효과를 최초 쓰기 전에 확인한다.
RawVortexOwner::RawVortexOwner(SidPool& pool,SquidOwner& base,std::span<std::uint8_t> players,
    SquidPostPopList& outposts,std::uint32_t& revision,VortexOwnerHooks hooks)
    :pool_(pool),base_(base),players_(players),outposts_(outposts),revision_(revision),hooks_(std::move(hooks)) {
    const auto stride=pool.Edition()==OriginalEdition::Patch1078 ? 0xb4U : 0xacU; // 실제 Player 기록의 크기다.
    if (&base.Pool()!=&pool || players.size()!=9*stride || !hooks_.islandNumber || !hooks_.islandExists ||
        !hooks_.setIslandOwner || !hooks_.regionOwnership || !hooks_.refreshProduction || !hooks_.missingIsland)
        throw std::invalid_argument("신전 Owner의 실제 Pool/9기록/필수 효과 누락");
}
// 현재 원본 가상 표를 직접 조회한다.
bool RawVortexOwner::Handles(Sid sid) const {
    const auto tables=pool_.Edition()==OriginalEdition::Patch1078 ? std::span<const std::uint32_t>(kPatchTables) : std::span<const std::uint32_t>(kCdTables);
    return std::ranges::find(tables,Read(pool_.Slot(sid),0))!=tables.end();
}
// PATCH+7c/+80 및 CD+74/+78의 같은 초기 신전 두DWORD다.
std::size_t RawVortexOwner::TempleField(std::uint32_t owner,bool copy) const {
    if (owner>kPlayerCount) throw std::out_of_range("신전 Owner 실제9 Player 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 해당 저장소의 판본이다.
    return owner*(patch ? 0xb4U : 0xacU)+(patch ? 0x7cU : 0x74U)+(copy ? 4U : 0U);
}
// 공통 Owner를 마지막 지역 갱신 뒤에 실행하며 이전 소유자 기준의 패치 순회를 유지한다.
void RawVortexOwner::Set(Sid sid,std::uint32_t owner) const {
    if (!Handles(sid)) throw std::invalid_argument("미복원 신전 가상 Owner 표");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 패치에만 별도 목록 순회와 revision 쓰기가 있다.
    const auto ownerOffset=patch ? 34U : 32U,extraOffset=patch ? 40U : 35U; // 실제 객체 필드 위치다.
    const auto previous=pool_.Slot(sid)[ownerOffset],extra=pool_.Slot(sid)[extraOffset]; // 외부 호출 전 원본 초기 조건이다.
    if (!(extra&8) && previous && Read(players_,TempleField(previous))==sid.value) {
        Write(players_,TempleField(previous),0);Write(players_,TempleField(previous,true),0);
    }
    if (!(pool_.Slot(sid)[11]&4) && !(extra&9)) {
        const auto island=hooks_.islandNumber(Position(pool_.Slot(sid),14),Position(pool_.Slot(sid),18)); // 현재 지면 지역이다.
        if (!hooks_.islandExists(island)) hooks_.missingIsland(sid,island,owner);
        hooks_.setIslandOwner(island,owner);
        hooks_.regionOwnership(Position(pool_.Slot(sid),14),Position(pool_.Slot(sid),18));
        if (patch) {
            // 현재 count와 각 SID·소유자·좌표를 효과 뒤에 다시 읽는다. free/dead 필터를 추가하지 않는다.
            for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(outposts_.count);++index) {
                if (index>=outposts_.entries.size() || outposts_.entries[index]>=pool_.Capacity()) throw std::out_of_range("신전 Owner 현재 추가 목록 범위 오류");
                const Sid other{static_cast<std::uint16_t>(outposts_.entries[index])}; // 이번 원본 목록 항목이다.
                if (pool_.Slot(other)[ownerOffset]==pool_.Slot(sid)[ownerOffset])
                    hooks_.regionOwnership(Position(pool_.Slot(other),14),Position(pool_.Slot(other),18));
            }
        }
    }
    base_.Set(sid,owner);
    const auto current=pool_.Slot(sid)[ownerOffset]; // 공통 몸체 이후의 실제 소유자 BYTE다.
    if (!(pool_.Slot(sid)[extraOffset]&9) && current && !Read(players_,TempleField(current))) {
        Write(players_,TempleField(current),sid.value);Write(players_,TempleField(current,true),sid.value);
    }
    hooks_.refreshProduction();if (patch) ++revision_;
}
// 조립 시 같은 월드인지 확인한다.
const SidPool& RawVortexOwner::Pool() const { return pool_; }
// 같은9기록을 참조하며 별도 Player 복사본을 돌려주지 않는다.
std::span<std::uint8_t> RawVortexOwner::Records() const { return players_; }
}
