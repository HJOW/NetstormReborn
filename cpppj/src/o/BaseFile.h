// 원본 BaseFile.cpp의 값형 읽기/VFS와 caller의 실제 FILE을 여는 전체부모다. 레지스트리·CD 자동탐색은 다른 장치가 담당한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <functional> //명시 글꼴 소비자의 디스크 이름 변환만 빌린다.
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "o/RawBaseFileOpenPolicy.h" // 실제 FILE/TAFF 열기 부모가 받는 상태/장치와 읽기전용 메타데이터다.

namespace netstorm::o {
class RawFileName; // VFS와 실제 Client의 모든 경로 소비자가 공유할 원문10칸 작성기다.
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
    std::uint32_t DataOffset() const; // 등록 때 읽은 데이터 시작 DWORD이며 payload를 읽거나 파일을 다시 열지 않는다.
    std::uint32_t Flags() const; // 등록 때 읽은 헤더 flags 원문DWORD다. 부분 파일 열기 뒤 flags&2의 키 설치 판정에 사용한다.
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
    explicit BaseFileSystem(std::filesystem::path base, std::filesystem::path secondary = {},std::shared_ptr<RawFileName> fileNames = {}); // 실행 위치 탐색에서 이미 사용한 고리가 있으면 같은 소유자를 인계받는다.
    std::shared_ptr<RawFileName> FileNameOwner() const; // 파일 조회와 HUD/저장 상태가 같은 고리 주소/번호를 빌린다. 파일 내용은 읽지 않는다.
    std::optional<BaseFileArchiveOpen> FindArchiveOpen(std::string_view name,bool secondary=false) const; // 독립Client의 예약 기본등록만 먼저 완료하고 선택목록의 첫 offset/length/xor/실제TAFF이름을 반환한다. 등록 뒤 metadata 조회는 payload/디스크를 다시 읽지 않는다.
    void Open(RawBaseFileState& state,std::string_view name,std::uint32_t mode,std::uint32_t create,const RawBaseFileIOHooks& io) const; // 전체41b4c0 부모를 같은 VFS/고리/현재 FILE에 연결한다. 호스트 typed marker는 원문 예외 분류이며256BYTE 예외객체 ABI는 별도다.
    void OpenWithDiskName(RawBaseFileState& state,std::string_view name,std::uint32_t mode,std::uint32_t create,const RawBaseFileIOHooks& io,const std::function<std::optional<std::string>(std::string_view)>& diskName) const; // ACP 글꼴 캐시만 현재 FILE의 disk query를 변환하며 root/TAFF/raw고리는 보존한다.
    //0041b9d0/41bc20/41bc30: 두 목록을 독립적으로 등록한다. 같은 목록의 중복·파일/헤더/인덱스 읽기 실패는false다.
    bool RegisterArchive(const std::filesystem::path& path,bool secondary=false);
    //00441790: 선택한 root의 *.tarc를 실제 Win32 열거 순서로 모두 등록한다. 잘못된 후보는 원문처럼 건너뛴다.
    void RegisterArchives(bool secondary=false);
    void DeferPrimaryArchiveRegistration(); // 원문Run전의Client생성자는I/O하지않는다. 독립값형Client파일조회는최초TAFF조회에서한번만기존자동등록을준비한다.
    //0041a4f0: 보조/CD 디스크 root만 바꾼다. 이미 등록한 아카이브 목록은 원문처럼 보존한다. 빈 경로는 기본 root다.
    void SetSecondaryDirectory(std::filesystem::path directory);
    const std::filesystem::path& SecondaryDirectory() const { return secondary_; } //같은현재보조root를빌리며조회로fallback/경로고리/등록목록을바꾸지않는다.
    // 상대 읽기: 기본 디스크→기본 TAFF→보조 디스크→보조 TAFF다. 절대 이름은 직접 읽는다.
    std::vector<std::uint8_t> Read(std::string_view name) const;
    //0041af80/41ad10: 내용물을 읽지 않고 실제 출처를 찾아 TAFF 또는 디스크의 쓰기 금지 상태를 반환한다. 빈 이름/조회·stat 실패는 원문 행 번호의 진단이다.
    bool ReadOnly(std::string_view name) const;
    // Read와 같은 순서로 찾되, 없으면 예외 대신 빈 값을 돌려준다(없어도 되는 설정 파일용).
    std::optional<std::vector<std::uint8_t>> TryRead(std::string_view name) const;
    std::optional<std::vector<std::uint8_t>> TryReadWithDiskName(std::string_view name,const std::function<std::optional<std::string>(std::string_view)>& diskName) const; //명시 ACP 글꼴 소비자만 디스크 이름을UTF-8로 변환한다. raw TAFF/방문순서/고리소비는 그대로다.
    std::optional<std::vector<std::uint8_t>> TryReadArchiveOnly(std::string_view name,bool secondary=false) const; //선택한 목록만 등록 순서로 읽고 다른 목록/디스크로 fallback하지 않는다.
    bool ContainsArchiveOnly(std::string_view name,bool secondary=false) const; //선택한41a340 목록의 이름 메타데이터만 조회하며 내용/복호화/디스크를 읽지 않는다.
    std::uint32_t ProbeExact(std::string_view name) const; // 클론 추가 한국어를 찾기 전에 현재 disk/TAFF의 같은 이름만 조회하며 영어 대체를 잠시 억제한다.
    std::uint32_t Probe(std::string_view name) const; //0041ab10의 출처0..5를 내용 읽기 없이 조회한다. 직접 경로 실패는 상대/TAFF로 fallback하지 않는다.
    std::uint32_t ProbeWithDiskName(std::string_view name,const std::function<std::optional<std::string>(std::string_view)>& diskName) const; // 같은 raw출처 순서에서 disk query만 변환하며 변환 실패도 다음 raw TAFF를 막지 않는다.
    // 캠페인 목록의 와일드카드: 디스크/TAFF/보조 경로를 합쳐 중복을 제거하고 이름순으로 반환한다.
    std::vector<std::string> Match(std::string_view pattern) const;
    // 원본 Filespec.cpp의 Win32 FindFirst/Next 순서다. 기본 디스크만 열거하며 TAFF/보조 경로/정렬을 적용하지 않는다.
    // 와일드카드가 없으면 존재 확인 없이 원문 .\\ 접두/구분자 이름 한 개다. 패턴은 원래 입력의 마지막 '\\' 접두만 유지하며 '/' 확장은 caller에서 변환한다.
    std::vector<std::string> MatchDisk(std::string_view pattern) const;
    // 원본4588a0 FileFinder의 기본 디스크→등록 TAFF→보조 디스크 스냅샷이다. 첫 대소문자 무시 중복만 남기며 정렬하지 않는다.
    std::vector<std::string> MatchFinder(std::string_view pattern) const;
private:
    void EnsurePrimaryArchives() const; // 독립조회에예약된기본목록만등록하며실제Startup의새VFS는예약하지않는다.
    bool RegisterArchiveCache(const std::filesystem::path& path,bool secondary) const; // 논리적등록캐시만갱신해const조회도정의된C++객체변경으로준비한다.
    void RegisterArchivesCache(bool secondary) const; // 기존Win32순서의등록몸체이며공개명시등록과예약실행이공유한다.
    // base_: 파생 처리에서 공통 동작을 위임하는 기본 객체.
    std::filesystem::path base_;
    // secondary_: 보조.
    std::filesystem::path secondary_;
    // archiveNames_: 검색 대상으로 등록한 TAFF 아카이브 이름 목록.
    mutable std::vector<std::string> archiveNames_;
    // archives_: 읽기 위해 열린 TAFF 아카이브 목록.
    mutable std::vector<TaffArchive> archives_;
    mutable std::vector<std::string> archivePaths_; // 중복 비교키와 별개인 기본 등록의 원래 대소문자/Unicode 실제 파일명이다.
    mutable std::vector<std::string> secondaryArchiveNames_; //00544f80 보조 목록의 대소문자 무시 중복 키이며 기본 목록과 별도다.
    mutable std::vector<TaffArchive> secondaryArchives_; //보조/CD 아카이브의 실제 등록 순서와 읽기 전용 바이트 소유자다.
    mutable std::vector<std::string> secondaryArchivePaths_; // 보조 등록과 같은 인덱스의 실제 TAFF 이름이며 기본목록과 분리한다.
    std::shared_ptr<RawFileName> fileNames_; // 새로운 독립 VFS에서만 BSS0을 만들고 실제 Client와 공유해 먼저 해제하지 않는다.
    mutable bool primaryArchivesPending_{}; // Client의독립조회호환만예약하며생성/디스크직접조회에는파일을읽지않는다.
};
}
