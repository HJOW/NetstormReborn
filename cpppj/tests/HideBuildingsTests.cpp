// 같은0059a8ac가실제캠페인표시와선택필터에서매번읽히는지검사한다.
#include "DudeSpawnSupport.h"
#include "client/GameWorld.h"
#include <stdexcept>
using namespace netstorm::client;
namespace {
// 실제현재게임설치에필요한외부장치만명시하며그림/선택/팔레트는기존모듈을쓴다.
struct HideBuildingsServices {
    SoundList names{OriginalEdition::Patch1078}; // 같은SoundList의안정된저장소다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 영어번역의명시입력이다.
    // 보호미션의실제로더가선택하는외부장치경계를공급한다.
    RawCampaignSessionServices Make() {
        names.Initialize();RawCampaignSessionServices result;result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[] { return 100.0; };result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("건물숨김검사에서삭제효과미선택"); };result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
}
// 실제RainTemple의normal/숨김/복원픽셀과같은현재SID의후보제외를함께확인한다.
TEST_CASE(HideBuildings_CurrentGetterChangesPixelsAndPick) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);HideBuildingsServices effects;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",effects.Make(),{1024,768});world.Resize(1024,768);
    std::uint32_t hide{},reads{};world.BindHideBuildings([&] { ++reads;return hide; }); // 메뉴와같은현재DWORD를한번만바인딩한다.
    constexpr std::uint32_t temple=15002; // 보호thewarbegins의실제RainTemple SID다.
    const auto* object=world.Object(temple);CHECK(object!=nullptr);if (!object) return;
    const auto p=world.ToScreen(object->x,object->y);const ScreenPoint hit{p.x-112,p.y-80}; // 기존캠페인검사와같은실제본체내부점이다.
    CHECK(world.Pick(hit)==temple && reads>0);
    const auto draw=[&] {
        std::vector<std::uint8_t> result(1024*768,255);std::size_t count{}; // 실제공통Draw만출력할명시대상표면이다.
        for (const auto& sprite:world.Sprites()) if (sprite.sourceSid==temple && !sprite.shadow && sprite.procedure) { // 같은신원의본체와그림자복제를구별한다.
            sprite.procedure->draw(result,1024,768,{0,0,1024,768});++count;
        }
        CHECK(count==1);return result;
    };
    const auto normal=draw();CHECK(std::any_of(normal.begin(),normal.end(),[](auto value) { return value!=255; }));
    hide=0x80000000U;const auto before=reads;CHECK(world.Pick(hit)!=temple && reads>before);const auto hidden=draw();CHECK(hidden!=normal);
    auto raw=world.Session().Core().pool.AllocatedBytes(Sid{temple});const auto originalType=raw[10]; // 캐시된표시TYPE와다르게한현재물리슬롯입력이다.
    for (const auto type:{84U,164U}) { raw[10]=static_cast<std::uint8_t>(type);CHECK(world.Pick(hit)!=temple); } // 원문의두예외TYPE도현재슬롯에서제외한다.
    raw[10]=originalType;
    hide=0;CHECK(world.Pick(hit)==temple);CHECK(draw()==normal);
    bool missing{};try { world.BindHideBuildings({}); } catch (const std::invalid_argument&) { missing=true; }CHECK(missing);
}
