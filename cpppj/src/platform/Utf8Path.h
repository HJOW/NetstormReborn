// Windows10/11의 한글·보충 평면 경로를 원본 ANSI 대신 UTF-8/네이티브 wide로 왕복한다.
#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace netstorm::platform { // 게임 내부 문자열과 OS 파일 경로 사이의 인코딩 경계다.
// UTF-8 문자열을 시스템 ANSI 코드 페이지를 거치지 않고 실제 파일 경로로 변환한다.
inline std::filesystem::path PathFromUtf8(std::string_view text) {
    if (text.empty()) return {}; // 빈 경로의 기존 의미를 유지하고 NULL 데이터 포인터를 변환기에 넘기지 않는다.
    return std::filesystem::path(std::u8string(text.begin(),text.end()));
}
// 실제 경로를 UTF-8로 반환한다. generic은 비교/장부용 '/' 표기를 선택한다.
inline std::string PathToUtf8(const std::filesystem::path& path,bool generic=false) {
    const auto text=generic ? path.generic_u8string() : path.u8string(); // OS 경로 문자를 손실 없이 UTF-8 바이트로 받는다.
    return {reinterpret_cast<const char*>(text.data()),text.size()};
}
}
