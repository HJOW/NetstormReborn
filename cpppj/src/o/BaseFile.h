// 원본 BaseFile.cpp의 읽기 경로 복원. 쓰기·레지스트리·CD 자동 탐색은 아직 옮기지 않았다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::o {
// 원본 TAFF 헤더와 설정 파일이 공유하는 XOR 키. 엔트리마다 키 위치는 0부터 시작한다.
inline constexpr std::string_view kXorKey = "mydoghasfleas";
// 파일·엔트리 시작 기준 XOR를 적용한다. 다시 적용하면 원래 바이트가 된다.
void ApplyXor(std::span<std::uint8_t> bytes);
// 파일을 읽기 전용으로 연다. 원본 디렉터리에 쓰는 동작은 없다.
std::vector<std::uint8_t> ReadFileBytes(const std::filesystem::path& path);
// 사용자 경로 표기를 원본 아카이브 경로로 정규화한다. '/'와 선행 구분자 허용은 새 확장이다.
std::string NormalizeGamePath(std::string_view name);

struct TaffEntry {
    std::string name;
    std::uint32_t offset{}; // 데이터 영역 기준 오프셋, 원본 레코드 +0.
    std::uint32_t size{}; // 원본 레코드 +4.
};

class TaffArchive {
public:
    // 원본 헤더·이름 인덱스·레코드를 리틀 엔디언으로 읽는다. 잘린 파일은 예외로 거부한다.
    explicit TaffArchive(std::vector<std::uint8_t> bytes);
    // 디스크에서 읽은 아카이브를 만든다.
    static TaffArchive Open(const std::filesystem::path& path);
    // 디렉터리의 원본 순서를 유지한 목록을 반환한다.
    const std::vector<TaffEntry>& Entries() const;
    // 대소문자를 무시하여 첫 일치 엔트리를 찾는다.
    const TaffEntry* Find(std::string_view name) const;
    // 이름으로 찾아 플래그 bit 1이 켜진 아카이브만 복호화한다.
    std::vector<std::uint8_t> Read(std::string_view name) const;
private:
    std::vector<std::uint8_t> bytes_;
    std::vector<TaffEntry> entries_;
    std::uint32_t dataOffset_{};
    std::uint32_t flags_{}; // 헤더 +0x18. bit 1 = XOR.
};

class BaseFileSystem {
public:
    // 기본 경로·보조 경로를 설정한다. 등록 순서는 호출자가 원본 순서에 맞춰 지정한다.
    explicit BaseFileSystem(std::filesystem::path base, std::filesystem::path secondary = {});
    // 원본 FUN_0041b9d0처럼 같은 아카이브 이름은 한 번만 등록한다.
    void RegisterArchive(const std::filesystem::path& path);
    // 상대 이름의 읽기: 기본 디스크 → 아카이브 → 보조 디스크. 절대 이름은 직접 읽는다.
    std::vector<std::uint8_t> Read(std::string_view name) const;
    // Read와 같은 순서로 찾되, 없으면 예외 대신 빈 값을 돌려준다(없어도 되는 설정 파일용).
    std::optional<std::vector<std::uint8_t>> TryRead(std::string_view name) const;
private:
    std::filesystem::path base_;
    std::filesystem::path secondary_;
    std::vector<std::string> archiveNames_;
    std::vector<TaffArchive> archives_;
};
}
