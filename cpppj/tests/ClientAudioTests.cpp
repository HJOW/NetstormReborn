// 클라이언트 소리·음악 묶음(ClientAudio)의 음량 단계 표, 천둥 효과음 미리 적재, 장치 없는 장면 전환을 검사한다.
// 기본 CTest는 소리 장치와 창을 열지 않는다. 실제 장치를 여는 반복 검사는 명시 명령 --inspect-client-audio 전용이다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/ClientAudio.h"
#include "client/ClientMain.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientSpGlobals.h"
#include <array>
#include <bit>
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
    // path: 경로. 입력 구성 또는 실행 결과 대조에 사용한다.
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

// 호출자가 제공한 날씨 경계와 팔레트 갱신 전/후 색 대입을 실제 ClientAudio→SceneMusic 경로에서 확인한다. 장치는 열지 않는다.
TEST_CASE(ClientAudio_WeatherHooksPreservePaletteAndTintOrder) {
    bool enabled=true;std::vector<std::string> events;ClientAudio* target=nullptr;
    std::array<std::uint32_t,4> nextTints{21,22,23,24};
    SceneMusicHooks hooks;
    // 모든 날씨에서 화면 갱신이 먼저 요청돼야 한다.
    hooks.refresh=[&]() { events.emplace_back("refresh"); };
    // 옵션은 매 날씨 변경마다 다시 읽는다.
    hooks.ascendancyPalette=[&]() { events.emplace_back("option");return enabled; };
    // 로드 전에는 기존 색 표의 tint, 로드 후에는 새 색 표를 유지한다.
    hooks.loadPalette=[&](std::string_view name) {
        CHECK(target->Scene().tint==target->Scene().tints[static_cast<std::size_t>(target->Scene().index)]);
        events.emplace_back(name);target->SetSceneTints(nextTints);
    };
    // 천둥 화면 경계는 팔레트 적용 뒤에만 호출된다.
    hooks.thunderFlash=[&]() { events.emplace_back("flash");CHECK(target->Scene().paletteDirty==1); };
    ClientAudio audio({},OriginalEdition::Patch1078,"english",[] { return 10.0; },1,std::move(hooks));target=&audio;
    audio.SetSceneTints({11,12,13,14});audio.StartScene(false);
    CHECK(audio.Scene().tint==14 && events.empty());
    // 네 원소를 한 바퀴 돌며 원본 파일 이름, 기존 색 대입, 새 색 표를 확인한다.
    for (int i=0;i<4;++i) {
        events.clear();const auto before=audio.Scene().tints;
        audio.NextSceneMusic();const auto index=static_cast<std::size_t>(audio.Scene().index);
        CHECK(events.size()==(index==2 ? 4U : 3U));
        CHECK(events[0]=="refresh" && events[1]=="option" && events[2]==kAscendancyPalettes[index]);
        CHECK(audio.Scene().tint==before[index] && audio.Scene().tints==nextTints);
        nextTints[index]+=10;
    }
    enabled=false;events.clear();const auto tint=audio.Scene().tint;const auto tints=audio.Scene().tints;
    audio.NextSceneMusic();
    CHECK((events==std::vector<std::string>{"refresh","option"}));
    CHECK(audio.Scene().tint==tint && audio.Scene().tints==tints);
    audio.Shutdown();CHECK(audio.Scene().tints==tints);
}

// 옵션 조회만 또는 팔레트 로더만 주면 반쪽 연결이다. OS 자원을 열기 전에 실패해야 한다.
TEST_CASE(ClientAudio_RejectsIncompletePaletteHooks) {
    SceneMusicHooks option;option.ascendancyPalette=[] { return true; };
    SceneMusicHooks loader;loader.loadPalette=[](std::string_view) {};
    CHECK(Throws([&] { ClientAudio audio({},OriginalEdition::Patch1078,"english",[] { return 0.0; },1,option); }));
    CHECK(Throws([&] { ClientAudio audio({},OriginalEdition::Patch1078,"english",[] { return 0.0; },1,loader); }));
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

// 같은 장면 owner의 songEnd8BYTE만 바꾸고 곡/다른 기한/음소거·장치·난수와 현재 시각 조회를 보존한다.
TEST_CASE(ClientAudio_WriteSongEndBitsPreservesCurrentOwnerAndOtherEffects) {
    TemporaryDirectory directory;std::uint32_t reads{},controlReads{},effects{},controlEffects{}; // 두 실제 소유자의 외부 호출을 독립 계수한다.
    const auto hooks=[](std::uint32_t& count) { // 실제 날씨 하위가 호출되는 경우만 관측하고 생성자에서 효과를 내지 않는다.
        SceneMusicHooks value;value.refresh=[&count] { ++count; };value.ascendancyPalette=[&count] { ++count;return false; };value.loadPalette=[&count](auto) { ++count; };value.thunderFlash=[&count] { ++count; };return value;
    };
    ClientAudio audio(directory.path,OriginalEdition::Patch1078,"english",[&reads] { ++reads;return 123.25; },0x12345678U,hooks(effects));
    ClientAudio control(directory.path,OriginalEdition::Patch1078,"english",[&controlReads] { ++controlReads;return 123.25; },0x12345678U,hooks(controlEffects));
    for (auto* current:{&audio,&control}) { // 같은 실제 공개 API로 비영 기한/색/카메라/음소거와 같은 첫 난수 소비를 준비한다.
        current->SetSceneTints({17,29,43,61});current->SetView({20,30,5,7,805,607});current->PushMute();current->StartScene(true);current->RequestMusic("defeat.mus");
    }
    const auto* owner=&audio.Scene();const auto before=*owner;const auto beforeReads=reads,beforeEffects=effects;auto report=Parse(audio.Describe());report.erase("songEnd"); // 실제 같은 상태/곡/장치 값을 고정한다.
    CHECK(before.defeatEnd==123.25 && before.songEnd==303.25 && !audio.DeviceReady());
    constexpr std::array<std::uint64_t,7> patterns{0x8000000000000000ULL,0x7ff0000000000001ULL,0xfff8000000001234ULL,0x0000000000000001ULL,0x7ff0000000000000ULL,0x405edd2f1a9fbe77ULL,0}; // -0/sNaN/음수qNaN/subnormal/무한/유한/+0의 원시8BYTE다.
    for (const auto bits:patterns) { // 매번 현재 같은 owner에 쓰고 다음 명령이 실제로 요구할 때까지 곡 부모를 실행하지 않는다.
        audio.WriteSongEndBits(bits);CHECK(&audio.Scene()==owner && std::bit_cast<std::uint64_t>(owner->songEnd)==bits);
        CHECK(owner->index==before.index && owner->fanfareEnd==before.fanfareEnd && owner->defeatEnd==before.defeatEnd && owner->resultState==before.resultState);
        CHECK(owner->battle==before.battle && owner->playersReady==before.playersReady && owner->localPlayer==before.localPlayer && owner->tint==before.tint && owner->tints==before.tints && owner->paletteDirty==before.paletteDirty);
        CHECK(reads==beforeReads && effects==beforeEffects && !audio.DeviceReady());
    }
    auto after=Parse(audio.Describe());after.erase("songEnd");CHECK(after==report); // 음소거 깊이/음량·곡 이름/장치·재생 일련번호·카메라도 그대로다.
    for (std::uint32_t draw=0;draw<8;++draw) { // 원시쓰기 없는 대조 소유자와 후속 실제 전투 난수/곡 순환을 비교한다.
        audio.StartScene(true);control.StartScene(true);CHECK(audio.Scene().index==control.Scene().index && audio.Scene().tint==control.Scene().tint);
        CHECK(Parse(audio.Describe())["musicName"]==Parse(control.Describe())["musicName"]);
    }
    CHECK(reads==controlReads && effects==controlEffects);
}

// 기존 원본 scenemusic-x86.tsv의 PATCH Start 여섯 시드 결과를 실제 음향의 외부 난수 대여에도 적용한다.
TEST_CASE(ClientAudio_SharedGameRandomMatchesNativeStartSeeds) {
    TemporaryDirectory directory;
    constexpr std::array<std::uint32_t,6> seeds{0,1,195890093U,305419896U,4294967295U,7}; // 독립 PE 관측의 rng 입력이며 제품 출력에서 역산하지 않는다.
    constexpr std::array<std::uint32_t,6> after{783557386U,65542U,783557386U,2366964587U,4294901760U,458776U}; // 같은 원본 관측의 현재00532710이다.
    constexpr std::array<std::int32_t,6> indices{0,1,0,1,2,1}; // 같은 관측의 Start 뒤00469f00까지 적용한 곡 색인이다.
    for (std::size_t index=0;index<seeds.size();++index) { // 각 입력을 독립 음향 수명으로 만들어 이전 곡 이름의 조기 반환을 섞지 않는다.
        netstorm::o::GameRandom shared(seeds[index]);
        {
            ClientAudio audio(directory.path,OriginalEdition::Patch1078,"english",[] { return 1000.0; },shared);
            CHECK(shared.State()==seeds[index]);audio.StartScene(false);CHECK(shared.State()==seeds[index] && audio.Scene().index==3);
            audio.StartScene(true);CHECK(shared.State()==after[index] && audio.Scene().index==indices[index]);
            audio.RequestMusic("defeat.mus");audio.SceneFrame();audio.NextSceneMusic();audio.WriteSongEndBits(0);audio.Shutdown();CHECK(shared.State()==after[index]);
        }
        CHECK(shared.State()==after[index]); // 음향 소멸도 빌린 게임 난수를 재시드하거나 더 소비하지 않는다.
    }
}

// 공통 난수의 외부 변경과 실제 분산 SP 쓰기가 음향 사이에서도 하나의 현재 스트림으로 이어져야 한다.
TEST_CASE(ClientAudio_SharedRandomInterleavesCurrentSpState) {
    TemporaryDirectory directory;auto actual=std::make_shared<ClientRandomGlobals>();auto control=std::make_shared<ClientRandomGlobals>();
    const auto fill=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa5}); }; // 실제 할당의 시작 바이트만 고정하고 SP 난수/비트 쓰기는 그대로 실행한다.
    ClientSpGlobals sp(actual,[] { return 0; },fill),reference(control,[] { return 0; },fill);
    sp.Store().Initialize();reference.Store().Initialize();CHECK(actual->Game().State()==control->Game().State());
    ClientAudio audio(directory.path,OriginalEdition::Patch1078,"english",[] { return 100.0; },actual->Game());
    for (std::uint32_t owner=1;owner<9;++owner) { // 메뉴·전투 전환과 실제 owner별 SP 잡음 쓰기를 같은 순서로 교차한다.
        const auto bits=std::bit_cast<std::uint32_t>(static_cast<float>(owner)*123.25F);
        sp.Store().Set(owner,bits);reference.Store().Set(owner,bits);CHECK(actual->Game().State()==control->Game().State());
        audio.StartScene(false);CHECK(actual->Game().State()==control->Game().State());
        static_cast<void>(control->Game().Next(4000));audio.StartScene(true);CHECK(actual->Game().State()==control->Game().State());
        sp.Store().Set(owner,bits+1);reference.Store().Set(owner,bits+1);CHECK(actual->Game().State()==control->Game().State() && sp.Store().Get(owner)==reference.Store().Get(owner));
    }
    actual->Game().SetState(7);audio.StartScene(true);CHECK(actual->Game().State()==458776U && audio.Scene().index==1); // 생성 뒤 재시드도 사본 아닌 현재 객체에서 즉시 읽는다.
}

// 실제 Client의 noAudio 분기도 장치·창 없이 공통 전역을 빌리고 초기화/재초기화 실패로 시드를 쓰지 않는다.
TEST_CASE(ClientAudio_ActualClientWithoutDeviceUsesGameGlobals) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;
    std::shared_ptr<ClientRandomGlobals> globals;
    {
        Client client(std::move(options));globals=client.RandomGlobals();globals->Game().SetState(1);client.InitializeNativeAudio();
        CHECK(client.Audio()==nullptr && !client.WorldAudio().DeviceReady() && globals->Game().State()==1);
        client.WorldAudio().StartScene(true);CHECK(globals->Game().State()==65542U && client.WorldAudio().Scene().index==1);
        client.WorldAudio().StartScene(false);CHECK(globals->Game().State()==65542U);
        CHECK(Throws([&] { client.InitializeNativeAudio(); }) && globals->Game().State()==65542U);
        globals->Game().SetState(0xffffffffU);client.WorldAudio().StartScene(true);CHECK(globals->Game().State()==4294901760U && client.WorldAudio().Scene().index==2);
    }
    CHECK(globals->Game().State()==4294901760U); // Client와 음향이 소멸한 뒤에도 보존한 실제 난수 owner의 상태는 같다.
}

// 외부 팔레트 장치 쌍의 구성 실패에도 빌린 난수의 현재 값과 원본0시드 첫 호출을 보존한다.
TEST_CASE(ClientAudio_SharedRandomFailedConstructionKeepsSeed) {
    TemporaryDirectory directory;netstorm::o::GameRandom shared;SceneMusicHooks invalid;invalid.ascendancyPalette=[] { return true; };
    CHECK(Throws([&] { ClientAudio audio(directory.path,OriginalEdition::Patch1078,"english",[] { return 0.0; },shared,invalid); }) && shared.State()==0);
    ClientAudio audio(directory.path,OriginalEdition::Patch1078,"english",[] { return 0.0; },shared);audio.StartScene(true);CHECK(shared.State()==783557386U && audio.Scene().index==0);
}

namespace netstorm::test {
// 지정한 원본 게임 폴더로 실제 장치와 음악 스레드를 열어 장면 전환·소리 끄기/켜기·종료를 두 번 반복한다.
// 숨긴 협조 창을 쓰고 공유 음소거(-10000)를 걸어 소리는 내지 않는다. 기본 CTest/파일 검사는 호출하지 않는다.
void InspectClientAudio(const std::filesystem::path& root) {
    // DirectSound 협조 수준에 필요한 숨긴 창이다. 보이게 하거나 포커스를 바꾸지 않는다.
    struct CooperativeWindow {
        // handle: 네이티브 자원이나 등록 대상을 찾는 핸들. 입력 구성 또는 실행 결과 대조에 사용한다.
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
        // 실제 위치 반복 재생이 최신 화면을 읽는지 확인한다. 800×600 중심 → 카메라 200 이동 → 화면 밖 → 재진입 순서다.
        // 공유 음소거가 유지되므로 음악과 효과음 모두 들리지 않는다. 좌우 계산은 기존 독립 x86 대조 검사에서 별도로 검증한다.
        const auto thunder=audio.Names().Lookup("thunderCrack.wav");
        audio.SetView({0,0,0,0,800,600});
        const auto centered=audio.Sounds().PlayLoopAt(25.0f,300.0f/11.0f,0,thunder);
        require(centered!=0 && audio.Sounds().IsPlaying(centered)!=0 && audio.Names().Field(centered,SoundField::Volume)==0,
            "화면 중심의 위치 반복 재생 실패",audio.Describe());
        audio.SetView({200,0,0,0,800,600});
        const auto shifted=audio.Sounds().PlayLoopAt(25.0f,300.0f/11.0f,centered,thunder);
        require(shifted==centered && static_cast<std::int32_t>(audio.Names().Field(shifted,SoundField::Volume))==-250,
            "카메라 이동 뒤 위치 음량 갱신 실패",audio.Describe());
        audio.SetView({1000,0,0,0,800,600});
        require(audio.Sounds().PlayLoopAt(25.0f,300.0f/11.0f,shifted,thunder)==0 && audio.Sounds().IsPlaying(shifted)==0,
            "화면 밖 반복 소리 정지 실패",audio.Describe());
        audio.SetView({0,0,0,0,800,600});
        const auto restarted=audio.Sounds().PlayLoopAt(25.0f,300.0f/11.0f,0,thunder);
        require(restarted!=0 && audio.Sounds().IsPlaying(restarted)!=0,"화면 재진입 반복 소리 재시작 실패",audio.Describe());
        audio.Sounds().Stop(restarted);
        // 소리 끄기: 스레드를 합류하고 장치를 닫는다.
        options.sound=false;audio.Interpret(native,options);
        values=Parse(audio.Describe());
        require(values["deviceReady"]=="0" && values["musicRuntime"]=="0" && values["musicActive"]=="0","소리 끄기 실패",audio.Describe());
        require(audio.View().right==800 && audio.View().bottom==600,"소리 끄기 뒤 화면 영역 손실",audio.Describe());
        // 소리 켜기: 장치를 다시 열고 같은 곡을 다시 시작한다.
        options.sound=true;audio.Interpret(native,options);pump(audio,1500);
        values=Parse(audio.Describe());
        require(values["deviceReady"]=="1" && values["musicActive"]=="1" && (std::stoul(values["musicBufferStatus"])&5U)==5U,"소리 켜기 실패",audio.Describe());
        require(audio.View().right==800 && audio.View().bottom==600,"소리 재초기화 뒤 화면 영역 손실",audio.Describe());
        // 전투 장면: 원소 곡 하나가 시작된다.
        audio.StartScene(true);pump(audio,1500);
        values=Parse(audio.Describe());
        require(values["sceneBattle"]=="1" && values["musicActive"]=="1" && values["workerFailed"]=="0","전투 곡 시작 실패",audio.Describe());
        std::printf("Client audio cycle %d: menu stream, positional loop/camera/offscreen/reentry, sound off/on, battle song %s passed\n",cycle,values["musicName"].c_str());
        audio.Shutdown();
    }
    std::printf("Client audio device: two cycles of initialize/stream/sound off-on/battle/shutdown passed; muted\n");
}
}
