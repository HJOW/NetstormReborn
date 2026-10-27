// 현재 원문 조회 순서와 번역/가상 효과 뒤 재조회를 보존한다.
#include "client/RawObjectMenuVariants.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 보호 PE의 실제 초기 명령 DWORD이며 호스트 함수 주소가 아니다.
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,9> kCommands{{
    {0x542ad4,0x030000f8},{0x542b5c,0x02000125},{0x542b60,0x02000126},{0x542b64,0x02000127},
    {0x542a2c,0x030000ce},{0x5427ac,0x0200002d},{0x5428c8,0x03000074},{0x542754,0x02000017},{0x542974,0x020000a0}}};
// 선택된 필수 경계를 빈 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("파생 객체 메뉴 실제 하위 미연결");return hook; }
}
// 확인한 원문 주소만 동일 초기 ID로 돌려준다.
std::optional<std::uint32_t> ObjectMenuVariantCommandValue(std::uint32_t address) {
    for (const auto& [source,code]:kCommands) if (source==address) return code; // 실제 첫 일치만 사용한다.
    return {};
}
// 기존 Help/Knowledge와 새 파생 명령이 같은 표를 공유한다.
bool IsObjectMenuVariantCommand(std::uint32_t code) {
    for (const auto& [source,value]:kCommands) if (value==code) return true; // 주소는 비교하지 않는다.
    return false;
}
// 판본을 혼합하거나 별도 슬롯 사본을 만들지 않는다.
RawObjectMenuVariants::RawObjectMenuVariants(const o::SidPool& pool,ObjectMenuVariantHooks hooks):pool_(pool),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("파생 객체 메뉴10.78 풀 필요");
}
// free/dead 필터는 원본 파생 앞에 추가하지 않는다.
std::span<const std::uint8_t> RawObjectMenuVariants::Raw(o::Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("파생 객체 메뉴 SID 범위");return pool_.Slot(sid);
}
// 정렬되지 않은 원본 바이트를 정확한 폭으로 결합한다.
std::uint32_t RawObjectMenuVariants::Read(o::Sid sid,std::size_t offset,std::size_t width) const {
    const auto raw=Raw(sid);std::uint32_t value{};
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8); // 같은 현재 슬롯의 필요한 바이트만 읽는다.
    return value;
}
// 각 소비 시점에 같은 실제 전역을 조회한다.
std::uint32_t RawObjectMenuVariants::Global(std::uint32_t address) const { return Need(hooks_.global)(address); }
// 원문 특수 접두 뒤 정확한 공통 부모만 실행한다.
void RawObjectMenuVariants::Open(o::Sid sid,std::uint32_t originalBody) const {
    Raw(sid);
    switch (originalBody) {
    case 0x41c210:
        if (!Global(0x594fa4) && Raw(sid)[34]==Need(hooks_.localOwner)()) {
            const auto hp=std::bit_cast<std::int32_t>(Read(sid,26));const auto label=Need(hooks_.translate)("Meltdown!"); // HP는 번역보다 먼저 보관한다.
            const auto maximum=Need(hooks_.maximumHp)(sid);const auto command=Global(0x542ad4);
            Need(hooks_.item)(label,command,sid.value,true,maximum/2<hp);
        }
        Need(hooks_.damageable)(sid);return;
    case 0x422000:
        if (Global(0x5c85a4)) {
            // 세 상태를 원문 번역/명령 조회 순서대로 작성한다.
            for (const auto& [text,address]:std::array<std::pair<std::string_view,std::uint32_t>,3>{{{"Set as Cracked",0x542b5c},{"Set as Normal",0x542b60},{"Set as Hard",0x542b64}}}) {
                const auto label=Need(hooks_.translate)(text);const auto command=Global(address);Need(hooks_.item)(label,command,sid.value,true,true);
            }
        }
        Need(hooks_.object)(sid);return;
    case 0x462390:
        if (Global(0x5c85a4)) Need(hooks_.item)("Turn To Spell!",Global(0x542a2c),sid.value,true,true);
        { const auto include=Global(0x55a480),command=Global(0x5427ac);Need(hooks_.item)("CHEAT Dig Up",command,sid.value,include,1); }
        Need(hooks_.object)(sid);return;
    case 0x462550: {
        const auto type=Raw(sid)[10];const auto command=Global(0x542754);const auto label=Need(hooks_.helpWord)();Need(hooks_.item)(label,command,type,true,true);return;
    }
    case 0x485600: {
        const auto pattern=Need(hooks_.translate)("Storm Power: ~i%d~.\n");const auto text=Need(hooks_.format)(pattern,static_cast<std::int32_t>(Read(sid,12,2)));Need(hooks_.header)(text);Need(hooks_.damageable)(sid);return;
    }
    case 0x48eca0: {
        const auto type=Raw(sid)[10];const auto count=Need(hooks_.count)(sid);Need(hooks_.salvage)(sid,type,count);Need(hooks_.object)(sid);return;
    }
    case 0x4dcca0:
        if (Need(hooks_.hasBrain)(sid)) Need(hooks_.aiMenu)(Raw(sid)[34]);
        if (Raw(sid)[34]==Global(0x540c70)) { const auto label=Need(hooks_.translate)("View Netstorm Knowledge");const auto command=Global(0x542974);Need(hooks_.item)(label,command,0,true,true); }
        Need(hooks_.separator)();Need(hooks_.damageable)(sid);return;
    case 0x420b30: {
        const bool enabled=!Need(hooks_.eventZero)(sid); // 원본은 모드/소유자를 읽기 전에 예약 존재를 보관한다.
        if (!Global(0x55a480) && !Global(0x5c85a4)) { const auto include=Raw(sid)[34]==Global(0x540c70);const auto command=Global(0x5428c8);Need(hooks_.item)("Detonate!",command,sid.value,include,enabled); }
        else Need(hooks_.item)("Turn To Obelisk!",Global(0x542a2c),sid.value,true,enabled);
        Need(hooks_.object)(sid);return;
    }
    default:throw std::logic_error("현재 객체 메뉴의 실제 파생+a8 미연결: "+std::to_string(originalBody));
    }
}
}
