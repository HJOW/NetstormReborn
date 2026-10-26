// 실제 로컬 미션 시작이 Tell/Help/HUD와 같은 배경80BYTE를 사용하는지 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientHtmlWindowGlobals.h"
#include "client/ClientMain.h"
#include "client/HtmlTextParser.h"
#include <algorithm>
#include <array>
#include <memory>
#include <functional>
#include <vector>

namespace { //실제 게임 계산과 구분된 외부 장치/관측만 공급한다.
using namespace netstorm::client;
using namespace netstorm::o;
struct BackgroundStateDevices {
    SoundList names{OriginalEdition::Patch1078}; // 모든 Session보다 오래 사는 실제 소리 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // GUI 없는 명시 번역 입력이다.
    double wall{100.25}; // 실제 wall getter가 매번 읽는 외부 시각 입력이다.
    std::vector<std::string> logs; // 실제 metadata/Login이 선택한 로그 경계만 관측한다.
    std::vector<std::uint32_t> pickupSounds; // 실제 Carrier 내용물 통지의 TYPE+7c 음향 호출만 관측한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> observe; // 수신 본문 직전의 원래 sender/패킷을 읽는 검사 관찰기다.
    RawCampaignSessionServices Make(const std::shared_ptr<ClientLocalStateGlobals>& owner,bool login) { // 같은 장기 owner를 실제 Session/Login/내용물 소비자에 제공한다.
        names.Initialize();RawCampaignSessionServices result;result.localStateGlobals=owner;result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[this] { return wall; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("State 공유 검사의 미설치 UI"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("State 공유 검사의 미선택 삭제"); };
        result.player.log=[](std::uint32_t,std::string_view) {};result.missionOutcome.demo=[] { return 0U; };
        result.carrierContent.sound=[this](std::uint32_t sound) { pickupSounds.push_back(sound); };
        if (login) {
            CampaignLoginServices effects; // 실제 로컬 handshake의 장치 경계만 제공하며 응답/상태를 대신 쓰지 않는다.
            effects.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };effects.wallSeconds=[this] { return wall; };effects.clock=[] { return 100250U; };
            effects.refresh=[] {};effects.playerUiBusy=[] { return false; };effects.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};
            effects.log=[this](std::string_view text) { logs.emplace_back(text); };effects.observePacket=[this](auto sender,bool,std::uint32_t,auto packet) { if (observe) observe(sender,packet); };
            result.localLogin=std::move(effects);result.ai.participation.clearChat=[](std::uint32_t) {};result.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
        }
        return result;
    }
};
}

// 첫 패킷 전의 원문 State7 첫BYTE 쓰기가 같은 Client 배경에 도달하며 인접 전역은 보존된다.
TEST_CASE(MissionBackgroundLifetime_LocalStartClearsSharedFirstByteBeforePackets) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));BackgroundStateDevices devices;const auto html=client.HtmlWindowGlobals();
    html->text.background.fill('X');const std::string initial="nsstart";std::copy(initial.begin(),initial.end(),html->text.background.begin());html->text.background[initial.size()]=0;html->text.suppressParagraph=0x98765432U;const auto before=html->text.background;std::uint32_t packets{};
    devices.observe=[&](auto,auto) { ++packets;CHECK(html->text.background[0]==0 && std::equal(before.begin()+1,before.end(),html->text.background.begin()+1) && html->text.suppressParagraph==0x98765432U); };
    auto services=devices.Make(client.LocalStateGlobals(),true);services.htmlWindowGlobals=html;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(html->text.background==before);session.LoadLocalConnection();
    CHECK(packets>0 && session.LocalConnectionPhase()==14 && HtmlBackgroundName(html->text).empty());CHECK(std::equal(before.begin()+1,before.end(),html->text.background.begin()+1) && html->text.suppressParagraph==0x98765432U);
}

// 같은 HTML owner를 여러 정상 시작 입력에 빌려 주어 캐시한 제목 대신 매번 현재 첫BYTE를 초기화한다.
TEST_CASE(MissionBackgroundLifetime_RepeatedCurrentOwnerKeepsTailAndParagraph) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto html=std::make_shared<ClientHtmlWindowGlobals>();BackgroundStateDevices devices;
    for (const std::string name:{"titleMenu","victory","nsstart",""}) { //원문 State7은 모든 현재 배경 이름에서 같은 첫BYTE만 지운다.
        auto state=std::make_shared<ClientLocalStateGlobals>(); //독립 시작 입력이다. 같은 Client 상태의 복귀/타이머 정리는 실제 GUI의 ClearWorld 경로에서 별도로 검증한다.
        html->text.background.fill('Q');std::copy(name.begin(),name.end(),html->text.background.begin());html->text.background[name.size()]=0;html->text.suppressParagraph+=7;const auto before=html->text;
        { auto services=devices.Make(state,true);services.htmlWindowGlobals=html;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.LoadLocalConnection();CHECK(session.LocalConnectionPhase()==14 && HtmlBackgroundName(html->text).empty()); }
        CHECK(html->text.background[0]==0 && std::equal(before.background.begin()+1,before.background.end(),html->text.background.begin()+1) && html->text.suppressParagraph==before.suppressParagraph);
    }
}
