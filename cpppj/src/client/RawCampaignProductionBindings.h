// 같은현재Player/Factory목록/TYPE/TotalMade허용표로원본생산Deck을재계산한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "o/RawProductionDeck.h"
#include "o/RawTotalMade.h"

namespace netstorm::client {
// 실제생산재계산의현재전역을참조한다.호출자가별도판정값을복사하지않는다.
struct CampaignProductionState {
    const bool& mission; // PATCH00594fa0의현재미션모드다.
    const std::uint32_t& fullDeck; // PATCH0054db14의전체Deck모드DWORD다.
    const std::uint32_t& freeTechnologyType; // PATCH005412bc의현재Mog153이며getTeleportTechFree설정이추가하는타입이다.
    const std::uint32_t& templeProduct; // PATCH0054126c의현재신전기본생산sunWalker133타입이다.
};
// 장치/메뉴를대체하지않고현재설정과원본진단만외부에연결한다.
struct CampaignProductionEffects {
    std::function<std::int32_t(std::string_view)> configInt; // 현재getTeleportTechFree를조회한다.
    std::function<void()> modeAssertion; // inFortMode||inBattleMode원본진단뒤계속진행하는경계다.
};
// 0044f7c0/00475530/004752b0(mode0)의계산을같은실제목록/내용물/Deck으로연결한다.
class RawCampaignProductionBindings {
public:
    // 모든참조는연결보다오래살아야하며Player/Deck/Carrier/Create는같은core.pool을공유한다.
    RawCampaignProductionBindings(RawWorldCore core,o::RawPlayerWorld& player,const o::RawCarrierPreDestroy& carrier,
        const o::RawContainedCreate& create,o::RawProductionDeck& deck,const o::RawTotalMade& allowed,
        const o::ContainedFinderState& contained,CampaignProductionState state,CampaignProductionEffects effects);
    // 0044f970의현재Deck조회다.재계산을추가로호출하지않는다.
    bool Contains(std::uint32_t type) const;
    // 현재기술부모와원본kindWORD조건을사용한다.표시Gump갱신은별도실제명령소비자의책임이다.
    void IndicateTechnology(std::uint32_t type) const;
    // 현재templateOwner또는명시owner의기존Deck을원문순서대로재계산하고같은토큰을반환한다.
    std::uint32_t RefreshCurrent() const;
    std::uint32_t Refresh(std::uint32_t owner) const;
    // Factory의실제+b4/00453f60→가상+80/004ad0f0→+2다.미복원가상표는거부한다.
    std::int32_t FactoryCapacity(o::Sid factory) const;
    // 004752b0(mode0)의실제생산포함과distinct종류별남은자리반환이다.허용표조회도생략하지않는다.
    std::int32_t AppendFactoryProducts(o::Sid factory,std::uint32_t deckToken) const;
    // UI/미션도같은현재TotalMade허용표를읽는다.금지된일반Factory생산을Deck에서임의제외하지않는다.
    bool Allowed(std::int32_t type) const;
    const o::SidPool& Pool() const; // 실제월드동일성검사용이다.
    const o::RawProductionDeck& Deck() const; // 같은현재Player+68수명저장소다.
private:
    // 원본DWORD목록을WORD로미리자르지않고현재풀범위를확인한다.
    o::Sid Factory(std::uint32_t value) const;
    RawWorldCore core_; // 단일Pool/TYPE/공통Factory목록을빌리는참조묶음이다.
    o::RawPlayerWorld& player_; // 현재두owner와9Player원문이다.
    o::RawProductionDeck& deck_; // 해당원문의Deck포인터와실제블록이다.
    const o::RawTotalMade& allowed_; // 튜토리얼/메뉴와공유해야하는금지DWORD표의실제몸체다.
    const o::ContainedFinderState& contained_; // 같은현재containedType/권한/진단전역이다.
    CampaignProductionState state_; // 현재생산모드와타입전역의안정참조다.
    CampaignProductionEffects effects_; // 현재설정/진단의실제소비자다.
    o::RawIndicateTechnology indicate_; // 동일기술부모/Factory/ContentHost의전체하위조합이다.
};
}
