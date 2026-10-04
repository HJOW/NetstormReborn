// 원본 Config.cpp의 파일 복호화·첫 일치 조회. 치환·레지스트리 경로는 아직 복원하지 않았다.
#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::o {
class ConfigText {
public:
    // UTF-8 설정 버퍼를 받아 원본 줄 순서를 그대로 유지한다.
    explicit ConfigText(std::string text);
    // 원본 FUN_00440380: 첫째·셋째 바이트 0으로 XOR 파일을 감지한다.
    static ConfigText FromBytes(std::span<const std::uint8_t> bytes);
    // 원본 FUN_0043fd70 ↔ CD 0042a270: 첫 일치 키의 줄 앞 위치를 찾는다.
    std::optional<std::size_t> FindKey(std::string_view key) const;
    // 원본 FUN_00440150 ↔ CD 0042ab70: 따옴표 제거, 따옴표 밖 주석 제거.
    std::optional<std::string> GetRaw(std::string_view key) const;
    // 원본 FUN_0043ffa0: 입력 끝의 줄바꿈을 CRLF 하나로 정리해 이어 붙인다. 앞 파일의 값이 우선한다.
    void Append(std::string_view text);
    // 복원·검증 도구가 읽은 버퍼를 확인할 수 있게 한다.
    const std::string& Text() const;
private:
    std::string text_;
};
}
