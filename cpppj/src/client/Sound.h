// Sound.cpp(원본 .\Sound.cpp / CD sound.cpp) 가운데 소리 이름 표(bigSoundList)의 확보·찾기·추가를 복원한다.
// 소리 장치(DirectSound) 초기화, 버퍼 읽기, 실제 재생/정지/음량은 아직 옮기지 않았다.
#pragma once
#include "o/RiftType.h"
#include <cstdint>
#include <functional>
#include <span>
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

// 소리 파일 이름을 항목으로 바꾸는 이름 표다. 원본 bigSoundList와 같은 0x8000바이트 배치를 그대로 쓴다.
// 사용: 만든 뒤 Initialize()를 한 번 부르고, 소리 이름마다 Lookup()으로 항목 값을 얻어 보관한다. 같은 이름은 같은 값을 돌려준다.
//       항목은 지워지지 않으며 표는 이 객체가 소유한다. 여러 스레드에서 함께 쓰지 않는다.
// 항목 배치: +0 장치 버퍼, +4 자기 자신(원본 항목), +8 복제 항목 연결, +0xc/+0x14/+0x18 재생 상태, +0x1c 이름(NUL 종료).
//       새 항목은 +0x10을 쓰지 않는다(원본과 같다). 재생 쪽 필드는 재생 계층을 옮길 때 채운다.
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
    // 범위: 같은 함수의 DirectSound 초기화는 옮기지 않았다.
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
}
