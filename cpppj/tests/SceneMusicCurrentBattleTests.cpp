// 실제 장면 음악 본체의 현재 battle 조회와 시각 관문·외부 효과 뒤 재조회만 검사한다. 창·음향 장치를 열지 않는다.
#include "TestSupport.h"
#include "client/ClientAudio.h"
#include "client/SoundSceneMusic.h"
#include "o/SquidOwner.h"
#include <functional>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 원문 관찰 fixture를 재계산하지 않고 현재 조회의 명시 입력/사건만 제공한다.
using namespace netstorm::client;
using netstorm::o::OriginalEdition;

// 파일/재생 장치 대신 곡 이름·길이0·명시 벽 시각을 공급하며 실제 SceneMusic을 호출하는 독립 장면이다.
struct CurrentBattleScene {
    SceneMusicState state; // fallback과 현재 조회를 구별하는 실제 제품 상태다.
    netstorm::o::GameRandom random{1U}; // 기존 원문 난수 본체를 사용하며 별도 난수 구현은 만들지 않는다.
    std::uint32_t current{}; // 외부 장면 전역의 현재 battle 입력이다.
    std::size_t battleReads{}; // 실제 현재 battle getter가 호출된 횟수다.
    std::vector<double> wallFeed; // 호출마다 소비할 명시 벽 시각이며 부족하면 실패한다.
    std::size_t wallReads{}; // 현재 입력 목록에서 소비한 시각 수다.
    std::string name{"before.mus"}; // 실제 select 경계가 바꾸는 현재 곡 이름이다.
    std::vector<std::string> events; // 시계·현재 조회·선택·날씨 효과의 실제 발생 순서다.
    std::function<void(std::string_view)> afterSelect; // 곡 선택 효과가 현재 전역을 바꾸는 명시 입력이다.
    std::function<void()> afterRefresh; // 날씨 화면 갱신 효과가 현재 전역을 바꾸는 명시 입력이다.
    SceneMusic music{state,random,Hooks()}; // 위 저장소를 빌리는 실제 장면 음악 본체다.

    // 생성은 callback만 연결하며 파일·원본 프로그램·OS 장치를 실행하지 않는다.
    CurrentBattleScene() { state.tints={11,22,33,44};BindReader(); }

    // 같은 현재 전역을 매 소비 시점에 읽게 연결하고 호출 횟수와 값을 남긴다.
    void BindReader() {
        state.currentBattle=[this] { ++battleReads;events.push_back("B:"+std::to_string(current));return current; };
    }

    // 새 호출에 필요한 정확한 벽 시각 목록을 설치한다. 추가 조회는 vector::at에서 실패한다.
    void Clock(std::initializer_list<double> values) { wallFeed.assign(values);wallReads=0; }

    // 원문이 요청한 다음 벽 시각 하나만 읽고 battle 조회와의 순서를 기록한다.
    double Wall() { events.push_back("T"+std::to_string(++wallReads));return wallFeed.at(wallReads-1); }

    // 실제 선택·날씨 외부 경계만 공급하고 선택하지 않는 희생/팔레트/번개 장치는 명시 거부한다.
    SceneMusicHooks Hooks() {
        SceneMusicHooks hooks; // 생성자가 요구하는 동일 제품 경계다.
        hooks.wallSeconds=[this] { return Wall(); };
        hooks.currentName=[this] { return std::string_view(name); };
        hooks.select=[this](std::string_view value) { name.assign(value);events.push_back("S:"+name);if (afterSelect) afterSelect(value); };
        hooks.duration=[] { return 0.0; }; // 길이를 알 수 없는 명시 입력이며 실제 Request의180초 관문을 사용한다.
        hooks.sacrificing=[](std::uint32_t)->bool { throw std::logic_error("이 독립 검사는 희생 경계를 선택하지 않는다"); };
        hooks.waitingRoom=[this] { events.push_back("W");return false; }; // 명시 비대기 입력이며 실제 MP 장면은 만들지 않는다.
        hooks.refresh=[this] { events.push_back("R");if (afterRefresh) afterRefresh(); };
        hooks.ascendancyPalette=[this] { events.push_back("O");return false; }; // 명시 설정0으로 파일 적재를 선택하지 않는다.
        hooks.loadPalette=[](std::string_view) { throw std::logic_error("이 독립 검사는 팔레트를 적재하지 않는다"); };
        hooks.thunderFlash=[] { throw std::logic_error("이 독립 검사는 실제 번개를 만들지 않는다"); };
        hooks.weatherSound=[](std::string_view) { throw std::logic_error("이 독립 검사는 음향 장치를 호출하지 않는다"); };
        return hooks;
    }
};
} // 독립 현재 전역 장치의 범위다.

// 선택적 조회가 없으면 fallback DWORD를 그대로 읽고, 연결하면0/비영 현재값과 이후 변경을 읽는다.
TEST_CASE(SceneMusicCurrentBattle_UsesCurrentProviderAndExplicitFallback) {
    CurrentBattleScene scene; // 실제 본체와 독립 현재 전역이다.
    scene.state.currentBattle={};scene.state.battle=9;
    CHECK(scene.music.CurrentBattle()==9 && scene.battleReads==0);
    scene.BindReader();scene.current=0;scene.state.battle=7;
    CHECK(scene.music.CurrentBattle()==0 && scene.battleReads==1);
    scene.current=17;
    CHECK(scene.music.CurrentBattle()==17 && scene.state.battle==7 && scene.battleReads==2);
    scene.state.currentBattle={};
    CHECK(scene.music.CurrentBattle()==7 && scene.battleReads==2);
}

// 만료 전·songEnd NaN·현재 시각 NaN은 현재 battle 조회 자체를 실행하지 않는다.
TEST_CASE(SceneMusicCurrentBattle_FrameTimeGatesDoNotReadMode) {
    CurrentBattleScene scene; // reader가 호출되면 즉시 실패하는 실제 장면 본체다.
    scene.state.currentBattle=[]()->std::uint32_t { throw std::logic_error("시각 관문 뒤에만 현재 battle을 읽어야 한다"); };
    scene.state.songEnd=100.0;scene.Clock({99.0});scene.music.Frame();
    CHECK(scene.wallReads==1 && scene.name=="before.mus");
    scene.state.songEnd=std::numeric_limits<double>::quiet_NaN();scene.Clock({100.0});scene.music.Frame();
    CHECK(scene.wallReads==1 && scene.name=="before.mus");
    scene.state.songEnd=0.0;scene.Clock({std::numeric_limits<double>::quiet_NaN()});scene.music.Frame();
    CHECK(scene.wallReads==1 && scene.battleReads==0 && scene.name=="before.mus");
}

// 만료 뒤에는 저장한 fallback과 반대인 현재0/1을 각각 한 번 읽어 실제 메뉴/원소 곡을 선택한다.
TEST_CASE(SceneMusicCurrentBattle_ExpiredFrameReadsTheCurrentModeOnce) {
    CurrentBattleScene scene; // 동일 장면에서 실제 현재 모드의 변경을 관측한다.
    scene.state.battle=0;scene.current=1;scene.state.index=3;scene.state.songEnd=10.0;scene.Clock({10.0,11.0});
    scene.music.Frame();
    CHECK(scene.battleReads==1 && scene.name=="wind22.mus" && scene.state.index==0);
    CHECK((scene.events==std::vector<std::string>{"T1","B:1","S:wind22.mus","T2","R","O"}));
    scene.events.clear();scene.state.battle=1;scene.current=0;scene.state.songEnd=20.0;scene.Clock({20.0,21.0});
    scene.music.Frame();
    CHECK(scene.battleReads==2 && scene.name=="ser22.mus" && scene.state.index==0);
    CHECK((scene.events==std::vector<std::string>{"T1","B:0","W","S:ser22.mus","T2"}));
}

// Start의 첫 조건은 한 번만 읽고 내부 Frame이 만료 전이면 다시 읽지 않는다.
TEST_CASE(SceneMusicCurrentBattle_StartReadsItsInitialModeOnlyOnceBeforeExpiry) {
    CurrentBattleScene scene; // 첫 조회1과 fallback0을 구별한다.
    scene.state.battle=0;scene.current=1;scene.Clock({10.0,11.0});scene.music.Start();
    CHECK(scene.battleReads==1 && scene.wallReads==2 && scene.name=="rain22.mus" && scene.state.index==1);
    CHECK((scene.events==std::vector<std::string>{"B:1","S:rain22.mus","T1","R","O","T2"}));
}

// 첫 메뉴 Request의 선택 효과가 현재 battle을1로 바꾸면 만료된 내부 Frame은 현재값을 다시 읽는다.
TEST_CASE(SceneMusicCurrentBattle_StartRereadsAfterRequestChangesMode) {
    CurrentBattleScene scene; // 명시 시각으로 내부 Frame만 첫 곡의180초 끝을 넘긴다.
    scene.current=0;scene.state.battle=1;scene.Clock({10.0,200.0,201.0});
    scene.afterSelect=[&](std::string_view value) { if (value=="ser22.mus") scene.current=1; }; // 실제 선택 callback 뒤 현재 전역 변경이다.
    scene.music.Start();
    CHECK(scene.battleReads==2 && scene.name=="wind22.mus" && scene.state.index==0 && scene.random.State()==1);
    CHECK((scene.events==std::vector<std::string>{"B:0","S:ser22.mus","T1","T2","B:1","S:wind22.mus","T3","R","O"}));
}

// 첫 원소 곡의 날씨 callback이 battle을0으로 바꾸면 내부 Frame은 메뉴 곡으로 넘어간다.
TEST_CASE(SceneMusicCurrentBattle_StartRereadsAfterWeatherChangesMode) {
    CurrentBattleScene scene; // 현재1과 fallback0의 차이 및 날씨 뒤 재조회를 관측한다.
    scene.current=1;scene.state.battle=0;scene.Clock({10.0,200.0,201.0});
    scene.afterRefresh=[&] { scene.current=0; }; // 별도 World 성공을 만들지 않는 명시 외부 전역 변경이다.
    scene.music.Start();
    CHECK(scene.battleReads==2 && scene.name=="ser22.mus" && scene.state.index==1);
    CHECK((scene.events==std::vector<std::string>{"B:1","S:rain22.mus","T1","R","O","T2","B:0","W","S:ser22.mus","T3"}));
}

// Next는 현재 battle을 읽는 함수가 아니므로 getter가 예외 장치여도 같은 다음 곡/날씨를 수행한다.
TEST_CASE(SceneMusicCurrentBattle_NextDoesNotReadCurrentMode) {
    CurrentBattleScene scene; // 희생 경계도 playersReady0으로 선택하지 않는다.
    scene.state.currentBattle=[]()->std::uint32_t { throw std::logic_error("Next에는 현재 battle 조회가 없다"); };
    scene.state.index=3;scene.Clock({10.0});scene.music.Next();
    CHECK(scene.battleReads==0 && scene.name=="wind22.mus" && scene.state.index==0);
    CHECK((scene.events==std::vector<std::string>{"S:wind22.mus","T1","R","O"}));
}

// 실제 ClientAudio의 값 조회와 진단도 같은 현재 getter를 읽으며 해제하면 기존 fallback으로 돌아간다.
TEST_CASE(SceneMusicCurrentBattle_ClientAudioQueryAndReportUseTheSameProvider) {
    ClientAudio audio({},OriginalEdition::Patch1078,"english",[] { return 0.0; },1U); // 생성만 하며 장치·음악 스레드·파일을 초기화하지 않는다.
    std::uint32_t current=7; // 실제 진단이 읽을 호출자 소유 현재 전역이다.
    std::size_t reads=0; // 값 조회와 진단이 수행한 현재 조회 횟수다.
    CHECK(audio.CurrentBattle()==0 && audio.Scene().battle==0);
    audio.BindCurrentBattle([&] { ++reads;return current; });
    CHECK(audio.CurrentBattle()==7 && reads==1);
    const auto first=audio.Describe(); // 장치 미초기화 상태의 실제 진단이다.
    CHECK(reads==2 && first.find("sceneBattle\t7\n")!=std::string::npos && audio.Scene().battle==0);
    current=0;
    const auto next=audio.Describe(); // 같은 provider가 바뀐 현재0을 다시 읽어야 한다.
    CHECK(reads==3 && next.find("sceneBattle\t0\n")!=std::string::npos);
    audio.BindCurrentBattle({});
    CHECK(audio.CurrentBattle()==0 && reads==3);
    const auto fallback=audio.Describe(); // 해제 뒤 진단도 fallback을 사용하며 예전 getter를 실행하지 않는다.
    CHECK(reads==3 && fallback.find("sceneBattle\t0\n")!=std::string::npos);
}

// 실제 ClientAudio의 StartScene(true) 뒤 같은 owner만0이 되는 정리 순서에서, 만료 시 메뉴 곡과 기존 색인을 보존한다.
TEST_CASE(SceneMusicCurrentBattle_ClientAudioKeepsOwnerAfterCleanupAndShutdown) {
    netstorm::o::SquidOwnerMode owner;owner.battle=true; // 실제 월드와 같은 모드 자료형이며 StartScene 사본과 구별한다.
    double wall=10.0; // 파일/장치 없이 음악 감독의 만료 조건에만 제공하는 벽 시각이다.
    ClientAudio audio({},OriginalEdition::Patch1078,"english",[&] { return wall; },1U); // 음향 장치를 초기화하지 않는다.
    audio.BindCurrentBattle([&] { return owner.battle ? 1U : 0U; });
    audio.StartScene(true); // 원문 cleanup은 실제 battle1에서 먼저 Start를 호출한다.
    const auto index=audio.Scene().index; // cleanup의 다음 battle0 쓰기가 바꾸면 안 되는 음악 색인이다.
    CHECK(index==1 && audio.Scene().battle==1 && audio.CurrentBattle()==1);
    owner.battle=false;audio.RequestMusic(kMenuMusic); // 원문 cleanup의0 쓰기와 State3의 직접 곡 요청 순서다.
    audio.Shutdown(); // 장치 종료가 현재 owner 연결을 끊어 옛 fallback1로 되돌리면 안 된다.
    CHECK(audio.CurrentBattle()==0 && audio.Scene().battle==1);
    wall=1000.0;audio.SceneFrame(); // 별도 Start(false) 없이도 만료는 현재 메뉴 모드를 읽어야 한다.
    CHECK(audio.CurrentBattle()==0 && audio.Scene().index==index && audio.Scene().battle==1);
    CHECK(audio.Describe().find("sceneBattle\t0\n")!=std::string::npos);
    audio.BindCurrentBattle({});CHECK(audio.CurrentBattle()==1); // 명시 fallback은 조회 연결 해제 때만 다시 활성화된다.
}
