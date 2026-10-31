// BaseFile의 FILE 위치·길이·쓰기·반납 순서를 복원하며 호스트 CRT 장치는 호출자가 제공한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace netstorm::o { // 원본 32비트 산술과 호스트 FILE/할당의 수명을 분리한다.
// 원문 여섯 필드의 의미를 보관한다. 포인터 토큰은 호스트 폭이며 24BYTE ABI로 직렬화하지 않는다.
struct RawBaseFileState {
    std::uintptr_t file{}; // +00의 현재 FILE 토큰이다. 원문 생성자는 이것만 먼저0으로 만든다.
    std::uint32_t flags{}; // +04의 bit0은 TAFF 내부 부분 파일이다. 다른 비트는 반납 때 보존한다.
    std::int32_t offset{}; // +08의 부분 파일 시작이며 seek/ftell의 DWORD 산술에 사용한다.
    std::int32_t length{}; // +0c의 부분 파일 길이다. EOF는 끝과 같은 위치에서 아직 false다.
    std::uintptr_t key{}; // +10의 현재 암호 키 할당 토큰이며 Close만으로 해제하지 않는다.
    std::uint32_t keyLength{}; // +14의 키 길이다. 소멸 때 키를 해제해도 이 값은 보존한다.
};
// 순수 FILE/할당 경계다. 콜백 뒤에는 원문의 위치에서 현재 필드를 다시 읽는다.
struct RawBaseFileIOHooks {
    std::function<void(std::uintptr_t,std::int32_t,std::int32_t)> seek; // fseek의 현재 FILE/offset/origin이다. CRT 실패 반환은 원문처럼 무시한다.
    std::function<std::int32_t(std::uintptr_t)> tell; // Windows long의 signed32 결과이며 -1도 그대로 사용한다.
    std::function<std::uint32_t(std::uintptr_t)> eofFlags; // FILE._flag의 현재 DWORD다. 원문 일반 EOF는 bit0x10 자체를 반환한다.
    std::function<void(std::uintptr_t)> close; // fclose이며 정상 반환 뒤에만 같은 필드를 초기화한다.
    std::function<void(std::uintptr_t)> freeKey; // 소멸의 현재 키 해제다. Close는 이 장치를 요구하지 않는다.
    std::function<std::span<const std::uint8_t>(std::uintptr_t,std::uint32_t)> key; // 현재 토큰의 읽기 전용 키 물리다. getter 자체는 원문 메모리 조회처럼 순수해야 한다.
    std::function<void(std::uintptr_t,std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> write; // fwrite(data,size,1,file)다. 부분 쓰기 반환값은 본체가 사용하지 않는다.
    std::function<void(std::uint32_t)> assertion; // Basefile.cpp의38b/3a4/3aa 진단이며 복귀 뒤 현재 상태로 계속한다.
    std::function<std::uintptr_t(std::string_view,std::string_view)> open; // fopen의 실제 이름/모드와 반환 FILE 토큰이다. 경로 문자 변환은 호스트 장치가 맡는다.
    std::function<void(std::uintptr_t,std::span<std::uint8_t>,std::uint32_t,std::uint32_t)> read; // fread(data,size,1,file)이며 짧은 읽기에서 미작성 BYTE를 보존한다.
    std::function<std::uintptr_t(std::uint32_t)> allocateKey; // operator_new(strlen+1)의 실제 반환 할당이다.
    std::function<std::span<std::uint8_t>(std::uintptr_t,std::uint32_t)> mutableKey; // 키 복사 대상의 실제 물리다. getter 자체는 필드/입력을 변경하지 않는다.
};
void ConstructBaseFile(RawBaseFileState& state); // PATCH41a5a0/CD4ba590의 세 필드0만 쓰며 flags/부분 파일 위치는 보존한다.
bool OpenBaseFileDisk(RawBaseFileState& state,std::string_view name,std::uint32_t mode,std::uint32_t create,const RawBaseFileIOHooks& hooks); // PATCH41a5b0/CD4ba5d0의 rb/r+b/w+b와 mode1 실패 뒤 생성만 수행한다. CD는 의미상 몸체를 대조해 정밀 매칭 후보와 구별했다.
bool OpenBaseFileSubfile(RawBaseFileState& state,std::string_view name,std::int32_t offset,std::int32_t length,const RawBaseFileIOHooks& hooks); // PATCH41b460/CD4ba630: rb 성공 뒤 부분 파일 필드/seek를 작성한다. CD는 별도 위치 helper를 호출한다.
void SetBaseFileKey(RawBaseFileState& state,std::optional<std::span<const std::uint8_t>> source,const RawBaseFileIOHooks& hooks); // PATCH41a620/CD4ba680의 이전 키 해제→길이→할당→NUL 포함 복사다.
void SeekBaseFile(RawBaseFileState& state,std::int32_t offset,std::int32_t origin,const RawBaseFileIOHooks& hooks); // PATCH41a700/CD4bafa0의 부분 파일 전용 끝 기준 부호까지 유지한다.
std::uint32_t BaseFileEof(RawBaseFileState& state,const RawBaseFileIOHooks& hooks); // PATCH41a6c0/CD4baf70의 원문 결과이며 bool로 정규화하지 않는다.
std::int32_t BaseFileLength(RawBaseFileState& state,const RawBaseFileIOHooks& hooks); // PATCH41aa80/CD4bb2d0의 tell→끝→tell→원래 위치다.
void CloseBaseFile(RawBaseFileState& state,const RawBaseFileIOHooks& hooks); // PATCH41a680/CD4baf10: FILE이0이면 다른 필드도 그대로 둔다.
void DestructBaseFile(RawBaseFileState& state,const RawBaseFileIOHooks& hooks); // PATCH41b410: Close 뒤 그 시점의 키만 해제한다.
void WriteBaseFile(RawBaseFileState& state,std::span<const std::uint8_t> bytes,std::int32_t offset,const RawBaseFileIOHooks& hooks); // PATCH41a910/CD4bb180의4096BYTE 분할·위치 기반 XOR다. 원본 입력은 바꾸지 않는다.
void ReadBaseFile(RawBaseFileState& state,std::span<std::uint8_t> bytes,std::int32_t offset,const RawBaseFileIOHooks& hooks,std::span<std::uint8_t> scratch={}); // PATCH41a7a0/CD4bb030. 암호 읽기의 현재4096BYTE 작업 물리는 호출자가 제공하며 짧은 읽기에서0으로 채우지 않는다.
}
