// 창 없는 실제 Client/Session의 공통 삭제가 쓴 손실 좌표를 같은 카메라 owner까지 전달한다.
#include "DudeSpawnSupport.h"
#include "CameraTestDevices.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/LostPositionKey.h"
#include "client/RawCampaignSession.h"
#include "o/RawFortObjectLoad.h" // 실제 현재 로딩 깊이를 바꾸지 않고 관측한다.
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace { // 게임 규칙은 실제 Session에 맡기고 장치/번역의 명시 입력만 공급한다.
struct LostPositionServices { // 음향 장치를 열지 않고 원문 unitLost 요청을 관찰하는 검사 수명이다.
    SoundList names{OriginalEdition::Patch1078}; // Session보다 오래 사는 실제 음향 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 설치가 읽는 명시 영어 번역 입력이다.
    std::vector<Sid> lost; // 공통 삭제가 실제로 요청한 unitLost SID만 기록한다.
    // 다른 게임 계산을 대체하지 않고 설치에 도달하는 외부 경계만 연결한다.
    RawCampaignSessionServices Make(Client& client) {
        names.Initialize();RawCampaignSessionServices services;services.soundNames=&names;services.unixSeconds=[] { return 946684800; };
        services.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; // 실제 Deck의 미작성 할당 바이트 입력이다.
        services.sound.wallSeconds=[] { return 100.0; };
        services.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        services.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        services.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        services.sound.stop=[](SoundHandle) {};services.sound.isPlaying=[](SoundHandle) { return 0U; }; // 실제 PCM 출력을 검사하지 않는 장치 경계다.
        services.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        services.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};services.install.refreshWindowTitle=[] {};services.install.resetCamera=[] {}; // HWND 없이 설치하고 카메라는 아래 실제 초기화 몸체로 준비한다.
        services.ownership.refreshProduction=[] {};services.ownership.themeMode=[] { return 0U; };
        services.assets.postPop.playImpactSound=[](std::uint32_t) {};services.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        services.priest.shield.noticeSound=[](const PriestShieldNotice&) {};services.priest.shield.translate=[](std::string_view key) { return std::string(key); };services.priest.shield.tell=[](std::string_view) {};
        services.deletion.selected=[] { return Sid{}; };services.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("손실 위치 검사에서 선택하지 않은 외부 삭제 효과"); };
        services.unitLost=[this](Sid sid) { lost.push_back(sid); }; // 공통 삭제의 실제 요청만 기록하며 좌표를 직접 쓰지 않는다.
        services.player.log=[](std::uint32_t,std::string_view) {};
        services.gameplayGlobals=client.GameplayGlobals();services.kencloudGlobals=client.KencloudGlobals();services.coreDataGlobals=client.CoreDataGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords(); // 같은 Client의 장기 owner를 Session에 빌린다.
        return services;
    }
};
}

// 명시 가상 삭제는 공격/승패를 진행하지 않으며 원문 소리 뒤 writer와 U 키의 현재 owner 접합만 확인한다.
TEST_CASE(LostPositionKey_ActualSessionDeletionWritesCurrentClientCamera) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));LostPositionServices external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make(client));
    try { session.Load(); }catch (const std::exception& error) { throw std::runtime_error(std::string("손실 위치 검사 Session.Load 단계: ")+error.what()); } // 최초 설치 오류를 뒤 명시 Pop의 입력 오류와 구별한다.
    auto core=session.Core();auto& gameplay=*client.GameplayGlobals();auto& auxiliary=*gameplay.GameAux();
    CHECK(session.World().Storage()->GameAux()==gameplay.GameAux());CHECK(&session.World().DeletionState().LostX()==&auxiliary.LostX() && &session.World().DeletionState().LostY()==&auxiliary.LostY());CHECK(&session.CameraGlobals()==&gameplay.Camera());
    auxiliary.ResetLostPositionForCleanup();CHECK(auxiliary.LostX()==-10000.0F && auxiliary.LostY()==-10000.0F); // 이전 무효 표식만 준비하며 관측할220 좌표는 실제 삭제가 기록해야 한다.
    const auto type=static_cast<std::uint32_t>(assets.game.TypeTable().Find("sunArcher"));CHECK(core.types[type].group!=10 && !(core.types[type].flags2&0x200000U)); // 원문 손실 통지가 제외하는 워크샾/장식 genus를 선택하지 않는다.
    CHECK(gameplay.Server());const auto victim=session.World().Create(type,0);CHECK(victim.value && victim.value>=core.pool.Layout().serverFirst);if (!victim.value) return;core.owner.Set(victim,session.LocalOwner());
    CHECK(session.FortObjectSettings().loadingDepth==0); // 현재 실제 비로딩 상태를 관측하며 검사를 통과시키려고 깊이를 바꾸지 않는다.
    try { session.World().Pop(victim,220.0F,220.0F,9); }catch (const std::exception& error) { throw std::runtime_error(std::string("손실 위치 검사 서버 Pop9 단계: ")+error.what()); } // 원문 FortObjectLoad도 사용하는 first1|server8 등록 입력이며 로딩 상태를 위조하지 않는다.
    CHECK(!(core.pool.Slot(victim)[11]&4U));external.lost.clear();
    CHECK(auxiliary.LostX()==-10000.0F && auxiliary.LostY()==-10000.0F);CHECK(session.World().Destroy(victim,0x10)); // 원문 client 삭제 플래그로 전송을 억제하고 같은 가상/공통 삭제를 실행한다.
    CHECK(core.pool.Slot(victim)[11]&1U);CHECK(external.lost==std::vector<Sid>{victim});CHECK(auxiliary.LostX()==220.0F && auxiliary.LostY()==220.0F);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    auto& camera=gameplay.Camera();auto& redraw=client.KencloudGlobals()->WorldRedraw();auto& revision=client.LocalStateGlobals()->MetaRebuild();camera.InitializeNativeRenderer(640,480,gameplay.WorldCells(),redraw,revision);camera.ApplyPixelsToRendererAndSnapshot(12,23);redraw=7;revision=11;std::uint32_t commits{}; // 현재 Renderer와 별개 목표가 움직이는지 확인할 초기 위치다.
    LostPositionKeyHooks keys;keys.paused=[&client] { return client.Paused(); };keys.center=[&](float x,float y) {
        CHECK(x==220.0F && y==220.0F);camera.CenterImmediately(x,y,640,480,redraw,revision,[&] { ++commits;netstorm::test::camera::CommitScreenless({camera,redraw,640,480}); }); // 실제433aa0/497960 본체를 사용하고 HWND/도킹 장치는 기존 독립 경계로 둔다.
    };
    CHECK(DispatchLostPositionKey({'u',0,0},auxiliary.LostX(),auxiliary.LostY(),gameplay.WorldCells(),keys));CHECK(commits==1 && redraw==8 && revision==12);CHECK(camera.Target().pixels[0]==3200 && camera.Target().pixels[1]==2180);CHECK(camera.RendererView().pixels==camera.Target().pixels && camera.Snapshot().pixels==camera.Target().pixels);CHECK(auxiliary.LostX()==220.0F && auxiliary.LostY()==220.0F);
}
