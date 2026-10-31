//읽기 금지 조회의 실제 출처 우선순위와 내용 읽기 없는 Unicode CRT 경계를 검증한다.
#define NOMINMAX
#include "TestSupport.h"
#include "o/BaseFile.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <stdexcept>

//자체 임시 파일만 만들고 원본 자료나 실행 창을 사용하지 않는 검사 장치다.
namespace {
using namespace netstorm::o;
std::atomic<std::uint32_t> readonlySequence{}; //동일 프로세스의 연속 검사가 공유하지 않는 자체 임시 이름 번호다.
//실제 OS 임시 부모와 접두를 재확인한 자체 폴더만 정리한다.
struct ReadOnlyTemporary {
    std::filesystem::path parent=std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()); //삭제를 허용한 실제 임시 부모다.
    std::filesystem::path path=parent/(L"netstorm-readonly-667-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(readonlySequence++)+L"-한글😀"); //Unicode 파일시스템 경계를 포함한 새 폴더다.
    ReadOnlyTemporary() { if (!std::filesystem::create_directory(path)) throw std::runtime_error("읽기 금지 검사 임시 경로 중복"); } //기존 경로에는 새 입력을 쓰지 않는다.
    ~ReadOnlyTemporary() { //읽기 금지 속성을 되돌리고 검증된 자체 하위만 삭제한다.
        std::error_code error;const auto actual=std::filesystem::weakly_canonical(path,error);
        if (error || actual.parent_path()!=parent || !actual.filename().native().starts_with(L"netstorm-readonly-667-")) return;
        for (const auto& entry:std::filesystem::recursive_directory_iterator(actual,error)) { //자체 입력의 읽기 금지 속성만 반납하여 실패한 단언 뒤에도 임시 파일을 남기지 않는다.
            if (entry.is_regular_file(error)) static_cast<void>(SetFileAttributesW(entry.path().c_str(),FILE_ATTRIBUTE_NORMAL));
        }
        std::filesystem::remove_all(actual,error);
    }
};
//Config/VFS가 사용하는 UTF-8 이름을 실제 native 파일 경로에서 만든다.
std::string ReadOnlyUtf8(const std::filesystem::path& path) { const auto bytes=path.u8string();return {bytes.begin(),bytes.end()}; }
//내용은 중요하지 않은 한 BYTE 파일을 생성하며 닫기까지 검사 입력 준비에서 마친다.
void WriteReadOnlyInput(const std::filesystem::path& path) { std::ofstream file(path,std::ios::binary);file.put('Q');if (!file) throw std::runtime_error("읽기 금지 검사 파일 생성 실패"); }
//내용이 없는 단일 d/a 엔트리를 가진 실제 TAFF를 만든다. 읽기 금지 조회에는 엔트리 이름만 필요하다.
void WriteReadOnlyArchive(const std::filesystem::path& path) {
    std::vector<std::uint8_t> bytes(0x3d);const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),bytes.begin());
    const std::array<std::pair<std::size_t,std::uint32_t>,5> fields{{{0x14,1},{0x1c,0x2c},{0x20,0x30},{0x24,13},{0x28,0x3d}}}; //단일 인덱스/엔트리/빈 내용물의 입력 오프셋이다.
    for (const auto [offset,value]:fields) { //헤더 DWORD를 호스트 엔디언에 의존하지 않고 기록한다.
        for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); //현재 DWORD의 낮은 BYTE부터 기록한다.
    }
    const std::string name="\\d\\a";std::copy(name.begin(),name.end(),bytes.begin()+0x38);std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    if (!file) throw std::runtime_error("읽기 금지 검사 TAFF 생성 실패");
}
//원문 진단 행 번호를 확인하되 예상하지 않은 다른 예외는 그대로 실패시킨다.
bool ReadOnlyDiagnostic(const BaseFileSystem& files,std::string_view name,std::string_view line) {
    try { static_cast<void>(files.ReadOnly(name)); }catch (const std::logic_error& error) { return std::string_view(error.what()).find(line)!=std::string_view::npos; }return false;
}
//배타적인 실제 Win32 파일 핸들을 정상/실패 모두에서 닫는다.
struct ReadOnlyExclusiveFile {
    HANDLE handle=INVALID_HANDLE_VALUE; //내용 읽기를 막는 검사 전용 핸들이며 VFS에 전달하지 않는다.
    explicit ReadOnlyExclusiveFile(const std::filesystem::path& path):handle(CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)) { if (handle==INVALID_HANDLE_VALUE) throw std::runtime_error("읽기 금지 검사 배타적 열기 실패"); } //파일 자체는 바꾸지 않는다.
    ~ReadOnlyExclusiveFile() { if (handle!=INVALID_HANDLE_VALUE) CloseHandle(handle); } //폴더 정리보다 먼저 열린 핸들을 반납한다.
};
}

//Unicode 상대/직접 이름의 실제 CRT mode와 NUL 종료·빈 이름·미발견·직접 stat 실패를 구별한다.
TEST_CASE(BaseFile_ReadOnlyUnicodeDiskAndOriginalDiagnostics) {
    ReadOnlyTemporary work;std::filesystem::create_directory(work.path/L"d");const auto path=work.path/L"d/맵😀.fort";WriteReadOnlyInput(path);BaseFileSystem files(work.path);
    const auto relative=ReadOnlyUtf8(std::filesystem::path(L"d/맵😀.fort")),absolute=ReadOnlyUtf8(path);CHECK(!files.ReadOnly(relative) && !files.ReadOnly(absolute));
    CHECK(SetFileAttributesW(path.c_str(),FILE_ATTRIBUTE_READONLY)!=0);CHECK(files.ReadOnly(relative) && files.ReadOnly(absolute));CHECK(files.ReadOnly(relative+std::string("\0unused",7)));
    CHECK(ReadOnlyDiagnostic(files,"","1152"));CHECK(ReadOnlyDiagnostic(files,"d/missing","1156"));CHECK(ReadOnlyDiagnostic(files,ReadOnlyUtf8(work.path/L"d/missing"),"1166"));
}

//기본 디스크→기본 TAFF→보조 디스크→보조 TAFF 순서이며 절대 장치 이름 실패는 읽기 정책처럼 TAFF로 재조회하지 않는다.
TEST_CASE(BaseFile_ReadOnlyUsesIndependentResolverPriority) {
    ReadOnlyTemporary work;const auto base=work.path/L"base",secondary=work.path/L"secondary";std::filesystem::create_directories(base/L"d");std::filesystem::create_directories(secondary/L"d");
    WriteReadOnlyArchive(base/L"one.tarc");WriteReadOnlyArchive(secondary/L"one.tarc");WriteReadOnlyInput(base/L"d/a");WriteReadOnlyInput(secondary/L"d/a");
    BaseFileSystem files(base,secondary),withoutPrimaryArchive(base,secondary);CHECK(files.RegisterArchive(base/L"one.tarc"));CHECK(files.RegisterArchive(secondary/L"one.tarc",true));CHECK(withoutPrimaryArchive.RegisterArchive(secondary/L"one.tarc",true));
    CHECK(!files.ReadOnly("d/a"));std::filesystem::remove(base/L"d/a");CHECK(files.ReadOnly("d/a"));CHECK(!withoutPrimaryArchive.ReadOnly("d/a"));
    std::filesystem::remove(secondary/L"d/a");CHECK(withoutPrimaryArchive.ReadOnly("d/a"));CHECK(ReadOnlyDiagnostic(files,ReadOnlyUtf8(base/L"d/a"),"1166"));
    std::filesystem::remove(base/L"one.tarc");std::filesystem::remove(secondary/L"one.tarc");CHECK(files.ReadOnly("d/a") && withoutPrimaryArchive.ReadOnly("d/a")); //등록된 TAFF 이름만 조회하며 원래 아카이브 파일을 다시 열지 않는다.
}

//직접 출처5는 내용물을 열 수 없는 파일도 stat만으로 판정하며 Read의 전체 바이트 읽기를 호출하지 않는다.
TEST_CASE(BaseFile_ReadOnlyDirectStatDoesNotOpenOrReadPayload) {
    ReadOnlyTemporary work;const auto path=work.path/L"독점😀.fort";WriteReadOnlyInput(path);BaseFileSystem files(work.path);ReadOnlyExclusiveFile locked(path);
    CHECK(!files.ReadOnly(ReadOnlyUtf8(path)));bool readFailed{};try { static_cast<void>(files.Read(ReadOnlyUtf8(path))); }catch (const std::runtime_error&) { readFailed=true; }CHECK(readFailed);
}
