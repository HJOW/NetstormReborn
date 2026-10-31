// 현재 카메라/지도 조회를 독립 장치 경계에 연결해 원문의 소비 순서와 장치 호출 뒤 재조회를 검사한다.
#include "TestSupport.h"
#include "client/ClientAudio.h"
#include "client/Sound.h"
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace { // 실제 창·소리·Native 실행 없이 현재 전역의 조회 시점만 관측하는 명시 장치다.
using namespace netstorm::client;
using netstorm::o::OriginalEdition;

// 이 검사의 유일한 명시 버퍼 토큰이며 실제 장치 주소로 해석하지 않는다.
constexpr SoundBuffer kCurrentViewBuffer=0x16001000;

// 기존 SoundDeviceHooks만 구현해 재생/조회 사이의 현재 전역 변경을 관측한다.
struct CurrentViewDevice {
    std::uint32_t flags{}; // 버퍼의 현재 재생 비트이며 Play/Stop만 바꾼다.
    std::vector<std::string> events; // 현재 조회와 실제 장치 경계의 발생 순서다.
    std::function<void()> afterStatus; // 상태 조회가 끝나기 전에 현재 지도/카메라를 바꾸는 명시 외부 효과다.
    std::function<void()> afterPlay; // 첫 재생 뒤 원문 후반 계산 전에 현재 지도를 바꾸는 명시 외부 효과다.

    // 필수 여덟 장치 경계를 같은 토큰의 상태에 연결하며 실제 파일이나 장치를 열지 않는다.
    SoundDeviceHooks Hooks() {
        SoundDeviceHooks hooks;
        hooks.status=[this](SoundBuffer) { events.push_back("Q");if (afterStatus) afterStatus();return flags; };
        hooks.play=[this](SoundBuffer,std::uint32_t loop) { flags=1U|((loop&1U) ? 4U : 0U);events.push_back("P");if (afterPlay) afterPlay(); };
        hooks.setPosition=[this](SoundBuffer,std::uint32_t) { events.push_back("C"); };
        hooks.setVolume=[this](SoundBuffer,std::int32_t value) { events.push_back("V:"+std::to_string(value));return 0; };
        hooks.setPan=[this](SoundBuffer,std::int32_t value) { events.push_back("N:"+std::to_string(value));return 0; };
        hooks.stop=[this](SoundBuffer) { flags=0;events.push_back("X"); };
        hooks.duplicate=[](SoundBuffer,SoundBuffer& copy) { copy=kCurrentViewBuffer;return 0; };
        hooks.load=[this](std::string_view) { events.push_back("L");return SoundLoad{kCurrentViewBuffer,0}; };
        return hooks;
    }
};

// 이미 적재된 하나의 항목과 실제 SoundPlayer를 사용해 위치 계산 외의 파일/버퍼 생성 경계를 제외한다.
struct CurrentViewScene {
    CurrentViewDevice device; // 실제 SoundPlayer보다 오래 살아 있는 명시 장치다.
    SoundState state; // 현재 조회와 기존 fallback을 구별하는 같은 재생 전역이다.
    SoundList list{OriginalEdition::Patch1078}; // 원문 배치의 실제 소리 이름/항목 표다.
    SoundPlayer player{list,state,device.Hooks()}; // 새 위치 알고리즘을 복제하지 않고 실제 제품 함수를 호출한다.
    SoundHandle sound{}; // 현재 표에서 조회한 실제 항목 토큰이다.

    // 실제 장치는 열지 않고 기존 독립 경계가 사용하는 준비/적재 입력만 제공한다.
    CurrentViewScene() {
        state.initialized=state.device=state.enabled=true;list.Initialize();sound=list.Lookup("current.wav");list.SetField(sound,SoundField::Buffer,kCurrentViewBuffer);
    }
};
}

// 조회 미연결은 명시 view를 유지하고, 연결된 조회는 이후 바뀐 현재값을 복사해 반환한다.
TEST_CASE(SoundCurrentView_UsesCurrentProviderAndExplicitFallback) {
    CurrentViewScene scene;scene.state.view={17,29,3,5,803,605};
    const auto fallback=scene.player.CurrentView();CHECK(fallback.cameraX==17 && fallback.cameraY==29 && fallback.left==3 && fallback.bottom==605);
    SoundView current{41,43,7,11,907,711};std::size_t reads=0;
    scene.state.currentView=[&] { ++reads;return current; }; // 실제 현재 전역의 소유자는 이 호출자이며 getter가 값을 초기화하지 않는다.
    const auto first=scene.player.CurrentView();current.cameraX=101;current.left=37;scene.state.view={900,901,902,903,1002,1003};
    const auto next=scene.player.CurrentView();CHECK(reads==2 && first.cameraX==41 && next.cameraX==101 && next.left==37);
    scene.state.currentView={};const auto explicitView=scene.player.CurrentView();CHECK(explicitView.cameraX==900 && explicitView.left==902);
}

// 한 번 재생의 투영→화면 안→좌우→음량 네 소비는 각각 현재값을 읽으며 다른 단계의 사각형을 재사용하지 않는다.
TEST_CASE(SoundCurrentView_OnceReadsProjectionVisibilityPanAndVolumeInOrder) {
    CurrentViewScene scene;std::size_t reads=0;
    const std::array<SoundView,4> views{{
        {100,20,1000,1000,1100,1100}, // 투영 결과(300,200)만 쓰며 이 단계의 화면 영역은 사용하지 않는다.
        {900,900,280,180,320,220}, // 투영한 점이 보이는지 별도의 현재 영역에서 판정한다.
        {900,900,0,0,400,400}, // 현재 가로 절반200·오프셋100의 좌우값2500이다.
        {900,900,0,0,800,400} // 다음 현재 영역의 가로 오프셋-100·세로0으로 음량-62다.
    }};
    scene.state.currentView=[&] { scene.device.events.push_back("R"+std::to_string(++reads));return views.at(reads-1); }; // 각 원문 소비 위치를 구별하는 독립 입력이다.
    const auto played=scene.player.PlayOnceAt(25.0F,20.0F,scene.sound,0,0);
    CHECK(played==scene.sound && reads==4);
    CHECK((scene.device.events==std::vector<std::string>{"R1","R2","R3","R4","Q","V:-62","N:2500","P"}));
}

// 화면 밖이면 두 번째 조회 뒤 반환하며, 반복 재생의 옵션 관문은 현재 조회 자체도 실행하지 않는다.
TEST_CASE(SoundCurrentView_StopsAtTheOriginalVisibilityAndEnabledGates) {
    CurrentViewScene scene;std::size_t reads=0;
    scene.state.currentView=[&] { ++reads;scene.device.events.push_back("R");return SoundView{0,0,1000,1000,1100,1100}; };
    CHECK(scene.player.PlayOnceAt(25.0F,20.0F,scene.sound,0,0)==0 && reads==2);
    CHECK((scene.device.events==std::vector<std::string>{"R","R"}));
    scene.state.enabled=false;CHECK(scene.player.PlayLoopAt(25.0F,20.0F,scene.sound,0)==0 && reads==2);
    CHECK((scene.device.events==std::vector<std::string>{"R","R"}));
}

// 기존 버퍼의 상태 조회 뒤 바뀐 현재 영역은 후반 좌우/음량에 반영하되 앞서 투영한 화면 점은 그대로 쓴다.
TEST_CASE(SoundCurrentView_LoopRereadsGeometryAfterStatusCallbacks) {
    CurrentViewScene scene;scene.device.flags=1;SoundView current{0,0,0,0,800,600};std::size_t reads=0;
    scene.state.currentView=[&] { ++reads;scene.device.events.push_back("R");return current; };
    scene.device.afterStatus=[&] { current={1000,2000,0,0,400,600}; }; // 새 카메라로 다시 투영하면 원문과 다른 화면 밖 결과가 된다.
    const auto played=scene.player.PlayLoopAt(25.0F,300.0F/11.0F,scene.sound,0);
    CHECK(played==scene.sound && reads==4);
    CHECK((scene.device.events==std::vector<std::string>{"R","R","Q","Q","R","R","V:-1000","N:10000"}));
}

// 처음 재생의 장치 callback도 현재 지도를 바꿀 수 있으므로 재생 전 화면을 후반 계산에 고정하지 않는다.
TEST_CASE(SoundCurrentView_LoopRereadsGeometryAfterPlayCallback) {
    CurrentViewScene scene;SoundView current{0,0,0,0,800,600};std::size_t reads=0;
    scene.state.currentView=[&] { ++reads;scene.device.events.push_back("R");return current; };
    scene.device.afterPlay=[&] { current={1000,2000,0,0,400,600}; };
    const auto played=scene.player.PlayLoopAt(25.0F,300.0F/11.0F,0,scene.sound);
    CHECK(played==scene.sound && reads==4);
    CHECK((scene.device.events==std::vector<std::string>{"R","R","Q","V:0","N:0","P","Q","R","R","V:-1000","N:10000"}));
}

// 실제 ClientAudio의 진단도 같은 현재 조회를 사용하고 장치 미초기화·fallback 교체에 의해 현재값을 잃지 않는다.
TEST_CASE(SoundCurrentView_ClientAudioViewAndReportUseTheSameCurrentSource) {
    ClientAudio audio({},OriginalEdition::Patch1078,"english",[] { return 0.0; },1U); // 생성만 하며 실제 파일·소리 장치를 초기화하지 않는다.
    audio.SetView({17,29,3,5,803,605});SoundView current{41,43,7,11,907,711};std::size_t reads=0;
    audio.BindCurrentView([&] { ++reads;return current; });
    const auto first=audio.View();current={101,103,13,17,1013,817};
    const auto report=audio.Describe();CHECK(reads==2 && first.cameraX==41 && first.left==7);
    CHECK(report.find("soundCameraX\t101\n")!=std::string::npos && report.find("soundCameraY\t103\n")!=std::string::npos);
    CHECK(report.find("soundLeft\t13\n")!=std::string::npos && report.find("soundTop\t17\n")!=std::string::npos);
    CHECK(report.find("soundRight\t1013\n")!=std::string::npos && report.find("soundBottom\t817\n")!=std::string::npos);
    audio.SetView({201,203,23,29,1223,929});CHECK(audio.View().cameraX==101 && reads==3);
    audio.BindCurrentView({});const auto fallback=audio.View();CHECK(reads==3 && fallback.cameraX==201 && fallback.left==23 && fallback.bottom==929);
}
