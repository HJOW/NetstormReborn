// 원본의 기술 보유/필터·설명 줄 나눔·언어3/제외·행 누적과 popup 표식을 보존한다.
#include "client/RawKnowledgeMenu.h"
#include "client/ThemeTechnologyCommand.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::client {
// 이름 비교는 기존 원본 ASCII 규칙을 사용한다.
std::uint32_t KnowledgeScriptCode(std::string_view name) {
    const auto lower=o::AsciiLower(name); // 세 기술 명령도 원본 ASCII 이름 비교를 공유한다.
    if (lower=="showtecsidemenu") return kShowTecSideMenuCommand;if (lower=="showtecmenu2") return kShowTecMenu2Command; //실제5434a8/b0의등록이름만추가하며기존F6는바꾸지않는다.
    return lower=="gettechnology" ? kGetTechnologyCommand : lower=="showtechnology" ? kShowTechnologyCommand : lower=="cheatalltechnology" ? kCheatAllTechnologyCommand : 0xffffffffU;
}
namespace {
// 선택된 실제 효과의 누락을 빈 성공으로 숨기지 않는다.
template<class F> const F& KnowledgeEffect(const F& effect) { if (!effect) throw std::logic_error("지식 창의 실제 게임/GUI 효과 연결 누락");return effect; }
// 원본 strlen/strcpy의 첫NUL 뒤는 설명·마나에 포함하지 않는다.
std::string KnowledgeText(std::string value) { const auto end=value.find('\0');if (end!=value.npos) value.resize(end);return value; }
}
// 카드마다 같은 기존 메뉴 크기/부착 몸체를 사용한다. 독자적인 그림·창 체계를 만들지 않는다.
namespace { //두원문카드의동일112BYTE수집/정렬/행배치를공유하고원소범위와머리만구별한다.
std::vector<std::uint32_t> ShowKnowledgeThemeMenu(std::span<const o::RiftTypeRecord> types,const KnowledgeMenuState& state,const KnowledgeMenuHooks& hooks,bool world) { //false는492b60의Storm,true는493210의World다.
    if (KnowledgeEffect(hooks.blocked)()) return {};
    KnowledgeEffect(hooks.cancel)();const auto technology=KnowledgeEffect(hooks.currentTechnology)();std::vector<KnowledgeRecord> records;
    // 원본 타입 순서를 그대로 순회하고 내용물 조회 뒤 선택한 네 테마의 범위를 판정한다.
    for (std::uint32_t type=0;type<types.size();++type) {
        if (types[type].group==10 || (types[type].flags2&0x200000U) || type==state.templeProduct) continue;
        const auto owned=KnowledgeEffect(hooks.owns)(technology,type);const auto mana=KnowledgeText(KnowledgeEffect(hooks.mana)(type));if (world ? types[type].theme<4 || types[type].theme>7 : types[type].theme<0 || types[type].theme>3) continue;
        KnowledgeRecord record;record.type=type;record.mana=mana;record.available=owned || mana.empty();record.description=KnowledgeText(KnowledgeEffect(hooks.description)(type));record.inProduction=KnowledgeEffect(hooks.inProduction)(type);
        if (record.mana.size()>9 || record.description.size()>81) throw std::length_error("원본 지식 행112BYTE의 문자열 범위");records.push_back(std::move(record));
    }
    if (records.size()>96) throw std::length_error("원본 지식 창의 머리 포함100개 메뉴 범위");SortKnowledgeRecords(records,types);
    std::array<std::int32_t,4> offsets{};std::vector<std::uint32_t> menus;const std::array<std::string_view,4> headings=world ? std::array<std::string_view,4>{"EARTH","FIRE","WATER.","MOON"} : std::array<std::string_view,4>{"WIND","RAIN","THUN.","SUN"}; // 원본 네 테마 머리의 생성 순서다.
    // 먼저 네 머리를 만들고 같은 정렬 행의 카드들을 같은 네 줄에 누적한다.
    for (std::size_t index=0;index<records.size()+4;++index) {
        const auto token=KnowledgeEffect(hooks.begin)();menus.push_back(token);KnowledgeEffect(hooks.node)(token).menuFlags|=0x1000000U;std::uint32_t theme{};
        if (index<4) {
            theme=static_cast<std::uint32_t>(index);KnowledgeEffect(hooks.title)(KnowledgeEffect(hooks.translate)(headings[index]));KnowledgeEffect(hooks.separator)();
            KnowledgeEffect(hooks.item)("~[Imana."+std::to_string(index+(world ? 12 : 4))+']',state.helpCommand,std::string("themeHelp"),true);
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
    KnowledgeEffect(hooks.mark)(world ? kShowTecMenu2Command : kShowTechnologyCommand);return menus;
}
}
std::vector<std::uint32_t> ShowKnowledgeMenu(std::span<const o::RiftTypeRecord> types,const KnowledgeMenuState& state,const KnowledgeMenuHooks& hooks) { return ShowKnowledgeThemeMenu(types,state,hooks,false); } //기존Storm호출의공개계약과기본F6를유지한다.
std::vector<std::uint32_t> ShowWorldKnowledgeMenu(std::span<const o::RiftTypeRecord> types,const KnowledgeMenuState& state,const KnowledgeMenuHooks& hooks) { return ShowKnowledgeThemeMenu(types,state,hooks,true); } //World원소는별도명령으로만선택한다.
// 원문4938c0은기술보유/번역을조회하지않고두고정메뉴의명령행을작성한다.
std::vector<std::uint32_t> ShowKnowledgeSideMenu(const KnowledgeMenuState& state,const KnowledgeMenuHooks& hooks) {
    if (KnowledgeEffect(hooks.blocked)()) return {};KnowledgeEffect(hooks.cancel)();std::vector<std::uint32_t> menus; //도달한현재메타맵관문뒤의원문두메뉴다.
    for (std::int32_t index=0;index<2;++index) { //Storm메뉴를먼저만들고World메뉴를그오른쪽에붙인다.
        const auto token=KnowledgeEffect(hooks.begin)();menus.push_back(token);KnowledgeEffect(hooks.node)(token).menuFlags|=0x1000000U;
        const auto command=index ? kShowTecMenu2Command : kShowTechnologyCommand;KnowledgeEffect(hooks.item)("~[IMANA.16]",command,0U,true);
        KnowledgeEffect(hooks.item)(index ? "~3      The Furies of the World" : "~3      The Furies of the Storm",command,0U,true);
        KnowledgeEffect(hooks.attach)(state.width/2-260+index*320,state.height/2-190);KnowledgeEffect(hooks.push)(token);KnowledgeEffect(hooks.width)(token,300.0F);KnowledgeEffect(hooks.height)(token,300.0F);KnowledgeEffect(hooks.pop)(token);
        auto& node=KnowledgeEffect(hooks.node)(token);node.targetWidth=static_cast<float>((static_cast<double>(node.width)-node.targetWidth)*0.5+node.targetWidth);node.flags&=~0x4000U;
    }
    for (const auto token:menus) static_cast<void>(KnowledgeEffect(hooks.position)(token)); //둘다완성한뒤실제현재위치를다시읽는다.
    for (const auto token:menus) KnowledgeEffect(hooks.node)(token).flags|=0x4000U; //원문의두번째순회에서만팝업표식을켠다.
    KnowledgeEffect(hooks.mark)(kShowTecSideMenuCommand);return menus;
}
}
