// 상단메뉴의준비/추적주소/현재게임조회가같은실제Session을사용하는지검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/RawGameMenus.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientGameplayGlobals.h"
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
    session.MissionStartSettings().DenyAscend()=-17;CHECK(hooks.global(0x59507c)==0xffffffefU);session.EnergySettings().userCheat=0x88;CHECK(hooks.global(0x5cab30)==0x88); // 다른현재게임소유자의상태를UI사본으로덮지않는다.
    CHECK(hooks.player(2).data()==records.data()+2*180 && hooks.player(2).size()==180);Put(records,2*180+0x2c,0x87654321U);CHECK(Get(hooks.player(2),0x2c)==0x87654321U);
    const auto battle=session.Core().ownerMode.battle;session.Core().ownerMode.battle=false;CHECK(hooks.global(0x594fbc)==0);session.Core().ownerMode.battle=true;CHECK(hooks.global(0x594fbc)==1);session.Core().ownerMode.battle=battle;
    session.HiddenPlayers()[2]=0x81;CHECK(hooks.global(0x52f820)==0x81);session.HiddenPlayers()[2]=0;
    const auto priest=hooks.priest(1);CHECK(priest && priest==session.FindPriestParticipation(1));CHECK(hooks.priestBusy(priest)==(session.Priests().PriestState().GroundImmobile(Sid{static_cast<std::uint16_t>(priest)}) ? 1U : 0U));
    const auto amount=session.Sp(1);session.SetSp(1,1234.5F);CHECK(hooks.money(1)==1234.5F);session.SetSp(1,amount);
    CHECK(hooks.templeCost()==session.Core().types[123].cost); // 같은불변원본TYPE의123비용을사용하고생산품133과혼동하지않는다.
    const auto logged=hooks.loggedIn();Put(records,8*180+0x2c,808);Put(records,8*180+0x50,3);CHECK(hooks.loggedIn()==logged+1);Put(records,8*180+0x2c,0);CHECK(hooks.loggedIn()==logged);
    hooks.refreshPriest(1);CHECK(session.Priests().Priests().OwnerState()[1]==0); // 같은원본사제목록의갱신결과를직접읽는다.
}
// 실제 메뉴 소비자가 같은 보호 프레임/현재 모드/장기 난수에 연결되고 공간 등록을 재생성하지 않는지 확인한다.
TEST_CASE(CampaignGameMenus_EditorBridgeQualityUsesLiveAssetsAndSharedRandom) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);GameMenuSessionServices devices;
    auto services=devices.Make();auto random=std::make_shared<ClientRandomGlobals>();auto globals=std::make_shared<ClientGameplayGlobals>();
    services.randomGlobals=random;services.gameplayGlobals=globals;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));
    CHECK(Throws([&] { session.SetAllBridgeQuality(2); }));session.Load();const auto core=session.Core();const auto bridge=core.bridgeState.bridgeType;
    const auto added=core.factory.Create(bridge);CHECK(core.pool.Slot(added)[10]==bridge); // 명시 새 void 다리도 실제 보호 TYPE/생성자/프레임을 쓴다.
    const auto before=std::vector<std::uint8_t>(core.pool.Bytes().begin(),core.pool.Bytes().end());const auto freeCount=core.pool.FreeCount();const auto* physical=core.pool.Bytes().data();
    std::array<std::uint32_t,4> hashBefore{};
    // 같은 실제 공간 해시의 네 단계 전체를 저장하며 명령 뒤 재등록이 없음을 확인한다.
    for (std::size_t level=0;level<hashBefore.size();++level) hashBefore[level]=netstorm::test::dudespawn::WordAdler<std::uint16_t>(core.hash.Entries(static_cast<int>(level)));
    const auto spotsBefore=Adler(core.spots);random->Game().SetState(0x12345678U);globals->world.editor=0;globals->world.missionPost=false;
    session.SetAllBridgeQuality(2);CHECK(std::equal(before.begin(),before.end(),core.pool.Bytes().begin()));CHECK(random->Game().State()==0x12345678U);
    std::uint32_t hardChoices=0;std::vector<Sid> bridges;
    // 실제 iterator와 같은 대상에서 현재 보호 프레임에 Hard 후보가 존재하는지만 센다.
    for (std::uint32_t index=2;index<core.pool.Capacity();++index) {
        const Sid sid{static_cast<std::uint16_t>(index)};const auto raw=core.pool.Slot(sid);
        if ((raw[11]&1U) || raw[10]!=bridge) continue;bridges.push_back(sid);
        const auto codes=core.frames[raw[10]].Codes();const auto current=Get(raw,36);CHECK(current<codes.size());
        const auto side=codes[current].side;
        if (std::any_of(codes.begin(),codes.end(),[side](const FrameCode& code) { return code.side==side && code.variant=='P' && (code.flags&0x40U); })) ++hardChoices;
    }
    CHECK(!bridges.empty() && hardChoices>0);GameRandom expected(0x12345678U);
    // Next의 limit과 무관한 상태 전진 횟수만 독립 예상하며 실제 선택 프레임은 보호 표의 속성으로 확인한다.
    for (std::uint32_t index=0;index<hardChoices;++index) static_cast<void>(expected.Next(1));
    globals->world.editor=1;session.SetAllBridgeQuality(2);CHECK(random->Game().State()==expected.State());
    // 모든 현재 다리의 수명 비트는 지워지고 선택 프레임도 같은 보호 표 안에 남아야 한다.
    for (const auto sid:bridges) {
        const auto raw=core.pool.Slot(sid);CHECK((Get(raw,12,2)&0x78U)==0);CHECK(Get(raw,36)<core.frames[raw[10]].Codes().size());
    }
    globals->world.editor=0;globals->world.missionPost=true;session.SetAllBridgeQuality(0);CHECK(random->Game().State()==expected.State());
    // missionPost 단독 관문도 같은 실제 소비자로 이어지고 Cracked는 게임 난수를 소비하지 않는다.
    for (const auto sid:bridges) CHECK((Get(core.pool.Slot(sid),12,2)&0x78U)==0x20U);
    session.SetAllBridgeQuality(1);CHECK(random->Game().State()==expected.State());
    // 원문의 직접 프레임 변경은 같은 풀/자유 목록/해시/spot을 그대로 보존한다.
    CHECK(core.pool.Bytes().data()==physical && core.pool.FreeCount()==freeCount && Adler(core.spots)==spotsBefore);
    // 직접 DWORD 쓰기의 공간 재등록 부재를 각 실제 해시 단계에서 확인한다.
    for (std::size_t level=0;level<hashBefore.size();++level) CHECK(hashBefore[level]==netstorm::test::dudespawn::WordAdler<std::uint16_t>(core.hash.Entries(static_cast<int>(level))));
}
