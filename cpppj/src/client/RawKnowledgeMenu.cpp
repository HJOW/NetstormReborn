// 원본의 기술 보유/필터·설명 줄 나눔·언어3/제외·행 누적과 popup 표식을 보존한다.
#include "client/RawKnowledgeMenu.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::client {
// 이름 비교는 기존 원본 ASCII 규칙을 사용한다.
std::uint32_t KnowledgeScriptCode(std::string_view name) { return o::AsciiLower(name)=="showtechnology" ? kShowTechnologyCommand : 0xffffffffU; }
namespace {
// 선택된 실제 효과의 누락을 빈 성공으로 숨기지 않는다.
template<class F> const F& KnowledgeEffect(const F& effect) { if (!effect) throw std::logic_error("지식 창의 실제 게임/GUI 효과 연결 누락");return effect; }
// 원본 strlen/strcpy의 첫NUL 뒤는 설명·마나에 포함하지 않는다.
std::string KnowledgeText(std::string value) { const auto end=value.find('\0');if (end!=value.npos) value.resize(end);return value; }
}
// 카드마다 같은 기존 메뉴 크기/부착 몸체를 사용한다. 독자적인 그림·창 체계를 만들지 않는다.
std::vector<std::uint32_t> ShowKnowledgeMenu(std::span<const o::RiftTypeRecord> types,const KnowledgeMenuState& state,const KnowledgeMenuHooks& hooks) {
    if (KnowledgeEffect(hooks.blocked)()) return {};
    KnowledgeEffect(hooks.cancel)();const auto technology=KnowledgeEffect(hooks.currentTechnology)();std::vector<KnowledgeRecord> records;
    // 원본 타입 순서를 그대로 순회하고 내용물 조회 뒤 테마0..3을 판정한다.
    for (std::uint32_t type=0;type<types.size();++type) {
        if (types[type].group==10 || (types[type].flags2&0x200000U) || type==state.templeProduct) continue;
        const auto owned=KnowledgeEffect(hooks.owns)(technology,type);const auto mana=KnowledgeText(KnowledgeEffect(hooks.mana)(type));if (types[type].theme<0 || types[type].theme>3) continue;
        KnowledgeRecord record;record.type=type;record.mana=mana;record.available=owned || mana.empty();record.description=KnowledgeText(KnowledgeEffect(hooks.description)(type));record.inProduction=KnowledgeEffect(hooks.inProduction)(type);
        if (record.mana.size()>9 || record.description.size()>81) throw std::length_error("원본 지식 행112BYTE의 문자열 범위");records.push_back(std::move(record));
    }
    if (records.size()>96) throw std::length_error("원본 지식 창의 머리 포함100개 메뉴 범위");SortKnowledgeRecords(records,types);
    std::array<std::int32_t,4> offsets{};std::vector<std::uint32_t> menus;const std::array<std::string_view,4> headings{"WIND","RAIN","THUN.","SUN"}; // 원본 네 테마 머리의 생성 순서다.
    // 먼저 네 머리를 만들고 같은 정렬 행의 카드들을 같은 네 줄에 누적한다.
    for (std::size_t index=0;index<records.size()+4;++index) {
        const auto token=KnowledgeEffect(hooks.begin)();menus.push_back(token);KnowledgeEffect(hooks.node)(token).menuFlags|=0x1000000U;std::uint32_t theme{};
        if (index<4) {
            theme=static_cast<std::uint32_t>(index);KnowledgeEffect(hooks.title)(KnowledgeEffect(hooks.translate)(headings[index]));KnowledgeEffect(hooks.separator)();
            KnowledgeEffect(hooks.item)("~[Imana."+std::to_string(index+4)+']',state.helpCommand,std::string("themeHelp"),true);
            KnowledgeEffect(hooks.item)("",state.helpCommand,std::string("themeHelp"),true);KnowledgeEffect(hooks.item)("",state.helpCommand,std::string("themeHelp"),true);
        } else {
            const auto& record=records[index-4];theme=types[record.type].theme;
            KnowledgeEffect(hooks.item)("~[I"+std::to_string(record.type)+'.'+(record.available ? "#" : "&")+']',state.helpCommand,record.type,true);
            auto first=record.description;std::string second;bool splitName{};
            // 원본은 마지막 공백/하이픈부터 역순으로 한 번만 자르고 첫 줄을 다시 줄이지 않는다.
            for (auto split=first.size();split>0;--split) if (split<first.size() && (first[split]==' ' || first[split]=='-') && split>1 && first.size()>8) { second=first.substr(split+1);first.resize(split);splitName=true;break; }
            if (KnowledgeEffect(hooks.excluded)(record.type)) { KnowledgeEffect(hooks.item)(KnowledgeEffect(hooks.translate)("~y~EExcluded"),state.helpCommand,record.type,record.available);KnowledgeEffect(hooks.item)("",state.helpCommand,record.type,record.available); }
            else if (state.language==3) { KnowledgeEffect(hooks.item)("",state.helpCommand,record.type,record.available);KnowledgeEffect(hooks.item)("",state.helpCommand,record.type,record.available); }
            else { KnowledgeEffect(hooks.item)("~5"+first,state.helpCommand,record.type,record.available);KnowledgeEffect(hooks.item)(splitName ? "~5"+second : "",state.helpCommand,record.type,record.available); }
        }
        const auto row=(theme+1U)&3U;const auto step=state.iconHeight+46;const auto y=step*static_cast<std::int32_t>(row)-(step*4)/2-18+state.height/2; // 네 줄의 원본 signed 정수 배치다.
        const auto x=static_cast<std::int32_t>(static_cast<double>(state.width/2)-static_cast<double>(state.iconWidth)*8.8*0.5)+offsets[row];
        KnowledgeEffect(hooks.attach)(x,y);KnowledgeEffect(hooks.push)(token);KnowledgeEffect(hooks.width)(token,static_cast<float>(static_cast<double>(state.iconWidth)*1.1));KnowledgeEffect(hooks.height)(token,static_cast<float>(step));KnowledgeEffect(hooks.pop)(token);
        auto& node=KnowledgeEffect(hooks.node)(token);node.targetWidth=static_cast<float>((static_cast<double>(node.width)-node.targetWidth)*0.5+node.targetWidth);offsets[row]=static_cast<std::int32_t>(static_cast<double>(offsets[row])+node.baseWidth+2.0);node.flags&=~0x4000U;
    }
    // 원본의 마지막 위치 순회를 같은 실제 getter에 전달한다. 계산된 전체 경계는 외부에서 사용하지 않는다.
    for (const auto token:menus) static_cast<void>(KnowledgeEffect(hooks.position)(token));
    // 모두 만든 뒤에만 popup bit를 켜서 작성 도중 앞 카드를 닫지 않는다.
    for (const auto token:menus) KnowledgeEffect(hooks.node)(token).flags|=0x4000U;
    KnowledgeEffect(hooks.mark)(kShowTechnologyCommand);return menus;
}
}
