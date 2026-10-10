// 원본 WinMain의 "init sound" 단계와 Interpret Options의 소리·음악 부분, 장면 음악 호출 위치를 클라이언트에 붙이는 소유자다.
// 이미 복원한 효과음(SoundList·SoundPlayer·SoundDevice)과 음악(MusicFileStore·MusicChannel·SoundMusic·MusicRuntime·MusicSelection)·
// 장면 음악(SceneMusic)을 한 객체에 담아 원본의 초기화·종료 순서대로 연다. 위치 효과음의 화면·카메라는 클라이언트가 공급한다.
#pragma once
#include "client/Sound.h"
#include "client/SoundDevice.h"
#include "client/SoundMusicFile.h"
#include "client/SoundMusicRuntime.h"
#include "client/SoundMusicSelection.h"
#include "client/SoundSceneMusic.h"
#include "o/RiftType.h"
#include "o/SpStore.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 옵션 음량 단계(soundVolume·musicVolume)를 장치에 넘기는 1/100 dB 음량으로 바꾼다.
// 단계 1 → -4000, 2 → -2000, 3 → -1000, 4 → -500이고 5와 그 밖의 값(0·음수·6 이상)은 0(최대)이다.
// 사용: 효과음은 SoundPlayer::SetMasterVolume에, 음악은 SoundMusic::SetVolume에 이 결과를 넘긴다.
// 원본: Interpret Options 00435605~00435631(효과음 점프 표 004359d0)과 0043563e~0043566a(음악 점프 표 004359e0).
//       `단계 - 1`을 부호 없이 3과 비교하므로 0 이하와 5 이상은 모두 기본 분기(0)로 간다. CD판은 비교하지 않았다.
// 이력: 2026-10-10 추가 — 클라이언트에 소리 옵션을 연결하면서(기계어를 직접 판독했고 x86 대조 도구는 아직 적용하지 않았다).
std::int32_t VolumeFromStep(std::int32_t step);

// Interpret Options가 읽는 소리 관련 설정을 한곳에 모은 값이다. 기본값은 원본 setup.cfg·options.cfg의 기본값이다.
// 사용: Client가 설정 객체에서 읽어 채우고 ClientAudio::Initialize·Interpret에 넘긴다.
// 이력: 2026-10-10 추가.
struct ClientAudioOptions {
    // 효과음 사용 여부(설정 sound, DAT_0054daa0)와 음악 사용 여부(설정 music, DAT_00531880)다.
    bool sound{true},music{true};
    // 장치 음질 0~3(설정 soundQuality, DAT_005318fc). 3은 22,050Hz 스테레오 16비트다.
    std::int32_t quality{3};
    // 효과음·음악 음량 단계(설정 soundVolume 기본 3, musicVolume 기본 2). VolumeFromStep으로 장치 음량이 된다.
    std::int32_t soundVolumeStep{3},musicVolumeStep{2};
    // 동시에 재생하는 효과음의 한도(설정 maxSimulSounds, 기본 8). 0이면 한도가 없다.
    std::int32_t maxSimulSounds{8};
    // 좌우 스피커를 바꾼다(설정 swapLeftRightSpeakers, DAT_0054db8c).
    bool swapSpeakers{};
};

// 클라이언트가 소유하는 소리·음악 묶음이다. 한 프로세스에 하나만 만든다.
// 사용: 생성자는 어떤 OS 자원도 열지 않는다. Initialize(창 핸들, 옵션)가 장치→미리 적재→음악 스레드 순서로 연다.
//       메인 루프는 매 프레임 CountPlaying()과 SceneFrame()을 부르고, 장면이 바뀌면 StartScene()·RequestMusic()을 부른다.
//       옵션이 바뀌면 Interpret()를 다시 부른다. 소멸자가 음악 스레드를 합류한 뒤 장치를 닫는다.
//       주 스레드에서만 부른다(음악 스레드와는 SoundMusic·장치의 잠금으로 분리돼 있다).
// 범위: 화면 경계는 생성자의 external로 연결한다. 희생·대기실·번개 효과는 클라이언트에서 아직 비어 있다.
//       효과음의 화면 영역·카메라는 SetView로 갱신한다. 현재 GUI의 표시 영역을 쓰며 원본 가변 패널 배치는 후속이다.
// 이력: 2026-10-10 추가.
class ClientAudio {
public:
    // gameDirectory 아래의 sound·music 폴더를 쓴다. language는 효과음 언어 폴더 이름(보통 "english"),
    // wallSeconds는 정지와 무관하게 흐르는 실시간 시계(초), randomSeed는 첫 원소 곡을 고르는 난수의 시작 상태다.
    // external은 화면/월드 경계다. 미제공 기능은 기본값으로 비우며 팔레트 설정 조회와 로더는 한 쌍으로 제공한다.
    ClientAudio(std::filesystem::path gameDirectory,o::OriginalEdition edition,std::string language,
        std::function<double()> wallSeconds,std::uint32_t randomSeed,SceneMusicHooks external={});
    // 음악 스레드를 합류하고 장치를 닫는다. 이미 닫혔으면 아무 일도 하지 않는다.
    ~ClientAudio();
    // 내부 객체가 서로의 주소를 보관하므로 복제·이동하지 않는다.
    ClientAudio(const ClientAudio&)=delete;
    ClientAudio& operator=(const ClientAudio&)=delete;
    // 원본 "init sound"(004aa600 → 004a9520("thunderCrack.wav") → 004aadd0): 장치를 열고, 천둥 효과음을 미리 적재하고, 음악 스레드를 만든다.
    // 이어 옵션의 나머지(음량·한도·좌우 바꿈)를 적용한다. 장치가 없거나 초기화에 실패하면 소리 없이 계속 실행한다(false).
    // window는 살아 있는 창 핸들(DirectSound 협조 수준에 필요)이다. 이미 초기화됐으면 Interpret와 같다.
    bool Initialize(std::uintptr_t window,const ClientAudioOptions& options);
    // 원본 Interpret Options(00435220)의 소리 부분: 소리가 켜져 있으면 장치를 열고 아니면 닫는다 → 효과음 음량 → 음악 음량 →
    // 현재 곡 이름을 음악 옵션으로 다시 선택한다(꺼졌으면 정지, 켜졌고 멈춰 있으면 다시 시작).
    // 사용: 옵션 메뉴에서 소리·음악·음량·좌우 바꿈을 바꾼 뒤 부른다.
    void Interpret(std::uintptr_t window,const ClientAudioOptions& options);
    // 음악 스레드를 합류하고 장치를 닫는다. 반복해서 불러도 된다. 이름 표와 현재 곡 이름은 보존한다.
    void Shutdown();
    // 메인 루프 3단계(원본 004a8e60): 재생 중인 효과음 수를 다시 센다.
    void CountPlaying();
    // 메인 루프 10단계(원본 00469f60): 곡이 끝났는지 확인하고 다음 곡을 요청한다. 게임 시계 정지와 무관하게 호출한다.
    void SceneFrame();
    // 장면에 들어가며 곡을 시작한다(원본 00469fc0). battle이 거짓이면 메뉴 곡 ser22.mus, 참이면 난수로 고른 원소 곡이다.
    void StartScene(bool battle);
    // 곡을 직접 요청한다(원본 00469db0). 결과 화면·희생 의식·대기실에서 쓴다. 현재 곡과 같으면 아무 일도 하지 않는다.
    void RequestMusic(std::string_view name);
    // 원본 다음 곡 요청(00469f00)이다. 곡 순환과 날씨를 함께 적용한다. 명시 검사 및 후속 장면 전환 호출자에서 쓴다.
    void NextSceneMusic();
    // 팔레트 로더가 채운 원소별 색 번호를 복사한다. 현재 tint는 바꾸지 않아 ApplyWeather의 팔레트 로드 전 대입 순서를 보존한다.
    void SetSceneTints(std::array<std::uint32_t,4> tints);
    // 효과음과 음악을 함께 음소거한다(원본 004aa900/004aa970 — 영상·안내 창이 열릴 때 쓰는 공유 음소거). 깊이가 0이 아닌 동안은 음량 옵션이 바뀌어도 예약만 된다.
    // 사용: 검사 실행이 장치와 음악 스트림을 실제로 돌리면서도 소리는 내지 않게 할 때 쓴다. PopMute가 마지막 해제에서 예약한 음량을 되돌린다.
    void PushMute();
    // PushMute 한 번을 되돌린다. 깊이가 0이면 아무 일도 하지 않는다.
    void PopMute();
    // 효과음 재생 계층이다. 월드가 소리를 낼 때 SoundList::Lookup으로 얻은 항목을 넘긴다.
    SoundPlayer& Sounds();
    // 효과음 이름 표다.
    SoundList& Names();
    // 위치 효과음이 읽는 화면 영역과 카메라를 교체한다. 주 스레드에서 화면 크기·카메라가 바뀐 직후 호출한다.
    // 재생 중인 반복 소리의 음량·좌우·화면 밖 정지는 다음 PlayLoopAt 요청이 처리한다. 장치 재초기화에도 이 값은 보존한다.
    // 원본: 00497220·004c6a40·004c6ac0·004c6b30이 읽는 카메라/화면 전역을 복원한 SoundState::view에 공급한다.
    void SetView(SoundView view);
    // 현재 위치 효과음의 화면·카메라를 읽는다. 검사 및 후속 가변 패널 연결에 사용한다.
    const SoundView& View() const;
    // 장면 음악의 상태(곡 색인·곡 끝 시각 등)다. 검사에서만 읽는다.
    const SceneMusicState& Scene() const;
    // 소리 장치가 열려 있으면 참이다.
    bool DeviceReady() const;
    // 검사용 상태 보고다. 장치·옵션·음량·지금 곡·채널 활성·곡 길이·재생 커서·음악 스레드 오류를 줄마다 `이름\t값`으로 적는다.
    std::string Describe();
private:
    // 장치를 열고(성공하면 true) 음악 스레드를 시작한다. 이미 열려 있으면 false다. Initialize·Interpret가 함께 쓴다.
    bool OpenDevice(std::uintptr_t window,int quality);
    // 음악 스레드를 합류한 뒤 장치를 닫는다(원본에 없는 순서 보장: 스레드가 닫힌 버퍼를 건드리면 안 된다).
    void CloseDevice();
    // 옵션 값을 상태와 음량에 적용한다(장치 열기·닫기는 하지 않는다).
    void ApplyValues(const ClientAudioOptions& options);

    // 효과음 이름 표와 재생 전역, 장치(동적 dsound.dll), 재생 계층이다. 선언 순서가 곧 생성 순서이며 소멸은 반대로 한다.
    SoundList list_;
    SoundState state_;
    SoundDevice device_;
    SoundPlayer player_;
    // 음악 파일 소유자와 두 채널(기본·보조), 음악 제어, 음악 스레드의 상태와 실행기다.
    MusicFileStore files_;
    MusicChannelState primaryState_,secondaryState_;
    MusicChannel primary_,secondary_;
    SoundMusic music_;
    MusicRuntimeState runtimeState_;
    MusicRuntime runtime_;
    // 현재 곡 이름·음악 옵션, 곡 선택 계층이다.
    MusicSelectionState selected_;
    MusicSelection selection_;
    // 장면 음악의 난수·상태·감독이다.
    o::GameRandom random_;
    SceneMusicState scene_;
    SceneMusic sceneMusic_;
    // 음악 스레드를 시작했는지(Shutdown 전까지 참)와 마지막으로 적용한 옵션이다.
    bool runtimeRunning_{};
    ClientAudioOptions options_;
};
}
