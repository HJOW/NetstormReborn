// 원본00440d40/004dde40/004dd340의 CD-ROM 열거·표시 파일 재귀 탐색을 분리한다.
#pragma once
#include "client/CdPresence.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client { // 경로 판단과 Windows10/11의 실제 W 파일 장치를 구별한다.
struct CdDirectoryEntry { std::string name;bool directory{}; }; // OS가 반환한 현재 이름과 디렉터리 속성이다.
struct CdDirectoryFirst { std::uint64_t handle{};CdDirectoryEntry entry; }; // 실제 열거 핸들과 첫 항목을 함께 보유한다.
struct CdDirectoryHooks { // 원본이 도달한 열거/종료/cancel 경계만 호출한다.
    std::function<std::vector<std::string>()> drives; // GetLogicalDriveStrings 순서의 NUL 없는 드라이브 루트다.
    std::function<std::uint32_t(std::string_view)> driveType; // 원문처럼 값5인 CD-ROM만 선택한다.
    std::function<std::optional<CdDirectoryFirst>(std::string_view)> first; // 원문 directory\\* 패턴의 FindFirstFile이다.
    std::function<std::optional<CdDirectoryEntry>(std::uint64_t)> next; // 같은 핸들의 다음 항목이며 EOF/실패는 빈 값이다.
    std::function<void(std::uint64_t)> close; // 원문이 닫는 위치에서만 같은 핸들을 반납한다.
    std::function<bool()> cancelled; //005e18b4의 현재 중단 표식이며 각 실제 항목을 처리하기 전에 읽는다.
};
bool FindCdDirectory(std::span<char> destination,const CdDirectoryHooks& hooks); // 최초 발견 디렉터리를 strncpy/NUL로 쓰며 실패는 기존 목적지를 보존한다.
std::string FindWindowsCdDirectory(); // 실제 CD-ROM만 읽고 UTF-8 CDDir을 반환한다. 부재는 빈 문자열이다.
CdPresenceHooks MakeWindowsCdPresenceHooks(); // 원문 루트 열거와 표시 파일 속성을 실제 Windows 장치에 연결한다. 재귀 CDDir 탐색을 대신 호출하지 않는다.
}
