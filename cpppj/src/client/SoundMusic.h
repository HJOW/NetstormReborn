// 원본 음악 채널의 파일 읽기·시작·정지·버퍼 갱신과 공유 음소거를 복원한다. 파일 열기/작업 스레드/클라이언트 부착은 후속이다.
#pragma once
#include "client/Sound.h"
#include <array>
#include <mutex>

namespace netstorm::client {
// 음악 채널의 x86 상태 크기다. 호스트의 mutex/COM 포인터는 이 바이트 배열에 넣지 않는다.
inline constexpr std::size_t kMusicChannelBytes=0x60;
// 원본 음악 버퍼의 기능 비트와 크기다. CreateSoundBuffer에 넘기며 PCM 형식과 무관하게 크기는 고정이다.
inline constexpr std::uint32_t kMusicBufferFlags=0xe2,kMusicBufferBytes=0x2af80;
// 원본 음악 채널의 DWORD 위치다. +0x18의 길이는 double 두 DWORD, +0x28은 WAVEFORMATEX 18바이트다.
// Flags 비트 1은 채널 활성, 2는 파일 반복이다. StopDepth는 중첩 정지 중 같은 버퍼의 재정지를 막는다.
enum class MusicField : std::uint32_t { Buffer=0,BufferBytes=4,Flags=8,File=0xc,Length=0x10,
    Duration=0x18,ReadOffset=0x20,WriteOffset=0x24,Format=0x28,DataOffset=0x3c,StopDepth=0x40 };
// 원본 채널 배치의 상태다. 버퍼/파일은 장치 경계의 32비트 토큰이고, 남은 바이트는 수정하지 않은 원본 필드를 보존한다.
// 사용: 파일 적재가 File/Length/Format/DataOffset/Duration, 버퍼 확보가 Buffer/BufferBytes를 채운 뒤 채널을 시작한다.
class MusicChannelState {
public:
    // 전체 x86 상태의 읽기/쓰기 보기다. 실제 장치 포인터나 실제 CRITICAL_SECTION을 여기에 넣으면 안 된다.
    std::span<std::uint8_t> Raw();
    // 검사·관찰용 읽기 전용 보기다.
    std::span<const std::uint8_t> Raw() const;
    // DWORD를 비정렬 little endian으로 읽는다. 범위 밖 필드는 예외다.
    std::uint32_t Field(MusicField field) const;
    // DWORD를 little endian으로 쓰며 나머지 바이트는 보존한다.
    void SetField(MusicField field,std::uint32_t value);
private:
    // 새 채널의 상태는 0으로 시작한다. 초기화/파일 적재의 전체 수명은 후속 계층이 맡는다.
    std::array<std::uint8_t,kMusicChannelBytes> raw_{};
};
// 음악 파일/버퍼 경계다. 생성자에는 필수 여섯 함수를 연결한다. seek/read/close는 원본 mmio 의미이며 파일을 새로 열지 않는다.
// 사용: read는 실제 읽은 바이트 수(-1=실패), seek는 새 위치(-1=실패), release는 원본 DWORD 반환을 signed로 읽은 값이다.
struct MusicChannelHooks {
    // 기존 효과음 버퍼와 같은 음량/정지 COM 호출이다. 반환 음량 HRESULT가 음수면 원본 문장을 기록한다.
    std::function<std::int32_t(SoundBuffer,std::int32_t)> setVolume;
    std::function<void(SoundBuffer)> stop;
    // 버퍼 COM 참조를 해제하고 토큰 매핑에서 제거한다. 보통 반환값은 남은 참조 수다.
    std::function<std::int32_t(SoundBuffer)> release;
    // 파일 토큰과 data 시작 위치를 받는 절대 seek다. 원본은 파일을 읽기 전용으로 연다.
    std::function<std::int32_t(std::uint32_t,std::uint32_t)> seek;
    // 파일에서 목적 span 크기만큼 읽기를 요청한다. 부분 읽기도 요청 크기와 다른 반환값으로 알린다.
    std::function<std::int32_t(std::uint32_t,std::span<std::uint8_t>)> read;
    // 파일 토큰을 닫는다. 원본 실패 경로는 닫은 뒤에도 상태의 File 값을 남긴다.
    std::function<void(std::uint32_t)> close;
    // 선택 기록/보고 경계와 재귀 잠금 관찰자다. lockEvent는 예외를 던지지 않아야 하며 실제 잠금은 채널이 소유한다.
    std::function<void(std::uint32_t,std::string_view)> log;
    SoundAssertReport report;
    std::function<void(bool entering)> lockEvent;
};
// DirectSound Lock의 두 쓰기 구간이다. Unlock까지 유효하며 첫 구간 뒤에 링 버퍼 시작 구간이 이어질 수 있다.
struct MusicBufferRegions { std::span<std::uint8_t> first,second; };
// 음악 버퍼의 생성/갱신 COM 경계다. HRESULT는 음수일 때 실패하며 status/cursor/lock 출력은 성공 시 유효하다.
// 사용: create는 원본 형식 18바이트와 기능 비트/크기를 받고 버퍼 토큰을 쓴다. lock의 두 span 합은 요청 길이여야 한다.
struct MusicBufferHooks {
    // x86 DSBUFFERDESC를 호스트 구조체로 바꾸어 버퍼를 만든다. 실패 때 출력 토큰 변경도 원본 상태에 반영된다.
    std::function<std::int32_t(std::span<const std::uint8_t>,std::uint32_t,std::uint32_t,SoundBuffer&)> create;
    // 재생/손실/반복 상태 비트를 읽는다.
    std::function<std::int32_t(SoundBuffer,std::uint32_t&)> status;
    // 손실된 버퍼를 복구한다.
    std::function<std::int32_t(SoundBuffer)> restore;
    // 현재 재생/쓰기 커서다. 원본의 갱신 길이 계산에는 재생 커서만 쓴다.
    std::function<std::int32_t(SoundBuffer,std::uint32_t&,std::uint32_t&)> cursor;
    // 지정 구간을 잠그고 두 span을 돌려준다. 정상 반환 뒤에는 예외 경로도 unlock한다.
    std::function<std::int32_t(SoundBuffer,std::uint32_t,std::uint32_t,MusicBufferRegions&)> lock;
    // 잠금 때 받은 span을 그대로 해제한다. IO 예외 정리 중에도 예외를 던지지 않아야 한다.
    std::function<std::int32_t(SoundBuffer,MusicBufferRegions)> unlock;
    // flags=1로 링 버퍼를 반복 재생한다. 파일 반복 플래그와 별개다.
    std::function<std::int32_t(SoundBuffer,std::uint32_t)> play;
};
// 한 음악 채널의 제어다. 원본 CRITICAL_SECTION과 같은 재귀 잠금으로 파일/버퍼 상태를 보호한다.
// 사용: state/hooks 대상은 이 객체보다 오래 살아야 한다. 소멸자는 파일/버퍼를 닫지 않으므로 수명 끝에 Stop을 명시 호출한다.
class MusicChannel {
public:
    // 장치를 열지 않고 상태/판본/필수 파일·버퍼 경계를 보관한다.
    MusicChannel(o::OriginalEdition edition,MusicChannelState& state,MusicChannelHooks hooks);
    // 현재 버퍼에 음량을 그대로 적용한다. 버퍼가 없어도 잠금/해제는 수행한다. 원본: 004a9fa0(CD는 음악 음량에 인라인).
    void SetVolume(std::int32_t volume);
    // data 시작으로 돌아가 성공하면 ReadOffset=0이다. 실패하면 파일을 닫고 잠금을 푼 뒤 기록한다. 원본: 004a9fe0 / CD 004377d0 호출 구간.
    bool Rewind();
    // 한 번 재생은 끝을 0으로 채우고 실제 읽은 길이를 반환한다. 반복은 한 번만 되감아 나머지를 읽는다(여러 바퀴로 확장하지 않는다).
    // 실패하면 0이며 부분 출력/이미 갱신한 위치는 남는다. 잘못된 길이/위치는 예외다. 원본: 004aa0e0 / CD 00439fb0.
    std::int32_t Read(std::span<std::uint8_t> target);
    // 되감기 성공 후 Flags=1 또는 3, WriteOffset=0으로 시작한다. 버퍼의 실제 Play는 후속 스트리밍 갱신이 수행한다.
    // Length/Buffer 0은 원본 assert 보고 뒤 계속한다. 원본: 004aaa40 / CD 004393b0 끝 인라인.
    bool Start(std::uint32_t loop);
    // 파일을 닫고 길이/duration/읽기 위치/format/data 위치만 지운다. 버퍼/flags/쓰기 위치/패딩은 유지한다. 원본: 004aa410 / CD 0043a130.
    void Close();
    // 재귀 정지 깊이가 0일 때 버퍼 정지→활성 비트 제거→버퍼 해제→파일 닫기→깊이 0 순서다. 원본: 004aa9c0 / CD 00439ea0.
    // 공개 정지에서도 WAVEFORMATEX 뒤의 패딩 WORD를 보존한다.
    void Stop();
    // 버퍼가 없을 때 원본 고정 크기로 생성한다. 기존 버퍼는 형식/크기를 바꾸지 않는다. 원본: 004aa040 / CD 004393b0 내부.
    bool EnsureBuffer(const MusicBufferHooks& hooks);
    // 활성 채널의 잃음 복구→음량→커서/구간 잠금→Read→Unlock→필요 시 반복 Play를 수행한다.
    // IO/COM 실패는 원본처럼 Stop하고 false다. 손상된 버퍼 크기/커서/구간은 예외다. 원본: 004aaad0 / CD 00439a20 내부.
    bool FillBuffer(const MusicBufferHooks& hooks,const SoundState& sound);
private:
    // 범위 전체를 잠금/해제로 감싸며 예외 경로도 잠금을 풀어 준다.
    struct Guard {
        MusicChannel& channel;
        // 재귀 잠금을 획득한다.
        explicit Guard(MusicChannel& owner);
        // 관찰자를 호출하고 재귀 잠금을 푼다.
        ~Guard();
    };
    // 요청 크기와 반환이 같아야 성공이다. 실패 시 닫지만 File 토큰은 지우지 않는다.
    bool ReadExact(std::span<std::uint8_t> target);
    // 선택 기록 경계에 채널 9의 원본 오류 문장을 전달한다.
    void Log(std::string_view text);
    // 상태 참조·경계·판본과 재귀 잠금이다. raw의 +0x44 잠금 바이트와 호스트 mutex를 분리한다.
    o::OriginalEdition edition_;MusicChannelState& state_;MusicChannelHooks hooks_;std::recursive_mutex mutex_;
};
// 효과음과 음악의 공통 음소거 제어다. 최초 PushMute에서 음량을 예약하고 마지막 PopMute에서 최신 예약값을 복원한다.
// 사용: 모두 같은 SoundState를 쓰는 effects/channel을 넘긴다. 음악 준비 전에도 전역 음량은 저장한다.
class SoundMusic {
public:
    // 외부의 효과음/음악 상태를 참조한다. OS 자원은 열지 않는다.
    SoundMusic(o::OriginalEdition edition,SoundState& state,SoundPlayer& effects,MusicChannel& channel);
    // 보류 중에는 예약 음악 음량만 바꾸고, 그 외에는 저장 후 음악 준비 상태에서 채널에 적용한다. 원본: 004aa5d0 / CD 00439dd0.
    void SetVolume(std::int32_t volume);
    // 깊이가 0이면 두 음량을 예약→효과음 -10000→음악 -10000, 그 뒤 DWORD 깊이를 증가시킨다. 원본: 004aa900 / CD 00438f10.
    void PushMute();
    // 양수 깊이만 줄이고 마지막 해제에서 두 예약 음량을 복원한다. 0/음수는 그대로다. 원본: 004aa970 / CD 00438f60.
    void PopMute();
    // 음악이 준비됐을 때 기본 채널을 정지한다. 원본: 004aad70 / CD 00439910.
    void Stop();
    // 음악 준비 상태에서 기본 채널을 한 번 갱신한다. CD 공개 갱신의 초기화 검사를 포함한다. 작업 스레드를 만들지 않는다.
    bool Update(const MusicBufferHooks& hooks);
private:
    // 판본별 음소거 해제 분기와 공유 전역, 실제 두 제어 계층을 참조한다.
    o::OriginalEdition edition_;SoundState& state_;SoundPlayer& effects_;MusicChannel& channel_;
};
}
