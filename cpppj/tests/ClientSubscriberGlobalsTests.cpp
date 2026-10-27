// 실제 Client/Session의 두 Subscriber alias를 검사한다. Run/창/음향 시작을 호출하지 않는다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/RawCampaignSession.h"
#include "o/RawChatManagerPrefix.h"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {
using namespace netstorm::client;
// 기존 실제 Client 생성자 검사처럼 보호 자산 읽기만 허용한다.
ClientOptions SubscriberClientOptions() {
    ClientOptions options; // 경로는 공용 tests에 복사한 뒤의 실제 저장소 기준이다.
    options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";return options;
}
// Session constructor의 도달 효과만 기존 실제 몸체에 제공한다. Subscriber 알고리즘을 대체하지 않는다.
struct SubscriberSessionDevices {
    SoundList names{OriginalEdition::Patch1078}; // Session보다 오래 사는 실제 소리 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 번역 효과의 NUL 포함 명시 입력이다.
    RawCampaignSessionServices Make(std::shared_ptr<ClientSubscriberGlobals> owner={}) {
        names.Initialize();RawCampaignSessionServices result;result.subscriberGlobals=std::move(owner);result.soundNames=&names; // 생략시 기존 독립 fallback을 선택한다.
        result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.production.effects.configInt=[](std::string_view key) { if (key!="getTeleportTechFree") throw std::logic_error("Subscriber 검사의 생산 설정");return 0; };
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
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("Subscriber 검사의 미선택 UI"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("Subscriber 검사의 미선택 삭제"); };
        result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
// Login Begin의 첫 현재 설정 조회에서 명시 중단하여 이후 packet의 정상 Subscriber 쓰기와 분리한다.
struct SubscriberBeginCheckpoint {};
}

// 메뉴/월드 부재에서도 실제 Client owner의 같은 constref를 반환하고 새 Client만 PE 초기값을 구성한다.
TEST_CASE(ClientSubscriberGlobals_ActualClientAndChatReferenceLifetime) {
    static_assert(std::is_same_v<decltype(std::declval<const Client&>().CurrentSubscriber()),const std::uint32_t&>); // temporary 값을 Chat constref에 연결하지 않는다.
    std::shared_ptr<ClientSubscriberGlobals> retained;std::weak_ptr<ClientSubscriberGlobals> lifetime; // Client 반납 뒤 실제 참조 보유자의 수명을 확인한다.
    {
        Client client(SubscriberClientOptions());retained=client.SubscriberGlobals();lifetime=retained;
        CHECK(retained && client.CurrentSubscriber()==0xffffffffU && retained->LoginSubscriber()==1);
        CHECK(&client.CurrentSubscriber()==&retained->MySubscriber());
        CHECK(&client.GameplayGlobals()->world.mySubscriber==&retained->MySubscriber() && &client.GameplayGlobals()->world.loginSubscriber==&retained->LoginSubscriber());
        std::uint32_t manager{},active{};netstorm::o::ChatManagerPrefixGlobals chat{manager,client.CurrentSubscriber(),active}; // 실제 Chat 전역의 참조형에 직접 연결한다.
        retained->MySubscriber()=0x12345678U;retained->LoginSubscriber()=77;
        CHECK(client.GameplayGlobals()->world.mySubscriber==0x12345678U && client.GameplayGlobals()->world.loginSubscriber==77);
        client.Configuration().Configuration().AppendSection("END");client.Configuration().SetInt("readOnlyForts",1);
        CHECK(&chat.subscriber==&client.CurrentSubscriber() && chat.subscriber==0x12345678U && retained->LoginSubscriber()==77);
    }
    CHECK(!lifetime.expired() && retained->MySubscriber()==0x12345678U && retained->LoginSubscriber()==77);
    {
        Client fresh(SubscriberClientOptions());CHECK(fresh.SubscriberGlobals().get()!=retained.get());
        CHECK(fresh.CurrentSubscriber()==0xffffffffU && fresh.SubscriberGlobals()->LoginSubscriber()==1 && retained->LoginSubscriber()==77);
    }
    retained.reset();CHECK(lifetime.expired()); // 반납한 참조/주소를 이후 역참조하지 않는다.
}

// 실제 Editor/Campaign 두 Session은 같은 두 DWORD만 빌리고 Editor 플래그는 새 Session에 전파하지 않는다.
TEST_CASE(ClientSubscriberGlobals_TwoActualSessionsShareOnlySubscribers) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto image=assets.files.Read("d/thewarbegins.fort"); // 보호 원문은 읽기만 한다.
    Client client(SubscriberClientOptions());auto owner=client.SubscriberGlobals();owner->MySubscriber()=31;owner->LoginSubscriber()=41;
    SubscriberSessionDevices firstDevices,secondDevices,independentDevices; // 모든 callback/소리표는 각 Session보다 먼저 생성한다.
    auto editorServices=firstDevices.Make(owner);editorServices.installMode=CampaignInstallMode::ExistingEditor;editorServices.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(image);
    editorServices.fortBadges=client.FortBadges();editorServices.fortTypeConversion=client.FortTypeConversion(); // 현재 Editor 생성자가 요구하는 실제 Client 장부와 변환표도 같은 수명으로 제공한다.
    RawCampaignSession editor(assets.game,assets.files,FortTemplate::Parse(image,assets.game.TypeTable()),"subscriber-editor",std::move(editorServices));
    RawCampaignSession campaign(assets.game,assets.files,FortTemplate::Parse(image,assets.game.TypeTable()),"subscriber-campaign",secondDevices.Make(owner));
    CHECK(&editor.CurrentSubscriber()==&owner->MySubscriber() && &campaign.CurrentSubscriber()==&client.CurrentSubscriber());
    CHECK(owner->MySubscriber()==31 && owner->LoginSubscriber()==41 && editor.Editor()==1 && campaign.Editor()==0);
    owner->MySubscriber()=0x80000000U;CHECK(editor.CurrentSubscriber()==0x80000000U && campaign.CurrentSubscriber()==0x80000000U);
    RawCampaignSession independent(assets.game,assets.files,FortTemplate::Parse(image,assets.game.TypeTable()),"subscriber-independent",independentDevices.Make());
    CHECK(independent.CurrentSubscriber()==0 && &independent.CurrentSubscriber()!=&owner->MySubscriber() && owner->LoginSubscriber()==41);
    CHECK(assets.files.Read("d/thewarbegins.fort")==image); // 선택한 보호 입력의 재읽기이며 전체1394manifest 검사는 아니다.
}

// bound Login은 Init 값을 재복사하지 않으며 기존 독립 장치는 legacy missionSubscriber 입력을 그대로 사용한다.
TEST_CASE(ClientSubscriberGlobals_LoginBeginPreservesBoundCurrentValues) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto image=assets.files.Read("d/thewarbegins.fort");
    Client client(SubscriberClientOptions());auto owner=client.SubscriberGlobals();owner->MySubscriber()=51;owner->LoginSubscriber()=61;
    for (const bool bound:{true,false}) { // 같은 checkpoint에서 명시 owner 유무만 달리하여 기존 fallback 계약과 비교한다.
        SubscriberSessionDevices devices;auto services=devices.Make(bound?owner:std::shared_ptr<ClientSubscriberGlobals>{});services.missionSubscriber=7;CampaignLoginServices login;std::size_t reached{}; // bound에서 이 값사본은 재복사되지 않아야 한다.
        login.configInt=[&](std::string_view key,std::int32_t)->std::int32_t { CHECK(key=="gameServerPort");++reached;throw SubscriberBeginCheckpoint{}; };
        services.localLogin=std::move(login);RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(image,assets.game.TypeTable()),"subscriber-login",std::move(services));bool stopped{};
        try { session.LoadLocalConnection(); } catch (const SubscriberBeginCheckpoint&) { stopped=true; }
        CHECK(stopped && reached==1 && session.CurrentSubscriber()==(bound?51U:7U));
        CHECK(owner->MySubscriber()==51 && owner->LoginSubscriber()==61); // packet 설치 전 명시 중단의 범위다. 전체 로그인 성공을 주장하지 않는다.
    }
}
