// CD 디렉터리 재귀 탐색과 별개인 원문의 루트 목록/표시 파일 확인 몸체다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 원문 CD 판단과 실제 Windows 파일 장치를 분리한다.
inline constexpr std::string_view kCdPresenceMarker="cd.netstorm.titanic.activision"; // 원문00532544의 같은 고정 표시 이름이다.
struct CdDriveRootsHooks { // 원문004dde40이 선택하는 OS/진단 경계다.
    std::function<std::uint32_t(std::span<char,128>)> logicalDrives; // GetLogicalDriveStringsA(128)의 현재 반환값과 작성 BYTE다.
    std::function<std::uint32_t(std::string_view)> driveType; // 현재 C문자열 루트의 GetDriveType 결과다. 정확5에서만 복사한다.
    std::function<void(std::uint32_t)> assertion; // 출력 포인터가NULL인 Winutil.cpp529 진단이다.
};
std::uint32_t CollectCdDriveRoots(std::uint32_t capacity,std::span<char> output,std::span<char,128> scratch,const CdDriveRootsHooks& hooks); // PATCH004dde40의 길이/엄격 복사 관문/끝 NUL·초과를 보존한다. output 물리와 capacity를 구별하며 CD004bf740의 NULL 진단 줄527은 별도다.
struct CdPresenceHooks { // 원문00440c60의 현재 경로/존재/루트 열거 경계다.
    std::function<std::string(std::string_view,std::string_view)> path; //459c60(-2,현재root,-3,표시이름,0)의 현재 경로다.
    std::function<bool(std::string_view)> exists; //004dd560의 현재 파일 속성 bool 판단이다. 디렉터리도 true다.
    std::function<std::uint32_t(std::span<char,128>)> roots; //004dde40(128,실제stack)의 반환과 출력이다.0이면 작성되지 않은 stack을 읽지 않는다.
};
std::uint32_t CheckCdPresence(std::span<char> cache,std::span<char,128> scratch,const CdPresenceHooks& hooks); // PATCH00440c60: 캐시 성공은 즉시1, 실패는 첫BYTE0 뒤 현재 CD 루트를 검사한다. CD004a8970의 성공 복사는 별도다.
}
