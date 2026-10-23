// 상단메뉴의준비/추적주소/현재게임조회가같은실제Session을사용하는지검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/RawGameMenus.h"
#include "o/RawEnergySupply.h"
#include "o/RawMissionStart.h"
#include <stdexcept>
using namespace netstorm::client;
namespace {
// 보호게임설치에필요한장치/번역만명시하며실제SP/사제/Player계산은대체하지않는다.
struct GameMenuSessionServices {
    SoundList names{OriginalEdition::Patch1078}; // 같은실제소리이름표의장기저장소다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 명시영어번역입력이다.
    // 원본미션의실제설치가선택하는외부장치만제공한다.
    RawCampaignSessionServices Make() {
        names.Initialize();RawCampaignSessionServices result;result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[] { return 100.0; };result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("게임메뉴검사에서삭제효과미선택"); };result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
}
// 메뉴준비는같은분산SP배열을회전하고추적주소는기존사제/SpStore소유자와동일하다.
TEST_CASE(CampaignGameMenus_SharedSpAndTracking) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);GameMenuSessionServices services; // 보호자산과명시장치수명이다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",services.Make());bool rejected=false; // 실제Session부모다.
    try { session.PrepareGameMenus(); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);session.Load();
    session.SetSp(1,1234.5F);const auto& store=session.SpStore();auto& tracking=session.GameMenuTrackingOffset();CHECK(&tracking==&store.TrackingOffsetCurrent());GameMenuBarState state(tracking); // 같은안정주소를실제bar상태에빌린다.
    std::array<std::uint32_t,9> amounts{};
    // 실제모든소유자의SP비트를메뉴준비전보관한다.
    for (std::uint32_t owner=0;owner<9;++owner) amounts[owner]=std::bit_cast<std::uint32_t>(session.Sp(owner));
    const auto offset=store.Offset(),oldTracking=std::bit_cast<std::uint32_t>(tracking);const auto pools=store.Pools();session.PrepareGameMenus(); // 메뉴의실제491680을직접호출한다.
    const auto advanced=offset+3U;CHECK(store.Offset()==(std::bit_cast<std::int32_t>(advanced)>9600 ? advanced-9600U : advanced));CHECK(store.Pools()!=pools);CHECK(std::bit_cast<std::uint32_t>(tracking)==oldTracking);
    // 회전뒤실제SP읽기의9개값은보존되며독립목록을돈으로대체하지않는다.
    for (std::uint32_t owner=0;owner<9;++owner) CHECK(std::bit_cast<std::uint32_t>(session.Sp(owner))==amounts[owner]);
    state.checksum=std::bit_cast<std::int32_t>(oldTracking+3U);CHECK(std::bit_cast<std::uint32_t>(store.TrackingOffsetCurrent())==oldTracking+3U); // 실제상위부모쓰기와동일주소임을확인한다.
}
// 실제현재모드/Player/숨김/사제/비용/SP를초기값사본이아닌공유저장소에서읽는다.
TEST_CASE(CampaignGameMenus_CurrentWorldReaders) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);GameMenuSessionServices services;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",services.Make());session.Load();
    GameMenusHooks ui;ui.global=[](std::uint32_t) { return 0x12345678U; };ui.editingMission=[] { return 33U; };ui.demo=[] { return 44U; };auto hooks=session.BindGameMenus(std::move(ui));auto records=session.PlayerRecords(); // 보존할UI조회와같은현재Player원문이다.
    CHECK(hooks.global(0x5c85a4)==session.Editor() && hooks.global(0x595344)==session.PlayerKnown() && hooks.global(0x540c70)==session.LocalOwner());CHECK(hooks.global(0x531860)==0x12345678U && hooks.editingMission()==33 && hooks.demo()==44);
    session.MissionStartSettings().denyAscend=-17;CHECK(hooks.global(0x59507c)==0xffffffefU);session.EnergySettings().userCheat=0x88;CHECK(hooks.global(0x5cab30)==0x88); // 다른현재게임소유자의상태를UI사본으로덮지않는다.
    CHECK(hooks.player(2).data()==records.data()+2*180 && hooks.player(2).size()==180);Put(records,2*180+0x2c,0x87654321U);CHECK(Get(hooks.player(2),0x2c)==0x87654321U);
    const auto battle=session.Core().ownerMode.battle;session.Core().ownerMode.battle=false;CHECK(hooks.global(0x594fbc)==0);session.Core().ownerMode.battle=true;CHECK(hooks.global(0x594fbc)==1);session.Core().ownerMode.battle=battle;
    session.HiddenPlayers()[2]=0x81;CHECK(hooks.global(0x52f820)==0x81);session.HiddenPlayers()[2]=0;
    const auto priest=hooks.priest(1);CHECK(priest && priest==session.FindPriestParticipation(1));CHECK(hooks.priestBusy(priest)==(session.Priests().PriestState().GroundImmobile(Sid{static_cast<std::uint16_t>(priest)}) ? 1U : 0U));
    const auto amount=session.Sp(1);session.SetSp(1,1234.5F);CHECK(hooks.money(1)==1234.5F);session.SetSp(1,amount);
    CHECK(hooks.templeCost()==session.Core().types[123].cost); // 같은불변원본TYPE의123비용을사용하고생산품133과혼동하지않는다.
    const auto logged=hooks.loggedIn();Put(records,8*180+0x2c,808);Put(records,8*180+0x50,3);CHECK(hooks.loggedIn()==logged+1);Put(records,8*180+0x2c,0);CHECK(hooks.loggedIn()==logged);
    hooks.refreshPriest(1);CHECK(session.Priests().Priests().ownerState[1]==0); // 같은원본사제목록의갱신결과를직접읽는다.
}
