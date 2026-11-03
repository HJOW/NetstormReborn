// 4de710의 File/버퍼 설치·닫기 전 정규화·닫은 뒤 파서를 연결한다.
#pragma once
#include "o/RawBaseFileIO.h"
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::o { class BaseFileSystem;class RawXlat; } // 현재 VFS와 같은 Client 번역 장부를 빌린다.
namespace netstorm::client { // 기존 값형 RawXlat::Load와 구별하는 실제10.78 파일 입구다.
enum class XlatFileFailure { Missing,Length }; // 원문9번 로그의 부재/길이실패 문구를 구별한다.
struct XlatFileAllocation { // 원문 할당 존재와 실제 쓰기 범위를 구별한다. 길이0도 논리 설치는 가능하다.
    std::uintptr_t token{}; // nullptr 진단을 위한 할당 식별자이며 호스트 포인터를32비트로 절단하지 않는다.
    std::span<std::uint8_t> bytes; // 실제 which 슬롯에 설치한 exactLength 저장소다.
};
struct XlatFileReadHooks { // 부모가 도달한 기존 owner/파일/파서 장치만 요구한다.
    std::function<bool(std::uint32_t)> hasBuffer; // 첫 중복 진단과 할당 직전의 같은 현재 슬롯을 읽는다.
    std::function<std::string(std::string_view)> path; // 같은 이름 고리의(-2,d,-3,literal)를 작성한다.
    std::function<bool(std::string_view)> exists; // 내용을 읽지 않는 실제 출처 관문이다.
    std::function<void(XlatFileFailure,std::string_view)> log; // File 열기 전 부재/열린 상태의 길이실패 로그다.
    std::function<void(o::RawBaseFileState&,std::string_view,std::uint32_t,std::uint32_t)> open; // 같은 현재 FILE/키의 mode0/create1이다.
    std::function<void(std::uint32_t)> releaseExisting; // 중복 진단 복귀 뒤의 기존 버퍼 해제 경계다. 실제 adapter는 미정 노드 포인터 재사용을 거부한다.
    std::function<XlatFileAllocation(std::uint32_t,std::uint32_t)> allocate; // 같은 which 슬롯의 정확한 요청 폭을 설치한 뒤 반환한다.
    std::function<void(std::uint32_t)> normalize; // 끝BYTE/NUL·CR 이동을 열린 File 안에서 수행한다.
    std::function<void(std::uint32_t)> parse; // 정상 File 소멸이 끝난 뒤 같은 버퍼를 기존 장부에 병합한다.
    std::function<void(std::uint32_t)> assertion; // Xlat.cpp의 e1/f2/103 진단이며 빈 성공으로 바꾸지 않는다.
};
// which 버퍼의 설치/실패 접두와 File 정리 위치를 원문 순서로 수행한다.
bool ReadXlatFile(o::RawBaseFileState& state,std::uint32_t which,std::string_view literal,
    const o::RawBaseFileIOHooks& io,const XlatFileReadHooks& hooks,std::span<std::uint8_t> scratch);
// 실제 VFS/Unicode FILE과 현재7칸 장부를 연결한다. literal은 d/를 붙이지 않은 파일 이름이다.
bool LoadNativeXlatFile(o::RawXlat& xlat,const o::BaseFileSystem& files,std::uint32_t which,std::string_view literal,
    const std::function<void(XlatFileFailure,std::string_view)>& log);
// German3→English1→Portuguese6 순서만 적재하며 부재/길이실패로 다음 파일을 취소하지 않는다.
void InitializeNativeXlat(o::RawXlat& xlat,const o::BaseFileSystem& files,
    const std::function<void(XlatFileFailure,std::string_view)>& log);
}
