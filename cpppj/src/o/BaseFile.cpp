// 원본: FUN_0041a240(이름 검색), FUN_0041a340(엔트리 위치·XOR 플래그), FUN_0041b4c0(열기).
// CD판은 동일 TAFF 형식을 사용한다. 헤더 오프셋은 두 판본 파일을 직접 대조한다.
// 범위: 읽기 전용 아카이브/VFS. 원본의 FILE* 대신 소유권 있는 바이트 배열을 쓴다.
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 TAFF v0.2 헤더 크기와 고정 매직.
constexpr std::size_t kHeaderSize = 0x2c;
// TAFF v0.2의 버전 문자열 뒤에는 DOS EOF 문자가 온다.
constexpr std::string_view kMagic = "TAFF v0.2\x1a";

// 덧셈 오버플로 없이 파일 내부 범위를 확인한다.
void RequireRange(std::size_t begin, std::size_t length, std::size_t end) {
    if (begin > end || length > end - begin) throw std::runtime_error("TAFF range outside file");
}
// 호스트의 포인터 크기·엔디언과 무관하게 원본 32비트 정수를 읽는다.
std::uint32_t ReadU32(std::span<const std::uint8_t> bytes, std::size_t pos) {
    RequireRange(pos, 4, bytes.size());
    return static_cast<std::uint32_t>(bytes[pos]) | (static_cast<std::uint32_t>(bytes[pos+1]) << 8)
        | (static_cast<std::uint32_t>(bytes[pos+2]) << 16) | (static_cast<std::uint32_t>(bytes[pos+3]) << 24);
}
// 디스크 파일이 있으면 읽는다. 열기 실패는 다음 조회 위치를 시도할 수 있게 빈 optional을 반환한다.
std::optional<std::vector<std::uint8_t>> TryRead(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return std::nullopt;
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream), {});
}
}

// 원본 XOR 산식: byte[i] ^= key[i % 13].
void ApplyXor(std::span<std::uint8_t> bytes) {
    // 모든 바이트에 순환 키를 적용한다.
    for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] ^= static_cast<std::uint8_t>(kXorKey[i % kXorKey.size()]);
}
// 읽기 실패를 호출자에게 전달한다.
std::vector<std::uint8_t> ReadFileBytes(const std::filesystem::path& path) {
    auto bytes = TryRead(path);
    if (!bytes) throw std::runtime_error("Unable to read: " + path.string());
    return std::move(*bytes);
}
// 원본 '.\\' 제거·대소문자 무시와 사용자용 '/' 허용을 한곳에서 처리한다.
std::string NormalizeGamePath(std::string_view name) {
    std::string result = AsciiLower(name);
    std::replace(result.begin(), result.end(), '\\', '/');
    if (result.starts_with("./")) result.erase(0, 2);
    const auto first = result.find_first_not_of('/');
    return first == std::string::npos ? std::string{} : result.substr(first);
}
// 헤더 +0x1c의 이름 인덱스를 따라 각 레코드의 위치를 확인한다.
TaffArchive::TaffArchive(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {
    RequireRange(0, kHeaderSize, bytes_.size());
    if (!std::equal(kMagic.begin(), kMagic.end(), bytes_.begin())) throw std::runtime_error("Invalid TAFF v0.2 magic");
    const auto count = ReadU32(bytes_, 0x14);
    flags_ = ReadU32(bytes_, 0x18);
    const auto index = ReadU32(bytes_, 0x1c);
    const auto directory = ReadU32(bytes_, 0x20);
    const auto directorySize = ReadU32(bytes_, 0x24);
    dataOffset_ = ReadU32(bytes_, 0x28);
    RequireRange(directory, directorySize, bytes_.size());
    RequireRange(dataOffset_, 0, bytes_.size());
    if (index < kHeaderSize || directory < kHeaderSize || dataOffset_ < directory || directorySize > dataOffset_ - directory)
        throw std::runtime_error("Invalid TAFF header layout");
    if (index > directory || count > (directory - index) / 4) throw std::runtime_error("Invalid TAFF name index");
    entries_.reserve(count);
    // 디렉터리 기준 상대 오프셋을 파일 위치로 바꿔 레코드를 읽는다.
    for (std::size_t i = 0; i < count; ++i) {
        const auto relative = ReadU32(bytes_, index + i * 4);
        RequireRange(relative, 8, directorySize);
        const auto pos = static_cast<std::size_t>(directory) + relative;
        const auto nameBegin = bytes_.begin() + static_cast<std::ptrdiff_t>(pos + 8);
        const auto directoryEnd = bytes_.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(directory) + directorySize);
        const auto nameEnd = std::find(nameBegin, directoryEnd, 0);
        if (nameEnd == directoryEnd) throw std::runtime_error("Unterminated TAFF name");
        TaffEntry entry{std::string(nameBegin, nameEnd), ReadU32(bytes_, pos), ReadU32(bytes_, pos+4)};
        RequireRange(entry.offset, entry.size, bytes_.size() - dataOffset_);
        entries_.push_back(std::move(entry));
    }
}
// 원본 파일의 내용만 읽고 객체를 만든다.
TaffArchive TaffArchive::Open(const std::filesystem::path& path) { return TaffArchive(ReadFileBytes(path)); }
// 아카이브의 모든 엔트리를 읽기 전용으로 공개한다.
const std::vector<TaffEntry>& TaffArchive::Entries() const { return entries_; }
// 원본 FUN_0041a240의 첫 일치 순차 검색을 유지한다.
const TaffEntry* TaffArchive::Find(std::string_view name) const {
    const auto key = NormalizeGamePath(name);
    // 같은 이름이 있어도 원본처럼 앞 엔트리를 선택한다.
    for (const auto& entry : entries_) if (NormalizeGamePath(entry.name) == key) return &entry;
    return nullptr;
}
// 엔트리별로 XOR 키를 처음부터 적용한다.
std::vector<std::uint8_t> TaffArchive::Read(std::string_view name) const {
    const auto* entry = Find(name);
    if (!entry) throw std::runtime_error("TAFF entry not found: " + std::string(name));
    const auto begin = bytes_.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(dataOffset_) + entry->offset);
    std::vector<std::uint8_t> result(begin, begin + entry->size);
    if ((flags_ & 2) != 0) ApplyXor(result);
    return result;
}
// 보조 경로 미지정 시 원본처럼 기본 경로를 다시 사용한다.
BaseFileSystem::BaseFileSystem(std::filesystem::path base, std::filesystem::path secondary)
    : base_(std::move(base)), secondary_(secondary.empty() ? base_ : std::move(secondary)) {}
// 명시적으로 등록한 아카이브만 등록 순서대로 사용한다.
void BaseFileSystem::RegisterArchive(const std::filesystem::path& path) {
    const auto key = AsciiLower(std::filesystem::absolute(path).lexically_normal().generic_string());
    if (std::find(archiveNames_.begin(), archiveNames_.end(), key) != archiveNames_.end()) return;
    auto archive = TaffArchive::Open(path);
    archives_.push_back(std::move(archive));
    archiveNames_.push_back(key);
}
// 원본 FUN_0041b4c0의 상대 경로 읽기 순서를 복원한다.
std::optional<std::vector<std::uint8_t>> BaseFileSystem::TryRead(std::string_view name) const {
    const std::filesystem::path direct{std::string(name)};
    if (direct.is_absolute()) return netstorm::o::TryRead(direct); // 절대 경로 실패 시 원본의 재조회는 미구현.
    const std::filesystem::path relative{NormalizeGamePath(name)};
    // 원본 데이터 파일은 Windows에서도 대소문자를 구분하지 않는다. Linux 경로 보정은 후속 작업이다.
    if (auto bytes = netstorm::o::TryRead(base_ / relative)) return bytes;
    // 기본 디스크 다음에 등록된 아카이브를 순서대로 찾는다.
    for (const auto& archive : archives_) if (archive.Find(name)) return archive.Read(name);
    return netstorm::o::TryRead(secondary_ / relative);
}
// 찾지 못하면 호출자에게 예외로 알린다.
std::vector<std::uint8_t> BaseFileSystem::Read(std::string_view name) const {
    auto bytes = TryRead(name);
    if (!bytes) throw std::runtime_error("Game file not found: " + std::string(name));
    return std::move(*bytes);
}
}
