// 메뉴 생성/번역·현재 포인터 비교·TYPE 수집/정렬·부분 작성의 원문 순서를 유지한다.
#include "client/NativeBattleInfoMenus.h"
#include "o/RawBattleOptionMutation.h"
#include "o/RawBattleOptionText.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace netstorm::client { // 아직 없는 실제 자식 장치를 가짜 항목/정렬 성공으로 대신하지 않는다.
namespace { // 원문 서식과 선택별 callback·현재 포인터 비교를 구별한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("BattleInfo 메뉴의 실제 현재 장치 미연결");return hook; } // 선택한 원문 위치에서만 장치를 요구한다.
std::string Format(std::string_view pattern,std::string_view value) { std::array<char,256> text;const std::string format(pattern),argument(value);const auto size=std::snprintf(text.data(),text.size(),format.c_str(),argument.c_str());if (size<0 || static_cast<std::size_t>(size)>=text.size()) throw std::out_of_range("BattleInfo 메뉴의 실제256BYTE 서식 범위");return {text.data(),static_cast<std::size_t>(size)}; } // 제목/설명을 고정%s 인자로 복사하고 실제256BYTE 한도를 유지한다.
// 각 행은 choice→current→명령→choice 재조회 순서이며 비교는 임시string의 주소/내용으로 바꾸지 않는다.
void Choices(std::span<const std::uint8_t> options,std::int32_t index,std::int32_t maximum,const NativeBattleInfoMenuHooks& h) {
    for (std::int32_t selected=0;selected<=maximum;++selected) {
        const auto first=Need(h.choice)(index,static_cast<std::uint32_t>(selected));const auto current=Need(h.value)(options,index);const bool enabled=first.identity!=current.identity;const auto parameter=(static_cast<std::uint32_t>(index)<<16)|static_cast<std::uint32_t>(selected);const auto command=Need(h.read)(0x542b7c);const auto text=Need(h.choice)(index,static_cast<std::uint32_t>(selected));Need(h.item)(text.text,command,parameter,enabled);
    } //포함 최댓값까지 원문 정수 순서로 모든 선택 행을 추가한다.
}
}
// 두 행 라벨은 원문처럼 별도 번역 호출 없이 실제 메뉴 항목 작성기로 전달한다.
void BuildBattleSortMenu(HudBattleInfoNode& node,const NativeBattleInfoMenuHooks& h) {
    Need(h.beginFancy)();Need(h.title)(Need(h.translate)("Sort Game Options By\n"));const auto first=Need(h.read)(0x542b80);Need(h.item)("Show Only Changed Options",first,1,node.mode!=1);const auto second=Need(h.read)(0x542b80);Need(h.item)("Show All Options",second,2,node.mode!=2);Need(h.attach)();
}
// 각 설정 쌍은 하위 작성/번역 효과가 끝난 뒤 다음 반복에서 현재 값을 다시 읽는다.
void BuildBattleOptionsMenu(HudBattleInfoNode& node,std::span<const std::uint8_t> options,const NativeBattleInfoMenuHooks& h) {
    Need(h.beginFancy)();Need(h.title)(Need(h.translate)("Battle Options\n"));
    for (std::int32_t index=1;index<22;++index) { //게임 종류0은 전체 설정 하위 메뉴에서 제외한다.
        if (!node.settings[static_cast<std::size_t>(index)][1]) continue;Need(h.beginVertical)();const auto maximum=o::MaximumBattleOptionChoice(index);Choices(options,index,maximum,h);const auto child=Need(h.take)();const auto name=o::BattleOptionNameText(static_cast<std::uint32_t>(index),h.translate);Need(h.submenu)(name,child);
    }
    Need(h.attach)();
}
// 원문의 포함최댓값은 제목 번역/작성 전에 캡처한다.
void BuildBattleOptionMenu(std::span<const std::uint8_t> options,std::int32_t index,const NativeBattleInfoMenuHooks& h) {
    Need(h.beginFancy)();const auto maximum=o::MaximumBattleOptionChoice(index);const auto name=o::BattleOptionNameText(static_cast<std::uint32_t>(index),h.translate);Need(h.title)(Format("%s\n",name));Choices(options,index,maximum,h);Need(h.attach)();
}
// 원문 여섯 지역 배열의 의미별 고정 용량과 수집/정렬/출력 순서를 보존한다.
void BuildBattleExcludeMenu(const NativeBattleInfoMenuHooks& h) {
    Need(h.beginFancy)();Need(h.title)(Need(h.translate)("Exclude which unit:\n"));std::array<std::vector<KnowledgeRecord>,6> groups;
    constexpr std::array<std::uint32_t,12> kExcludedGlobals{0x5412d0,0x54126c,0x5411e8,0x5411ec,0x5411f0,0x5411a8,0x541240,0x54123c,0x541280,0x541298,0x5412b0,0x54117c}; //원문12개비교의주소/단축평가순서다.
    for (std::int32_t number=70;number<Need(h.typeCount)();++number) { //각조건에서현재TYPE개수를다시읽는다.
        const auto type=static_cast<std::uint32_t>(number);if (Need(h.globallyExcluded)(type)) continue;bool excluded{};
        for (const auto address:kExcludedGlobals) { if (type==Need(h.read)(address)) { excluded=true;break; } } //앞에서일치하면뒤TYPE별칭을읽지않는다.
        if (excluded) continue;const auto& record=Need(h.type)(type);std::size_t group{};
        if (record.flags2&0x100U) { if (type==Need(h.read)(0x5412f8)) continue;group=5; }
        else switch (record.theme) { case 0:group=0;break;case 1:group=1;break;case 2:group=2;break;case 3:group=3;break;default:group=4;break; }
        const auto capacity=group==4 ? 100U:50U;if (groups[group].size()>=capacity) throw std::out_of_range("BattleInfo 제외 메뉴의 원문 지역 지식 배열 범위");groups[group].push_back(Need(h.knowledge)(type));
    }
    constexpr std::array<std::size_t,6> kSortOrder{3,0,2,1,4,5}; //원문Sun/Wind/Thunder/Rain/기타/Spells의CRT호출순서다.
    for (const auto group:kSortOrder) SortKnowledgeRecords(groups[group],Need(h.types)()); //빈 배열도같은기존정렬부모에전달한다.
    constexpr std::array<std::size_t,5> kMenuOrder{5,3,0,1,2}; //기타그룹은수집/정렬하지만원문은메뉴에출력하지않는다.
    constexpr std::array<std::string_view,6> kHeadings{"Wind","Rain","Thunder","Sun","","Spells"}; //각원문번역키다.
    for (const auto group:kMenuOrder) { //원문Spells/Sun/Wind/Rain/Thunder순서로존재하는하위만작성한다.
        if (groups[group].empty()) continue;Need(h.beginVertical)();
        for (const auto& record:groups[group]) { const auto description=Need(h.description)(record.type);const auto label=Format("~y~E%s",description);const auto command=Need(h.read)(0x5429e0);Need(h.item)(label,command,record.type,true); } //정렬때의설명을재사용하지않고각행작성때현재설명을다시읽는다.
        const auto heading=Need(h.translate)(kHeadings[group]);const auto child=Need(h.take)();Need(h.submenu)(heading,child);
    }
    Need(h.attach)();
}
// 처음 TYPE 주소 조회 뒤 Fancy를 만들고 현재 번역/설명을 실제 서식 위치에서 읽는다.
void BuildBattleRemoveMenu(std::uint32_t type,const NativeBattleInfoMenuHooks& h) {
    static_cast<void>(Need(h.type)(type));Need(h.beginFancy)();const auto pattern=Need(h.translate)("Remove exclusion of %s?\n");const auto description=Need(h.description)(type);Need(h.title)(Format(pattern,description));const auto yes=Need(h.translate)("Yes");const auto command=Need(h.read)(0x5429e4);Need(h.item)(yes,command,type,true);const auto no=Need(h.translate)("No");const auto second=Need(h.read)(0x5429e4);Need(h.item)(no,second,0,true);Need(h.attach)();
}
}
