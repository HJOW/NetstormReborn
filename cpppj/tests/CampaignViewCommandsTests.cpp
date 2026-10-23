// 실제 GameWorld/Session의 Home·명시 SID/owner·Carrier Kernel 예약과 Reload 현재 목록을 검증한다.
#include "DudeSpawnSupport.h"
#include "client/GameWorld.h"
#include "o/RawFortObjectLoad.h"
#include <stdexcept>

using namespace netstorm::client;
namespace {
// 보호 미션 설치에 필요한 외부 음향/번역만 제공하며 카메라·Player·사제·Regular는 실제 본체를 사용한다.
struct CameraCommandServices {
    SoundList names{OriginalEdition::Patch1078};std::array<std::uint8_t,4> you{'Y','o','u',0}; // 장기 소리 이름표와 명시 번역 입력이다.
    // 별도 창/장치 없이 같은 원본 Session을 설치하는 실제 외부 효과 계약이다.
    RawCampaignSessionServices Make() {
        names.Initialize();RawCampaignSessionServices result;result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("View 검사에서 미선택 삭제 효과"); };result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
// 카메라만 읽는 두 raw 자산을 같은 실제 풀에 설치하며 가상 생성/Pop 동작을 대신하지 않는다.
Sid CameraCommandTarget(RawCampaignSession& session,float x,float y) {
    const auto core=session.Core();const auto sid=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));return sid;
}
}
// Home은 Player+7c 우선/+84 fallback이며 dead 첫 대상이면 다른 후보를 찾지 않는다.
TEST_CASE(CampaignViewCommands_GameWorldCurrentHomeAndExplicitSid) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices services;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",services.Make(),{640,480});auto& session=world.Session();const auto core=session.Core();auto types=session.World().MutableTypes(); // 실제 GameWorld가 같은 Session을 소유한다.
    types[70].footX=3;types[70].footY=2;types[70].flags2=0;const auto first=CameraCommandTarget(session,10,20),second=CameraCommandTarget(session,30,40);auto players=session.PlayerRecords();const auto offset=session.LocalOwner()*180U;Put(players,offset+0x7c,first.value);Put(players,offset+0x84,second.value);
    world.Home(false);CHECK(world.ToScreen(0,0).x==208 && world.ToScreen(0,0).y==42);core.pool.AllocatedBytes(first)[11]=2;world.Home(false);CHECK(world.ToScreen(0,0).x==208 && world.ToScreen(0,0).y==42); // 비0 dead SID는 +84로 재선택하지 않는다.
    Put(players,offset+0x7c,0);world.Home(false);CHECK(world.ToScreen(0,0).x==-112 && world.ToScreen(0,0).y==-178);Put(players,offset+0x84,0);world.Home(false);CHECK(world.ToScreen(0,0).x==-112 && world.ToScreen(0,0).y==-178);
    core.pool.AllocatedBytes(first)[11]=0;world.Resize(800,600);world.CenterOnObject(first.value);CHECK(world.ToScreen(0,0).x==288 && world.ToScreen(0,0).y==102); // 현재 장치 크기와 전체 foot을 쓰고 테두리 clamp를 추가하지 않는다.
    bool rejected{};try { world.CenterOnObject(0x10000U+first.value); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);world.CenterOnObject(0);CHECK(world.ToScreen(0,0).x==288 && world.ToScreen(0,0).y==102);
}
// 실제4950d0 참여 조회·491 Regular·원본005c89b8을 같은 Session에서 사용한다.
TEST_CASE(CampaignViewCommands_ActualPriestPreviewAndResetDepth) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices services;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",services.Make(),{640,480});auto& session=world.Session();const auto core=session.Core();const auto found=session.FindPriestParticipation(session.LocalOwner());CHECK(found!=0);if (!found) return;const Sid priest{static_cast<std::uint16_t>(found)};auto raw=core.pool.AllocatedBytes(priest);auto types=session.World().MutableTypes(); // 같은 실제 살아 있는 사제 목록을 조회한다.
    types[raw[10]].footX=3;types[raw[10]].footY=2;types[raw[10]].flags2|=0x210000;Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,18,std::bit_cast<std::uint32_t>(30.0F));raw[40]=0;
    session.World().SetLoadingDepth(4);CameraHomeState state;state.suppressionDepth=99;const auto initialCount=core.kernel.Size();std::uint32_t applied{};
    session.ResetCamera(state,[&](auto,auto) { ++applied;CHECK(session.FortObjectSettings().loadingDepth==5 && state.suppressionDepth==5); });CHECK(applied==1 && state.suppressionDepth==4 && session.FortObjectSettings().loadingDepth==4 && core.kernel.Size()==initialCount);
    world.Home(true);CHECK(world.ToScreen(0,0).x==48 && world.ToScreen(0,0).y==-68 && core.kernel.Size()==initialCount); // 깊이4에서는 실제 preview가 생성하지 않는다.
    session.World().SetLoadingDepth(0);world.ViewPriest(session.LocalOwner());const auto* preview=core.processes.FindEvent(priest,0x491);CHECK(preview && std::bit_cast<std::uint32_t>(preview->Payload())==0x3e4ccccd && core.kernel.Size()==initialCount+1);
    world.CenterCamera(100,100);const auto previous=world.ToScreen(0,0);CHECK(session.FindPriestParticipation(257)==0);world.ViewPriest(257);CHECK(world.ToScreen(0,0).x==previous.x && world.ToScreen(0,0).y==previous.y); // 명시 owner257을 로컬 owner로 바꾸지 않는다.
    world.Home(true);CHECK(world.ToScreen(0,0).x==48 && world.ToScreen(0,0).y==-68);
}
// Reload 목록은 사본이 아니라 현재 signed count와 현재 DWORD 항목을 재조회한다.
TEST_CASE(CampaignViewCommands_ReloadCurrentListsAndModeContract) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices services;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",services.Make());session.Load();const auto core=session.Core();auto& factories=core.bookkeeping.factories;const auto saved=factories;const auto firstCount=session.GameMenuReloadCount(false); // first는 별도 outposts이며 factories 변경과 독립이다.
    factories.entries={100U,200U,300U};factories.count=2;CHECK(session.GameMenuReloadCount(true)==2 && session.GameMenuReloadSid(true,0)==100 && session.GameMenuReloadSid(true,1)==200 && session.GameMenuReloadCount(false)==firstCount);
    factories.entries[0]=400;factories.count=1;CHECK(session.GameMenuReloadCount(true)==1 && session.GameMenuReloadSid(true,0)==400);factories.count=0xffffffffU;CHECK(session.GameMenuReloadCount(true)==-1);factories=saved;
    bool rejected{};try { session.ReloadTheamAt(std::bit_cast<std::uint32_t>(10.0F),std::bit_cast<std::uint32_t>(20.0F),1); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected); // 이 명령 어댑터는 원본 caller의 mode0만 수용한다.
}
