// 원래 소유자/로컬 표식 비트를 현재 소유자 바이트와 구별하고 원본 호출/쓰기 순서를 보존한다.
#include "o/RawPriestOwner.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 판본 공통 타입/단어 위치와 low 7비트·로컬 표식 마스크다.
constexpr std::size_t kWord=12;
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
    if (!Handles(sid)) throw std::invalid_argument("사제 소유자 가상 표 불일치");
    const auto slot=pool_.Slot(sid);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()}; // 실제 가상 진입 뒤에는 TYPE/free/dead를 선행 거부하지 않는다.
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const bool initialBattle=mode_.battle; // 요청0 진단의 행 번호는 최초에 선택한 전투/비전투 분기를 따른다.
    const bool changesOriginal=!initialBattle || state_.loadingDepth!=0;
    const auto effective=changesOriginal ? player : static_cast<std::uint32_t>(raw[kWord]&kOriginalPlayer);
    const bool callsBase=(!changesOriginal || effective!=0) && (!patch || effective<=kPlayerCount);
    if (patch && changesOriginal && player==0) {
        const auto assertion=hooks_.assertion; // 진단 장치가 정상 복귀하면 요청0의 원문 후단으로 진행한다.
        if (!assertion) throw std::logic_error("사제 원래 소유자0의 실제 진단 장치 미연결");
        assertion("newPlayerId != INVALID_PLAYER_ID",".\\Priest.cpp",initialBattle ? 0x5bU : 0x68U);
    }
    if (changesOriginal && player!=0) {
        raw[kWord]=static_cast<std::uint8_t>((raw[kWord]&kLocalFlag)|(player&kOriginalPlayer));
        hooks_.notify(sid);
    }
    if (callsBase) hooks_.base(sid,effective);
    if (state_.loadingDepth==0 || mode_.battle) {
        // 공통 소유자 호출 뒤의 현재 raw 단어를 고친다. high byte(+13)는 두 판본 모두 보존한다.
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
