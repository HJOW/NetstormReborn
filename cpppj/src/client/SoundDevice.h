// 원본 Sound.cpp의 WAVE 읽기와 DirectSound 장치를 연결한다. 생성자는 장치를 열지 않는다.
#pragma once
#include "client/Sound.h"
#include "client/SoundMusic.h"
#include <array>
#include <filesystem>
#include <memory>
#include <optional>

namespace netstorm::client {
// 원본 mmio가 읽은 WAVEFORMATEX 18바이트, data 청크 위치와 표본이다. PCM 짧은 fmt의 bits/cbSize는 원본처럼 보정한다.
// 사용: ReadSoundWave의 결과를 버퍼 만들기에 전달한다. 장치와 무관하여 파일 검사에서도 쓸 수 있다.
struct SoundWave {
    std::array<std::uint8_t,18> format{};
    std::uint32_t dataOffset{};
    std::vector<std::uint8_t> samples;
};
// WAV를 읽기 전용 mmio로 열어 WAVE→fmt→data 순서로 읽는다. 누락/불완전 fmt/빈 data는 빈 값을 반환한다.
// Winmm이 파일/RIFF 끝까지 줄인 data 크기를 그대로 사용하며 실제 파일 범위를 넘으면 거부한다.
// 사용: report는 짧은 비PCM fmt의 원본 assert 보고다. 불완전한 헤더/비PCM 추가 코덱 자료는 안전하게 거부한다.
// 원본: 004a8b50/004a8cf0/004a8d30, CD 004375c0/00437790/00437800. 오디오 장치를 열지 않는다.
std::optional<SoundWave> ReadSoundWave(const std::filesystem::path& path,o::OriginalEdition edition,SoundAssertReport report={});
// 패턴을 실제 디스크 파일로 바꾸는 경계다. alternate=false는 주 경로, true는 보조/CD 경로다.
// 사용: 아카이브 항목은 mmio가 직접 읽을 수 없으므로 디스크 경로를 반환하는 resolver를 넘긴다.
using SoundFileResolver=std::function<std::optional<std::filesystem::path>(std::string_view pattern,bool alternate)>;
// 주/보조 디스크 경로에서 원본 FindFirstFile 방식으로 첫 파일을 찾는 resolver다. 원본 파일은 변경하지 않는다.
SoundFileResolver MakeDiskSoundResolver(std::filesystem::path primary,std::filesystem::path secondary={});
// 소리 이름을 "이름-*.확장자"와 "이름*.확장자"로 만들고 언어/기본·주/보조의 원본 여덟 경로 순서로 찾는다.
// 원본: 004a9170 / CD 00438000. soundDirectory는 원본 소리 디렉터리 설정(보통 "sound")이다.
std::optional<std::filesystem::path> FindSoundFile(std::string_view name,std::string_view soundDirectory,
    std::string_view language,const SoundFileResolver& resolver);
// 전체 경로의 첫 '-' 다음 signed 십진수를 원본 sscanf("%*[^-]-%ld.%*[^-]")처럼 읽는다. 읽지 못하면 0이다.
// 사용: 적재 성공/실패 표식이 0이 아니면 소리별 감쇠로 저장한다. 음수·공백·부분 숫자도 원본처럼 허용한다.
std::int32_t SoundFileAttenuation(std::string_view path);

// 동적 dsound.dll·주 버퍼·효과음/음악 버퍼를 소유하는 Windows 장치다. 원본 COM 순서와 장치 경계를 연결한다.
// 사용: list/state/resolver를 준비→Initialize(실제 HWND, 음질 0~3)→Hooks를 SoundPlayer에 전달→Shutdown.
//       이 객체는 Player/Process보다 오래 살아야 하고 list/state는 이 객체보다 오래 살아야 한다. 단일 스레드 전용이다.
//       Shutdown은 표의 버퍼만 비우며 이름/감쇠/사슬은 보존한다. 장치 포인터는 32비트 토큰으로 매핑하여 64비트 빌드를 지원한다.
// 범위: 효과음 수명과 음악 버퍼 COM 경계다. 초기화/종료의 음악 스레드 호출(004aadd0/004aaf00)은 후속 연결한다.
class SoundDevice {
public:
    // 자원 없이 만든다. resolver는 필수이고 디렉터리·언어는 파일 선택 순서에 사용한다.
    SoundDevice(SoundList& list,SoundState& state,SoundFileResolver resolver,
        std::string language="english",std::string soundDirectory="sound",SoundAssertReport report={});
    // 열린 모든 버퍼와 장치/DLL을 해제한다. 장치를 여는 동작은 없다.
    ~SoundDevice();
    // 소유 자원과 Hooks의 this 참조가 움직이지 않도록 복사·이동을 막는다.
    SoundDevice(const SoundDevice&)=delete;
    SoundDevice& operator=(const SoundDevice&)=delete;
    SoundDevice(SoundDevice&&)=delete;
    SoundDevice& operator=(SoundDevice&&)=delete;
    // DirectSoundCreate→DSSCL_PRIORITY→주 버퍼→음질 하향을 실행한다. 이미 초기화됐으면 false, 범위 밖 음질은 3이다.
    // window는 살아 있는 HWND의 정수값이다. 0이면 장치를 열기 전에 invalid_argument를 던진다.
    // 원본: 004aa600 / CD 00437820. 실패는 false이며 COM 생성 이후 실패하면 소리 옵션도 끈다.
    bool Initialize(std::uintptr_t window,int quality=3);
    // 버퍼·주 버퍼·장치·DLL을 해제하고 initialized/device/playing과 표의 버퍼 필드를 비운다. 반복 호출해도 된다.
    // 원본: 004a8ef0 / CD 00437d30. 원본에서 빠진 버퍼/DLL 해제도 소유권에 따라 처리한다.
    void Shutdown();
    // 원본 버퍼의 상태/재생/위치/음량/좌우/정지/복제와 WAV 적재를 호출하는 여덟 경계를 돌려준다.
    // load는 장치가 없으면 {0,0}, 파일/버퍼 실패이면 {kSilentSoundBuffer,감쇠}다. log는 호출자가 필요하면 연결한다.
    SoundDeviceHooks Hooks();
    // 음악 파일의 seek/read/close 경계에 이 장치의 음량/정지/참조 해제를 붙인다. 파일은 여기서 열지 않는다.
    // 사용: 연결한 MusicChannel은 장치 Shutdown 전에 Stop한다. Hooks/토큰은 이 장치의 수명 안에서만 유효하다.
    MusicChannelHooks BindMusicBuffers(MusicChannelHooks files);
    // 음악 버퍼의 생성·상태/복구·커서·잠금/해제·반복 재생을 실제 DirectSound COM에 연결한다.
    // 작업 스레드를 만들지 않으며 현재 SoundDevice와 같이 단일 스레드에서 호출한다.
    MusicBufferHooks MusicBuffers();
private:
    // Windows COM 포인터와 DLL 핸들을 숨기는 소유 구조체다.
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
