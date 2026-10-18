// 원본 Factory의 생산/지식 하위 메뉴와 기본 Damageable 메뉴를 같은 controlled 트리에 작성한다.
#pragma once
#include "client/RawObjectMenu.h"
namespace netstorm::client {
// 실제 명령·제외 TYPE 전역이다. 다른 종류의 생산 금지 목록과 합치지 않는다.
struct FactoryMenuState {
    std::uint32_t outpost{126},manaType{103}; // 541250과5411f4의 원본 기본 타입이다.
    std::uint32_t helpCommand{0x02000017},researchCommand{0x03000080},emptyCommand{0x02000089},upgradeCommand{0x03000032}; // 실제542754/5428f8/542918/5427c0이다.
    std::array<std::uint32_t,20> excluded{158,133,100,101,102,84,122,121,138,144,150,73,123,124,125,85,86,87,88,126}; // 원본540d0의 제외 전역 순서다.
};
// 실제 게임 조회는 Session, 문자열과 작성/토큰은 Client의 장기 controlled 메뉴에 연결한다.
struct FactoryMenuHooks {
    std::function<std::uint32_t()> localOwner,currentTechnology; // 현재 로컬 소유자와 실제 기술 부모다.
    std::function<bool()> editor; // 실제 편집 DWORD의 현재 비영 여부다.
    std::function<std::uint32_t(o::Sid,std::uint32_t)> technologyType; // 4acac0(parent,4,0,index)의 종류 WORD다.
    std::function<int(o::Sid)> capacity,level; // 현재 가상+b4와+80의 실제 반환값이다.
    std::function<bool(std::uint32_t)> allowed,inProduction; // 같은 TotalMade와 현재 Player Deck의 조회다.
    std::function<std::int32_t(std::uint32_t)> cost; // 같은 원본 정수 비용 표다.
    std::function<float(std::uint32_t)> money; // 원본 현재 SP를 읽는다.
    std::function<std::string(std::uint32_t)> description,mana; // 실제 TYPE의 번역 설명과 최대9바이트 마나 문자열이다.
    std::function<std::string()> helpWord; // 현재 실제 도움말 표시 원문이다.
    std::function<std::string(std::string_view,std::span<const PriestMenuArgument>)> format; // 번역과 원본 고정 printf다.
    std::function<void()> beginFancy; // 같은477ae0의 새 메뉴 칸을 만든다.
    std::function<void(std::string_view)> title,header; // 실제477ee0/478020의 제목·머리 작성이다.
    std::function<std::uint32_t()> takeMenu; // 같은475c00의 현재 칸 취득이다.
    std::function<void(std::string_view,std::uint32_t)> submenu; // 실제 enabled1/checked0이다.
    std::function<void(std::string_view,std::uint32_t,ObjectMenuParameter,bool,bool)> item; // 원본 include/enabled를 구분하고 forceColor는0이다.
    std::function<void(o::Sid)> baseMenu; // 실제44cbd0 Damageable 부모 메뉴다.
};
// 4735c0의 ASCII 마나 문자를 원본 그림자/활성 프레임으로 바꾼다. 점은 출력·활성 개수에서 제외한다.
std::string FormatFactoryMana(std::string_view text,std::int32_t active,std::uint32_t manaType=103);
// 454c00/4540d0/4752b0의 원본 순서·지식 정렬·슬롯 계산만 수행한다. 생산 명령 실행은 별도다.
class RawFactoryMenu {
public:
    RawFactoryMenu(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,const o::ContainedFinderState& contained,
        const o::SquidPostPopList& excluded,FactoryMenuState& state,FactoryMenuHooks hooks); // 같은 단일 풀/현재 타입/전역을 빌린다.
    void Open(o::Sid factory) const; // 내 Factory만 생산·지식을 작성하고 항상 기본 메뉴를 마지막에 호출한다.
    int CurrentProduction(o::Sid factory) const; // 실제 생산 하위 메뉴를 작성하고 distinct 종류 기준 남은 슬롯을 반환한다.
    std::string Slots(std::int32_t count) const; // 454040의0번역/음수빈값/양수?아이콘을 반환한다.
private:
    struct Knowledge { std::uint32_t type{};bool available{},inProduction{};std::string mana,description; }; // 원본70바이트 행에서 실제 사용한 필드들이다.
    void Available(o::Sid factory) const; // 현재 기술/테마로 후보를 만들고 같은 CRT 순서로 정렬한다.
    void Sort(std::span<Knowledge> records) const; // 원본4e6010/shortsort의 동률 교환 순서다.
    int Compare(const Knowledge& first,const Knowledge& second) const; // 453f70의 flags 우선순위와 ASCII stricmp다.
    std::span<const std::uint8_t> Raw(o::Sid sid) const; // 실제 두 가상 표와 물리 SID를 확인한다.
    const o::RiftTypeRecord& Type(std::uint32_t type) const; // 같은 현재 TYPE를 읽는다.
    std::string Format(std::string_view key,std::initializer_list<PriestMenuArgument> args={}) const; // 원본256바이트 서식 한도를 보존한다.
    const o::SidPool& pool_; // 게임과 같은50바이트 슬롯 풀이다.
    std::span<const o::RiftTypeRecord> types_; // 같은 현재188개 TYPE다.
    const o::ContainedFinderState& contained_; // 같은 실제 Form 검색 정책이다.
    const o::SquidPostPopList& excluded_; // 52f7e0의 현재 건설 제외 목록이다.
    FactoryMenuState& state_; // 실제 명령/타입 전역을 현재 값으로 읽는다.
    FactoryMenuHooks hooks_; // 같은 게임·번역·controlled 메뉴 경계다.
};
// 메뉴 스택·글꼴·정적 문자열 토큰을 복제하지 않고 기존 몸체에 연결한다.
void BindFactoryMenuControls(FactoryMenuHooks& hooks,const MenuGumpWidgets& widgets,const MenuGumpControls& controls,
    const MenuGump& menus,std::function<std::uint32_t(std::string_view)> encodeString);
}
