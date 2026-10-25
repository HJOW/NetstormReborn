// 실제 LocalLogin Core alias와 같은 TYPE/다이스를 사용한 현재 Core 갱신을 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace netstorm::o;
using namespace netstorm::client;
// 기존 ThemeSessionServices/LocalLogin 검사의 외부 소리·번역·설정만 공급한다.
struct CoreRefreshSessionDevices {
    SoundList names{OriginalEdition::Patch1078}; // 실제 Session보다 오래 사는 소리 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 명시 headless 번역 입력이다.
    // Player/기술/LocalLogin/다이스/현재Core는 실제 Session 몸체에 맡긴다.
    RawCampaignSessionServices Make(bool login) {
        names.Initialize();RawCampaignSessionServices result; // 게임 계산을 반환하는 가짜 hook은 넣지 않는다.
        result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current?current:sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("Core갱신검사의미설치UI대상"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("Core갱신검사의미선택삭제"); };
        result.player.log=[](std::uint32_t,std::string_view) {};result.missionOutcome.demo=[] { return 0U; };
        if (login) {
            CampaignLoginServices effects; // 실제 로컬 handshake의 장치 경계만 제공한다.
            effects.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };effects.wallSeconds=[] { return 100.25; };effects.clock=[] { return 100250U; };
            effects.refresh=[] {};effects.playerUiBusy=[] { return false; };effects.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};effects.log=[](std::string_view) {};
            result.localLogin=std::move(effects);result.ai.participation.clearChat=[](std::uint32_t) {};result.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
        }
        return result;
    }
};
// 실제 Player/Core/다이스 필드폭으로만 명시 입력을 쓴다.
void CoreRefreshPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 정확한DWORD 입력이다.
}
// 같은 실제 저장소의 DWORD를 읽는다.
std::uint32_t CoreRefreshRead(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value{}; // 낮은 BYTE부터 조립할 원시 비트다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8); // 인접Core필드를 읽지 않는다.
    return value;
}
}

// 실제 handshake가 선택한 Player+6c를 갱신하며 별도 Core 복사/Clientalias를 만들지 않는다.
TEST_CASE(SessionPlayerCoreRefresh_CurrentLoginAliasAndActualDaisCount) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CoreRefreshSessionDevices devices; // Session보다 오래 사는 읽기전용 자료/장치다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(true));
    session.LoadLocalConnection();CHECK(session.LocalConnectionPhase()==14);session.ClearTechnology(1);session.AddTechnology(1,"windBattery;sunAviary");
    const auto world=session.Core();auto records=session.PlayerRecords();auto current=records.subspan(session.LocalOwner()*180+0x6c,10); // 실제 Login이 같은 Player기록을 선택한 alias다.
    current[5]=0;CoreRefreshPut(current,6,0xffffffffU);
    const auto dais=session.World().Create(165,2);auto raw=world.pool.AllocatedBytes(dais);raw[34]=static_cast<std::uint8_t>(session.LocalOwner());CoreRefreshPut(raw,36,120); // 실제 다이스ctor 뒤 count3을 나타내는 명시현재frame이다.
    const auto owner=session.LocalOwner(),selected=session.TemplateOwner();session.RefreshCurrentPlayerCore();
    CHECK(CoreRefreshRead(current,6)==0x11U && current[5]==2); // Native실제속성 windBattery bit4/sunAviary bit0와다이스count3−1이다.
    CHECK(session.LocalOwner()==owner && session.TemplateOwner()==selected && session.LocalConnectionPhase()==14);
    CoreRefreshPut(raw,36,3);session.RefreshCurrentPlayerCore();CHECK(CoreRefreshRead(current,6)==0x11U && current[5]==2); // 현재다이스count가내려가도기존Core+5하한은줄이지않는다.
}

// 같은 실제 Core 선택이 없으면 첫 store 전에 멈추며 Login 없는 경로도 명시 선택 뒤에는 같은 부모를 사용한다.
TEST_CASE(SessionPlayerCoreRefresh_RequiresActualSelectionAndSupportsSharedCoreWithoutLogin) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CoreRefreshSessionDevices devices; // 실제 장치 수명을 보존한다.
    auto gameplay=std::make_shared<ClientGameplayGlobals>();auto selected=std::make_shared<ClientCoreDataGlobals>();auto players=std::make_shared<ClientPlayerRecords>();players->BindCoreData(selected);auto services=devices.Make(false);
    services.gameplayGlobals=gameplay;services.coreDataGlobals=selected;services.playerRecords=players; // 실제 Login 객체 없이 같은 장기 슬롯과 Native Player 초기화/선택 장치를 빌린다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"bc8fury76",std::move(services));session.Load();
    const auto owner=session.LocalOwner(),templateOwner=session.TemplateOwner();const std::uint32_t metamap=0;selected->Select(gameplay->player,metamap,0,1);CHECK(selected->Current().empty()); // 실제 선택 해제 뒤에만 첫 store의 물리 거부를 검사한다.
    const auto before=std::vector<std::uint8_t>(session.PlayerRecords().begin(),session.PlayerRecords().end());bool rejected{}; // Core+6을쓰기전선택필수실패여야한다.
    try { session.RefreshCurrentPlayerCore(); }catch (const std::out_of_range& error) { rejected=std::string(error.what()).find("현재Core갱신의실제store물리범위")!=std::string::npos; }
    CHECK(rejected && std::equal(before.begin(),before.end(),session.PlayerRecords().begin()));
    selected->Select(gameplay->player,metamap,owner,templateOwner);session.ClearTechnology(owner);session.AddTechnology(owner,"windBattery;sunAviary");auto current=selected->Current();current[5]=0;CoreRefreshPut(current,6,0xffffffffU);
    const auto dais=session.World().Create(165,2);auto raw=session.Core().pool.AllocatedBytes(dais);raw[34]=static_cast<std::uint8_t>(owner);CoreRefreshPut(raw,36,120);session.RefreshCurrentPlayerCore();
    CHECK(session.PlayerCoreGlobals()==selected && current.data()==session.PlayerRecords().data()+owner*180+0x6c && CoreRefreshRead(current,6)==0x11U && current[5]==2 && session.LocalConnectionPhase()==0); // 실제 기술/다이스 결과와 현재 슬롯을 검사하며 Login 유무를 성공 관문으로 삼지 않는다.
}

// 미복원가상80은기술bit완료뒤선택위치에서중단하고앞선직접효과를되돌리지않는다.
TEST_CASE(SessionPlayerCoreRefresh_UnknownSelectedVirtualKeepsTechnologyPrefix) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CoreRefreshSessionDevices devices; // 같은실제Session/Player/기술장치를사용한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(true));session.LoadLocalConnection();
    session.ClearTechnology(1);session.AddTechnology(1,"windBattery;sunAviary");auto records=session.PlayerRecords();auto current=records.subspan(session.LocalOwner()*180+0x6c,10);current[5]=0;CoreRefreshPut(current,6,0xffffffffU);
    const auto world=session.Core();const auto dais=session.World().Create(165,2);auto raw=world.pool.AllocatedBytes(dais);raw[34]=static_cast<std::uint8_t>(session.LocalOwner());CoreRefreshPut(raw,0,0xdeadbeefU);bool rejected{}; // 물리TYPE/owner는맞고선택VT만미복원인명시입력이다.
    try { session.RefreshCurrentPlayerCore(); }catch (const std::logic_error& error) { rejected=std::string(error.what()).find("가상80몸체")!=std::string::npos; }
    CoreRefreshPut(raw,0,0x507950U); // 같은실제생성VT로복구한뒤Session을소멸한다.원문중단효과를rollback한것으로합산하지않는다.
    CHECK(rejected && CoreRefreshRead(current,6)==0x11U && current[5]==0);
}
