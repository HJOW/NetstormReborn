// 클라이언트 소리·음악 묶음(ClientAudio)의 음량 단계 표, 천둥 효과음 미리 적재, 장치 없는 장면 전환을 검사한다.
// 기본 CTest는 소리 장치와 창을 열지 않는다. 실제 장치를 여는 반복 검사는 명시 명령 --inspect-client-audio 전용이다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/ClientAudio.h"
#include <array>
#include <chrono>
#include <climits>
#include <filesystem>
#include <map>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace {
using namespace netstorm::client;
using netstorm::o::OriginalEdition;

// 검사 전용 임시 디렉터리를 소유한다. 소리·음악 파일을 만들지 않으므로 장치 없는 경로만 쓴다.
struct TemporaryDirectory {
    std::filesystem::path path;
    // 프로세스와 시각으로 이름을 구분한 새 임시 디렉터리를 만든다.
    TemporaryDirectory():path(std::filesystem::temp_directory_path()/("NetstormCppClientAudio-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::filesystem::create_directories(path);
    }
    // 이 검사에서 만든 디렉터리만 지운다.
    ~TemporaryDirectory() { std::error_code ignored;std::filesystem::remove_all(path,ignored); }
};
// 예외를 던지는지만 본다.
bool Throws(const std::function<void()>& call) { try { call(); } catch (const std::exception&) { return true; }return false; }
// Describe()의 `이름\t값` 줄을 사전으로 바꾼다.
std::map<std::string,std::string> Parse(const std::string& text) {
    std::map<std::string,std::string> values;
    std::istringstream input(text);std::string line;
    // 한 줄씩 탭 앞뒤로 나눈다.
    while (std::getline(input,line)) {
        const auto tab=line.find('\t');
        if (tab!=std::string::npos) values[line.substr(0,tab)]=line.substr(tab+1);
    }
    return values;
}
}

// 원본 점프 표(004359d0·004359e0)의 값이다. 단계 - 1을 부호 없이 3과 비교하므로 0 이하와 5 이상은 모두 0(최대)이다.
TEST_CASE(ClientAudio_VolumeStepFollowsOriginalJumpTable) {
    CHECK(VolumeFromStep(1)==-4000);
    CHECK(VolumeFromStep(2)==-2000);
    CHECK(VolumeFromStep(3)==-1000);
    CHECK(VolumeFromStep(4)==-500);
    CHECK(VolumeFromStep(5)==0);
    // 범위 밖: 0과 음수는 부호 없는 비교에서 큰 값이 되어 기본 분기로 간다.
    for (const int step:{0,-1,-4,6,7,100,INT_MIN,INT_MAX}) CHECK(VolumeFromStep(step)==0);
}

// 천둥 효과음 미리 적재(004a9520): 버퍼가 0일 때만 장치 적재를 부르고, 적재된 버퍼가 있을 때만 감쇠를 적는다.
TEST_CASE(ClientAudio_PreloadLoadsOnlyUnloadedEntries) {
    SoundList list(OriginalEdition::Patch1078);list.Initialize();SoundState state;state.initialized=true;
    std::vector<std::string> loads;SoundLoad next{0x1234,7};
    SoundDeviceHooks hooks;
    hooks.status=[](SoundBuffer) { return 0U; };hooks.play=[](SoundBuffer,std::uint32_t) {};hooks.setPosition=[](SoundBuffer,std::uint32_t) {};
    hooks.setVolume=[](SoundBuffer,std::int32_t) { return 0; };hooks.setPan=[](SoundBuffer,std::int32_t) { return 0; };
    hooks.stop=[](SoundBuffer) {};hooks.duplicate=[](SoundBuffer,SoundBuffer&) { return -1; };
    hooks.load=[&](std::string_view name) { loads.emplace_back(name);return next; };
    SoundPlayer player(list,state,hooks);
    // 처음에는 적재하고 버퍼와 감쇠를 항목에 적는다.
    const auto thunder=player.Preload("thunderCrack.wav");
    CHECK(loads.size()==1 && loads[0]=="thunderCrack.wav");
    CHECK(list.Field(thunder,SoundField::Buffer)==0x1234 && list.Field(thunder,SoundField::Attenuation)==7U);
    // 이미 적재됐으면 이름의 대소문자가 달라도 다시 적재하지 않고 같은 항목을 돌려준다.
    CHECK(player.Preload("THUNDERCRACK.WAV")==thunder && loads.size()==1);
    // 장치가 없어 버퍼가 0이면 감쇠를 쓰지 않고, 다음 호출에서 다시 시도한다.
    next={0,9};
    const auto missing=player.Preload("noDevice.wav");
    CHECK(loads.size()==2 && list.Field(missing,SoundField::Buffer)==0 && list.Field(missing,SoundField::Attenuation)==0);
    CHECK(player.Preload("noDevice.wav")==missing && loads.size()==3);
    // 파일이 없다는 표식은 버퍼로 적히고 감쇠도 함께 적히며 다시 적재하지 않는다.
    next={kSilentSoundBuffer,3};
    const auto silent=player.Preload("noFile.wav");
    CHECK(list.Field(silent,SoundField::Buffer)==kSilentSoundBuffer && list.Field(silent,SoundField::Attenuation)==3U);
    CHECK(player.Preload("noFile.wav")==silent && loads.size()==4);
    // 원본 004a9520은 준비 여부·소리 옵션을 검사하지 않는다: 준비되지 않아도 적재 경계를 부른다.
    state.initialized=false;state.enabled=false;next={0x55,1};
    const auto unready=player.Preload("unready.wav");
    CHECK(loads.size()==5 && list.Field(unready,SoundField::Buffer)==0x55U);
    // 빈 이름은 이름 표 계약대로 거부한다.
    CHECK(Throws([&] { player.Preload(""); }));
}

// 소리 장치가 없는 환경에서도 장면 전환·곡 끝 확인·종료가 예외 없이 동작하고 원본 규칙대로 곡 색인과 끝 시각을 유지한다.
TEST_CASE(ClientAudio_SceneRunsWithoutDevice) {
    TemporaryDirectory directory;double wall=100.0;
    ClientAudio audio(directory.path,OriginalEdition::Patch1078,"english",[&wall] { return wall; },12345U);
    CHECK(!audio.DeviceReady());
    audio.CountPlaying();
    // 메뉴 장면: 색인 3과 메뉴 곡. 파일이 없어 길이를 모르므로 끝 시각은 지금 + 180초다.
    audio.StartScene(false);
    auto values=Parse(audio.Describe());
    CHECK(values["musicName"]=="ser22.mus" && values["sceneIndex"]=="3" && values["sceneBattle"]=="0");
    CHECK(audio.Scene().songEnd==280.0 && values["deviceReady"]=="0" && values["musicActive"]=="0" && values["workerFailed"]=="0");
    // 곡이 끝나기 전 프레임은 아무것도 바꾸지 않는다.
    wall=279.5;audio.SceneFrame();
    CHECK(audio.Scene().songEnd==280.0 && Parse(audio.Describe())["musicName"]=="ser22.mus");
    // 전투 장면: 난수로 고른 색인의 다음 원소 곡을 고르고 곡 끝 시각을 다시 정한다.
    wall=100.0;audio.StartScene(true);
    const std::array<const char*,4> elements{"wind22.mus","rain22.mus","thu22.mus","sun22.mus"};
    const auto first=audio.Scene().index;
    CHECK(first>=0 && first<=3 && audio.Scene().battle==1U);
    CHECK(Parse(audio.Describe())["musicName"]==elements[static_cast<std::size_t>(first)] && audio.Scene().songEnd==280.0);
    // 곡 끝 시각에 이르면 색인을 하나 올려(4로 나눈 나머지) 다음 원소 곡을 요청한다.
    wall=280.0;audio.SceneFrame();
    const auto second=audio.Scene().index;
    CHECK(second==(first+1)%4 && Parse(audio.Describe())["musicName"]==elements[static_cast<std::size_t>(second)] && audio.Scene().songEnd==460.0);
    // 직접 요청: 같은 이름은 무시하고 다른 이름이면 바꾼다.
    audio.RequestMusic("sacrifice.mus");
    CHECK(Parse(audio.Describe())["musicName"]=="sacrifice.mus");
    // 창 핸들 없이 장치를 열 수 없다: 예외를 던지고 상태를 바꾸지 않는다.
    CHECK(Throws([&] { audio.Initialize(0,ClientAudioOptions{}); }));
    CHECK(!audio.DeviceReady());
    // 반복 종료와 소멸은 안전하다.
    audio.Shutdown();audio.Shutdown();
}

namespace netstorm::test {
// 지정한 원본 게임 폴더로 실제 장치와 음악 스레드를 열어 장면 전환·소리 끄기/켜기·종료를 두 번 반복한다.
// 숨긴 협조 창을 쓰고 공유 음소거(-10000)를 걸어 소리는 내지 않는다. 기본 CTest/파일 검사는 호출하지 않는다.
void InspectClientAudio(const std::filesystem::path& root) {
    // DirectSound 협조 수준에 필요한 숨긴 창이다. 보이게 하거나 포커스를 바꾸지 않는다.
    struct CooperativeWindow {
        HWND handle{};
        // 시스템 STATIC 클래스를 쓰므로 별도 창 클래스 등록이 필요 없다.
        CooperativeWindow():handle(CreateWindowExW(0,L"STATIC",L"NetstormCpp client audio inspection",WS_POPUP,0,0,1,1,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr)) {
            if (!handle) throw std::runtime_error("소리 검사 창 생성 실패");
        }
        // 소리 묶음이 먼저 닫힌 뒤 창을 없앤다.
        ~CooperativeWindow() { DestroyWindow(handle); }
    } window;
    const auto native=reinterpret_cast<std::uintptr_t>(window.handle);
    const auto start=std::chrono::steady_clock::now();
    // 실시간 시계(초)다.
    const auto wall=[start] { return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count(); };
    // 일정 시간 동안 메인 루프처럼 효과음 수 세기와 장면 확인을 반복한다.
    const auto pump=[&](ClientAudio& audio,int milliseconds) {
        const auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(milliseconds);
        // 15ms마다 한 프레임씩 돈다.
        while (std::chrono::steady_clock::now()<until) { audio.CountPlaying();audio.SceneFrame();std::this_thread::sleep_for(std::chrono::milliseconds(15)); }
    };
    // 조건이 거짓이면 실패 이유를 담아 예외를 던진다.
    const auto require=[](bool condition,const std::string& what,const std::string& report) { if (!condition) throw std::runtime_error(what+"\n"+report); };
    constexpr std::uint32_t ringBytes=0x2af80;
    // 같은 순서를 두 번 반복해 장치·스레드·파일의 수명이 되풀이돼도 안전한지 본다.
    for (int cycle=1;cycle<=2;++cycle) {
        ClientAudio audio(root,OriginalEdition::Patch1078,"english",wall,static_cast<std::uint32_t>(cycle));
        ClientAudioOptions options;
        require(audio.Initialize(native,options),"장치 초기화 실패","");
        audio.PushMute();audio.StartScene(false);pump(audio,2500);
        auto values=Parse(audio.Describe());
        require(values["deviceReady"]=="1" && values["musicRuntime"]=="1" && values["musicName"]=="ser22.mus" && values["musicActive"]=="1","메뉴 곡 시작 실패",audio.Describe());
        require((std::stoul(values["musicBufferStatus"])&5U)==5U && std::stoul(values["musicReadOffset"])>ringBytes && values["workerFailed"]=="0","메뉴 곡 스트림 진행 실패",audio.Describe());
        // 소리 끄기: 스레드를 합류하고 장치를 닫는다.
        options.sound=false;audio.Interpret(native,options);
        values=Parse(audio.Describe());
        require(values["deviceReady"]=="0" && values["musicRuntime"]=="0" && values["musicActive"]=="0","소리 끄기 실패",audio.Describe());
        // 소리 켜기: 장치를 다시 열고 같은 곡을 다시 시작한다.
        options.sound=true;audio.Interpret(native,options);pump(audio,1500);
        values=Parse(audio.Describe());
        require(values["deviceReady"]=="1" && values["musicActive"]=="1" && (std::stoul(values["musicBufferStatus"])&5U)==5U,"소리 켜기 실패",audio.Describe());
        // 전투 장면: 원소 곡 하나가 시작된다.
        audio.StartScene(true);pump(audio,1500);
        values=Parse(audio.Describe());
        require(values["sceneBattle"]=="1" && values["musicActive"]=="1" && values["workerFailed"]=="0","전투 곡 시작 실패",audio.Describe());
        std::printf("Client audio cycle %d: menu stream, sound off/on, battle song %s passed\n",cycle,values["musicName"].c_str());
        audio.Shutdown();
    }
    std::printf("Client audio device: two cycles of initialize/stream/sound off-on/battle/shutdown passed; muted\n");
}
}
