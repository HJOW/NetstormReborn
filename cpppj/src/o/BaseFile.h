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

// TAFF 아카이브 안의 파일 이름·위치·크기 기록이다.
struct TaffEntry {
    // name: 이름으로 조회·표시할 때 사용하는 문자열.
    std::string name;
    std::uint32_t offset{}; // 데이터 영역 기준 오프셋, 원본 레코드 +0.
    std::uint32_t size{}; // 원본 레코드 +4.
};

// TAFF 아카이브를 읽고 파일 항목을 조회하는 저장소이다.
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
    // bytes_: 필드 접근이나 파일 읽기에 사용하는 원시 바이트.
    std::vector<std::uint8_t> bytes_;
    // entries_: 현재 등록하거나 읽은 항목 목록.
    std::vector<TaffEntry> entries_;
    // dataOffset_: 파일에서 실제 데이터가 시작되는 바이트 위치.
    std::uint32_t dataOffset_{};
    std::uint32_t flags_{}; // 헤더 +0x18. bit 1 = XOR.
};

// 디스크·보조 디렉터리·아카이브 순서로 게임 파일을 찾는 파일 시스템이다.
class BaseFileSystem {
public:
    // 기본 경로·보조 경로를 설정한다. 등록 순서는 호출자가 원본 순서에 맞춰 지정한다.
    explicit BaseFileSystem(std::filesystem::path base, std::filesystem::path secondary = {});
    //0041b9d0/41bc20/41bc30: 두 목록을 독립적으로 등록한다. 같은 목록의 중복·파일/헤더/인덱스 읽기 실패는false다.
    bool RegisterArchive(const std::filesystem::path& path,bool secondary=false);
    //00441790: 선택한 root의 *.tarc를 실제 Win32 열거 순서로 모두 등록한다. 잘못된 후보는 원문처럼 건너뛴다.
    void RegisterArchives(bool secondary=false);
    //0041a4f0: 보조/CD 디스크 root만 바꾼다. 이미 등록한 아카이브 목록은 원문처럼 보존한다. 빈 경로는 기본 root다.
    void SetSecondaryDirectory(std::filesystem::path directory);
    // 상대 읽기: 기본 디스크→기본 TAFF→보조 디스크→보조 TAFF다. 절대 이름은 직접 읽는다.
    std::vector<std::uint8_t> Read(std::string_view name) const;
    // Read와 같은 순서로 찾되, 없으면 예외 대신 빈 값을 돌려준다(없어도 되는 설정 파일용).
    std::optional<std::vector<std::uint8_t>> TryRead(std::string_view name) const;
    std::optional<std::vector<std::uint8_t>> TryReadArchiveOnly(std::string_view name,bool secondary=false) const; //선택한 목록만 등록 순서로 읽고 다른 목록/디스크로 fallback하지 않는다.
    bool ContainsArchiveOnly(std::string_view name,bool secondary=false) const; //선택한41a340 목록의 이름 메타데이터만 조회하며 내용/복호화/디스크를 읽지 않는다.
    // 캠페인 목록의 와일드카드: 디스크/TAFF/보조 경로를 합쳐 중복을 제거하고 이름순으로 반환한다.
    std::vector<std::string> Match(std::string_view pattern) const;
    // 원본 Filespec.cpp의 Win32 FindFirst/Next 순서다. 기본 디스크만 열거하며 TAFF/보조 경로/정렬을 적용하지 않는다.
    // 와일드카드가 없으면 원본처럼 존재 확인 없이 그 이름 한 개를 반환한다. cpppj의 Windows 경계다.
    std::vector<std::string> MatchDisk(std::string_view pattern) const;
    // 원본4588a0 FileFinder의 기본 디스크→등록 TAFF→보조 디스크 스냅샷이다. 첫 대소문자 무시 중복만 남기며 정렬하지 않는다.
    std::vector<std::string> MatchFinder(std::string_view pattern) const;
private:
    // base_: 파생 처리에서 공통 동작을 위임하는 기본 객체.
    std::filesystem::path base_;
    // secondary_: 보조.
    std::filesystem::path secondary_;
    // archiveNames_: 검색 대상으로 등록한 TAFF 아카이브 이름 목록.
    std::vector<std::string> archiveNames_;
    // archives_: 읽기 위해 열린 TAFF 아카이브 목록.
    std::vector<TaffArchive> archives_;
    std::vector<std::string> secondaryArchiveNames_; //00544f80 보조 목록의 대소문자 무시 중복 키이며 기본 목록과 별도다.
    std::vector<TaffArchive> secondaryArchives_; //보조/CD 아카이브의 실제 등록 순서와 읽기 전용 바이트 소유자다.
};
}
