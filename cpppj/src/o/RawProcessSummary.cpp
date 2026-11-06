//00471890/0041bff0/0049a830/0049a840의 실제 조회·서식 순서를 유지한다.
#include "o/RawProcessSummary.h"
#include "o/Kernel.h"
#include "o/RiftType.h"
#include <array>
#include <stdexcept>
#include <string>

namespace netstorm::o { // 판본별 이름표/3999·39999슬롯과 부모 소비 순서를 구별한다.
namespace { // 원문 C 문자열과 필요한 장치 검사만 공통화한다.
//실제 선택된 장치의 누락은 앞선 효과를 유지한 채 호출자에게 전달한다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("프로세스 요약의 실제 장치 미연결");return value; }
//%s의 첫NUL 종료를 보존하며 줄바꿈/탭/서식문자 자체는 바꾸지 않는다.
std::string_view CString(std::string_view value) { return value.substr(0,value.find('\0')); }
//원문의 DWORD SID를 좁히기 전에 물리 풀 범위를 검사한다.
std::uint8_t TypeAt(const SidPool& pool,std::uint32_t sid) { if (sid>=pool.Capacity()) throw std::out_of_range("프로세스 요약의 물리 SID 범위");return pool.Slot(Sid{static_cast<std::uint16_t>(sid)})[10]; }
// CD00443400의0051c868 앞10개 포인터와 기존52개 이름을 사용한다. NULL9의 표현은 실제CD CRT004f4920/004f6c60 관측값이다.
std::string_view CdProcessSummaryTypeName(std::uint32_t type) {
    constexpr std::array<std::string_view,10> first{"fakeThreeByThreeSurface","sunFlyerBomb","altar","dais","rune","forceField","bombSpecialOne","daisExtraFrames","fenceShield","(null)"}; //20바이트 TYPE 축약 이름과 구별되는 원문 전체 이름이다.
    if (type<first.size()) return first[type];
    const auto rest=RiftTypeTable::ProcessTypeNames(OriginalEdition::Cd1072);type-=static_cast<std::uint32_t>(first.size());
    if (type>=rest.size()) throw std::out_of_range("CD 프로세스 요약의 원본 이름 포인터 표 범위");return rest[type];
}
}
//앞10개의 원래 포인터 이름 뒤에 기존53개 전체 프로세스 이름 표가 이어진다.
std::string_view ProcessSummaryTypeName(std::uint32_t type) {
    constexpr std::array<std::string_view,10> first{"HealEffect","MeteorEffect","BOMBmeteor","bombIMano","bombIIMano","bombIIIMano","bombLightingZap","bombLightingwave","RUIN","Monument"}; //00540f78..00540f9c의 실제 문자열이다.
    if (type<first.size()) return first[type];
    const auto rest=RiftTypeTable::ProcessTypeNames(OriginalEdition::Patch1078);type-=static_cast<std::uint32_t>(first.size());
    if (type>=rest.size()) throw std::out_of_range("프로세스 요약의 원본 이름 포인터 표 범위");return rest[type];
}
// 현재 슬롯은Name뒤재조회한다. PATCH는부모조건/출력을따로읽고CD는같은한번의부모값을보관한다.
void WriteRawProcessSummary(const SidPool& pool,std::span<const RiftTypeRecord> types,const ProcessSummaryHooks& hooks) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078; // 물리 판본으로 원문의 슬롯 끝/부모 재조회를 선택한다.
    if (types.size()!=(patch ? 188U:171U)) throw std::invalid_argument("프로세스 요약의 판본별 풀/TYPE 표 필요");
    const auto file=Need(hooks.open)("procSumm.txt","wb");
    const auto end=patch ? kProcessCapacity:kCdProcessCapacity; // 원문 끝 번호39999/3999의 다음 sentinel은 읽지 않는다.
    for (std::uint32_t slot=1;slot<=end;++slot) { // 슬롯0을제외하고판본별현재슬롯을직접읽는다.
        const auto process=Need(hooks.slot)(slot);if (!process) continue;
        const auto formType=TypeAt(pool,Need(hooks.form)(process));const auto name=patch ? ProcessSummaryTypeName(formType):CdProcessSummaryTypeName(formType);
        const auto current=Need(hooks.slot)(slot);std::string line="pType="+std::string(name);
        const auto observedParent=Need(hooks.parent)(current); // CD004ed439의EBX는조건/이름/출력전체에서같은값이다.
        if (!observedParent) line+=" sid=0, attachToType=NULL\n";
        else {
            const auto parent=patch ? Need(hooks.parent)(current):observedParent;const auto type=TypeAt(pool,static_cast<std::uint32_t>(parent));
            if (type>=types.size()) throw std::out_of_range("프로세스 요약의 현재 부모 TYPE 범위");
            line+=" sid="+std::to_string(parent)+", attachToType="+std::string(CString(types[type].name))+"\n";
        }
        Need(hooks.write)(file,line);
    }
    Need(hooks.close)(file);
}
}
