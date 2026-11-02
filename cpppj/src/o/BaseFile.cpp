// 원본: FUN_0041a240(이름 검색), FUN_0041a340(엔트리 위치·XOR 플래그), FUN_0041b4c0(열기).
// CD판은 동일 TAFF 형식을 사용한다. 헤더 오프셋은 두 판본 파일을 직접 대조한다.
// 범위: 값형 아카이브/VFS와 caller의 FILE/키를 여는 전체부모. TAFF snapshot 읽기와 실제부분파일 열기를 구별한다.
#include "o/BaseFile.h"
#include "o/FileSpecQuery.h"
#include "o/RawBaseFileReadPolicy.h"
#include "o/RawFileName.h" // 디스크 root 조합도 HUD와 같은 원문 고리를 전진시킨다.
#include "o/RawBaseFileIO.h"
#include "o/RawColorMapCache.h" // 기존 EX_UNABLE_TO_OPEN_FILE marker와 같은 정확한 typed 분류를 사용한다.
#include "o/OriginalText.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <sys/stat.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // Win32 매크로가 기존 표준 함수 이름과 충돌하지 않게 한다.
#endif
#include <windows.h>
#endif

namespace netstorm::o {
namespace {
// 원본 TAFF v0.2 헤더 크기와 고정 매직.
constexpr std::size_t kHeaderSize = 0x2c;
// TAFF v0.2의 버전 문자열 뒤에는 DOS EOF 문자가 온다.
constexpr std::string_view kMagic = "TAFF v0.2\x1a";

// Windows10/11의 한글/보충 평면 파일 이름은 원본ANSI 대신 API 내부 UTF-8에서 native wide 경로로 변환한다.
std::filesystem::path NativePath(std::string_view value) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(value.data()),value.size())); }
// 경로를 현재 코드 페이지로 손실시키지 않고 목록/오류 메시지의 UTF-8로 돌려준다.
std::string PathText(const std::filesystem::path& value) { const auto bytes=value.generic_u8string();return {bytes.begin(),bytes.end()}; }

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
// 같은 FILE/키 장치를 정상/예외 이탈에서 반납하며 원문24BYTE 객체 ABI로 노출하지 않는다.
struct BaseFileLifetime {
    RawBaseFileState state{}; // 현재 실제 파일/키 토큰과 여섯 원문 필드다.
    RawBaseFileIOHooks hooks; // state보다 먼저 소멸하지 않는 현재 CRT/메모리 장치다.
    ~BaseFileLifetime() { DestructBaseFile(state,hooks); } // 읽기 실패에서도 현재 FILE과 키를 한 번 반납한다.
};
// 디스크 열기/길이/읽기는 공통 원문 몸체를 사용한다. 열기 실패만 다음 위치를 조회할 수 있는 빈 optional이다.
std::optional<std::vector<std::uint8_t>> TryRead(const std::filesystem::path& path) {
    BaseFileLifetime file;bool complete=true;ConstructBaseFile(file.state);
    file.hooks.open=[&path](auto,auto mode)->std::uintptr_t { // Windows10/11의 원본 ANSI와 다른 wide CRT 경로 호환이며 모드는 원문의 rb다.
        if (mode!="rb") throw std::logic_error("읽기 전용 BaseFile의 실제 모드 위반");
#ifdef _WIN32
        return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),L"rb"));
#else
        return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),"rb"));
#endif
    };
    file.hooks.tell=[](auto token) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(token))); };
    file.hooks.seek=[](auto token,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(token),offset,origin)); };
    file.hooks.close=[](auto token) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(token))); };
    file.hooks.read=[&complete](auto token,auto bytes,auto size,auto count) { const auto read=std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(token));complete=(!size || !count || read==count); };
    if (!OpenBaseFileDisk(file.state,PathText(path),0,1,file.hooks)) return std::nullopt;
    const auto length=BaseFileLength(file.state,file.hooks);if (length<0) throw std::runtime_error("BaseFile의 실제 signed 파일 길이 조회 실패");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));ReadBaseFile(file.state,bytes,-1,file.hooks);
    if (!complete) throw std::runtime_error("BaseFile의 전체 바이트 읽기 중 파일 변경/부분 I/O"); // 원문 미정 tail을0으로 만들어 정상 자산으로 반환하지 않는 호스트 물리 경계다.
    return bytes;
}
// 이미 소유한 TAFF 내용물도 같은 부분 파일/키/분할 읽기 몸체를 사용한다. 실제 OS FILE은 앞선 전체 읽기에서 반납했다.
std::vector<std::uint8_t> ReadTaffPayload(std::span<const std::uint8_t> source,bool encrypted) {
    if (source.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::out_of_range("TAFF 내용물의 원문 signed 길이 범위");
    std::vector<std::uint8_t> key;std::size_t position{};BaseFileLifetime file;ConstructBaseFile(file.state);file.state.file=1;file.state.flags=1;file.state.length=static_cast<std::int32_t>(source.size());
    file.hooks.tell=[&position](auto) { return static_cast<std::int32_t>(position); };
    file.hooks.close=[](auto) {}; // 이미 소유한 불변 바이트의 부분 파일 핸들만 반납하며 디스크를 다시 열지 않는다.
    file.hooks.read=[&](auto,auto destination,auto size,auto count) { const auto length=static_cast<std::size_t>(size)*count;RequireRange(position,length,source.size());std::copy_n(source.data()+position,length,destination.data());position+=length; };
    file.hooks.allocateKey=[&key](auto length)->std::uintptr_t { key.resize(length);return 2; };
    file.hooks.mutableKey=[&key](auto token,auto length)->std::span<std::uint8_t> { if (token!=2 || length>key.size()) throw std::out_of_range("TAFF 현재 키 쓰기 물리 범위");return key; };
    file.hooks.key=[&key](auto token,auto length)->std::span<const std::uint8_t> { if (token!=2 || length>key.size()) throw std::out_of_range("TAFF 현재 키 읽기 물리 범위");return key; };
    file.hooks.freeKey=[&key](auto token) { if (token!=2) throw std::logic_error("TAFF 반납 키 토큰 오류");key.clear(); };
    if (encrypted) SetBaseFileKey(file.state,std::span(reinterpret_cast<const std::uint8_t*>(kXorKey.data()),kXorKey.size()+1),file.hooks);
    std::vector<std::uint8_t> result(source.size());std::array<std::uint8_t,4096> scratch; // 불변 입력은 매 읽기 전체를 쓰므로 미정 바이트를 읽지 않는다.
    ReadBaseFile(file.state,result,-1,file.hooks,scratch);return result;
}
//0041b040의44BYTE 헤더/strcmp/두 fread가 성공할 물리인지 확인한다. 내용물 레코드 자체의 host 범위 검사는 기존 parser에 남긴다.
bool RegisterableHeader(std::span<const std::uint8_t> bytes) {
    if (bytes.size()<kHeaderSize || !std::equal(kMagic.begin(),kMagic.end(),bytes.begin()) || bytes[kMagic.size()]!=0) return false;
    const auto count=ReadU32(bytes,0x14),index=ReadU32(bytes,0x1c),directory=ReadU32(bytes,0x20),length=ReadU32(bytes,0x24); //원문이 할당/seek/read에 사용하는 실제 헤더다.
    return index<=bytes.size() && count<=(bytes.size()-index)/4 && directory<=bytes.size() && length<=bytes.size()-directory;
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
    if (!bytes) throw std::runtime_error("Unable to read: " + PathText(path));
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
std::uint32_t TaffArchive::DataOffset() const { return dataOffset_; } // 등록한 헤더 메타데이터만 반환하고 현재 payload를 읽지 않는다.
std::uint32_t TaffArchive::Flags() const { return flags_; } // 다른 헤더 비트를 bool로 축약하지 않고 같은DWORD를 반환한다.
// 엔트리별로 XOR 키를 처음부터 적용한다.
std::vector<std::uint8_t> TaffArchive::Read(std::string_view name) const {
    const auto* entry = Find(name);
    if (!entry) throw std::runtime_error("TAFF entry not found: " + std::string(name));
    const auto begin=static_cast<std::size_t>(dataOffset_)+entry->offset;
    return ReadTaffPayload(std::span<const std::uint8_t>(bytes_).subspan(begin,entry->size),(flags_&2U)!=0);
}
// 보조 경로 미지정 시 원본처럼 기본 경로를 다시 사용한다.
BaseFileSystem::BaseFileSystem(std::filesystem::path base, std::filesystem::path secondary,std::shared_ptr<RawFileName> fileNames)
    : base_(std::move(base)), secondary_(secondary.empty() ? base_ : std::move(secondary)),fileNames_(fileNames ? std::move(fileNames):std::make_shared<RawFileName>()) {}
// 실제 shared 소유권이므로 Client 임시 문자열 장치와 VFS 중 어느 쪽을 먼저 반납해도 빌린 고리가 남는다.
std::shared_ptr<RawFileName> BaseFileSystem::FileNameOwner() const { return fileNames_; }
std::optional<BaseFileArchiveOpen> BaseFileSystem::FindArchiveOpen(std::string_view name,bool secondary) const { // 원문41a340의 첫 메타데이터 일치와 선택목록만 사용한다.
    EnsurePrimaryArchives();const auto& archives=secondary?secondaryArchives_:archives_;const auto& paths=secondary?secondaryArchivePaths_:archivePaths_;
    for (std::size_t index=0;index<archives.size();++index) { // 기존 등록/중복 순서를 바꾸거나 payload를 복호화하지 않는다.
        const auto& archive=archives[index];const auto* entry=archive.Find(name);if (!entry) continue;
        return BaseFileArchiveOpen{paths.at(index),std::bit_cast<std::int32_t>(archive.DataOffset()+entry->offset),std::bit_cast<std::int32_t>(entry->size),archive.Flags()&2U};
    }
    return {};
}
void BaseFileSystem::Open(RawBaseFileState& state,std::string_view name,std::uint32_t mode,std::uint32_t create,const RawBaseFileIOHooks& io) const { // 실제 게임의 현재 root/등록/고리와 caller의 동일 FILE/키 수명을 빌린다.
    std::string primary,secondary;BaseFileOpenPolicyHooks hooks; // root 콜백 뒤 반환뷰는 다음 같은 콜백까지 유지한다.
    const auto rootText=[](const std::filesystem::path& root) { auto text=PathText(std::filesystem::absolute(root).lexically_normal());std::replace(text.begin(),text.end(),'/','\\');return text; }; // 원문 getcwd 배열을 Windows10/11 Unicode 게임 root로 대체한다.
    hooks.primaryRoot=[&]()->std::string_view { primary=rootText(base_);return primary; };hooks.secondaryRoot=[&]()->std::string_view { secondary=rootText(secondary_);return secondary; };
    hooks.archive=[&](auto query,bool cd) { return FindArchiveOpen(query,cd); };hooks.search=FileSpecDirectoryHooks(base_);
    hooks.unableToOpen=[](auto failed) { const std::string text(failed);std::printf("unable to open: %s\n",text.c_str());throw ColorCacheUnableToOpen{}; }; // 원문 이름 진단과 typed 분류만 보존하며 MSVC32 예외 payload/SEH ABI를 가장하지 않는다.
    auto devices=io;devices.open=[&](std::string_view file,std::string_view fileMode)->std::uintptr_t { // OS에 전달하는 상대 이름만 가상 게임 CWD에서 해석하며 공유고리를 추가 소비하지 않는다.
        if (!io.open) throw std::logic_error("BaseFile 전체 열기의 실제 FILE 장치 미연결");
#ifdef _WIN32
        if (!file.empty() && !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,file.data(),static_cast<int>(file.size()),nullptr,0) && GetLastError()==ERROR_NO_UNICODE_TRANSLATION) return 0; // 원문 BYTE 절삭이 UTF-8 중간이면 disk 실패만 반환하고 뒤 TAFF의 원시 이름 조회는 유지한다.
#endif
        auto path=NativePath(file);const auto root=std::filesystem::absolute(base_).lexically_normal();const auto drive=path.root_name(); // 직접 장치/다른 드라이브/UNC는 원문 OS 의미를 보존한다.
        if (!path.is_absolute()) {
            if (!path.has_root_name()) path=root/path;
#ifdef _WIN32
            else if (!path.has_root_directory() && drive.native().size()==2 && drive.native()[1]==L':' && AsciiLower(PathText(drive))==AsciiLower(PathText(root.root_name()))) path=root/path.relative_path();
#endif
        }
        return io.open(PathText(path),fileMode);
    };
    std::string input(name.substr(0,name.find('\0')));input+='\0';OpenBaseFileByPolicy(state,{reinterpret_cast<const std::uint8_t*>(input.data()),input.size()},mode,create,*fileNames_,devices,hooks); // 문자열 API는 원본 입력을 안정된NUL 물리로 소유하며 raw 부모 API는 외부 변경 가능한 전체 물리도 지원한다.
}
// 두 단일 목록의 중복 관문은 파일을 다시 읽기 전에 적용하며 실패한 후보는 목록에 남기지 않는다.
bool BaseFileSystem::RegisterArchive(const std::filesystem::path& path,bool secondary) { return RegisterArchiveCache(path,secondary); } // 기존명시등록API는동일한반환/등록순서를유지한다.
bool BaseFileSystem::RegisterArchiveCache(const std::filesystem::path& path,bool secondary) const { // 파일루트가아닌논리적아카이브목록만갱신한다.
    const auto utf8=std::filesystem::absolute(path).lexically_normal().generic_u8string();auto key=AsciiLower({reinterpret_cast<const char*>(utf8.data()),utf8.size()}); //Windows10/11 한글 경로는 원본ANSI와 달리 UTF-8 중복 키로 보존한다.
    auto& names=secondary?secondaryArchiveNames_:archiveNames_;auto& archives=secondary?secondaryArchives_:archives_;auto& paths=secondary?secondaryArchivePaths_:archivePaths_;
    if (std::find(names.begin(),names.end(),key)!=names.end()) return false;
    auto bytes=netstorm::o::TryRead(path);if (!bytes || !RegisterableHeader(*bytes)) return false;
    auto archive=TaffArchive(std::move(*bytes));auto physical=std::filesystem::absolute(path).lexically_normal();physical.make_preferred();const auto native=physical.u8string();std::string original(native.begin(),native.end());archives.reserve(archives.size()+1);names.reserve(names.size()+1);paths.reserve(paths.size()+1); // 세목록 확보가 끝나야 동일 인덱스의 등록을 확정한다.
    archives.push_back(std::move(archive));names.push_back(std::move(key));paths.push_back(std::move(original));return true;
}
// 원본은 FindFirstFileA/Next를 사용한다. Windows10/11의 한글 설치/CD 폴더를 위해 W API로 같은 열거 순서를 받는다.
void BaseFileSystem::RegisterArchives(bool secondary) { RegisterArchivesCache(secondary); } // 명시기본등록은대기예약도완료한다.
void BaseFileSystem::DeferPrimaryArchiveRegistration() { primaryArchivesPending_=true; } // 파일/디렉터리를열지않고독립조회호환을예약한다.
void BaseFileSystem::EnsurePrimaryArchives() const { if (primaryArchivesPending_) RegisterArchivesCache(false); } // 재진입/다음조회에서중복등록하지않는다.
void BaseFileSystem::RegisterArchivesCache(bool secondary) const { // 실제Startup은별도원문부모를사용하고이몸체는독립값형등록을유지한다.
    if (!secondary) primaryArchivesPending_=false;
#ifdef _WIN32
    const auto& root=secondary?secondary_:base_;WIN32_FIND_DATAW found{};const auto handle=FindFirstFileW((root/L"*.tarc").c_str(),&found); //정렬하거나 netstorm.tarc를 앞으로 이동하지 않는다.
    if (handle==INVALID_HANDLE_VALUE) return;
    try {
        do { static_cast<void>(RegisterArchiveCache(root/found.cFileName,secondary)); } while (FindNextFileW(handle,&found)); //디렉터리 후보도 원문 fopen 실패와 같은false로 처리한다.
    }catch (...) { FindClose(handle);throw; }
    FindClose(handle);
#else
    throw std::logic_error("원본 TAFF 등록의 실제 Win32 열거는 Windows에서만 지원합니다");
#endif
}
// 현재 보조 경로만 바꾸고 기존 두 등록 목록의 소유권/순서를 유지한다.
void BaseFileSystem::SetSecondaryDirectory(std::filesystem::path directory) {
    secondary_=directory.empty()?base_:std::move(directory);
}
// 원본 FUN_0041b4c0의 상대 경로 읽기 순서를 복원한다.
std::optional<std::vector<std::uint8_t>> BaseFileSystem::TryRead(std::string_view name) const {
    auto primary=PathText(std::filesystem::absolute(base_).lexically_normal()),secondary=PathText(std::filesystem::absolute(secondary_).lexically_normal()); //원문 getcwd/CD의 native 구분자를 Windows10/11 UTF-8로 보존한다.
    std::replace(primary.begin(),primary.end(),'/','\\');std::replace(secondary.begin(),secondary.end(),'/','\\');
    return ReadBaseFileByPolicy(name,primary,[&](BaseFileReadSite site,std::string_view query)->std::optional<std::vector<std::uint8_t>> {
        if (site==BaseFileReadSite::PrimaryArchive || site==BaseFileReadSite::SecondaryArchive) return TryReadArchiveOnly(query,site==BaseFileReadSite::SecondaryArchive);
        const auto filename=site==BaseFileReadSite::DirectDisk ? std::string(query) : JoinBaseFileRoot(site==BaseFileReadSite::PrimaryDisk?primary:secondary,query,*fileNames_);
#ifdef _WIN32
        //원본의 BYTE 접두 절삭이 UTF-8 글자 중간에 끝나면 Windows10/11 W 경로로 열 수 없다. 디스크 실패를 반환하되 뒤 TAFF의 원시 이름 조회는 계속한다.
        if (!filename.empty() && !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,filename.data(),static_cast<int>(filename.size()),nullptr,0) && GetLastError()==ERROR_NO_UNICODE_TRANSLATION) return {};
#endif
        auto path=NativePath(filename); //원본ANSI와 달리 Windows10/11 native wide 파일 열기를 사용한다.
        if (site==BaseFileReadSite::DirectDisk && !path.has_root_directory()) {
            const auto root=NativePath(primary),drive=path.root_name(); //원본은게임폴더로chdir한다. cpppj는Windows10/11 호스트의CWD를바꾸지않으므로같은드라이브의상대경로만그게임폴더기준으로해석한다.
            if (drive.native().size()==2 && drive.native()[1]==L':' && AsciiLower(PathText(drive))==AsciiLower(PathText(root.root_name()))) path=root/path.relative_path();
        }
        return netstorm::o::TryRead(path);
    },fileNames_.get());
}
// 등록 archive만 조회하는 새 읽기 경계이며 기존 TryRead/Read의 disk 우선순위는 바꾸지 않는다.
std::optional<std::vector<std::uint8_t>> BaseFileSystem::TryReadArchiveOnly(std::string_view name,bool secondary) const {
    EnsurePrimaryArchives(); // 실제Startup뒤에는예약이없고독립Client조회만목록을준비한다.
    const auto& archives=secondary?secondaryArchives_:archives_; //서로 다른 원문 단일 연결 목록이다.
    for (const auto& archive:archives) if (archive.Find(name)) return archive.Read(name); //기존 등록 순서의 첫 일치만 읽고 어떤 디스크 경로도 호출하지 않는다.
    return std::nullopt;
}
// 외부 실행 경로의 존재 판정은 파일 내용 읽기와 독립이다.
bool BaseFileSystem::ContainsArchiveOnly(std::string_view name,bool secondary) const {
    EnsurePrimaryArchives(); // 내용읽기없이등록목록을준비한뒤같은메타데이터를조회한다.
    const auto& archives=secondary?secondaryArchives_:archives_; //외부 Help도 실제 두 집합을 같은 메타데이터 조회로 구별한다.
    for (const auto& archive:archives) if (archive.Find(name)) return true; //등록 순서의 첫 이름 일치에서 끝난다.
    return false;
}
// 원문 rb 열기/닫기와 TAFF 이름 조회만 사용한다. Windows10/11의 한글 경로는 기존 UTF-8/W 경계로 처리한다.
std::uint32_t BaseFileSystem::Probe(std::string_view name) const {
    return ProbeBaseFileByPolicy(name,[&](auto site,auto query) {
        if (site==BaseFileReadSite::PrimaryArchive || site==BaseFileReadSite::SecondaryArchive) return ContainsArchiveOnly(query,site==BaseFileReadSite::SecondaryArchive);
        auto selected=std::string(query);if (site!=BaseFileReadSite::DirectDisk) selected=JoinBaseFileRoot(PathText(std::filesystem::absolute(site==BaseFileReadSite::PrimaryDisk ? base_:secondary_).lexically_normal()),query,*fileNames_); // 원문처럼해당disk에도달했을때의현재root만작성하며metadata성공앞의불필요한root/CWD조회는하지않는다.
        auto path=NativePath(selected);
#ifdef _WIN32
        if (site==BaseFileReadSite::DirectDisk && !path.has_root_directory()) {
            const auto drive=path.root_name();if (drive.native().size()==2 && drive.native()[1]==L':') { const auto root=std::filesystem::absolute(base_).lexically_normal();if (AsciiLower(PathText(drive))==AsciiLower(PathText(root.root_name()))) path=root/path.relative_path(); } // 원본chdir대신같은드라이브상대이름만현재게임root로해석하며절대/UNC는root조회도하지않는다.
        }
        auto* file=::_wfopen(path.c_str(),L"rb");
#else
        auto* file=std::fopen(path.c_str(),"rb");
#endif
        if (!file) return false;static_cast<void>(std::fclose(file));return true;
    },fileNames_.get(),FileSpecDirectoryHooks(base_));
}
// 찾지 못하면 호출자에게 예외로 알린다.
std::vector<std::uint8_t> BaseFileSystem::Read(std::string_view name) const {
    auto bytes = TryRead(name);
    if (!bytes) throw std::runtime_error("Game file not found: " + std::string(name));
    return std::move(*bytes);
}
//0041ad10은 장치 이름을 존재 확인 없이 직접 반환한다. 일반 이름만 기본 disk/TAFF/보조 disk/TAFF를 열기·이름 조회로 찾으며0041b4c0 읽기 실패 재조회와 구별한다.
// 원본0041af80 접합 근거: docs/exe/cpp-login-template-lifetime-20261101.md.
bool BaseFileSystem::ReadOnly(std::string_view name) const {
    name=name.substr(0,name.find('\0'));if (name.empty()) throw std::logic_error("원본 Basefile.cpp 진단1152: fileName && *fileName");
    auto primary=PathText(std::filesystem::absolute(base_).lexically_normal()),secondary=PathText(std::filesystem::absolute(secondary_).lexically_normal()); //원문 getcwd/CD root를 실제 게임 자료 root로 대여하며 프로세스 CWD는 바꾸지 않는다.
    std::replace(primary.begin(),primary.end(),'/','\\');std::replace(secondary.begin(),secondary.end(),'/','\\');
    const bool device=!ReadBaseFileDevice(name,*fileNames_).empty(); // 원문41ad10의 장치 getter도 같은10칸 고리를 먼저 전진시킨다.
    const auto native=[&](std::string_view value) { //Windows10/11 Unicode 설치 이름은 원본 ANSI 대신 UTF-8/W CRT로 보존한다. 같은 드라이브의 상대 이름은 원본 게임 CWD 기준이다.
        auto path=NativePath(value);const auto drive=path.root_name();
        if (device && !path.has_root_directory() && drive.native().size()==2 && drive.native()[1]==L':' && AsciiLower(PathText(drive))==AsciiLower(PathText(NativePath(primary).root_name()))) path=NativePath(primary)/path.relative_path();
        return path;
    };
    const auto opens=[&](std::string_view value) { //0041ad10/41aae0의 rb 열기/즉시 닫기이며 길이·내용·암호 키에는 접근하지 않는다.
        const auto path=native(value);
#ifdef _WIN32
        auto* file=::_wfopen(path.c_str(),L"rb");
#else
        auto* file=std::fopen(path.c_str(),"rb");
#endif
        if (!file) return false;static_cast<void>(std::fclose(file));return true;
    };
    std::string resolved; //직접 출처5 또는 기본1/보조3에서 선택한 실제 stat 이름이며 아카이브 이름은 stat하지 않는다.
    if (device) resolved=name;
    else {
        resolved=JoinBaseFileRoot(primary,name,*fileNames_);
        if (!opens(resolved)) {
            if (ContainsArchiveOnly(name)) return true;
            resolved=JoinBaseFileRoot(secondary,name,*fileNames_);
            if (!opens(resolved)) {
                if (ContainsArchiveOnly(name,true)) return true;
                throw std::logic_error("원본 Basefile.cpp 진단1156: trueFileName");
            }
        }
    }
    const auto path=native(resolved);
#ifdef _WIN32
    struct _stat64 state{}; //원문 stat의 mode 쓰기 비트만 소비하며64비트 호스트의 파일 크기 표현은 판정에 쓰지 않는다.
    if (::_wstat64(path.c_str(),&state)!=0) throw std::logic_error("원본 Basefile.cpp 진단1166: !result");
    return (state.st_mode&_S_IWRITE)==0;
#else
    struct stat state{}; //Windows 이외 빌드 도구에서도 원문 소유자 쓰기 비트 계약만 보존한다.
    if (::stat(path.c_str(),&state)!=0) throw std::logic_error("원본 Basefile.cpp 진단1166: !result");
    return (state.st_mode&S_IWUSR)==0;
#endif
}
// 원본00459610/004597c0의 디스크 열거만 사용한다. 메뉴별 정렬은 이 원시 경계에서 수행하지 않는다.
std::vector<std::string> BaseFileSystem::MatchDisk(std::string_view pattern) const {
#ifdef _WIN32
    FileSpecQuery spec(pattern,FileSpecDirectoryHooks(base_));std::vector<std::string> names; // 원래 입력의 첫NUL/접두·slash·UNC와 실패 핸들까지 공통 FileSpec이 소유한다.
    while (spec.Valid()) { names.emplace_back(spec.Text());static_cast<void>(spec.Next()); } // 디렉터리/명시 부재 한 경로와 실제 OS 순서를 그대로 복사하고 같은 핸들을 끝에서 반납한다.
    return names;
#else
    throw std::logic_error("원본 Filespec의 실제 Win32 디스크 열거는 Windows에서만 지원합니다");
#endif
}
// 실제 FileFinder 목록은 기본 디스크/기본 TAFF/보조 디스크/보조 TAFF의 첫 열거 순서를 유지한다.
std::vector<std::string> BaseFileSystem::MatchFinder(std::string_view pattern) const {
    EnsurePrimaryArchives(); // 독립Client의최초목록조회도TAFF파일을누락하지않는다.
    const auto normalized=NormalizeGamePath(pattern);const auto wanted=NativePath(normalized);const auto directory=wanted.parent_path(),maskPath=wanted.filename();const auto mask=PathText(maskPath);std::vector<std::string> result;
    const auto append=[&](std::string name) { //587f0은 첫 문자열의 대소문자/바이트를 보존하고 나중 중복만 제외한다.
        std::replace(name.begin(),name.end(),'/','\\');if (name.empty() || name.front()!='\\') name.insert(name.begin(),'\\');
        const auto key=AsciiLower(name);if (std::none_of(result.begin(),result.end(),[&](const auto& old) { return AsciiLower(old)==key; })) result.push_back(std::move(name));
    };
    const auto disk=[&](const std::filesystem::path& root) { // FileSpec의 실제 Win32 순서를 가져와 원문의 기본 경로 접두부만 제거한다.
        const auto absoluteRoot=std::filesystem::absolute(root).lexically_normal();auto prefix=PathText(absoluteRoot);std::replace(prefix.begin(),prefix.end(),'/','\\');while (!prefix.empty() && prefix.back()=='\\') prefix.pop_back(); //끝 구분자는 반환 상대 이름에 한 번만 남기며 UTF-8 바이트를 보존한다.
        auto nativePattern=std::string(pattern);std::replace(nativePattern.begin(),nativePattern.end(),'/','\\'); // 현대 VFS의 '/' 경로 확장만 caller에서 native로 변환하며 원문 FileSpec 입력 계약은 바꾸지 않는다.
        const BaseFileSystem finder(root);for (const auto& name:finder.MatchDisk(nativePattern)) { // 디렉터리 속성도 FileSpec과 같이 제외하지 않는다.
            const auto path=NativePath(name);auto absolute=PathText(std::filesystem::absolute(path.is_absolute()?path:root/path).lexically_normal());std::replace(absolute.begin(),absolute.end(),'/','\\');
            if (absolute.size()>prefix.size() && absolute.compare(0,prefix.size(),prefix)==0 && absolute[prefix.size()]=='\\') append(absolute.substr(prefix.size())); // 원문처럼 경로 접두부 비교는 대소문자를 바꾸지 않는다.
        }
    };
    const auto matches=[&](std::string name) { // 현재 TAFF의 한 디렉터리에서 ASCII *·? 이름을 비교한다.
        name=AsciiLower(name);std::size_t n{},m{},star=std::string::npos,retry{};
        while (n<name.size()) { if (m<mask.size() && (mask[m]=='?' || mask[m]==name[n])) { ++m;++n; }else if (m<mask.size() && mask[m]=='*') { star=m++;retry=n; }else if (star!=std::string::npos) { m=star+1;n=++retry; }else return false; } // 마지막 별표의 소비 길이만 늘려 재시도한다.
        while (m<mask.size() && mask[m]=='*') ++m;return m==mask.size(); // 끝의 별표는 빈 문자열에도 일치한다.
    };
    disk(base_);for (const auto& archive:archives_) for (const auto& entry:archive.Entries()) { const auto name=NativePath(NormalizeGamePath(entry.name));if (name.parent_path()==directory && matches(PathText(name.filename()))) append(entry.name); } //현재 등록 TAFF/이름 인덱스 순서를 그대로 보존한다.
    if (base_.native()!=secondary_.native()) {
        disk(secondary_);
        for (const auto& archive:secondaryArchives_) for (const auto& entry:archive.Entries()) { const auto name=NativePath(NormalizeGamePath(entry.name));if (name.parent_path()==directory && matches(PathText(name.filename()))) append(entry.name); } //보조 목록도 같은 등록/이름 인덱스 순서이며 기본과의 중복은 append가 제거한다.
    }
    return result; //원문은 두 경로 문자열이 같으면 보조 디스크와 TAFF 탐색을 함께 생략한다.
}
// 기존 정렬 목록 API다. 실제 Tell은 index 확정 전에 정렬하지 않는 MatchFinder를 사용한다.
std::vector<std::string> BaseFileSystem::Match(std::string_view pattern) const {
    EnsurePrimaryArchives(); // 기존정렬API의등록목록계약을같이유지한다.
    const auto normalized = NormalizeGamePath(pattern);
    const auto path=NativePath(normalized);
    const auto directory = path.parent_path();
    const auto mask=PathText(path.filename());
    // 파일 이름의 *·?를 ASCII 대소문자 없이 비교한다.
    const auto matches = [&mask](std::string name) {
        name = AsciiLower(name); std::size_t n = 0, m = 0, star = std::string::npos, retry = 0;
        // 실패할 때 직전 *의 소비 길이를 한 문자 늘린다.
        while (n < name.size()) {
            if (m < mask.size() && (mask[m] == '?' || mask[m] == name[n])) { ++m; ++n; }
            else if (m < mask.size() && mask[m] == '*') { star = m++; retry = n; }
            else if (star != std::string::npos) { m = star + 1; n = ++retry; }
            else return false;
        }
        // 마지막 *는 빈 문자열에도 일치한다.
        while (m < mask.size() && mask[m] == '*') ++m;
        return m == mask.size();
    };
    std::vector<std::string> result;
    // 디스크 파일은 이름만 읽고 내용은 변경하지 않는다.
    const auto disk = [&](const std::filesystem::path& root) {
        const auto folder = path.is_absolute() ? directory : root / directory;
        if (!std::filesystem::is_directory(folder)) return;
        // 요청한 한 디렉터리만 열거한다.
        for (const auto& entry:std::filesystem::directory_iterator(folder)) if (entry.is_regular_file() && matches(PathText(entry.path().filename())))
            result.push_back(PathText(directory/entry.path().filename()));
    };
    disk(base_);
    // 아카이브 이름도 같은 디렉터리·패턴으로 거른다.
    for (const auto& archive : archives_)
        // 원본 디렉터리의 엔트리를 읽기만 한다.
        for (const auto& entry : archive.Entries()) {
            const auto name=NativePath(NormalizeGamePath(entry.name));
            if (name.parent_path()==directory && matches(PathText(name.filename()))) result.push_back(PathText(name));
        }
    disk(secondary_);
    for (const auto& archive:secondaryArchives_) { //기존 정렬 API도 보조 TAFF의 이름을 포함한다.
        for (const auto& entry:archive.Entries()) { //각 아카이브의 원래 이름 인덱스를 읽고 최종 정렬/중복 제거는 기존 경계에서 수행한다.
            const auto name=NativePath(NormalizeGamePath(entry.name));if (name.parent_path()==directory && matches(PathText(name.filename()))) result.push_back(PathText(name));
        }
    }
    std::sort(result.begin(), result.end(), [](const std::string& a, const std::string& b) { return AsciiLower(a) < AsciiLower(b); });
    result.erase(std::unique(result.begin(), result.end(), [](const std::string& a, const std::string& b) { return AsciiLower(a) == AsciiLower(b); }), result.end());
    return result;
}
}
