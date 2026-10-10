// Sound.cpp(원본 .\Sound.cpp / CD sound.cpp) 가운데 소리 이름 표(bigSoundList)와 재생 계층(전역/위치 재생·정지·재생 여부)을 복원한다.
// 소리 장치의 초기화·WAVE 읽기·버퍼 COM 호출은 SoundDevice.h의 실제 장치가 맡고 SoundDeviceHooks로 연결한다.
#pragma once
#include "o/RiftType.h"
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client {
// 소리 이름 표의 한 항목을 가리키는 값이다. 0은 "없음"이고, 그 밖의 값은 kSoundListBase + 표 안의 바이트 오프셋이다.
// 사용: SoundList::Lookup이 돌려준 값을 소리 프로세스와 재생 요청에 그대로 넘긴다. 호스트 메모리 주소가 아니다.
// 원본은 표 안 항목의 32비트 포인터를 쓴다. 포인터 산술(자기 참조, 이름 포인터 − 0x1f)을 같은 숫자로 재현하려고 고정 기준값을 더한다.
// 이력: 2026-10-10 추가 — 보호막 소리 조회를 실제 이름 표에 연결하기 위해.
using SoundHandle=std::uint32_t;
// 표 전체 크기(원본 new(0x8000))와, 새 항목을 더 넣지 않는 시작 오프셋 한도(0x7fb0)다.
// 사용: 마지막 빈 위치가 한도 이상이면 Lookup은 새 이름 대신 대체 소리를 돌려준다.
// 원본: 004a8d50 / CD 00437b29의 `list + 0x7fb0` 비교, 004aa600 / CD 00437820의 확보 크기.
// 이력: 2026-10-10 추가.
inline constexpr std::uint32_t kSoundListBytes=0x8000,kSoundListLimit=0x7fb0;
// 항목 머리 크기(이름은 +0x1c부터)와, 표 안을 가리키는 이름 포인터에서 빼는 값이다.
// 사용: 항목의 이름은 Raw()의 오프셋 + kSoundHeaderBytes에서 NUL까지다. kSoundPointerBias는 LookupPointer만 쓴다.
// 원본: 004a8d50의 `lea eax,[ebp-0x1f]` / CD 00437b71. 머리 크기 0x1c와 다른 0x1f를 그대로 따른다.
// 이력: 2026-10-10 추가.
inline constexpr std::uint32_t kSoundHeaderBytes=0x1c,kSoundPointerBias=0x1f;
// SoundHandle의 기준값이다. 기계어 대조 도구(tools/decomp_soundprocess_oracle.py)가 표를 올리는 가상 주소와 같아서
// 자기 참조 필드까지 포함한 표 전체 바이트를 원본 관찰과 그대로 비교할 수 있다.
// 사용: 항목 오프셋은 `handle - kSoundListBase`로 얻는다. 값을 바꾸면 대조 fixture의 표 Adler-32와 맞지 않는다.
// 이력: 2026-10-10 추가.
inline constexpr SoundHandle kSoundListBase=0x15000000;
// 소리 장치 초기화가 표에 처음 넣는 이름이다. 표가 가득 찼을 때 새 이름 대신 이 항목을 돌려준다.
// 원본: 004aa600 / CD 00437820의 "nonexistant.wav"(원본 철자 그대로).
// 이력: 2026-10-10 추가.
inline constexpr std::string_view kFallbackSoundName="nonexistant.wav";
// 원본 assert 보고(조건식, 줄 번호)를 받는 함수다. 원본은 보고한 뒤 실행을 계속하므로 이 함수가 돌아오면 그대로 진행한다.
// 사용: 연결하지 않으면 보고를 건너뛴다. 진단이 필요하면 기록하거나 예외를 던지는 함수를 준다.
// 원본: 004e0620 / CD 00490040 (조건식, 파일, 줄)의 cdecl 보고 함수.
// 이력: 2026-10-10 추가.
using SoundAssertReport=std::function<void(std::string_view expression,int line)>;
// 장치 버퍼를 가리키는 값이다(원본은 IDirectSoundBuffer 포인터). 0은 "아직 적재하지 않음"이다.
// 사용: 장치 경계(SoundDeviceHooks)가 만들어 돌려준 값을 항목의 +0에 보관하고 그대로 장치 호출에 넘긴다. 이 계층은 값을 해석하지 않는다.
// 이력: 2026-10-10 추가 — 재생 계층을 옮기면서.
using SoundBuffer=std::uint32_t;
// "파일이 없거나 버퍼를 만들지 못한 소리"를 뜻하는 표식이다. 항목의 +0이 이 값이면 재생·상태 조회·정지를 장치에 요청하지 않는다.
// 사용: 적재 경계가 실패를 알릴 때 이 값을 돌려준다. 재생 함수는 이 값을 만나면 아무것도 하지 않고 항목을 돌려준다.
// 원본: 전역 DAT_00542494 / CD 0051a6fc(초기값 0xffffffff, 다시 대입되지 않는다).
// 이력: 2026-10-10 추가.
inline constexpr SoundBuffer kSilentSoundBuffer=0xffffffff;
// 장치 음량/좌우 값의 범위다(1/100 dB). 음량은 [-10000, 0]으로, 위치 계산 결과는 [-10000, 10000]으로 자른다.
// 원본: 004a9750 등의 0xffffd8f0 비교, 004c6a90의 0x2710 비교.
// 이력: 2026-10-10 추가.
inline constexpr std::int32_t kSoundMinimum=-10000,kSoundMaximum=10000;
// 항목 머리의 DWORD 필드 위치다.
//   Buffer      +0x00 장치 버퍼(0 = 미적재, kSilentSoundBuffer = 소리 없음)
//   Root        +0x04 같은 이름의 첫 항목(자기 자신이면 원본 항목)
//   Next        +0x08 같은 소리를 겹쳐 재생하려고 만든 다음 복제 항목(없으면 0)
//   Attenuation +0x0c 파일 이름의 "-숫자"에서 읽는 소리별 감쇠. 복제 항목에는 쓰지 않는다(0으로 남는다)
//   Volume      +0x10 마지막으로 재생/갱신할 때의 음량 인자
//   Serial      +0x14 재생을 시작할 때마다 매기는 일련번호
//   Priority    +0x18 재생할 때의 우선 인자
// 사용: SoundList::Field/SetField에 준다. 재생 계층과 장치 계층, 검사가 쓴다.
// 원본: 004a9727~004a974b / CD 0043864c~00438668의 필드 쓰기.
// 이력: 2026-10-10 추가.
enum class SoundField : std::uint32_t { Buffer=0x00,Root=0x04,Next=0x08,Attenuation=0x0c,Volume=0x10,Serial=0x14,Priority=0x18 };

// 소리 파일 이름을 항목으로 바꾸는 이름 표다. 원본 bigSoundList와 같은 0x8000바이트 배치를 그대로 쓴다.
// 사용: 만든 뒤 Initialize()를 한 번 부르고, 소리 이름마다 Lookup()으로 항목 값을 얻어 보관한다. 같은 이름은 같은 값을 돌려준다.
//       항목은 지워지지 않으며 표는 이 객체가 소유한다. 여러 스레드에서 함께 쓰지 않는다.
// 항목 배치: 머리 0x1c바이트(SoundField 참고) 뒤에 +0x1c 이름(NUL 종료). Lookup이 만든 새 항목은 +0x10을 쓰지 않는다(원본과 같다).
//       재생 계층(SoundPlayer)이 Field/SetField/AppendDuplicate로 머리 필드와 복제 항목을 다룬다.
// 원본: 전역 bigSoundList(DAT_005c7b44 / CD 0051a74c), firstFreeSound(DAT_005c7b48 / CD 0051a750),
//       대체 소리(DAT_005c7b4c / CD 0051a754).
// 이력: 2026-10-10 추가 — 보호막의 priestForceField.wav 조회를 실제 표로 바꾸기 위해.
class SoundList {
public:
    // 0으로 채운 빈 표를 만든다. report는 원본 assert 보고를 받을 함수이며 없어도 된다.
    // 원본: 004aa600 / CD 00437820의 new(0x8000)·0 채움·firstFreeSound 대입 부분.
    // 이력: 2026-10-10 추가.
    explicit SoundList(o::OriginalEdition edition,SoundAssertReport report={});
    // 아직 하지 않았으면 대체 소리 "nonexistant.wav"를 첫 항목으로 등록한다. 여러 번 불러도 한 번만 등록한다.
    // 원본: 004aa600 / CD 00437820에서 표를 처음 확보한 직후의 조회와 대체 소리 전역 대입.
    // 역할: 이름 표만 준비한다. 같은 원본 함수의 DirectSound 초기화는 SoundDevice::Initialize가 맡는다.
    // 이력: 2026-10-10 추가.
    void Initialize();
    // 이름을 ASCII 대소문자 구분 없이 찾아 항목 값을 돌려준다. 없으면 표 끝에 추가한다.
    // 표가 가득 찼으면(마지막 빈 위치 ≥ 0x7fb0) 대체 소리 값을 돌려준다(Initialize 전이면 0).
    // 사용: name은 표 밖의 문자열이어야 하며 비어 있거나 NUL을 포함하면 std::invalid_argument를 던진다.
    //       추가할 이름이 표 끝을 넘으면 std::length_error를 던진다(원본은 검사 없이 넘겨 쓴다).
    // 판본 차이: CD판은 순회하는 기존 항목 이름의 마지막 글자가 'v'가 아니면 "s->isRobust()"(줄 438)를 보고한다.
    // 원본: 004a8ee0 → 004a8d50 / CD 00437d20 → 00437b29. 비교는 C 로캘의 _stricmp(ASCII A~Z만 접는다).
    // 이력: 2026-10-10 추가.
    SoundHandle Lookup(std::string_view name);
    // C 문자열 이름을 받는 원본 진입 그대로의 형태다. name이 null이면 표를 보지 않고 0을 돌려준다.
    // 원본: 004a8d7d / CD 00437b58의 null 검사.
    // 이력: 2026-10-10 추가.
    SoundHandle Lookup(const char* name);
    // 표 안을 가리키는 포인터 값을 받는 원본 경로다. 표를 읽지 않고 pointer − 0x1f를 돌려준다.
    // 사용: pointer는 kSoundListBase 이상, kSoundListBase + 0x8000 미만이어야 한다(아니면 std::out_of_range).
    //       결과가 항목의 시작이 아닐 수 있으므로 원본 호출 관계를 옮길 때만 쓴다.
    // 원본: 004a8d50 / CD 00437b29의 표 범위 검사 분기.
    // 이력: 2026-10-10 추가.
    SoundHandle LookupPointer(SoundHandle pointer) const;
    // 대체 소리 항목 값이다. Initialize 전에는 0이다.
    SoundHandle Fallback() const;
    // 다음 새 항목이 들어갈 위치(항목 값 형식)다.
    SoundHandle FirstFree() const;
    // 항목의 이름을 돌려준다. entry가 표 안 항목의 시작이 아니면 결과는 의미가 없고, 표 밖이면 std::out_of_range를 던진다.
    // 돌려준 문자열은 이 객체가 살아 있는 동안 유효하다.
    std::string_view Name(SoundHandle entry) const;
    // 표 전체 바이트(0x8000)다. 검사와 재생 계층이 읽는다.
    std::span<const std::uint8_t> Raw() const;
    // 항목 머리의 DWORD 필드를 읽는다. entry가 표 안 항목 위치가 아니면 std::out_of_range를 던진다.
    // 이력: 2026-10-10 추가 — 재생 계층을 옮기면서.
    std::uint32_t Field(SoundHandle entry,SoundField field) const;
    // 항목 머리의 DWORD 필드를 쓴다. 재생 계층과 장치 계층(버퍼 적재/해제)이 쓰며 값의 뜻은 검사하지 않는다.
    // 이력: 2026-10-10 추가.
    void SetField(SoundHandle entry,SoundField field,std::uint32_t value);
    // 같은 소리를 겹쳐 재생할 복제 항목을 표 끝에 만든다. named의 이름을 복사하고, previous의 Root를 물려받아 previous의 Next에 잇는다.
    // 새 항목의 Buffer는 buffer, Next는 0이며 나머지 머리 필드(감쇠 포함)는 쓰지 않는다. 돌려주는 값은 새 항목이다.
    // 사용: 재생 계층만 부른다. 원본과 달리 표 끝을 넘으면 std::length_error를 던진다(원본은 한도 검사 없이 쓴다).
    // 원본: 004a96e7~004a9721 / CD 0043859f~004385f0.
    // 이력: 2026-10-10 추가.
    SoundHandle AppendDuplicate(SoundHandle named,SoundHandle previous,SoundBuffer buffer);
    // 이 표가 따르는 판본이다(보고 줄 번호 등 판본 차이에 쓴다).
    o::OriginalEdition Edition() const;
private:
    // 표 오프셋에서 NUL까지의 이름을 읽는다. 표 끝을 넘으면 끝에서 자른다.
    std::string_view NameAt(std::uint32_t offset) const;
    // 표 오프셋에 리틀 엔디언 DWORD를 쓴다.
    void Put(std::uint32_t offset,std::uint32_t value);
    // 보고 함수가 연결돼 있으면 원본 assert 보고를 전달한다.
    void Report(std::string_view expression,int line) const;
    o::OriginalEdition edition_;
    SoundAssertReport report_;
    std::vector<std::uint8_t> raw_;
    std::uint32_t firstFree_{};
    SoundHandle fallback_{};
    bool initialized_{};
};

// 화면에 보이는 영역과 카메라 원점이다. 위치 재생의 화면 안 판정과 음량/좌우 계산에 쓴다.
// 사용: 클라이언트가 화면 크기나 카메라가 바뀔 때 갱신한다. right − left가 2 이상이어야 한다(아니면 계산 함수가 std::domain_error를 던진다).
// 원본: 카메라 원점 DAT_0059a91c/0059a920 (CD 00565c34/00565c38), 화면 영역 DAT_005ca9cc~005ca9d8 (CD 00583e20~00583e2c).
// 이력: 2026-10-10 추가.
struct SoundView { std::int32_t cameraX{},cameraY{},left{},top{},right{640},bottom{480}; };
// 화면 픽셀 좌표의 한 점이다.
// 이력: 2026-10-10 추가.
struct SoundPoint { std::int32_t x{},y{}; };
// 월드 좌표(칸 단위)를 화면 픽셀로 바꾼다: x × 16 + 0.5, y × 11 + 0.5를 0쪽으로 절삭해 카메라 원점을 뺀다.
// 절삭은 원본 CRT처럼 64비트로 한 뒤 하위 32비트를 쓴다(범위를 넘거나 NaN이면 0).
// 원본: 00497220 / CD 00455c90 (소속 미확인 — Sound.cpp가 아니라 좌표 도우미다).
// 이력: 2026-10-10 추가.
SoundPoint SoundToScreen(const SoundView& view,float x,float y);
// 점이 화면 영역에서 좌우 40, 상하 30픽셀 여유 안에 있는지 본다. 위치 재생은 이 안에 있을 때만 소리를 낸다.
// 원본: 004c6b30 / CD 004cdeb0.
// 이력: 2026-10-10 추가.
bool SoundOnScreen(const SoundView& view,SoundPoint point);
// 점의 좌우 치우침을 장치 좌우 값으로 바꾼다: 화면 가로 중심에서의 거리 c에 대해 ±10000 × c² ÷ (가로 절반)², [-10000, 10000].
// swap이 참이면 좌우를 뒤집는다. 곱셈은 원본처럼 32비트로 넘친다(화면에서 아주 먼 점의 값은 물리적 의미가 없다).
// 원본: 004c6a40 / CD 004cdd00, 옵션 swapLeftRightSpeakers.
// 이력: 2026-10-10 추가.
std::int32_t SoundPan(const SoundView& view,SoundPoint point,bool swap);
// 점이 화면 중심에서 먼 정도를 음량 감쇠로 바꾼다: −1000 × (dx² + dy²) ÷ (가로 절반)², [-10000, 10000]. 곱셈은 32비트로 넘친다.
// 원본: 004c6ac0 / CD 004cde00.
// 이력: 2026-10-10 추가.
std::int32_t SoundVolume(const SoundView& view,SoundPoint point);

// 재생 계층이 읽고 쓰는 원본 전역이다. 값은 클라이언트가 소유한다.
// 사용: 장치 초기화가 성공하면 initialized·device를 켜고 옵션에서 enabled·maxPlaying·swapSpeakers를 채운다. SoundPlayer보다 오래 살아야 한다.
// 이력: 2026-10-10 추가.
struct SoundState {
    // 소리 장치 초기화가 끝났는지(DAT_005c7b08 / CD 0051a6f0)와 장치 객체가 있는지(DAT_005c7b0c / CD 0051a6f4 != 0)다.
    bool initialized{},device{};
    // 옵션 sound(DAT_0054daa0 / CD 0052e824)와 swapLeftRightSpeakers(DAT_0054db8c / CD 0052e934)다.
    bool enabled{},swapSpeakers{};
    // 옵션 maxSimulSounds(DAT_005424a8 / CD 0051a73c, 기본 8). 0이면 한도가 없다. 우선 재생은 이 한도를 건너뛴다.
    std::int32_t maxPlaying{8};
    // 지금 재생 중이라고 센 버퍼 수다(DAT_005c7b38 / CD 0051a738). 재생/정지로 증감하며 자연 종료한 소리는 Recount로 다시 센다.
    std::int32_t playing{};
    // 전체 효과음 음량이다(DAT_005c7b20 / CD 0051a720, 1/100 dB).
    std::int32_t masterVolume{};
    // 음량 변경 보류 깊이(DAT_005c7b28 / CD 0051a728). 0 이외에는 전체 음량을 적용하지 않고 아래 예약값만 바꾼다.
    // 음악과 공유하는 원본 음소거 호출 계층이 관리할 값이다. 음수도 변경을 보류하며, bool로 축약하지 않는다.
    std::int32_t volumeHoldDepth{};
    // 보류 중 가장 최근에 요청한 효과음 음량이다(DAT_005c7b2c / CD 0051a72c). 보류가 없을 때의 변경은 이 값을 건드리지 않는다.
    std::int32_t pendingMasterVolume{};
    // 음악 갱신 계층 준비 상태(DAT_005c7b5c / CD 0051a764). 효과음 장치 준비와 별도이며 작업 스레드 초기화가 설정한다.
    bool musicInitialized{};
    // 현재 음악 음량(DAT_005c7b24 / CD 0051a724)과 보류 중 최신 요청(DAT_005c7b30 / CD 0051a730)이다.
    std::int32_t musicVolume{},pendingMusicVolume{};
    // 다음 재생에 매길 일련번호다(DAT_005c7b34 / CD 0051a734).
    std::uint32_t serial{};
    // 화면 영역과 카메라 원점이다.
    SoundView view;
};
// 적재 경계의 결과다. buffer는 새 장치 버퍼·kSilentSoundBuffer(파일 없음)·0(장치 없음) 가운데 하나이고,
// attenuation은 파일 이름의 "-숫자"에서 읽은 감쇠다(없으면 0). buffer가 0이면 attenuation은 쓰이지 않는다.
// 원본: 004a9170 / CD 00438000이 항목의 +0과 +0xc에 쓰는 값.
// 이력: 2026-10-10 추가.
struct SoundLoad { SoundBuffer buffer{};std::int32_t attenuation{}; };
// 재생 계층의 장치 경계다. 원본은 이 자리에서 DirectSound 버퍼의 COM 메서드를 직접 부른다. log를 뺀 여덟 개는 모두 연결해야 한다.
// 사용: 장치 계층(또는 검사용 상태표)을 넣어 SoundPlayer에 넘긴다. 반환값은 원본 HRESULT 그대로다(음수 = 실패).
// 이력: 2026-10-10 추가.
struct SoundDeviceHooks {
    // GetStatus(+0x24): 비트 1 재생 중, 2 버퍼를 잃음, 4 반복 재생 중.
    std::function<std::uint32_t(SoundBuffer)> status;
    // Play(+0x30)의 flags(1 = 반복). 앞의 두 인자는 원본에서 항상 0이다.
    std::function<void(SoundBuffer,std::uint32_t)> play;
    // SetCurrentPosition(+0x34).
    std::function<void(SoundBuffer,std::uint32_t)> setPosition;
    // SetVolume(+0x3c), SetPan(+0x40).
    std::function<std::int32_t(SoundBuffer,std::int32_t)> setVolume,setPan;
    // Stop(+0x48).
    std::function<void(SoundBuffer)> stop;
    // IDirectSound::DuplicateSoundBuffer(+0x14): 성공하면 copy에 새 버퍼를 쓴다.
    std::function<std::int32_t(SoundBuffer original,SoundBuffer& copy)> duplicate;
    // 항목 적재(wav 찾기/읽기/버퍼 만들기). 인자는 항목의 이름이다.
    std::function<SoundLoad(std::string_view name)> load;
    // 기록 출력(채널, 완성된 문장). 없으면 건너뛴다. 원본: 004c2630 / CD 0048ca70.
    std::function<void(std::uint32_t channel,std::string_view text)> log;
};

// 소리 항목의 재생·정지·재생 여부를 맡는 재생 계층이다.
// 사용: 월드마다(실제로는 클라이언트에 하나) 만들고, SoundList::Lookup으로 얻은 항목을 넘겨 재생한다.
//       list·state·장치 효과 대상은 이 객체보다 오래 살아야 한다. 필수 장치 효과가 빠져 있으면 std::invalid_argument를 던진다.
//       재생 함수가 돌려준 값은 "실제로 재생에 쓴 항목"이며 요청한 항목과 다를 수 있다(같은 소리가 이미 재생 중이면 복제 항목을 쓴다).
// 판본 차이: 기록 문장/보고 줄 번호와 CD의 전체 표 이름 견고성 검사를 보존한다. 패치판의 변조 감지용 고의 고장(DAT_005318ec == 7이면 null 버퍼로 재생)은 옮기지 않았다.
// 이력: 2026-10-10 추가.
class SoundPlayer {
public:
    // list는 항목을 담은 이름 표, state는 재생 전역, hooks는 장치 경계, report는 assert 보고를 받을 함수(없어도 된다)다.
    // log를 뺀 장치 효과가 하나라도 비어 있으면 std::invalid_argument를 던진다.
    SoundPlayer(SoundList& list,SoundState& state,SoundDeviceHooks hooks,SoundAssertReport report={});
    // 항목을 재생한다. loop가 0이 아니면 반복 재생, volume은 음량 인자(전체 음량과 소리별 감쇠를 더해 [-10000, 0]으로 자른다), pan은 좌우 값이다.
    // priority가 0이면 동시 재생 한도(maxPlaying)에 걸릴 때 재생하지 않는다. limit가 0이 아니면 이 소리가 이미 limit개 이상 재생 중일 때 재생하지 않는다.
    // 요청한 항목이 재생 중이면 같은 소리의 사슬에서 쉬고 있는 항목을 쓰고, 없으면 버퍼를 복제해 새 항목을 만든다.
    // 반환: 재생에 쓴 항목. 준비되지 않았거나 한도에 걸리면 0. 소리 없음 표식이거나 복제에 실패하면 재생하지 않고 관련 항목을 돌려준다.
    // 원본: 004a9550 / CD 004383f0.
    // 이력: 2026-10-10 추가.
    SoundHandle Play(SoundHandle sound,std::uint32_t loop,std::int32_t volume,std::int32_t pan,std::uint32_t priority,std::int32_t limit);
    // 월드 좌표 (x, y)에서 나는 반복 소리를 한 프레임 처리한다. current는 지난 호출이 돌려준 항목(없으면 0), sound는 내고 싶은 소리다.
    // 이름이 다르면 current를 멈추고 sound로 바꾼다. 화면 안이고 재생 중이 아니면 Play(소리, 반복, 0, 0, 우선 없음, 한도 1)로 시작한다.
    // 화면 밖이면 재생 중인 소리를 멈추고 0을 돌려준다. 재생 중이면 위치에 맞춰 음량/좌우를 다시 정한다.
    // 반환: 계속 쥐고 있을 항목(다음 호출의 current). 소리 옵션이 꺼졌거나 장치가 없으면 0.
    // 원본: 004a9860 / CD 00438800 (패치판은 Play가 인라인돼 있다).
    // 이력: 2026-10-10 추가.
    SoundHandle PlayLoopAt(float x,float y,SoundHandle current,SoundHandle sound);
    // 월드 좌표 (x, y)가 화면 안이면 위치에 맞는 음량/좌우로 Play(sound, loop, 음량, 좌우, priority, 0)를 부른다. 화면 밖이면 0이다.
    // 원본: 004a9c30 / CD 00438ae0.
    // 이력: 2026-10-10 추가.
    SoundHandle PlayOnceAt(float x,float y,SoundHandle sound,std::uint32_t loop,std::uint32_t priority);
    // 항목이 재생 중이면 멈추고 처음 위치로 되돌린다. 0·미적재·소리 없음 항목과 재생 중이 아닌 항목은 그대로 둔다.
    // 원본: 004a97e0 / CD 004386f0.
    // 이력: 2026-10-10 추가.
    void Stop(SoundHandle sound);
    // 항목이 지금 재생 중이면 1이다. 준비되지 않았거나 0·미적재·소리 없음 항목이면 0이다.
    // 원본: 004a9810 / CD 004387a0.
    // 이력: 2026-10-10 추가.
    std::uint32_t IsPlaying(SoundHandle sound);
    // 이름으로 항목을 찾아(없으면 등록해) Play를 부른다. loop가 0이 아닌데 그 이름에 복제 항목이 있으면 "qq"(패치 줄 1007 / CD 1002)를 보고한다.
    // 원본: 004a9cb0 / CD 00438b50.
    // 이력: 2026-10-10 추가.
    SoundHandle PlayByName(std::string_view name,std::uint32_t loop,std::int32_t volume,std::int32_t pan,std::uint32_t priority,std::int32_t limit);
    // 이름으로 항목을 찾아 PlayOnceAt(x, y, 항목, loop, 우선 없음)을 부른다.
    // 원본: 004a9d70 / CD 00438c00.
    // 이력: 2026-10-10 추가.
    SoundHandle PlayNameAt(float x,float y,std::string_view name,std::uint32_t loop);
    // 전체 표의 실제 버퍼 상태를 다시 세어 playing을 대입하고 반환한다. 자연 종료한 효과음의 누적 재생 수를 바로잡을 때 호출한다.
    // 초기화·옵션 여부와 무관하게 실행하며 미적재·소리 없음 버퍼는 제외한다. CD는 모든 이름의 마지막 글자 'v'를 검사한다(줄 0x1e7).
    // 원본: 004a8e60 / CD 00437c80.
    std::int32_t Recount();
    // 이름을 조회/등록하고 유일한 항목인지 보고한 뒤 원본 항목만 정지한다. 복제 항목들을 정지하지 않는다.
    // 복제 항목이 있으면 "sound->isUnique()"를 보고한다(패치 줄 0x3f8 / CD 0x3f3). 옵션·초기화 여부로 건너뛰지 않는다.
    // 원본: 004a9d20 / CD 00438bc0.
    void StopByName(std::string_view name);
    // 이름을 먼저 조회/등록하고 IsPlaying으로 원본 항목의 상태를 반환한다. 소리 옵션이 꺼져 있어도 이름은 등록된다.
    // 원본: 004a9da0 / CD 00438c30.
    std::uint32_t IsPlayingByName(std::string_view name);
    // on이 0이면 이름으로 정지하고, 그 밖에는 원본 항목이 멈춰 있을 때만 이름으로 재생한다(좌우·우선·한도 인자 모두 0).
    // 초기화 전·옵션 꺼짐·null 이름이면 조회 없이 0. 그 외에는 재생 성공 여부와 무관하게 on 값을 그대로 반환한다.
    // 원본: 004a9de0 / CD 00438c80. name은 NUL 종료 문자열이며 빈 이름은 SoundList의 계약에 따라 거부한다.
    std::int32_t SetNamePlaying(std::int32_t on,const char* name,std::uint32_t loop,std::int32_t volume);
    // 전체 표에서 재생 비트와 반복 비트가 모두 켜진 버퍼만 정지한다. 복제 항목도 검사하며 한 번 재생하는 소리는 유지한다.
    // 옵션·초기화와 무관하고 재생 수를 먼저 다시 세지 않는다. 상태 조회 두 번 뒤 StopBuffer의 잃음 검사를 거친다.
    // 원본: 004a9e50 / CD 00438d20.
    void StopLoops();
    // 전체 음량을 저장하고 초기화된 표의 모든 실제 버퍼에 Volume − Attenuation + 음량을 적용한다(32비트 넘침, 자르기 없음).
    // 멈춘 버퍼에도 적용하며 실패 HRESULT는 무시한다. 보류 깊이가 0이 아니면 pendingMasterVolume만 바꾼다.
    // 원본: 004a9f10 / CD 00438e50. CD 이름 견고성 보고 줄은 0x442다.
    void SetMasterVolume(std::int32_t volume);
private:
    // 항목의 버퍼가 재생 중인지 장치에 묻는다. 소리 없음 표식이면 묻지 않고 0이다. 원본: 004a8a80 (CD는 인라인).
    std::uint32_t BufferPlaying(SoundHandle entry);
    // 항목의 버퍼를 멈추고 처음으로 되돌린 뒤 재생 수를 줄인다. 버퍼를 잃었으면 "!isLost()"를 보고한다. 원본: 004a8ad0 (CD는 인라인).
    void StopBuffer(SoundHandle entry);
    // 항목 머리 필드를 읽는다(SoundList::Field의 줄임).
    std::uint32_t Get(SoundHandle entry,SoundField field) const;
    // 항목 머리 필드를 쓴다(SoundList::SetField의 줄임).
    void Set(SoundHandle entry,SoundField field,std::uint32_t value);
    // 인자 + 전체 음량 − 소리별 감쇠를 32비트로 더해 [-10000, 0]으로 자른다.
    std::int32_t Gain(SoundHandle entry,std::int32_t volume) const;
    // 기록 경계가 연결돼 있으면 (채널, 문장)을 전달한다.
    void Log(std::uint32_t channel,const std::string& text) const;
    // 보고 경계가 연결돼 있으면 (조건식, 줄)을 전달한다. 돌아온 뒤 호출자가 실행을 계속한다.
    void Report(std::string_view expression,int line) const;
    SoundList& list_;
    SoundState& state_;
    SoundDeviceHooks hooks_;
    SoundAssertReport report_;
};
}
