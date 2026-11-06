// 소유자 쓰기 앞의 전체 TYPE/목록 사전 검사를 추가하지 않는다.
#include "o/RawCommonOwner.h"
#include <stdexcept>
namespace netstorm::o { // 같은 물리 슬롯을 유지하며 현재 모드/TYPE를 반복 조회한다.
namespace { // 콜백 교체 중에도 선택한 callable의 호출 수명을 보관한다.
template<class F> F NeedOwner(const F& value) { if (!value) throw std::logic_error("공통 소유자의 선택된 실제 장치 미연결");return value; }
}
void SetCommonSquidOwner(SidPool& pool,Sid sid,std::uint32_t player,const CommonOwnerHooks& hooks) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    const auto diagnose=[&](std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) { NeedOwner(hooks.assertion)(expression,patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",patch ? patchLine : cdLine); }; // 범위 진단 이후에도 현재 상태로 진행한다.
    if (NeedOwner(hooks.challenge)()) {
        if (!player || player>=0x28U) diagnose("0 < newPlayerId && newPlayerId < CL_NUM_CHAL_PLAYERS",0x9ff,0x9be);
    } else if (player>=9U) diagnose("newPlayerId == INVALID_PLAYER_ID ||( FIRST_REAL_PLAYER <= newPlayerId && newPlayerId < MAX_PLAYERS)",0xa02,0x9c1);
    const auto slot=pool.Slot(sid);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()};const auto owner=patch ? 34U : 32U,extra=patch ? 40U : 35U; // 진단 뒤 물리 슬롯만 요구하며 TYPE/free를 선행 거부하지 않는다.
    if ((NeedOwner(hooks.fort)() || NeedOwner(hooks.battle)()) && (NeedOwner(hooks.genus)(raw[10])&0x4200U)) NeedOwner(hooks.remove)(raw[owner],sid);
    bool cdActive=false;
    if (!patch) { const auto battle=NeedOwner(hooks.battle)();const auto fort=NeedOwner(hooks.fort)();cdActive=fort || battle; } // CD는 BYTE 저장 직전 두 모드를 읽어 둔다.
    raw[owner]=static_cast<std::uint8_t>(player);
    const bool active=patch ? (NeedOwner(hooks.fort)() || NeedOwner(hooks.battle)()) : cdActive;
    if (active && (NeedOwner(hooks.genus)(raw[10])&0x4200U) && !(raw[11]&4U) && !(raw[extra]&8U)) NeedOwner(hooks.append)(raw[owner],sid);
}
}
