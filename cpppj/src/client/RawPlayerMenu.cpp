// 실제 Player 메뉴의 현재 조회·행 포함/활성·문자열 버퍼·호출 순서를 유지한다.
#include "client/RawPlayerMenu.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 도달한 미연결 효과를 빈 목록/성공으로 바꾸지 않는다.
template<class Function> const Function& Required(const Function& function) { if (!function) throw std::logic_error("실제 Player 메뉴 하위 미연결");return function; }
}
// 이번 구현은 목표 판본PATCH만 받으며 세 실제 표 크기를 확인한다.
RawPlayerMenu::RawPlayerMenu(const o::RawPlayerIdentity& identity,std::span<const std::uint32_t> alliances,std::span<const std::uint8_t> hidden,std::span<const std::uint32_t> priestStates,PlayerMenuState& state,PlayerMenuHooks hooks)
    :identity_(identity),alliances_(alliances),hidden_(hidden),priestStates_(priestStates),state_(state),hooks_(std::move(hooks)) {
    if (identity.Edition()!=o::OriginalEdition::Patch1078 || alliances.size()!=81 || hidden.size()!=9 || priestStates.size()!=9) throw std::invalid_argument("Player 메뉴 PATCH 공유 상태 오류");
}
// 현재 battle은 Identity와 같은00594fbc다.
bool RawPlayerMenu::Battle() const { return Required(identity_.CurrentInputs().battle)(); }
// 현재 mission은 별도00594fa4다.
bool RawPlayerMenu::Mission() const { return Required(hooks_.mission)(); }
// 현재 metamap은 Identity와 같은00594fc8다.
bool RawPlayerMenu::Meta() const { return Required(identity_.CurrentInputs().metamap)(); }
// 현재 editor는 Identity와 같은005c85a4다.
bool RawPlayerMenu::Editor() const { return Required(identity_.CurrentInputs().editor)(); }
//00540c74의 전체DWORD다.
std::uint32_t RawPlayerMenu::MySubscriber() const { return Required(identity_.CurrentInputs().subscriber)(); }
//00540c70의 현재 소유자다.
std::uint32_t RawPlayerMenu::LocalOwner() const { return Required(identity_.CurrentInputs().localOwner)(); }
// 실제 getter를 반복 호출하는 원본 위치를 합치지 않는다.
std::uint32_t RawPlayerMenu::Owner(std::uint32_t subscriber) const { return identity_.OwnerForSubscriber(subscriber); }
// 현재 같은9 Player의 원문을 읽는다.
std::span<const std::uint8_t> RawPlayerMenu::Player(std::uint32_t owner) const { if (owner>=9) throw std::out_of_range("Player 메뉴 owner 범위 오류");return identity_.PlayerRecords().subspan(owner*180,180); }
// 현재 같은40 metadata 기록을 읽는다.
std::span<const std::uint8_t> RawPlayerMenu::Metadata(std::uint32_t index) const { const auto meta=identity_.MetaRecords();if (meta.size()!=40*460 || index>=40) throw std::logic_error("Player 메뉴 metadata 미연결/범위 오류");return meta.subspan(index*460,460); }
// 원본 little endian 폭을 유지한다.
std::uint32_t RawPlayerMenu::Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) { if (offset+width>bytes.size()) throw std::out_of_range("Player 메뉴 필드 범위 오류");std::uint32_t value=0;
    // 정렬되지 않은 원문 바이트를 낮은 순서로 조립한다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(bytes[offset+index])<<(8*index);return value;
}
// 원본 정상 NUL 문자열만 제공한다.
std::string_view RawPlayerMenu::Text(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t capacity) { if (offset+capacity>bytes.size()) throw std::out_of_range("Player 메뉴 문자열 범위 오류");const auto part=bytes.subspan(offset,capacity);const auto end=std::find(part.begin(),part.end(),std::uint8_t{});if (end==part.end()) throw std::length_error("Player 메뉴 NUL 없는 문자열");return {reinterpret_cast<const char*>(part.data()),static_cast<std::size_t>(end-part.begin())}; }
//0048f550은 metamap 진단 뒤 현재 포인터의 번호 하위BYTE를 반환한다.
std::uint32_t RawPlayerMenu::Number(std::uint32_t owner) const { Player(owner);if (Meta()) Required(hooks_.assertion)(PlayerMenuAssertion::MetamapNumber);return owner&255U; }
//004922e0의 Player flags22·ownerState2와 무효 번호 진단이다.
bool RawPlayerMenu::Dead(std::uint32_t owner) const { if (owner<1 || owner>8) { Required(hooks_.assertion)(PlayerMenuAssertion::PriestOwner);return true; }return !(Player(owner)[0x50]&0x22) || priestStates_[owner]==2; }
//00492340의 유효/참여 조건과 ownerState1을 그대로 유지한다.
bool RawPlayerMenu::Captured(std::uint32_t owner) const { if (owner<1 || owner>8) { Required(hooks_.assertion)(PlayerMenuAssertion::PriestOwner);return true; }return (Player(owner)[0x50]&0x22) && priestStates_[owner]==1; }
//00425e90은 편집 검사보다 SP를 먼저 읽으며 NaN을 지불 가능으로 취급하지 않는다.
bool RawPlayerMenu::CanAfford(float amount) const { const auto sp=Required(hooks_.money)(LocalOwner());return Editor() || amount<=sp; }
// 번역된 고정 서식과 원본256바이트/NUL 한도다.
std::string RawPlayerMenu::Format(std::string_view key,std::initializer_list<PriestMenuArgument> args) const { return Print(Required(hooks_.translate)(key),args); }
// 원본에 번역 호출이 없는 asksend 및 조회가 사이에 있는 서식도 같은 출력 한도를 사용한다.
std::string RawPlayerMenu::Print(std::string_view pattern,std::initializer_list<PriestMenuArgument> args) const { auto text=Required(hooks_.print)(pattern,{args.begin(),args.size()});const auto nul=text.find('\0');if (nul!=std::string::npos) text.resize(nul);if (text.size()>=256) throw std::length_error("Player 메뉴256바이트 서식 초과");return text; }
// 원본 strcpy처럼 NUL까지 쓰며 정적 버퍼 뒤 바이트를 지우지 않는다.
void RawPlayerMenu::Store(std::span<char> target,std::string_view text) { if (text.size()>=target.size()) throw std::length_error("Player 메뉴 정적 버퍼 초과");std::copy(text.begin(),text.end(),target.begin());target[text.size()]=0; }
// 현재 정적 토큰을 최신 문자열로 해석한다. 임시 문자열 고리로 수명을 바꾸지 않는다.
std::string_view RawPlayerMenu::TextParameter(std::uint32_t token) const { const auto& bytes=token==state_.missionTextToken ? state_.sendMission : token==state_.lobbyTextToken ? state_.sendLobby : throw std::out_of_range("Player 메뉴 문자열 토큰 오류");const auto end=std::find(bytes.begin(),bytes.end(),'\0');if (end==bytes.end()) throw std::length_error("Player 메뉴 정적 NUL 오류");return {bytes.data(),static_cast<std::size_t>(end-bytes.begin())}; }
// 원본 세 송금 경로의 include/enabled와 반복 현재 owner/SP 조회를 유지한다.
void RawPlayerMenu::SendMoney(std::uint32_t subscriber,std::uint32_t owner) const {
    Required(hooks_.beginVertical)();const bool mission=Mission();const auto text=Print("asksend(%d)",{static_cast<std::int32_t>(Owner(subscriber))});Store(mission ? std::span(state_.sendMission) : std::span(state_.sendLobby),text);
    bool hidden=false;if (!mission) { static_cast<void>(Owner(MySubscriber()));hidden=hidden_[owner]!=0; }
    constexpr std::array<float,5> amounts{300,700,1000,1,1}; // 실제 지불 가능 색 조회 순서다.
    constexpr std::array<std::string_view,5> labels{"Send %s300~[IfortGump.3]","Send %s700~[IfortGump.3]","Send %s1000~[IfortGump.3]","Send %s X~[IfortGump.3]","Send %s All~[IfortGump.3]"}; // 원본 고정 다섯 서식이다.
    // 금액은 색만 바꾸며 행 활성화는 현재 연결/허용 플래그의 별도 계산이다.
    for (std::size_t index=0;index<amounts.size();++index) {
        const bool enabled=(!hidden || index==4) && (Player(owner)[0x50]&0x21) && Required(hooks_.allowSend)();
        const bool affordable=CanAfford(amounts[index]);const auto label=Format(labels[index],{std::string(affordable ? "" : "~r")});
        if (mission && index==4) Required(hooks_.item)(label,state_.tell,state_.missionTextToken,1,enabled);
        else { const auto current=Owner(subscriber);Required(hooks_.item)(label,state_.send[index],current,hidden ? 0U : 1U,enabled); }
    }
    const auto label=Format("Send Money");const auto child=Required(hooks_.takeMenu)();Required(hooks_.submenu)(label,child);if (!Required(hooks_.fort)()) Required(hooks_.separator)();
}
// 전체 원본 분기에서 현재 Player/모드와 앞서 저장한 번호를 구분한다.
std::uint32_t RawPlayerMenu::Open(std::uint32_t subscriber) const {
    auto owner=Owner(subscriber);if (!owner) return 0;
    const auto name=Mission() ? std::string(Text(Player(owner),0xc,32)) : identity_.NameForSubscriber(subscriber);
    Required(hooks_.beginFancy)();const auto title=Format("Player %s\n",{name});Required(hooks_.title)(title);
    if (Battle() && Player(owner)[0x30]) Required(hooks_.parseAddress)(Text(Player(owner),0x30,32),0);
    if (Meta()) Required(hooks_.parseAddress)(Text(Metadata(owner),0x1a0,44),0);
    if (!Editor()) {
        if (Battle()) {
            owner=Owner(subscriber);
            if (!Mission()) { const auto text=Format((Player(owner)[0x50]&1) ? "Status: ~iOnline~.\n" : "Status: ~r~EDISCONNECTED~.\n");Required(hooks_.header)(text); }
            const auto condition=Format(Dead(owner) ? "Dead" : Captured(owner) ? "Captured" : "Alive");const auto priest=Format("Priest: ~i%s~.\n",{condition});Required(hooks_.header)(priest);bool allied=false;
            // 현재 대상→후보와 후보→대상의 두 방향을 별도로 읽는다.
            for (std::uint32_t candidate=1;candidate<9;++candidate) if (Read(Player(candidate),0x2c) && candidate!=owner && alliances_[owner*9+candidate]) {
                const auto pattern=Required(hooks_.translate)("Allied with ~i%s~.");auto text=Print(pattern,{std::string(Text(Player(candidate),0xc,32))});if (!alliances_[candidate*9+owner]) text+=Format("(One-Way)");text+='\n';if (text.size()>=256) throw std::length_error("Player 메뉴 동맹 서식 초과");Required(hooks_.header)(text);allied=true;
            }
            if (!allied) { const auto text=Format("Allied with nobody.\n");Required(hooks_.header)(text); }
        }
        bool chatItem=false;
        if (!Mission() || subscriber==MySubscriber()) {
            static_cast<void>(identity_.AddressForSubscriber(subscriber));
            if (Meta()) {
                o::PlayerIdentityRecord finder{};o::RawPlayerIdentity::Initialize(finder,1);
                // 실제 Finder의 subscriberDWORD만 넣고 같은 조회 몸체로 현재 metadata 번호를 찾는다.
                for (std::size_t byte=0;byte<4;++byte) finder[4+byte]=static_cast<std::uint8_t>(subscriber>>(8*byte));
                if (identity_.Resolve(finder) && Read(finder,0)) {
                    const auto index=Read(finder,0);const auto entry=Metadata(index);const auto version=Required(hooks_.translate)("Version: ~i%d.%d~.\n");auto text=Print(version,{static_cast<std::int32_t>(Read(entry,0xb0,2)),static_cast<std::int32_t>(Read(entry,0xb2,2))});Required(hooks_.header)(text);
                    const auto reliability=Required(hooks_.translate)("Reliability: ~i%d%%~.\n");const auto score=Required(hooks_.reliability)(index);text=Print(reliability,{std::bit_cast<std::int32_t>(score)});Required(hooks_.header)(text);
                    const auto games=std::min<std::uint32_t>(Read(entry,0x54,2),0x7fbc);text=Format("Games Played: ~i%d~.\n",{static_cast<std::int32_t>(games)});Required(hooks_.header)(text);
                    if (Meta() || !Mission()) { text=Format("View Player Info");Required(hooks_.item)(text,state_.viewInfo,subscriber,subscriber,1); }
                }
            }
            const bool ignored=Required(hooks_.ignored)(subscriber);const auto whispered=Required(hooks_.whispered)(subscriber);
            if (subscriber!=MySubscriber()) {
                const auto command=state_.ignore[ignored ? 1 : 0];const auto text=Format("Ignore chat");Required(hooks_.checked)(ignored,text,command,subscriber);chatItem=true;
                if (subscriber!=MySubscriber()) { const auto token=state_.whisper[whispered ? 1 : 0];const auto label=Format("Whisper To");Required(hooks_.checked)(whispered,label,token,subscriber); }
            }
        }
        if (Battle()) {
            owner=Owner(subscriber);if (chatItem) Required(hooks_.separator)();const auto local=Owner(MySubscriber());Player(local);
            const auto label=Format("Find Priest!");const bool enabled=!Dead(owner);Required(hooks_.item)(label,state_.findPriest,owner,1,enabled);
            if (subscriber!=MySubscriber() && !hidden_[Number(local)]) SendMoney(subscriber,owner);
            if (Battle() && !Required(hooks_.fort)()) {
                const auto me=Owner(MySubscriber());Player(me);static_cast<void>(Owner(MySubscriber()));const auto target=Owner(subscriber);
                if (subscriber!=MySubscriber() && !hidden_[Number(me)]) {
                    const bool allied=(Read(Player(LocalOwner()),0x54)&(1U<<(target&31)))!=0;
                    const bool allianceEnabled=Required(allied ? hooks_.allowBreak : hooks_.allowOffer)() && !hidden_[target];const auto text=Format(allied ? "Break Alliance" : "Request Alliance");Required(hooks_.item)(text,allied ? state_.breakAlliance : state_.requestAlliance,target,1,allianceEnabled);
                }
            }
        }
    }
    return Required(hooks_.takeMenu)();
}
// 실제 controlled 메뉴/스택에 원본 포함/활성 조건을 그대로 전달한다.
void BindPlayerMenuControls(PlayerMenuHooks& hooks,const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menus) {
    hooks.beginFancy=[&widgets] { widgets.BeginFancy(); };hooks.beginVertical=[&widgets] { widgets.BeginVertical(); };hooks.separator=[&controls] { controls.AddSeparator(); };
    hooks.title=[&controls](std::string_view text) { controls.AddTitle(text); };hooks.header=[&controls](std::string_view text) { controls.AddHeader(text); };hooks.takeMenu=[&menus] { return menus.TakeCurrent(); };
    hooks.item=[&controls](std::string_view text,std::uint32_t command,std::uint32_t parameter,std::uint32_t include,std::uint32_t enabled) { controls.AddText(text,command,parameter,include!=0,enabled!=0,false); };
    hooks.checked=[&controls](std::uint32_t checked,std::string_view text,std::uint32_t command,std::uint32_t parameter) { controls.AddChecked(checked!=0,text,command,parameter,true,true); };hooks.submenu=[&controls](std::string_view text,std::uint32_t child) { controls.AddSubmenu(text,child,true,false); };
}
// 실제 Player 메뉴의 유일한 입력은 subscriberDWORD다. 원본 호출자의 여분 두 인자는 사용하지 않는다.
void BindObjectPlayerMenu(ObjectMenuHooks& hooks,const RawPlayerMenu& players) { hooks.playerMenu=[&players](std::uint32_t subscriber,bool,bool) { return players.Open(subscriber); }; }
}
