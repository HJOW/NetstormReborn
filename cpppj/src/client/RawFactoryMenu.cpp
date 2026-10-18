// 원본 Factory 지식 목록은 생산 포함 여부/허용/테마/슬롯·정렬의 서로 다른 조건을 보존한다.
#include "client/RawFactoryMenu.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::client {
namespace {
constexpr std::array<std::uint32_t,7> kPriority{0x800,0x8000000,0x10000,0x4000000,0x20000000,0x40000,0x20000}; // 453f70의 첫 공통 플래그에서 멈추는 순서다.
// 선택된 실제 경계가 미연결이면 빈 성공 대신 즉시 알린다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Factory 메뉴 실제 조회/작성 미연결");return function; }
// 비정렬 원문 DWORD/WORD를 낮은 바이트부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 이웃 필드를 섞지 않고 명시 폭만 읽는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value;
}
}
// 현재 실제 마나 초기화 표는11문자만 쓰고 그 밖의 ASCII는 BSS0이다.
std::string FormatFactoryMana(std::string_view text,std::int32_t active,std::uint32_t manaType) {
    constexpr std::string_view kLetters="wrtsWRTS.?>"; // 실제540ad8 문자열의 물리 그림 순서다.
    std::string result;int count{};
    // 원본 C 문자열의 첫NUL에서 멈추며 연속 점은 개수에 넣지 않는다.
    for (const auto letter:text) {
        if (!letter) break;if (letter=='.') continue;if (static_cast<unsigned char>(letter)>=128) throw std::out_of_range("마나 원본 signed ASCII 표 범위");
        const auto found=kLetters.find(letter);auto frame=found==std::string_view::npos ? 0 : static_cast<int>(found);if (count<active) frame+=11;
        result+="~[I"+std::to_string(manaType)+"."+std::to_string(frame)+"]";++count;
    }
    if (result.size()>=250) throw std::length_error("원본 마나 문자열250바이트 진단");return result;
}
// 생성 중 게임 상태/메뉴를 바꾸지 않고 단일 참조만 보관한다.
RawFactoryMenu::RawFactoryMenu(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,const o::ContainedFinderState& contained,
    const o::SquidPostPopList& excluded,FactoryMenuState& state,FactoryMenuHooks hooks):pool_(pool),types_(types),contained_(contained),excluded_(excluded),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("Factory 메뉴10.78 실제 저장소 필요");
}
// 같은+a8/+ac를 가진 두 원본 Factory 표만 받는다.
std::span<const std::uint8_t> RawFactoryMenu::Raw(o::Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("Factory 메뉴 SID 범위");const auto raw=pool_.Slot(sid);const auto table=Read(raw,0);
    if (table!=0x509148 && table!=0x509210) throw std::logic_error("Factory 메뉴 파생 가상 표 미복원");return raw;
}
// 번역/조회 후에도 실제 타입 번호를 현재 표에서 읽는다.
const o::RiftTypeRecord& RawFactoryMenu::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("Factory 메뉴 TYPE 범위");return types_[type]; }
// 실제 번역 서식의 첫NUL/원본 스택 버퍼 한도를 유지한다.
std::string RawFactoryMenu::Format(std::string_view key,std::initializer_list<PriestMenuArgument> args) const {
    auto result=Need(hooks_.format)(key,{args.begin(),args.size()});const auto zero=result.find('\0');if (zero!=std::string::npos) result.resize(zero);
    if (result.size()>=256) throw std::length_error("Factory 메뉴 원본 서식256바이트 한도");return result;
}
// 0은 번역된None이다. 음수는 원본 repeat가 빈 문자열을 만드는 별도 경로다.
std::string RawFactoryMenu::Slots(std::int32_t count) const {
    const auto none=Format("None");if (!count) return none;if (count<0) return {}; // 원본은 양수/음수에서도 먼저None 번역을 조회한다.
    if (count>=256) throw std::out_of_range("Factory 슬롯 원본256바이트 버퍼");return FormatFactoryMana(std::string(static_cast<std::size_t>(count),'?'),-1,state_.manaType);
}
// 내 일반 Factory만 특수 메뉴를 붙이고 Damageable 부모는 항상 마지막에 실행한다.
void RawFactoryMenu::Open(o::Sid factory) const {
    if (Raw(factory)[34]==Need(hooks_.localOwner)() && Raw(factory)[10]!=state_.outpost) Available(factory);Need(hooks_.baseMenu)(factory);
}
// 같은 종류가 중복돼도 항목은 각각 작성하며 남은 슬롯은 처음 한 번만 줄인다.
int RawFactoryMenu::CurrentProduction(o::Sid factory) const {
    Need(hooks_.beginFancy)();Need(hooks_.title)(Format("Current Production\n"));auto remaining=Need(hooks_.capacity)(factory);
    std::array<bool,255> seen{};std::size_t written{};o::RawContainedFinder finder(pool_,contained_);
    // 실제 Form 순서와 flags16을 유지한다. 금지된 종류도 슬롯은 차지한다.
    for (auto sid=finder.Begin(factory);sid.value;sid=finder.Next()) {
        const auto raw=pool_.Slot(sid);if (!(raw[20]&0x10)) continue;const auto type=Read(raw,18,2);Type(type);
        if (type>=seen.size()) throw std::out_of_range("Factory 생산 중복 표 범위");
        if (Need(hooks_.allowed)(type)) { Need(hooks_.item)(Format("%s~i%s%s~.",{std::string{},std::string{},Need(hooks_.description)(type)}),state_.helpCommand,type,true,true);++written; }
        if (!seen[type]) { --remaining;seen[type]=true; }
    }
    if (!written) Need(hooks_.header)(Format("No Production Yet.\n"));Need(hooks_.item)(Need(hooks_.helpWord)(),state_.helpCommand,std::string("researchHelp"),true,true);
    const auto label=Format("View Current Production");const auto child=Need(hooks_.takeMenu)();Need(hooks_.submenu)(label,child);return remaining;
}
// 기술 후보를 원래 순서로 만든 뒤 실제 CRT 비교/교환으로 정렬한다.
void RawFactoryMenu::Available(o::Sid factory) const {
    const auto remaining=CurrentProduction(factory);Need(hooks_.beginFancy)();Need(hooks_.title)(Format("Knowledge Available\n"));
    Need(hooks_.header)(Format("Production Slots Available: %s\n",{Slots(remaining)}));const auto technology=Need(hooks_.currentTechnology)();
    if (technology>=pool_.Capacity()) throw std::out_of_range("Factory 현재 기술 부모 범위");std::vector<Knowledge> records;
    // 원본은 첫0종류에서 끝나며 중복 기술도 별도 행으로 보존한다.
    for (std::uint32_t index=0;;++index) {
        const auto type=Need(hooks_.technologyType)(o::Sid{static_cast<std::uint16_t>(technology)},index);if (!type) break;const auto& record=Type(type);
        if (std::find(state_.excluded.begin(),state_.excluded.end(),type)!=state_.excluded.end()) continue;
        if (Type(Raw(factory)[10]).theme!=record.theme && (Type(Raw(factory)[10]).theme!=3 || !(record.flags2&0x800))) continue;
        Knowledge row;row.type=type;row.available=remaining>=1;row.mana=Need(hooks_.mana)(type);row.description=Need(hooks_.description)(type);row.inProduction=Need(hooks_.inProduction)(type);
        const auto manaEnd=row.mana.find('\0'),descriptionEnd=row.description.find('\0');if (manaEnd!=std::string::npos) row.mana.resize(manaEnd);if (descriptionEnd!=std::string::npos) row.description.resize(descriptionEnd); // 원본 두strcpy의 첫NUL 경계다.
        if (row.mana.size()>9 || row.description.size()>81) throw std::length_error("Factory 원본70바이트 지식 행 범위");records.push_back(std::move(row));if (records.size()>200) throw std::length_error("Factory 원본 지식 후보200행 범위");
    }
    Sort(records);bool any{};
    // 생산 중인 행만 숨긴다. 금지 목록은 include, 슬롯 부족은 enabled에만 적용한다.
    for (const auto& row:records) if (!row.inProduction) {
        any=true;const auto label=Format("%s%s",{row.description,std::string{}});const auto blocked=excluded_.Items();
        const bool include=std::find(blocked.begin(),blocked.end(),row.type)==blocked.end();Need(hooks_.item)(label,state_.researchCommand,(row.type<<16)|factory.value,include,row.available);
    }
    if (!any) Need(hooks_.item)(Format("All Knowledge is in Production"),state_.emptyCommand,0U,true,false);
    const auto label=Format("Put Knowledge into Production");const auto child=Need(hooks_.takeMenu)();Need(hooks_.submenu)(label,child);
    if (Need(hooks_.level)(factory)<3) {
        const auto cost=Need(hooks_.cost)(Raw(factory)[10]);const auto money=Need(hooks_.money)(Need(hooks_.localOwner)());
        const bool affordable=Need(hooks_.editor)() || static_cast<double>(cost)<=static_cast<double>(money);
        Need(hooks_.item)(Format("Upgrade costs %s%d~[IfortGump.3]",{std::string(affordable ? "" : "~r"),cost}),state_.upgradeCommand,std::uint32_t(factory.value),true,true);
    }
}
// 첫 공통 우선 플래그에서 분류를 끝내고 같은 분류는 원본 C 로캘 이름으로 비교한다.
int RawFactoryMenu::Compare(const Knowledge& first,const Knowledge& second) const {
    const auto a=Type(first.type).flags2,b=Type(second.type).flags2;
    // 한쪽에만 우선 플래그가 있으면 즉시 반환한다. 둘 다 있으면 뒤 플래그를 보지 않는다.
    for (const auto bit:kPriority) { if (a&bit) { if (!(b&bit)) return -1;break; }if (b&bit) return 1; }
    // 첫NUL까지의 unsigned ASCII 차이를 반환한다. 대문자만 소문자로 접는다.
    for (std::size_t index=0;;++index) {
        auto x=index<first.description.size() ? static_cast<unsigned char>(first.description[index]) : 0U;auto y=index<second.description.size() ? static_cast<unsigned char>(second.description[index]) : 0U;
        if (x>='A' && x<='Z') x+=32;if (y>='A' && y<='Z') y+=32;if (x!=y) return static_cast<int>(x)-static_cast<int>(y);if (!x) return 0;
    }
}
// 원본 CRT의 작은 구간 선택/중앙 피벗/동률 제거/큰 구간 보관을 같은 순서로 실행한다.
void RawFactoryMenu::Sort(std::span<Knowledge> entries) const {
    constexpr std::ptrdiff_t kSmall=8; // 원본 CRT가 shortsort를 선택하는 포함 구간 길이다.
    constexpr std::size_t kRanges=30; // 원본 CRT 구간 스택의 최대 쌍 개수다.
    struct Range { std::ptrdiff_t low{},high{}; }; // 현재 포함 구간의 첫/마지막 인덱스다.
    std::array<Range,kRanges> stack{};
    std::size_t used=0;
    Range range{0,static_cast<std::ptrdiff_t>(entries.size())-1};
    // 주소 대신 현재 인덱스의 원소를 비교한다.
    const auto compare=[&](std::ptrdiff_t a,std::ptrdiff_t b) { return Compare(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)]); };
    // 원본 byte swap과 같은 항목 전체 교환이다.
    const auto swap=[&](std::ptrdiff_t a,std::ptrdiff_t b) { if (a!=b) std::swap(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)]); };
    // 큰 구간을 고정 스택에 보관하며 다음에 작은 구간을 처리한다.
    const auto push=[&](Range next) { if (used==stack.size()) throw std::logic_error("Factory 지식 정렬 스택 넘침");stack[used++]=next; };
    // 현재 구간과 보관했던 구간이 모두 끝날 때까지 원본 분기 순서를 따른다.
    for (;;) {
        if (range.high-range.low+1<=kSmall) {
            // small sort는 최대 항목을 마지막으로 보내는 선택 정렬이다.
            while (range.low<range.high) {
                auto maximum=range.low;
                // 엄격히 큰 항목만 최대 위치로 바꾼다.
                for (auto current=range.low+1;current<=range.high;++current)
                    if (compare(current,maximum)>0) maximum=current;
                swap(maximum,range.high);--range.high;
            }
        } else {
            auto middle=range.low+(range.high-range.low+1)/2;
            if (compare(range.low,middle)>0) swap(range.low,middle);
            if (compare(range.low,range.high)>0) swap(range.low,range.high);
            if (compare(middle,range.high)>0) swap(middle,range.high);
            auto left=range.low,right=range.high;
            // 실제 CRT처럼 피벗의 자기 비교를 건너뛰고 교차할 때까지 분할한다.
            for (;;) {
                if (left<middle) {
                    // 피벗 앞의 작은/동률 항목을 지난다.
                    do { ++left; } while (left<middle && compare(left,middle)<=0);
                }
                if (left>=middle) {
                    // 피벗 뒤의 작은/동률 항목을 끝까지 검사한다.
                    do { ++left; } while (left<=range.high && compare(left,middle)<=0);
                }
                // 오른쪽은 엄격히 큰 항목만 지난다.
                do { --right; } while (right>middle && compare(right,middle)>0);
                if (left>right) break;
                swap(left,right);if (middle==right) middle=left;
            }
            ++right;
            if (right>middle) {
                // 피벗 오른쪽의 실제0 비교 항목도 제거한다.
                do { --right; } while (right>middle && compare(right,middle)==0);
            }
            if (right<=middle) {
                // 피벗을 건너뛰고 왼쪽 끝까지 동률 구간을 제거한다.
                do { --right; } while (right>range.low && compare(right,middle)==0);
            }
            if (right-range.low<range.high-left) {
                if (left<range.high) push({left,range.high});range.high=right;
                if (range.low<range.high) continue;
            } else {
                if (range.low<right) push({range.low,right});range.low=left;
                if (range.low<range.high) continue;
            }
        }
        if (!used) return;
        range=stack[--used];
    }
}
// 기존 controlled 스택과 수명이 같은 정적 인자 토큰만 사용한다.
void BindFactoryMenuControls(FactoryMenuHooks& hooks,const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menus,std::function<std::uint32_t(std::string_view)> encodeString) {
    hooks.beginFancy=[&widgets] { widgets.BeginFancy(); };hooks.title=[&controls](auto text) { controls.AddTitle(text); };hooks.header=[&controls](auto text) { controls.AddHeader(text); };
    hooks.takeMenu=[&menus] { return menus.TakeCurrent(); };hooks.submenu=[&controls](auto text,auto child) { controls.AddSubmenu(text,child,true,false); };
    hooks.item=[&controls,encode=std::move(encodeString)](auto text,auto command,ObjectMenuParameter parameter,bool include,bool enabled) {
        const auto token=std::holds_alternative<std::uint32_t>(parameter) ? std::get<std::uint32_t>(parameter) : Need(encode)(std::get<std::string>(parameter));controls.AddText(text,command,token,include,enabled,false);
    };
}
}
