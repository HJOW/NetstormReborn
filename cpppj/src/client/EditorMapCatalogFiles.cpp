// source-first 합성 대신 최초 loose 이관 뒤 실제 한 작업 디렉터리의 OS 순서만 사용한다.
#include "client/EditorMapCatalogFiles.h"
#include "client/EditorFiles.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX //metadata 잠금 장치의 Win32 매크로가 표준 수치 범위를 바꾸지 않게 한다.
#endif
#include <windows.h>
#endif

namespace netstorm::client { //게임 State/목록 규칙은 기존 원문 몸체에 남긴다.
namespace { //장부 형식과 실제 root 경계의 공통 계산을 모은다.
constexpr std::array<std::uint8_t,8> kMagic{'N','S','E','F','O','R','T',1}; //UTF-8 경로 바이트를 담는 버전1 binary 장부 서명이다.
constexpr std::uint8_t kPending=1,kCompleted=2; //복사 전 durable 표시와 최초 이관 완료/이후 삭제 보존 표시다.
constexpr std::string_view kLedger="catalog-v1.dat"; //work 바깥의 검증된 형제-state 안에서만 사용하는 장부 이름이다.
// root 요소 경계와 원본 ASCII 대소문자 비교를 함께 사용한다.
bool Inside(const std::filesystem::path& root,const std::filesystem::path& path) { auto a=o::AsciiLower(root.generic_string());const auto b=o::AsciiLower(path.generic_string());if (a==b) return true;if (!a.ends_with('/')) a+='/';return b.starts_with(a); }
// 존재하는 부모의 reparse를 해석하고 없는 꼬리는 절대 정상 경로로 유지한다.
std::filesystem::path Actual(const std::filesystem::path& path) { return std::filesystem::weakly_canonical(std::filesystem::absolute(path)); }
// root 대소문자와 무관하게 검증된 전체 상대 경로를 얻고 첫점/파일명을 축약하지 않는다.
std::string Under(const std::filesystem::path& root,const std::filesystem::path& path) {
    const auto prefix=root.generic_string(),name=path.generic_string();if (!Inside(root,path)) throw std::invalid_argument("편집 이관 root 상대 범위 위반");return name.size()==prefix.size() ? std::string(".") : name.substr(prefix.size()+1);
}
// source/work identity는 고정 UTF-8 바이트로 기록하여 호스트 코드페이지 출력을 쓰지 않는다.
std::string Identity(const std::filesystem::path& path) { const auto bytes=path.generic_u8string();return o::AsciiLower({reinterpret_cast<const char*>(bytes.data()),bytes.size()}); }
// binary 장부 DWORD를 little endian으로만 기록한다.
void Word(std::vector<std::uint8_t>& bytes,std::uint32_t value) { for (unsigned shift=0;shift<32;shift+=8) bytes.push_back(static_cast<std::uint8_t>(value>>shift)); } //네 BYTE의 물리 순서를 유지한다.
// 장부 문자열은 길이와 원시 바이트를 기록하며 첫 NUL 절단을 추가하지 않는다.
void Text(std::vector<std::uint8_t>& bytes,std::string_view value) { if (value.size()>std::numeric_limits<std::uint32_t>::max()) throw std::length_error("편집 이관 장부 문자열 범위");Word(bytes,static_cast<std::uint32_t>(value.size()));bytes.insert(bytes.end(),value.begin(),value.end()); }
// 손상 장부를 빈 장부로 바꾸지 않는 경계 검사 reader다.
struct LedgerReader {
    std::span<const std::uint8_t> bytes;std::size_t at{}; //같은 읽기 원문과 현재 물리 cursor다.
    std::uint32_t Word() { if (at>bytes.size() || bytes.size()-at<4) throw std::runtime_error("편집 이관 장부 DWORD 손상");std::uint32_t value{};for (unsigned shift=0;shift<32;shift+=8) value|=std::uint32_t(bytes[at++])<<shift;return value; } //네 BYTE 밖 접근은 명시 실패다.
    std::string Text() { const auto size=Word();if (size>bytes.size()-at) throw std::runtime_error("편집 이관 장부 문자열 손상");std::string value(reinterpret_cast<const char*>(bytes.data()+at),size);at+=size;return value; } //길이 검사 뒤 원문을 그대로 빌려 복사한다.
};
}
// 모든 경로는 같은 장부/잠금을 거치고 새 Session마다 이관 정책을 복제하지 않는다.
struct EditorCatalogWorkspace::Impl {
    std::shared_ptr<EditorFiles> files;EditorCatalogSourceDevices source;std::filesystem::path sourceRoot;EditorFiles metadata; //work/source/형제 metadata의 실제 수명과 명시 루트다.
    std::map<std::string,std::uint8_t> entries;bool loaded{},poisoned{}; //물리 전체 키의 durable 상태와 같은 owner의 durable 갱신 실패 표식이다.
#ifdef _WIN32
    HANDLE lock{INVALID_HANDLE_VALUE}; //한 metadata namespace의 복수 프로세스 이관을 명시 거부하는 실제 잠금이다.
#endif
    Impl(std::shared_ptr<EditorFiles> work,EditorCatalogSourceDevices devices):files(std::move(work)),source(std::move(devices)),sourceRoot(Actual(source.root)),metadata(files->SiblingNamespace("-state")) { //생성은 root만 검증하고 장부/잠금은 첫 I/O까지 열지 않는다.
        if (Inside(sourceRoot,files->RootPath()) || Inside(files->RootPath(),sourceRoot)) throw std::invalid_argument("편집 source/work root 중첩");
    }
    ~Impl() {
#ifdef _WIN32
        if (lock!=INVALID_HANDLE_VALUE) CloseHandle(lock); //이미 완료된 장부는 보존하고 OS 잠금만 반납한다.
#endif
    }
    // 절대 source/work 표기는 명시 root 전체로 역매핑하고 상대 경로는 기존 게임 표기를 정규화한다.
    std::string Key(std::string_view configured) const {
        if (configured.find('\0')!=std::string_view::npos) throw std::invalid_argument("편집 이관 경로 NUL 범위");std::filesystem::path input(configured);std::string relative;
        if (!input.is_absolute()) { relative=std::string(configured);std::replace(relative.begin(),relative.end(),'\\','/');if (relative.starts_with("./")) relative.erase(0,2);const auto first=relative.find_first_not_of('/');relative=first==std::string::npos ? std::string{} : relative.substr(first); } //OS 파일명 표기는 보존하고 장부 비교에서만 ASCII case를 접는다.
        else {
            input=std::filesystem::absolute(input).lexically_normal();
            if (Inside(files->RootPath(),input)) relative=files->RelativePath(input.string()).generic_string(); //work 내부 reparse 거부를 source 매핑으로 우회하지 않는다.
            else if (Inside(sourceRoot,input)) { const auto parent=Actual(input.parent_path());if (!Inside(sourceRoot,parent)) throw std::invalid_argument("편집 source 폴더 링크 범위");relative=Under(sourceRoot,input); }
            else { const auto actual=Actual(input);if (Inside(files->RootPath(),actual)) relative=files->RelativePath(actual.string()).generic_string();else if (Inside(sourceRoot,actual)) relative=Under(sourceRoot,actual);else throw std::invalid_argument("편집 이관 외부 절대 경로"); }
        }
        relative=std::filesystem::path(relative).lexically_normal().generic_string();return files->RelativePath(relative).generic_string();
    }
    // source reparse도 명시 root 안인지 검사하고 원본에는 어떤 쓰기도 하지 않는다.
    std::filesystem::path SourcePath(std::string_view key) const { const auto path=sourceRoot/std::filesystem::path(key);if (!Inside(sourceRoot,Actual(path))) throw std::invalid_argument("편집 원본 입력의 외부 링크 경계");return path; }
    // lock 획득과 완전한 장부 읽기가 성공해야 실제 이관/현재 읽기를 계속한다.
    void Load() {
        if (poisoned) throw std::runtime_error("편집 이관 장부 갱신 실패 owner의 재사용 금지");if (loaded) return;metadata.EnsureDirectory(".");
#ifdef _WIN32
        if (lock==INVALID_HANDLE_VALUE) { const auto path=metadata.ResolvePath("catalog.lock");lock=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if (lock==INVALID_HANDLE_VALUE) throw std::runtime_error("편집 이관 장부가 다른 owner에서 사용 중입니다"); }
#endif
        const auto bytes=metadata.TryRead(kLedger);if (!bytes) { if (std::filesystem::exists(metadata.ResolvePath(kLedger))) throw std::runtime_error("기존 편집 이관 장부 읽기 실패");loaded=true;return; } //존재/접근 오류와 실제 부재를 분리하여 tombstone을 새 빈 장부로 덮지 않는다.
        if (bytes->size()<kMagic.size() || !std::equal(kMagic.begin(),kMagic.end(),bytes->begin())) throw std::runtime_error("편집 이관 장부 버전/서명 손상");
        LedgerReader read{*bytes,kMagic.size()};if (read.Text()!=Identity(sourceRoot) || read.Text()!=Identity(files->RootPath())) throw std::runtime_error("편집 이관 장부 root identity 불일치");const auto count=read.Word();std::map<std::string,std::uint8_t> current;
        for (std::uint32_t index=0;index<count;++index) { //전체 항목을 검사한 뒤에만 현재 장부를 채택한다.
            const auto key=read.Text();const auto state=read.Word();if ((state!=kPending && state!=kCompleted) || o::AsciiLower(Key(key))!=key || !current.emplace(key,static_cast<std::uint8_t>(state)).second) throw std::runtime_error("편집 이관 장부 항목 손상");
        }
        if (read.at!=bytes->size()) throw std::runtime_error("편집 이관 장부 끝 범위 손상");entries=std::move(current);loaded=true;
    }
    // durable 갱신 실패 뒤는 현재 owner도 계속 성공 처리하지 못하도록 loaded를 먼저 내린다.
    void State(const std::string& key,std::uint8_t state) {
        auto next=entries;next[o::AsciiLower(key)]=state;std::vector<std::uint8_t> bytes(kMagic.begin(),kMagic.end());Text(bytes,Identity(sourceRoot));Text(bytes,Identity(files->RootPath()));if (next.size()>std::numeric_limits<std::uint32_t>::max()) throw std::length_error("편집 이관 장부 항목 수 범위");Word(bytes,static_cast<std::uint32_t>(next.size()));
        for (const auto& [name,value]:next) { Text(bytes,name);Word(bytes,value); } //정렬된 장부는 OS 목록 순서와 별개이며 단순 durable 직렬화다.
        loaded=false;poisoned=true;metadata.WriteAtomic(kLedger,bytes);entries=std::move(next);loaded=true;poisoned=false;
    }
    // 한번 완료된 경로가 사라져도 복사하지 않는다. 중단 Pending은 실제 대상이 있을 때만 완료한다.
    void Import(const std::string& key,const std::optional<std::vector<std::uint8_t>>& archive=std::nullopt) {
        Load();const auto target=files->ResolvePath(key);const auto found=entries.find(o::AsciiLower(key));const auto exists=std::filesystem::exists(target);
        if (found!=entries.end()) { if (found->second==kCompleted) return;if (!exists) throw std::runtime_error("편집 이관 중단 Pending 항목의 작업본이 없습니다:"+key);State(key,kCompleted);return; }
        if (exists) { State(key,kCompleted);return; }
        const auto physical=SourcePath(key);const auto present=std::filesystem::exists(physical);if (!present && !archive) return; //명시 source 부재는 이관 완료나 Pending으로 기록하지 않는다.
        State(key,kPending);
        if (present && std::filesystem::is_directory(physical)) files->EnsureDirectory(key); //디렉터리는 내용 복사 없이 실제 후보 이름만 대응시킨다.
        else if (present) files->CopyMissing(physical,key);
        else static_cast<void>(files->PreparePath(key,*archive)); //명시 archive 실제 설치만 작업 파일/이관 상태를 만든다.
        State(key,kCompleted);
    }
};
// 파일 장치의 실제 shared owner가 없으면 아직 어떤 경로/장부도 사용하지 않는다.
EditorCatalogWorkspace::EditorCatalogWorkspace(std::shared_ptr<EditorFiles> files,EditorCatalogSourceDevices source) {
    if (!files || source.root.empty()) throw std::invalid_argument("편집 workspace의 명시 파일/source root 누락");impl_=std::make_unique<Impl>(std::move(files),std::move(source));
}
EditorCatalogWorkspace::~EditorCatalogWorkspace()=default; //동기 UI/Session이 끝난 뒤 실제 잠금을 반납한다.
EditorFiles& EditorCatalogWorkspace::Files() const { return *impl_->files; } //Native 파일 부모의 기존 장치를 그대로 빌린다.
std::filesystem::path EditorCatalogWorkspace::Resolve(std::string_view configuredPath) const { return impl_->files->ResolvePath(impl_->Key(configuredPath)); } //파일명/첫점 추정 없이 같은 물리 전체 키를 쓴다.
void EditorCatalogWorkspace::ImportDisk(std::string_view configuredPattern) {
    const auto pattern=impl_->Key(configuredPattern);const std::filesystem::path requested(pattern);const o::BaseFileSystem source(impl_->sourceRoot);
    if (requested.filename().string().find_first_of("*?")!=std::string::npos && std::filesystem::is_directory(impl_->SourcePath(requested.parent_path().string()))) impl_->files->EnsureDirectory(requested.parent_path().string()); //빈 source 폴더의 . / .. OS 후보도 실제 빈 작업 폴더에서 열거한다.
    for (const auto& path:source.MatchDisk(pattern)) { const auto leaf=std::filesystem::path(path).filename().string();if (leaf=="." || leaf=="..") continue;impl_->Import(impl_->Key(path)); } //점 두 후보는 복사/장부를 만들지 않고 실제 work 열거가 그대로 돌려준다.
}
std::filesystem::path EditorCatalogWorkspace::Prepare(std::string_view configuredPath) {
    const auto key=impl_->Key(configuredPath);impl_->Import(key);if (impl_->entries.contains(o::AsciiLower(key)) || std::filesystem::exists(impl_->files->ResolvePath(key))) return impl_->files->ResolvePath(key);
    if (impl_->source.archiveOnly) { const auto bytes=impl_->source.archiveOnly(key);if (bytes) impl_->Import(key,bytes); }return impl_->files->ResolvePath(key); //완료 missing에서는 archive를 조회하지 않는다.
}
std::vector<std::string> EditorCatalogWorkspace::Enumerate(std::string_view configuredPattern) { const auto key=impl_->Key(configuredPattern);ImportDisk(configuredPattern);return impl_->files->MatchDisk(key); } //가공 없는 실제 work OS 결과를 원문 목록 몸체로 넘긴다.
std::optional<std::vector<std::uint8_t>> EditorCatalogWorkspace::Read(std::string_view configuredPath) {
    const auto key=impl_->Key(configuredPath);if (key.find_first_of("*?")==std::string::npos) { impl_->Import(key);if (impl_->entries.contains(o::AsciiLower(key))) return impl_->files->TryRead(key); }if (auto bytes=impl_->files->TryRead(key)) return bytes;
    return impl_->source.archiveOnly ? impl_->source.archiveOnly(key) : std::nullopt; //이관되지 않은 archive-only 내용은 loose 목록/장부에 추가하지 않는다.
}
}
