// 원래 소유자/로컬 표식 비트를 현재 소유자 바이트와 구별하고 원본 호출/쓰기 순서를 보존한다.
#include "o/RawPriestOwner.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 판본 공통 타입/단어 위치와 low 7비트·로컬 표식 마스크다.
constexpr std::size_t kType=10,kWord=12;
constexpr std::uint8_t kOriginalPlayer=0x7f,kLocalFlag=0x80;
// raw 가상 표는 little endian DWORD이며 정렬되지 않은 주소도 읽을 수 있다.
std::uint32_t Vtable(std::span<const std::uint8_t> raw) {
    std::uint32_t value=0;
    // 네 바이트를 호스트 엔디언과 무관하게 합친다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[i])<<(8*i);
    return value;
}
}
// 원본 알림도 호출 순서를 검증할 수 있게 생략하지 않는다.
RawPriestOwner::RawPriestOwner(SidPool& pool,const SquidOwnerMode& mode,const PriestOwnerState& state,PriestOwnerHooks hooks)
    :pool_(pool),mode_(mode),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.base || !hooks_.notify) throw std::invalid_argument("사제 소유자 효과 누락");
}
// 다른 타입의 가상 표를 사제로 바꾸어 처리하지 않는다.
bool RawPriestOwner::Handles(Sid sid) const {
    return Vtable(pool_.Slot(sid))==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchPriestVtable : kCdPriestVtable);
}
// 사제 몸체의 공통 소유자 호출 여부와 이후 비트 변경 순서를 그대로 적용한다.
void RawPriestOwner::Set(Sid sid,std::uint32_t player) const {
    auto raw=pool_.AllocatedBytes(sid);
    if (raw[kType]!=kPriestType || !Handles(sid)) throw std::invalid_argument("사제 소유자 가상 표/타입 불일치");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const bool changesOriginal=!mode_.battle || state_.loadingDepth!=0;
    const auto effective=changesOriginal ? player : static_cast<std::uint32_t>(raw[kWord]&kOriginalPlayer);
    const bool callsBase=(!changesOriginal || effective!=0) && (!patch || effective<=kPlayerCount);
    // 원본 assert 뒤의 부분 변경은 호스트에서 효과 전 예외로 옮긴다. 무시되는 전투 인자는 검사하지 않는다.
    if (patch && changesOriginal && player==0) throw std::invalid_argument("사제 원래 소유자 0(원본 assert)");
    if (callsBase && (effective>kPlayerCount || (mode_.challenge && effective==0)))
        throw std::out_of_range("사제 공통 소유자 범위 오류(원본 assert)");
    if (changesOriginal && player!=0) {
        raw[kWord]=static_cast<std::uint8_t>((raw[kWord]&kLocalFlag)|(player&kOriginalPlayer));
        hooks_.notify(sid);
    }
    if (callsBase) hooks_.base(sid,effective);
    if (state_.loadingDepth==0 || mode_.battle) {
        // 공통 소유자 호출 뒤의 현재 raw 단어를 고친다. high byte(+13)는 두 판본 모두 보존한다.
        raw=pool_.AllocatedBytes(sid);
        const bool local=(mode_.fort && effective==state_.fortPlayer) || (mode_.battle && effective==state_.battlePlayer);
        raw[kWord]=static_cast<std::uint8_t>((raw[kWord]&kOriginalPlayer)|(local ? kLocalFlag : 0));
        hooks_.notify(sid);
    }
}
// 섬/종유석 소유자 분배기와 합성할 수 있는 사제 전용 가상 분배기다.
std::function<void(Sid,std::uint32_t)> MakePriestOwnerDispatch(RawPriestOwner& priest,std::function<void(Sid,std::uint32_t)> base) {
    if (!base) throw std::invalid_argument("사제 소유자 분배의 base 누락");
    return [&priest,base=std::move(base)](Sid sid,std::uint32_t player) {
        if (priest.Handles(sid)) priest.Set(sid,player);
        else base(sid,player);
    };
}
}
