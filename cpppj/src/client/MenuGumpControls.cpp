// 판본별 색 조회/명령 비트/서식/현재 controlled 칸 재조회와 실제 항목/제목 호출 순서를 유지한다.
#include "client/MenuGumpControls.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 항목의 텍스트/스타일/명시 인자·체크/비활성/자식/선택과 fancy 메뉴 비트다.
constexpr std::uint32_t kText=0x10000,kStyled=0x400,kParameter=0x80,kChecked=4,kDisabled=0x200,kChild=0x20,kSelected=8,kFancy=0x800000;
// 각 래퍼가 전달하는 기본 flags와 원본 스택 포맷 버퍼 크기다.
constexpr std::uint32_t kNormalItem=kText|kStyled|kParameter,kSubmenuItem=kText|kStyled,kSeparatorItem=0x10010;
constexpr std::size_t kFormatBytes=256;
// 원본 문자열은 첫 NUL에서 끝난다. 호스트 string_view 뒤의 바이트를 포맷/검색에 사용하지 않는다.
std::string_view CString(std::string_view source) { return source.substr(0,source.find('\0')); }
// 원본 256바이트 지역 결과를 넘는 입력만 거부한다. NUL까지 들어갈 1바이트를 확보한다.
std::string Bounded(std::string value) {
    if (value.size()>=kFormatBytes) throw std::length_error("메뉴 원본 포맷 버퍼 한도 초과");return value;
}
// 실제 추가 인자가 없는 원본 포맷 경로다. %%는 한 '%'로 출력한다.
// 다른 '%' 변환은 이 래퍼가 원본에서 인자를 넘기지 않는 무효 스택 입력이므로 호스트에서 거부한다.
std::string NoArguments(std::string_view source) {
    source=CString(source);std::string result;
    // 출력 순서와 %% 소비를 유지하며 원본 버퍼 밖 결과를 쓰기 전에 확인한다.
    for (std::size_t index=0;index<source.size();++index) {
        if (source[index]=='%') {
            if (index+1>=source.size() || source[index+1]!='%') throw std::invalid_argument("무인자 메뉴 서식에 변환 인자 없음");
            ++index;
        }
        result.push_back(source[index]);
    }
    return Bounded(std::move(result));
}
}
// 같은 기본/파생 저장소만 필수다. 설정 경계는 실제 패치 색 조회에 도달할 때 확인한다.
MenuGumpControls::MenuGumpControls(GumpCreateState& create,const MenuGumpState& state,const MenuGump& menu,
    const MenuGumpLayout& layout,const ScrollingTextGumpMarkup& title,MenuGumpControlHooks hooks)
    :create_(create),state_(state),menu_(menu),layout_(layout),title_(title),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.base) throw std::invalid_argument("controlled 메뉴 공유 저장소 필요");
}
// 제목/자식은 메뉴 아닌 기본 파생 객체도 조회할 수 있다.
GumpCreateNode& MenuGumpControls::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("controlled 기본 노드 없음");return *node;
}
// controlled/표시/입력과 같은 실제 메뉴를 반환한다.
MenuGumpNode& MenuGumpControls::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=&Base(token)) throw std::runtime_error("controlled 메뉴 공유 노드 없음");return *node;
}
// 항목 수와 별개로 실제 도달한 40칸 범위를 검사한다.
MenuGumpItem& MenuGumpControls::Item(std::uint32_t token,std::int32_t index) const {
    if (index<0 || static_cast<std::size_t>(index)>=kMenuItemCapacity) throw std::out_of_range("controlled 메뉴 항목 범위 오류");
    return Node(token).items[static_cast<std::size_t>(index)];
}
// CD 선택 조회는 null 등록을 허용한다. 문맥 재귀는 도달한 null에서 실제 노드 계약으로 거부한다.
std::uint32_t MenuGumpControls::Registered(std::uint32_t id) const {
    if (id>=create_.entries.size()) throw std::out_of_range("controlled 자식 등록 ID 범위 오류");return create_.entries[id];
}
// 10.78/10.72의 호출 규약 차이를 현재 저장소 판본에서 결정한다.
bool MenuGumpControls::Patch() const { return create_.collection.edition==o::OriginalEdition::Patch1078; }
// 패치의 null 현재 메뉴 assert는 include 검사보다 먼저다.
std::uint32_t MenuGumpControls::CurrentRequired() const { const auto token=menu_.Current();Node(token);return token; }
// 패치에 대응 함수가 없는 CD 참고 몸체를 잘못 호출하면 변경 전에 거부한다.
void MenuGumpControls::RequireCd() const { if (Patch()) throw std::invalid_argument("CD 전용 메뉴 helper"); }
// 누락 설정도 두 번의 원본 조회를 수행하고 값은 각 0 초기값을 유지한다.
std::array<std::int32_t,2> MenuGumpControls::Colors(std::string_view foreground,std::string_view background) const {
    if (!hooks_.readInt) throw std::invalid_argument("메뉴 색 설정 ReadInt 경계 필요");
    std::array<std::int32_t,2> result{};hooks_.readInt(foreground,result[0]);hooks_.readInt(background,result[1]);return result;
}
// 고정 원본 "~[C.%d]~[B.%d]~E%s"의 signed DWORD 십진수 결과를 생성한다.
std::string MenuGumpControls::Colored(std::string_view text,const std::array<std::int32_t,2>& colors) const {
    return Bounded("~[C."+std::to_string(colors[0])+"]~[B."+std::to_string(colors[1])+"]~E"+std::string(CString(text)));
}
// CD는 null 현재 칸에서 조용히 생략한다. 패치는 include가 false여도 먼저 assert 계약을 확인한다.
void MenuGumpControls::AddText(std::string_view text,std::uint32_t command,std::uint32_t parameter,bool include,bool enabled,bool forceColor) const {
    if (!Patch()) {
        const auto token=menu_.Current();if (!token || !include) return;
        const auto source=std::string(CString(text));layout_.Append(token,source.c_str(),kNormalItem|(command ? 1U : 0U)|(enabled ? 0U : kDisabled),command,parameter,0);return;
    }
    CurrentRequired();if (!include) return;
    const auto colors=Colors("MenuColor","MenuBGColor");
    const auto source=(enabled || forceColor) ? Colored(text,colors) : NoArguments(text);
    layout_.Append(CurrentRequired(),source.c_str(),kNormalItem|(command ? 1U : 0U)|(enabled ? 0U : kDisabled),command,parameter,0);
}
// 패치 command 비트는 명령 값과 무관하게 1이다. CD는 명령 0의 미작성 필드를 보존한다.
void MenuGumpControls::AddChecked(bool checked,std::string_view text,std::uint32_t command,std::uint32_t parameter,bool include,bool enabled) const {
    if (!Patch()) {
        const auto token=menu_.Current();if (!token || !include) return;
        const auto source=std::string(CString(text));layout_.Append(token,source.c_str(),kNormalItem|(command ? 1U : 0U)|(checked ? kChecked : 0U)|(enabled ? 0U : kDisabled),command,parameter,0);return;
    }
    CurrentRequired();if (!include) return;
    const auto colors=Colors("MenuCheckedColor","MenuCheckedBGColor");
    const auto source=enabled ? Colored(text,colors) : NoArguments(text);
    layout_.Append(CurrentRequired(),source.c_str(),kNormalItem|1U|(checked ? kChecked : 0U)|(enabled ? 0U : kDisabled),command,parameter,0);
}
// 원본 제목 슬롯/색·본문의 포맷 순서를 사용한다. fancy Parse 뒤 메뉴 Reflow를 추가하지 않는다.
void MenuGumpControls::Heading(std::string_view text,bool title) const {
    std::string source;
    if (Patch()) {
        const auto colors=title ? Colors("MenuTitleColor","MenuTitleBGColor") : Colors("MenuHeaderColor","MenuHeaderBGColor");
        source=Bounded(std::string(title ? "~3~E" : "~E")+"~[C."+std::to_string(colors[0])+"]~[B."+std::to_string(colors[1])+"]"+std::string(CString(text))+"~.");
    } else source=title ? Bounded("~3"+std::string(CString(text))+"~.") : NoArguments(text);
    const auto token=CurrentRequired();
    if (Node(token).menuFlags&kFancy) {
        const auto target=Node(token).title;if (!target) throw std::runtime_error("fancy 메뉴 제목 없음");title_.Parse(target,source);return;
    }
    if (!Patch()) { AddText(source,0,0,true,false);return; }
    const auto colors=Colors("MenuColor","MenuBGColor");source=Colored(source,colors);
    layout_.Append(CurrentRequired(),source.c_str(),kNormalItem|kDisabled,0,0,0);
}
// 제목은 패치 ~3~E/색과 CD ~3만 구분하고 같은 현재 fancy/일반 경로를 사용한다.
void MenuGumpControls::AddTitle(std::string_view text) const { Heading(text,true); }
// 머리는 패치 ~E/색과 CD의 원문 무인자 포맷을 구분한다.
void MenuGumpControls::AddHeader(std::string_view text) const { Heading(text,false); }
// CD의 '~' 검색은 첫 NUL 앞에서만 한다. 자식 Attach/flags 0x20은 실제 Append에 맡긴다.
void MenuGumpControls::AddSubmenu(std::string_view text,std::uint32_t child,bool enabled,bool checked) const {
    if (!Patch()) {
        const auto token=menu_.Current();if (!token) return;
        text=CString(text);const auto source=std::string(text);
        layout_.Append(token,source.c_str(),kText|(text.find('~')!=std::string_view::npos ? kStyled : 0U)|(enabled ? 0U : kDisabled)|(checked ? kChecked : 0U),0,0,child);return;
    }
    CurrentRequired();const auto colors=Colors("MenuSubColor","MenuSubBGColor");
    const auto source=enabled ? Colored(text,colors) : NoArguments(text);
    layout_.Append(CurrentRequired(),source.c_str(),kSubmenuItem|(enabled ? 0U : kDisabled)|(checked ? kChecked : 0U),0,0,child);
}
// 두 wrapper의 정상 항목 쓰기/구분선 생략은 기존 Append와 같다. 명령/인자 bits가 없어 전달 1도 쓰이지 않는다.
void MenuGumpControls::AddSeparator() const { layout_.Append(CurrentRequired(),"",kSeparatorItem,1,1,0); }
// 메뉴는 지연 Kill 뒤 실제 소멸 전까지 개수에 남는다.
bool MenuGumpControls::NoMenus() const { return state_.instances==0; }
// 같은 context 참조를 재귀에 넘기며 숨김/동적 자식도 원본 0x20 비트로 방문한다.
void MenuGumpControls::CopyChildren(std::uint32_t token,const std::array<std::uint32_t,8>& context,std::vector<std::uint32_t>& active) const {
    if (std::find(active.begin(),active.end(),token)!=active.end()) throw std::runtime_error("CD 메뉴 문맥 재귀 순환");active.push_back(token);
    // 매 반복 끝에 현재 항목 수를 읽으며 원본의 자식별 복사→재귀 순서를 유지한다.
    for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(Node(token).itemCount);++index) {
        if (!(Item(token,index).flags&kChild)) continue;
        const auto child=Registered(Item(token,index).childId);auto& destination=Base(child).textContext;
        // REP MOVSD처럼 순서대로 복사하므로 다른 방문 자식이 바꾼 현재 입력 문맥을 다시 읽는다.
        for (std::size_t word=0;word<context.size();++word) destination[word]=context[word];
        CopyChildren(child,context,active);
    }
    active.pop_back();
}
// 자신과 저장 StyleText 조각은 변경하지 않으며 각 자식 기본 문맥만 쓴다.
void MenuGumpControls::CopyChildTextContextCd(std::uint32_t token,const std::array<std::uint32_t,8>& context) const {
    RequireCd();std::vector<std::uint32_t> active;CopyChildren(token,context,active);
}
// 원본은 먼저 현재 강조를 지운 다음 기존 flags의 자식 여부/현재 등록/visible를 읽고 바로 반환한다.
bool MenuGumpControls::ClearSelectionUntilVisibleChildCd(std::uint32_t token) const {
    RequireCd();
    // 첫 표시 자식을 찾으면 뒤 항목의 강조는 지우지 않는 원본 부분 갱신을 유지한다.
    for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(Node(token).itemCount);++index) {
        auto& item=Item(token,index);const auto flags=item.flags;item.flags&=~kSelected;
        if (flags&kChild) { const auto child=Registered(item.childId);if (child && (Base(child).flags&1U)) return true; }
    }
    return false;
}
// 현재 실제 설정을 읽어 color DWORD에 반영하고 조회 반환값은 원본 wrapper처럼 버린다.
void BindMenuGumpControlConfig(MenuGumpControlHooks& hooks,o::ConfigInterface& config) {
    hooks.readInt=[&config](std::string_view key,std::int32_t& value) { config.ReadInt(key,value); };
}
}
