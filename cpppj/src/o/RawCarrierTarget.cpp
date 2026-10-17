// 같은 대상의 재시도는 최초 발자국/명령 인자를 유지하고 부모 위치를 다시 읽지 않는다.
#include "o/RawCarrierTarget.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 선택된 원본 하위가 없으면 빈 동작으로 대체하지 않는다.
template<class F> const F& Need(const F& f) { if (!f) throw std::logic_error("운반 대상 이동의 실제 하위 미연결");return f; }
// 비정렬 현재 DWORD를 원본 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) { std::uint32_t value{};
    // 네 바이트의 정확한 폭만 조합한다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value;
}
}
// 아직 별도인 CD 이동을 PATCH 본문으로 실행하지 않는다.
RawCarrierTarget::RawCarrierTarget(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> players,
    const bool& boss,const RawBoardTarget& board,CarrierTargetHooks hooks):pool_(pool),types_(types),players_(players),boss_(boss),board_(board),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || players.size()!=9*0xb4) throw std::invalid_argument("운반 대상 이동의 판본/TYPE/Player9 오류");
}
// 원본 좌표는 비정렬 float32 두 개다.
RawPathPoint RawCarrierTarget::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// AI 존재 여부를 초기에 만든 별도 bool 배열로 대신하지 않는다.
bool RawCarrierTarget::Controlled(Sid sid) const { const auto owner=pool_.Slot(sid)[34];if (!owner) return false;if (owner>=9) throw std::out_of_range("운반 대상 이동의 현재 owner 범위 오류");return Read(players_,owner*0xb4+0xb0)!=0; }
// 첫 발자국 효과 뒤 부모 위치를 읽고 재시도에서는 사각형/인자를 보존한다.
CarrierPathResult RawCarrierTarget::StartTarget(Sid parent,Sid target,std::uint32_t replaceStart,std::uint32_t helper) const {
    const auto rectangle=Need(hooks_.footprint)(target);const auto point=board_.Nearest(rectangle,Position(parent),1);
    auto result=Need(hooks_.start)({parent,point,replaceStart,helper,0,0,0});
    // 첫 시도 실패 뒤 북/동/남/서의 네 변만 순서대로 시도한다.
    for (std::uint32_t side=0;side<7 && result==0;side+=2) result=Need(hooks_.start)({parent,board_.Side(rectangle,side,1),replaceStart,helper,0,0,0});return result;
}
// 두 단일 접근 명령은 발자국 효과 전에 부모 좌표를 보관한다.
CarrierPathResult RawCarrierTarget::GoReturn(Sid parent,Sid target) const {
    if (Controlled(parent)) return 0;const auto source=Position(parent);const auto rectangle=Need(hooks_.footprint)(target);return Need(hooks_.start)({parent,board_.Nearest(rectangle,source,1),0,0,0,0,0});
}
// free만 검사하고 dead 비트/owner/특정 타입 번호 필터를 임의로 추가하지 않는다.
CarrierPathResult RawCarrierTarget::GoGeyser(Sid parent,Sid target) const {
    if (Controlled(parent) || (pool_.Slot(target)[11]&1U)) return 0;const auto kind=pool_.Slot(target)[10];if (kind>=types_.size()) throw std::out_of_range("간헐천 이동 대상 TYPE 오류");if (!(types_[kind].flags2&0x10000000U)) return 0;
    const auto source=Position(parent);const auto rectangle=Need(hooks_.footprint)(target);return Need(hooks_.start)({parent,board_.Nearest(rectangle,source,1),0,0,0,0,0});
}
// 선택 효과가 권한을 바꿔도 원본은 다시 boss를 검사하지 않는다.
CarrierPathResult RawCarrierTarget::GoHome(Sid parent) const { if (boss_) { const auto target=Need(hooks_.chooseReturn)(parent);if (target.value) return StartTarget(parent,target,0,0); }return 0; }
// 실제 월드의 풀 주소만 공개한다.
const SidPool& RawCarrierTarget::Pool() const { return pool_; }
}
