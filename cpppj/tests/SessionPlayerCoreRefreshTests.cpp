#include "client/GameWorld.h"
#include <bit>
// 실제 LocalLogin Core alias와 같은 TYPE/다이스를 사용한 현재 Core 갱신을 검사한다.
#include "DudeSpawnSupport.h"
#include "CameraTestDevices.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h" // 실제 커서의 Placement 멤버를 조회하는 검사에 완전한 선언을 제공한다.
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "o/RawFortTerritorySave.h"
#include "client/GameWorld.h"
#include "client/ClientUserSelectionCleanup.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace netstorm::o;
using CameraViewWords=std::array<std::uint32_t,16>; // padding 없는 실제16DWORD 복사의 검사 관측형이다.
using namespace netstorm::client;
// 기존 ThemeSessionServices/LocalLogin 검사의 외부 소리·번역·설정만 공급한다.
struct CoreRefreshSessionDevices {
    SoundList names{OriginalEdition::Patch1078}; // 실제 Session보다 오래 사는 소리 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 명시 headless 번역 입력이다.
    // Player/기술/LocalLogin/다이스/현재Core는 실제 Session 몸체에 맡긴다.
    RawCampaignSessionServices Make(bool login) {
        names.Initialize();RawCampaignSessionServices result;result.cameraCommit=netstorm::test::camera::CommitScreenless; // 원문497960 본체와 명시 비GUI 장치를 사용한다.
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

// 실제 지도 로더가 Client의 섬/청크에 쓰고 같은 Session의 count와 Territory snapshot이 현재 공유 물리를 읽는다.
TEST_CASE(SessionIslandChunkStorage_ActualLoadAndSnapshotUseSameCurrentRecords) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CoreRefreshSessionDevices devices;auto surface=MakeRawSurfaceStorage(assets.game);auto templates=std::make_shared<CampaignTemplateStorage>(); // 실제 파일은 읽기만 하고 Session/Template stream은 독립 소유한다.
    auto services=devices.Make(false);services.surfaceStorage=surface;services.templateStorage=templates;const auto* islands=surface->islands.records.data();const auto* chunks=surface->chunks.Entries().data(); // 전체 교체 대신 같은 고정 물리인지 관측한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"bc8fury76",std::move(services));session.Load();
    const auto count=session.IslandCount();CHECK(count>0 && count<=100 && CoreRefreshRead(surface->islands.records,0)==count && session.World().Storage()==surface);
    CoreRefreshPut(surface->islands.records,0,count-1);CHECK(session.IslandCount()==count-1);CoreRefreshPut(surface->islands.records,0,count); // 현재 consumer가 constructor 때의 사본을 읽는지 구별한 뒤 실제 지도 count로 되돌린다.
    const auto slot=session.TemplateOwner();const auto original=templates->Template(slot).Section("Territory");std::int32_t selectedRegion=-1;ChunkEntry* selectedChunk{}; // 실제 현재 지도와 활성 원문 Territory에서만 저장 대상 하나를 찾는다.
    for (std::int32_t region=0;region<20 && !selectedChunk;++region) { // 일반 snapshot은 기존record의 bit1이 있는 현재 지역만 덮어쓴다.
        if (original.size()<120 || !(original[static_cast<std::size_t>(region)*6+1]&1U)) continue;
        const auto island=FindFortTerritoryIsland(surface->islands.records,session.Core().ownerMode.fort,static_cast<std::int32_t>(slot),region);if (island==127) continue; // 실제 owner/존재 관문을 그대로 사용한다.
        for (int y=0;y<surface->chunks.YLength() && !selectedChunk;++y) { // 실제 writer와 같은 y 바깥/x 안쪽 조회 순서다.
            for (int x=0;x<surface->chunks.XLength();++x) { // 첫 실제 일치 record만 현재 입력으로 바꾼다.
                auto& entry=surface->chunks.At(x,y);if (entry.island==island) { selectedRegion=region;selectedChunk=&entry;break; }
            }
        }
    }
    CHECK(selectedChunk && selectedRegion>=0);if (!selectedChunk) throw std::logic_error("공유 섬/청크 검사의 실제 저장 대상 없음");
    selectedChunk->record[4]=0x4d;selectedChunk->record[5]=0x72;session.SnapshotFortTerritory(slot);const auto saved=templates->Template(slot).Section("Territory"); // 전체 파일 저장이나 임의 좌표 생성 없이 같은 실제 snapshot reader를 실행한다.
    CHECK(saved.size()==120 && saved[static_cast<std::size_t>(selectedRegion)*6+4]==0x4d && saved[static_cast<std::size_t>(selectedRegion)*6+5]==0x72);
    CHECK(surface->islands.records.data()==islands && surface->chunks.Entries().data()==chunks); // loaded 이후 reader도 최초 배열/지도 물리를 유지한다.
}

// 실제 GameWorld/Session/입력이 같은9DWORD를 읽고 실제 객체의+7c 선택 비트도 공통 adapter에서 지운다.
TEST_CASE(GameWorldSelectionGlobals_ActualWorldInputAndCleanupShareCurrentOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CoreRefreshSessionDevices devices;auto gameplay=std::make_shared<ClientGameplayGlobals>();auto surface=MakeRawSurfaceStorage(assets.game,gameplay->Authority(),gameplay->GameAux());auto services=devices.Make(false);services.gameplayGlobals=gameplay;services.surfaceStorage=surface;
    auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable());std::uint32_t cursorCalls{}; // 실제 장치 선택만 외부에서 관찰한다.
    {
        GameWorld world(assets.game,assets.files,std::move(fort),{},"bc8fury76",std::move(services),{640,480});CHECK(&world.SelectionGlobals()==&gameplay->selection);
        UserSelectionHooks selection;selection.paused=[] { return 0U; };UserHighlightHooks highlight;highlight.selectCursor=[&](auto cursor) { CHECK(cursor==1);++cursorCalls; };world.BindUserInput(std::move(selection),std::move(highlight),{});
        auto& owner=gameplay->selection;owner.selected=0xfedcba98U;owner.mode=2;owner.mouseType=71;owner.multipleCount=-3;owner.remembered={17,19};world.SetMousePoint({-33,65});
        CHECK(world.Selected()==0xfedcba98U && world.InputMode()==2 && owner.mouse.x==-33 && &world.HighlightSettings().selectedSid==&owner.selected);
        world.ClearSelection();CHECK(owner.selected==0 && owner.mode==0 && owner.mouseType==0 && owner.multipleCount==0 && cursorCalls==1);
        const auto sid=world.Session().World().Create(94);auto raw=world.Session().Core().pool.AllocatedBytes(sid);raw[40]|=4;owner.multiple={sid.value};owner.multipleCount=1;owner.selected=sid.value;owner.mode=2;owner.mouseType=71;
        ClearClientUserSelection(*gameplay,*surface,[&](auto cursor) { CHECK(cursor==1);++cursorCalls; });CHECK(!(raw[40]&4) && owner.multipleCount==0 && owner.selected==0 && owner.mode==0 && owner.mouseType==0 && cursorCalls==2);
        owner.mouseType=0x89abcdefU;world.ClearSelection();CHECK(owner.mouseType==0x89abcdefU && owner.multiple[0]==sid.value); // 동기 adapter가 기존 입력/activeSelection 수명을 교체하지 않는다.
    }
    CHECK(!gameplay->Borrowed() && gameplay->selection.mouseType==0x89abcdefU && gameplay->selection.mouse.x==-33 && gameplay->selection.remembered[1]==19 && !gameplay->selection.multiple.empty()); // Runtime 반납 뒤에도 선택 전역의 현재 값은 남는다.
    ClearClientUserSelection(*gameplay,*surface);CHECK(gameplay->selection.mouseType==0x89abcdefU && gameplay->selection.multipleCount==0); // 같은mode0의 후속 빈 경로는 Session을 다시 요구하지 않는다.
    CoreRefreshSessionDevices independentDevices;auto independent=independentDevices.Make(false);independent.surfaceStorage=surface; // Gameplay를 생략한 독립 호출자는 기존 Storage의 같은 Authority만 채택한다.
    auto nextFort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable());
    { GameWorld next(assets.game,assets.files,std::move(nextFort),{},"bc8fury76",std::move(independent),{640,480});CHECK(&next.SelectionGlobals()!=&gameplay->selection && next.Selected()==0 && &next.Session().Core().processState.Server()==&surface->Authority()->server); } // 새 선택 owner와 공유 권한 owner를 구별한다.
}

// 실제 raw GameWorld의 투영/미니맵/viewport가 Client의 현재 분수 카메라를 픽셀에서 재구성하지 않는다.
TEST_CASE(CameraView_ActualWorldProjectionAndResizePreserveFraction) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CoreRefreshSessionDevices devices;auto gameplay=std::make_shared<ClientGameplayGlobals>();auto services=devices.Make(false);services.gameplayGlobals=gameplay;auto& camera=gameplay->Camera();std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(640,480,256,redraw,revision);
    {
        auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"bc8fury76",std::move(services),{640,480});CHECK(&world.Session().CameraGlobals()==&camera);SetCameraViewWorld(camera.RendererView(),1.25F,2.5F);camera.CopyRendererSnapshot();const auto before=std::bit_cast<CameraViewWords>(camera.Snapshot());const auto screen=world.ToScreen(-0.1,1.5);CHECK(screen.x==-21 && screen.y==-10);const auto cell=world.ToCell({0,0});CHECK(cell.x==2 && cell.y==3);CHECK(world.MinimapView().camera[0]==0x3fa00000U && world.MinimapView().camera[1]==0x40200000U);
        world.Resize(640,480);world.Resize(800,600);CHECK(std::bit_cast<CameraViewWords>(camera.Snapshot())==before && camera.Target().limits[0]==3296 && camera.Target().limits[1]==2216);SetCameraViewWorld(camera.Snapshot(),2.25F,3.5F);const auto current=world.ToScreen(1.0,1.0);CHECK(current.x==-20 && current.y==-27 && world.MinimapView().camera[0]==0x40100000U);CHECK(world.Session().Cursor().Placement().ScreenOriginX()==36);
    }
    CHECK(!gameplay->Borrowed() && camera.Snapshot().worldBits[0]==0x40100000U && camera.Snapshot().worldBits[1]==0x40600000U);
}
