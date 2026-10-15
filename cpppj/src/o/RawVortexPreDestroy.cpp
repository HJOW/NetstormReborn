// 신전 삭제 중 callback 뒤의 owner/extra/목록 개수를 원문 위치마다 다시 읽는다.
#include "o/RawVortexPreDestroy.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 비정렬 Player/좌표의 실제 DWORD만 읽고 쓴다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::uint32_t(bytes[offset])|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24); }
// 원본 네 BYTE의 이웃을 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); }
// 선택 위치의 미연결 효과는 앞선 원문 쓰기를 보존한 채 명시 거부한다.
template<class Function> const Function& Need(const Function& value) { if (!value) throw std::logic_error("신전 삭제의 실제 하위 소비자 미연결");return value; }
}
// PATCH의 실제 현재 자료 크기를 확인하며 신전 종류를 임의 성공으로 대체하지 않는다.
RawVortexPreDestroy::RawVortexPreDestroy(SidPool& pool,std::span<std::uint8_t> players,SquidPostPopList& temples,
    SquidPostPopList& residences,const SquidPostPopList& outposts,const std::uint32_t& loading,VortexPreDestroyHooks hooks)
    :pool_(pool),players_(players),temples_(temples),residences_(residences),outposts_(outposts),loading_(loading),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || players.size()!=9*0xb4) throw std::invalid_argument("신전 삭제의 실제 PATCH/Player 저장소 오류");
}
// 실제 원본 목록 helper0040ea00은 중복을 모두 제거하며 순서를 유지한다.
void RawVortexPreDestroy::Remove(SquidPostPopList& list,std::uint32_t sid) const {
    static_cast<void>(list.Items());std::uint32_t removed{};
    // 제거 후보의 꼬리 stale DWORD는 원문처럼 지우지 않는다.
    for (std::uint32_t index=0;index<list.count;++index) { const auto current=list.entries[index];if (current==sid) ++removed;else list.entries[index-removed]=current; }list.count-=removed;
}
// 해당 효과 직전의 raw 좌표만 반환한다.
std::array<float,2> RawVortexPreDestroy::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 현재 owner를 매번 다시 읽어 진단/UI 효과가 변경한 기록을 반영한다.
std::size_t RawVortexPreDestroy::Field(Sid sid,std::size_t field) const { const auto owner=pool_.Slot(sid)[34];if (owner>=9) throw std::out_of_range("신전 삭제의 현재 Player 범위 오류");return owner*0xb4+field; }
// 원본 진입 extra 검사와 Damageable 이후 extra 검사는 서로 다른 현재 읽기다.
void RawVortexPreDestroy::PreDestroy(Sid sid,std::uint32_t flags) const {
    if (!(pool_.Slot(sid)[40]&9U)) {
        Remove(temples_,sid.value);Remove(residences_,sid.value);const auto point=Position(sid);const auto island=Need(hooks_.islandNumber)(point[0],point[1]);
        if (!Need(hooks_.islandExists)(island)) Need(hooks_.assertion)(0x2ab);
        if (Need(hooks_.islandTemple)(island)!=sid.value) Need(hooks_.assertion)(0x2ac);
        Need(hooks_.clearIslandTemple)(island);Need(hooks_.islandOwner)(island,0);const auto now=Position(sid);Need(hooks_.regionOwnership)(now[0],now[1]);
        // outpost 목록 count와 부모/후보 owner를 효과 이후 현재 값으로 검사한다.
        for (std::uint32_t index=0;index<outposts_.count;++index) {
            const auto value=outposts_.Items()[index];if (value>=pool_.Capacity() || value>65535) throw std::out_of_range("신전 삭제의 outpost SID 범위 오류");const Sid current{static_cast<std::uint16_t>(value)};
            if (pool_.Slot(current)[34]==pool_.Slot(sid)[34]) { const auto outpost=Position(current);Need(hooks_.regionOwnership)(outpost[0],outpost[1]); }
        }
    }
    Need(hooks_.damageablePre)(sid,flags);
    if (loading_==0 && !(pool_.Slot(sid)[40]&9U)) Need(hooks_.refreshProduction)();
    if (pool_.Slot(sid)[34]!=0) {
        if (Read(players_,Field(sid,0x7c))==sid.value) { Put(players_,Field(sid,0x7c),0);const auto value=Read(players_,Field(sid,0x7c));Put(players_,Field(sid,0x80),value); }
        if (Read(players_,Field(sid,0x84))==sid.value) { Put(players_,Field(sid,0x84),0);const auto value=Read(players_,Field(sid,0x84));Put(players_,Field(sid,0x88),value); }
    }
}
}
