// 클라이언트의 소리·음악 소유자. 원본 WinMain "init sound" 순서와 Interpret Options의 소리 부분을 복원된 계층에 잇는다.
#include "client/ClientAudio.h"
#include <exception>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// "init sound"에서 장치를 연 직후 미리 적재하는 효과음이다. 천둥 곡이 시작될 때 지연 없이 나게 한다.
// 원본: 00438dc0 안의 `FUN_004a9520("thunderCrack.wav")`(문자열 인자).
// 이력: 2026-10-10 추가 — 클라이언트에 소리를 연결하면서.
constexpr std::string_view kPreloadedSound="thunderCrack.wav";
// 효과음 폴더 이름과 음악 폴더 이름이다. 게임 폴더 아래의 상대 경로이며 원본 설정 SoundDir·MusicDir의 값("\SOUND"·"\MUSIC")에 해당한다.
// 이력: 2026-10-10 추가.
constexpr const char* kSoundDirectory="sound";
constexpr const char* kMusicDirectory="music";

// 장면 음악이 쓰는 경계 가운데 아직 월드·화면이 없어 비워 두는 것들을 채운다. 비어 있어도 안전한 값을 돌려준다.
// 사용: 월드(희생 판정·대기실)와 화면(갱신·팔레트·번개)이 복원되면 해당 항목을 실제 연결로 바꾼다.
// 이력: 2026-10-10 추가 — 날씨 팔레트는 적용하지 않으므로 ascendancyPalette 조회를 거짓으로 둔다(색만 바꾸고 팔레트를 안 바꾸는 중간 상태를 피한다).
SceneMusicHooks UnconnectedSceneBoundaries(std::function<double()> wallSeconds) {
    SceneMusicHooks hooks;
    hooks.wallSeconds=std::move(wallSeconds);
    // 희생 판정은 월드가 필요하다: 아직 항상 "진행 중 아님"이다.
    hooks.sacrificing=[](std::uint32_t) { return false; };
    // 멀티플레이 대기실은 3차 목표다.
    hooks.waitingRoom=[] { return false; };
    // 화면 갱신·날씨 팔레트·번개 효과는 화면 복원 때 연결한다.
    hooks.refresh=[] {};
    hooks.ascendancyPalette=[] { return false; };
    hooks.loadPalette=[](std::string_view) {};
    hooks.thunderFlash=[] {};
    return hooks;
}
}

// 점프 표 004359d0·004359e0과 같은 값이다. `step - 1`을 부호 없이 3과 비교해 범위 밖은 모두 0이다.
std::int32_t VolumeFromStep(std::int32_t step) {
    // 부호 없는 비교: 0 이하(음수 포함)는 큰 값으로 바뀌어 기본 분기로 간다.
    const auto index=static_cast<std::uint32_t>(step)-1U;
    switch (index) {
    case 0: return -4000;
    case 1: return -2000;
    case 2: return -1000;
    case 3: return -500;
    default: return 0;
    }
}

// 선언 순서대로 만든다: 이름 표/상태 → 장치 → 재생 → 음악 파일 → 두 채널 → 음악 제어 → 스레드 실행기 → 곡 선택 → 장면 음악.
// 어떤 OS 자원도 여기서 열지 않는다(장치·스레드는 Initialize가 연다).
ClientAudio::ClientAudio(std::filesystem::path gameDirectory,o::OriginalEdition edition,std::string language,
    std::function<double()> wallSeconds,std::uint32_t randomSeed)
    :list_(edition),
     device_(list_,state_,MakeDiskSoundResolver(gameDirectory),language,kSoundDirectory),
     player_(list_,state_,device_.Hooks()),
     primary_(edition,primaryState_,device_.BindMusicBuffers(files_.FileHooks())),
     secondary_(edition,secondaryState_,device_.BindMusicBuffers(files_.FileHooks())),
     music_(edition,state_,player_,primary_),
     runtime_(state_,runtimeState_,primary_,secondary_,music_,device_.MusicBuffers(),MakeWin32MusicRuntimeHooks()),
     selection_(selected_,music_,MusicDirectories{std::string(kMusicDirectory),std::nullopt},
         files_.OpenHooks(MakeDiskSoundResolver(gameDirectory)),device_.MusicBuffers()),
     random_(randomSeed),
     sceneMusic_(scene_,random_,MakeSceneMusicHooks(selection_,selected_,primaryState_,player_,UnconnectedSceneBoundaries(std::move(wallSeconds)))) {}

// 스레드 합류 → 장치 닫기 순서를 Shutdown이 지킨다. 그 뒤 멤버가 선언의 반대 순서로 사라진다.
ClientAudio::~ClientAudio() {
    try { Shutdown(); } catch (...) {}
}

// 옵션 값을 먼저 반영하고(원본 Interpret Options가 장치 초기화보다 앞서 전역을 채운다), 장치를 연다.
// 원본은 소리 옵션과 무관하게 "init sound"에서 장치를 연다. 실패하면 장치 계층이 소리 옵션을 끈다.
bool ClientAudio::Initialize(std::uintptr_t window,const ClientAudioOptions& options) {
    options_=options;
    ApplyValues(options);
    return OpenDevice(window,options.quality);
}

// 소리가 켜져 있으면(그리고 아직 열려 있지 않으면) 장치를 열고, 꺼져 있으면 닫는다. 그 뒤 음량과 현재 곡을 옵션에 맞춘다.
void ClientAudio::Interpret(std::uintptr_t window,const ClientAudioOptions& options) {
    options_=options;
    ApplyValues(options);
    // 원본 `if (sound != 0 && !첫 해석) 004aa600(품질) else 004a8ef0()`. 이미 열려 있으면 장치 계층이 false를 돌려주므로 다시 열지 않는다.
    if (options.sound) { if (!state_.initialized) OpenDevice(window,options.quality); }
    else CloseDevice();
    // 원본 `00435160(현재 곡 이름, 음악 옵션)`: 꺼지면 정지, 켜져 있으면 멈춰 있을 때 다시 시작한다. 이름이 비어 있으면 고를 곡이 없다.
    const std::string current(selected_.Current());
    if (!current.empty()) selection_.Select(current.c_str(),options.music ? 1 : 0);
}

// 음악 스레드를 합류한 뒤 장치를 닫는다.
void ClientAudio::Shutdown() {
    CloseDevice();
}

// 원본 메인 루프 3단계. 초기화·옵션과 무관하게 실제 버퍼 상태로 센다.
void ClientAudio::CountPlaying() {
    player_.Recount();
}

// 장면 음악 감독의 프레임 확인이다.
void ClientAudio::SceneFrame() {
    sceneMusic_.Frame();
}

// 전투 여부만 바꾸고 감독의 Start를 부른다. 플레이어 표 준비 여부는 월드가 생기기 전까지 0으로 둔다(희생 판정을 건너뛴다).
void ClientAudio::StartScene(bool battle) {
    scene_.battle=battle ? 1U : 0U;
    sceneMusic_.Start();
}

// 감독의 곡 요청을 그대로 전달한다.
void ClientAudio::RequestMusic(std::string_view name) {
    sceneMusic_.Request(name);
}

// 음악 제어가 효과음 음량 예약까지 함께 맡는다.
void ClientAudio::PushMute() { music_.PushMute(); }

// 마지막 해제에서 예약한 두 음량을 되돌린다.
void ClientAudio::PopMute() { music_.PopMute(); }

// 월드가 효과음을 낼 때 쓰는 재생 계층이다.
SoundPlayer& ClientAudio::Sounds() { return player_; }

// 효과음 이름 표다.
SoundList& ClientAudio::Names() { return list_; }

// 장면 음악의 읽기 전용 상태다.
const SceneMusicState& ClientAudio::Scene() const { return scene_; }

// 장치 초기화가 끝났는지다.
bool ClientAudio::DeviceReady() const { return state_.initialized; }

// 원본 순서: 004aa600(장치) → 004a9520(천둥 효과음 적재) → 004aadd0(음악 스레드). 음악 스레드를 못 만들면 장치를 닫고 소리를 끈다.
bool ClientAudio::OpenDevice(std::uintptr_t window,int quality) {
    if (!device_.Initialize(window,quality)) return false;
    player_.Preload(kPreloadedSound);
    try {
        runtime_.Initialize();
    } catch (const std::exception&) {
        // 스레드/이벤트를 만들지 못했다: 반쯤 열린 상태를 남기지 않는다.
        device_.Shutdown();
        state_.enabled=false;
        return false;
    }
    runtimeRunning_=true;
    return true;
}

// 음악 스레드가 닫힌 버퍼를 건드리지 않도록 스레드를 먼저 합류한다(원본 004a8ef0에는 없는 순서 보장).
void ClientAudio::CloseDevice() {
    if (runtimeRunning_) {
        runtime_.Shutdown();
        runtimeRunning_=false;
    }
    device_.Shutdown();
}

// 소리 전역과 음량을 옵션으로 채운다. 음량 두 호출은 초기화 여부와 무관하게 부른다(원본 00435631·0043566a).
void ClientAudio::ApplyValues(const ClientAudioOptions& options) {
    state_.enabled=options.sound;
    state_.swapSpeakers=options.swapSpeakers;
    state_.maxPlaying=options.maxSimulSounds;
    selected_.enabled=options.music ? 1 : 0;
    player_.SetMasterVolume(VolumeFromStep(options.soundVolumeStep));
    music_.SetVolume(VolumeFromStep(options.musicVolumeStep));
}

// 줄마다 `이름\t값`. 장치에서 읽는 값은 장치가 열려 있고 음악 버퍼가 있을 때만 적는다.
std::string ClientAudio::Describe() {
    std::ostringstream out;
    // 한 줄을 적는 도우미다.
    const auto line=[&out](const char* name,const auto& value) { out<<name<<'\t'<<value<<'\n'; };
    line("deviceReady",state_.initialized ? 1 : 0);
    line("soundEnabled",state_.enabled ? 1 : 0);
    line("musicOption",selected_.enabled);
    line("musicRuntime",state_.musicInitialized ? 1 : 0);
    line("masterVolume",state_.masterVolume);
    line("musicVolume",state_.musicVolume);
    line("maxPlaying",state_.maxPlaying);
    // 공유 음소거 깊이와, 음소거 중 요청된 최신 음량(해제될 때 적용된다)이다.
    line("muteDepth",state_.volumeHoldDepth);
    line("pendingMasterVolume",state_.pendingMasterVolume);
    line("pendingMusicVolume",state_.pendingMusicVolume);
    line("playingEffects",state_.playing);
    // 지금까지 효과음을 재생한 횟수(다음 재생 일련번호)다. 천둥 곡이 시작되면 천둥 효과음 때문에 0보다 커진다.
    line("soundSerial",state_.serial);
    line("musicName",std::string(selected_.Current()));
    line("musicActive",primary_.Active() ? 1 : 0);
    line("musicDuration",MusicChannelDuration(primaryState_));
    // 파일에서 읽어 링 버퍼에 채운 위치와 데이터 전체 길이(바이트)다. 시간이 지나며 읽은 위치가 버퍼 크기를 넘어 전진하면 장치 커서가 소비한 만큼 스레드가 다시 채웠다는 뜻이다.
    line("musicReadOffset",primaryState_.Field(MusicField::ReadOffset));
    line("musicDataLength",primaryState_.Field(MusicField::Length));
    line("sceneIndex",scene_.index);
    line("sceneBattle",scene_.battle);
    line("songEnd",scene_.songEnd);
    const auto token=primaryState_.Field(MusicField::Buffer);
    if (state_.initialized && token!=0) {
        const auto buffers=device_.MusicBuffers();
        std::uint32_t status=0,play=0,write=0;
        // 음악 버퍼의 실제 장치 상태: 비트 1 재생 중, 4 반복 재생. 재생 커서가 시간에 따라 전진하면 장치가 소리를 내고 있다.
        if (buffers.status(token,status)>=0) line("musicBufferStatus",status);
        if (buffers.cursor(token,play,write)>=0) { line("musicPlayCursor",play);line("musicWriteCursor",write); }
    }
    line("workerFailed",runtime_.WorkerFailure() ? 1 : 0);
    return out.str();
}
}
