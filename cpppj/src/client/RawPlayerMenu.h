// PATCH Player 메뉴를 실제 Identity/Player/동맹/사제 상태와 controlled 메뉴에 연결한다.
#pragma once
#include "client/RawObjectMenu.h"

namespace netstorm::client {
// 원본이 진단 후 계속하는 사제 번호/메타맵 번호 조회다.
enum class PlayerMenuAssertion { PriestOwner,MetamapNumber };
// 두 정적 asksend 버퍼와 현재 명령 전역이다. 버퍼 주소 의미를 opaqueDWORD로 유지한다.
struct PlayerMenuState {
    std::array<char,104> sendMission{},sendLobby{}; // PATCH005cad98/005cae00의 현재 NUL 문자열이다.
    std::uint32_t missionTextToken{0x005cad98},lobbyTextToken{0x005cae00}; // 호스트 주소로 역참조하지 않는 원본 정적 식별자다.
    std::uint32_t viewInfo{0x0200009b},findPriest{0x020000b8},tell{0x02000095}; // 실제 Player 정보/사제 찾기/안내 명령이다.
    std::array<std::uint32_t,2> ignore{0x020000ee,0x020000ef},whisper{0x02000027,0x02000101}; // 상태0/비영의 원본 토글 명령이다.
    std::array<std::uint32_t,5> send{0x02000120,0x02000108,0x020000ec,0x02000100,0x0200012b}; //300/700/1000/X/All 순서다.
    std::uint32_t requestAlliance{0x020000fb},breakAlliance{0x020000fa}; // 실제 동맹 요청/해제 명령이다.
};
// 실제 메뉴 부모 밖의 현재 모드/목록/주소/통신·SP·번역/작성 경계다.
struct PlayerMenuHooks {
    std::function<bool()> mission,fort,allowSend,allowOffer,allowBreak; //00594fa4/00594fa0/0052f7ed/eb/ec의 현재 상태다.
    std::function<void(PlayerMenuAssertion)> assertion; // 실제 진단을 성공/무시로 대체하지 않는다.
    std::function<void(std::string_view,std::uint32_t)> parseAddress; //004e0cd0의 실제 주소 해석이며 포트0이다.
    std::function<std::uint32_t(std::uint32_t)> reliability; //00478310의 현재 metadata 신뢰도다.
    std::function<bool(std::uint32_t)> ignored; //0042e390의 실제 정수 subscriber 목록 조회다.
    std::function<std::uint8_t(std::uint32_t)> whispered; //0042f640의 실제 이름 목록 결과 하위BYTE다.
    std::function<float(std::uint32_t)> money; // 실제0040ec50 분산 SP이며 메뉴 비교가 매번 새로 읽는다.
    std::function<std::string(std::string_view)> translate; // 원본 번역 조회이며 뒤의 인자 조회보다 먼저 실행되는 위치를 보존한다.
    std::function<std::string(std::string_view,std::span<const PriestMenuArgument>)> print; // 번역하지 않는 실제 고정%s/%d/%% 서식이다.
    std::function<void()> beginFancy,beginVertical,separator; // 같은 실제 controlled 메뉴 작성이다.
    std::function<void(std::string_view)> title,header; // 원본 제목/머리 작성이다.
    std::function<std::uint32_t()> takeMenu; // 실제 현재 메뉴 pop이다.
    std::function<void(std::string_view,std::uint32_t)> submenu; // include/enabled 기본값1인 자식이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> item; // label,command,parameter,include,enabled이며forceColor0이다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t,std::uint32_t)> checked; // check,label,command,parameter이며include/enabled1이다.
};
// PATCH004c9700 전체 메뉴 부모다. 큰 의존 몸체는 명시 경계이며 실제 메뉴 명령 실행과 구분한다.
class RawPlayerMenu {
public:
    // Identity와 같은 기록/현재 전역을 빌리고9×9방향 동맹/9숨김/9사제 상태도 복사하지 않는다.
    RawPlayerMenu(const o::RawPlayerIdentity& identity,std::span<const std::uint32_t> alliances,
        std::span<const std::uint8_t> hidden,std::span<const std::uint32_t> priestStates,PlayerMenuState& state,PlayerMenuHooks hooks);
    // 실제 subscriberDWORD를 받아 메뉴를 작성해 현재 스택에서 꺼낸다. owner0이면0이다.
    std::uint32_t Open(std::uint32_t subscriber) const;
    // Tell 소비자는 정적 토큰을 현재 버퍼로 읽는다. 버퍼 갱신 뒤에도 같은 토큰 의미가 유지된다.
    std::string_view TextParameter(std::uint32_t token) const;
private:
    // 현재 실제 모드/로컬/subscriber를 같은 Identity 전역 getter로 읽는다.
    bool Battle() const;bool Mission() const;bool Meta() const;bool Editor() const;
    std::uint32_t MySubscriber() const;std::uint32_t LocalOwner() const;std::uint32_t Owner(std::uint32_t subscriber) const;
    // 원본 기록/WORD/DWORD/이름과 사제/플레이어 번호 조회를 보존한다.
    std::span<const std::uint8_t> Player(std::uint32_t owner) const;
    std::span<const std::uint8_t> Metadata(std::uint32_t index) const;
    static std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4);
    static std::string_view Text(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t capacity);
    std::uint32_t Number(std::uint32_t owner) const;
    bool Dead(std::uint32_t owner) const;bool Captured(std::uint32_t owner) const;
    // 현재 금액/편집 상태와 고정 서식·정적 버퍼 수명을 보존한다.
    bool CanAfford(float amount) const;
    std::string Format(std::string_view key,std::initializer_list<PriestMenuArgument> args={}) const;
    std::string Print(std::string_view pattern,std::initializer_list<PriestMenuArgument> args={}) const;
    static void Store(std::span<char> target,std::string_view text);
    void SendMoney(std::uint32_t subscriber,std::uint32_t owner) const;
    const o::RawPlayerIdentity& identity_; // 같은 실제 조회/기록/전역이다.
    std::span<const std::uint32_t> alliances_; // 방향별81DWORD다.
    std::span<const std::uint8_t> hidden_; // 실제9개 숨김 BYTE다.
    std::span<const std::uint32_t> priestStates_; // 실제 사제 ownerState9DWORD다.
    PlayerMenuState& state_; // 현재 명령과 정적 버퍼다.
    PlayerMenuHooks hooks_; // 실제 현재 조회/외부 효과다.
};
// 같은 메뉴 스택/정수 명령을 연결한다. 실제 현재 문자열은 TextParameter로 소비한다.
void BindPlayerMenuControls(PlayerMenuHooks& hooks,const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menus);
void BindObjectPlayerMenu(ObjectMenuHooks& hooks,const RawPlayerMenu& players);
}
