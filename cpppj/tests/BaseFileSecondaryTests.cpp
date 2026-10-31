// 독립 PE의 등록 결과와 두 TAFF 목록/실제 디스크의 우선순위를 확인한다.
#define NOMINMAX
#include "RawSceneSupport.h"
#include "client/ClientMain.h"
#include "client/TechnicalHelp.h"
#include "o/BaseFile.h"
#include <windows.h>
#include <atomic>
#include <filesystem>
#include <fstream>

// 입력 생성과 자체 임시 폴더만 소유하며 원본 파일은 읽거나 수정하지 않는다.
namespace {
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// 같은 프로세스의 빠른 연속 사례에도 임시 이름이 겹치지 않게 한다.
std::atomic<std::uint32_t> nextTemporary{};
// 절대 임시 부모/접두를 확인한 자체 폴더만 정리한다.
struct TaffTemporary {
    std::filesystem::path parent=std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()); // OS가 선택한 실제 임시 부모다.
    std::filesystem::path path=parent/(L"netstorm-taff-643-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(nextTemporary++)); // 현재 검사만 생성하는 명시 이름이다.
    TaffTemporary() { if (!std::filesystem::create_directory(path)) throw std::runtime_error("TAFF 검사 임시 경로 중복"); } //기존 경로에는 새 입력을 쓰지 않는다.
    ~TaffTemporary() { // symlink/상위 경로로 바뀐 대상은 재귀 삭제하지 않는다.
        std::error_code error;const auto actual=std::filesystem::weakly_canonical(path,error);
        if (!error && actual.parent_path()==parent && actual.filename().native().starts_with(L"netstorm-taff-643-")) std::filesystem::remove_all(actual,error);
    }
};
// Native 관측기와 같은 비순차 인덱스/두 한 BYTE 내용물을 입력한다. 반환 기대값은 계산하지 않는다.
std::vector<std::uint8_t> TaffInput(std::string_view kind) {
    std::vector<std::uint8_t> data(0x56);const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),data.begin());
    const std::array<std::pair<std::size_t,std::uint32_t>,11> fields{{{0x14,2},{0x1c,0x2c},{0x20,0x34},{0x24,0x20},{0x28,0x54},{0x2c,0x10},{0x30,0},{0x34,0},{0x38,1},{0x44,1},{0x48,1}}}; // 실제 헤더/이름 인덱스/디렉터리 입력이다.
    for (const auto [offset,value]:fields) Put(data,offset,value); // 입력 DWORD만 원래 폭으로 쓴다.
    const std::string first="\\d\\a",second="\\d\\b";std::copy(first.begin(),first.end(),data.begin()+0x3c);std::copy(second.begin(),second.end(),data.begin()+0x4c);
    data[0x54]=kind=="other"?'X':'A';data[0x55]=kind=="other"?'Y':'B';
    if (kind=="bad-magic") data[0]=0;else if (kind=="bad-nul") data[10]='A';else if (kind=="short-header") data.resize(20);else if (kind=="short-index") data.resize(0x30);else if (kind=="short-directory") data.resize(0x50);
    return data;
}
// 명시 scratch 파일만 교체한다. 원본 게임이나 다른 사용자의 파일을 대상으로 받지 않는다.
void WriteTaff(const std::filesystem::path& path,std::string_view kind) {
    if (kind=="missing") { std::filesystem::remove(path);return; }
    const auto bytes=TaffInput(kind);std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    if (!file) throw std::runtime_error("TAFF 검사 입력 쓰기 실패");
}
// 기본/보조 디스크의 한 BYTE를 준비한다.
void WriteByte(const std::filesystem::path& path,char byte) { std::ofstream file(path,std::ios::binary);file.put(byte);if (!file) throw std::runtime_error("TAFF 검사 디스크 입력 쓰기 실패"); }
// 현재 Windows 경로를 Config가 사용하는 UTF-8로 전달한다.
std::string TaffUtf8(const std::filesystem::path& path) { const auto bytes=path.u8string();return {bytes.begin(),bytes.end()}; }
}

// 두 목록의 독립성·첫 등록·대소문자 중복·실패 후 재시도를 실제 PE27사례와 비교한다.
TEST_CASE(BaseFile_SecondaryReplaysIndependentNativeRegistration27) {
    const auto rows=LoadFixture(NETSTORM_TAFF_REGISTRATION_FIXTURE).rows;std::size_t count{}; // 최초 Native 반환만 기대값으로 사용한다.
    for (const auto& row:rows) { // 매 관측을 새로운 VFS/자체 scratch에서 재생한다.
        TaffTemporary work;BaseFileSystem files(work.path);std::vector<std::string> returns;
        for (const auto& request:Split(row.at(0),';')) { // 같은 물리 파일 교체도 원문 입력 순서 그대로 수행한다.
            const auto fields=Split(request,',');const auto path=work.path/fields.at(1);WriteTaff(path,fields.at(2));returns.push_back(files.RegisterArchive(path,fields.at(0)=="1")?"1":"0");
        }
        std::string actual;for (const auto& value:returns) { if (!actual.empty()) actual+=',';actual+=value; } // Native의 실제 cdecl 반환 배열 형식이다.
        CHECK(actual==row.at(1));CHECK(files.ContainsArchiveOnly("d/a")==!row.at(2).empty());CHECK(files.ContainsArchiveOnly("d/a",true)==!row.at(3).empty());++count;
    }
    CHECK(count==27);
}

// 기본 디스크/기본 TAFF/보조 디스크/보조 TAFF의 네 단계와 각 목록의 첫 등록을 확인한다.
TEST_CASE(BaseFile_SecondaryReadPriorityAndSeparateArchiveOnlyViews) {
    TaffTemporary base,secondary;std::filesystem::create_directory(base.path/"d");std::filesystem::create_directory(secondary.path/"d");
    WriteTaff(base.path/"one.tarc","valid");WriteTaff(secondary.path/"one.tarc","other");BaseFileSystem files(base.path,secondary.path);
    CHECK(files.RegisterArchive(secondary.path/"one.tarc",true));WriteByte(secondary.path/"d/a",'S');WriteByte(base.path/"d/a",'P');
    CHECK(files.Read("d/a")==std::vector<std::uint8_t>{'P'});std::filesystem::remove(base.path/"d/a");CHECK(files.Read("d/a")==std::vector<std::uint8_t>{'S'});
    CHECK(files.RegisterArchive(base.path/"one.tarc"));CHECK(files.Read("d/a")==std::vector<std::uint8_t>{'A'});
    CHECK(files.TryReadArchiveOnly("d/a")==std::vector<std::uint8_t>{'A'} && files.TryReadArchiveOnly("d/a",true)==std::vector<std::uint8_t>{'X'});
    BaseFileSystem onlySecondary(base.path,secondary.path);CHECK(onlySecondary.RegisterArchive(secondary.path/"one.tarc",true));std::filesystem::remove(secondary.path/"d/a");CHECK(onlySecondary.Read("d/a")==std::vector<std::uint8_t>{'X'});
    CHECK(!onlySecondary.ContainsArchiveOnly("d/a") && onlySecondary.ContainsArchiveOnly("d/a",true));
    onlySecondary.SetSecondaryDirectory(base.path);CHECK(onlySecondary.Read("d/a")==std::vector<std::uint8_t>{'X'}); //원문 root 변경은 등록 목록을 해제하지 않는다.
}

// 자동 등록은 Win32 순서를 그대로 사용하고 실패 후보/디렉터리 하나가 정상 후보 등록을 막지 않는다.
TEST_CASE(BaseFile_SecondaryAutomaticRegistrationAndFinderPreserveNativeOrder) {
    TaffTemporary base,secondary;const auto korean=base.path/L"한글😀";std::filesystem::create_directory(korean);std::filesystem::create_directory(korean/L"folder.tarc");
    WriteTaff(korean/"z.tarc","valid");WriteTaff(korean/"a.tarc","other");WriteTaff(korean/"bad.tarc","bad-nul");
    WIN32_FIND_DATAW data{};const auto handle=FindFirstFileW((korean/L"*.tarc").c_str(),&data);std::optional<std::uint8_t> first; //동일 OS의 독립 열거에서 첫 정상 파일만 관측한다.
    if (handle!=INVALID_HANDLE_VALUE) { do { if (std::wstring_view(data.cFileName)==L"z.tarc") { first='A';break; }if (std::wstring_view(data.cFileName)==L"a.tarc") { first='X';break; } }while (FindNextFileW(handle,&data));FindClose(handle); } //제품의 열거/등록 함수를 사용해 기대 순서를 계산하지 않는다.
    if (!first) throw std::runtime_error("TAFF 검사 독립 Win32 열거 실패");BaseFileSystem files(korean,secondary.path);files.RegisterArchives();CHECK(files.Read("d/a")==std::vector<std::uint8_t>{*first});
    WriteTaff(secondary.path/"only.tarc","valid");files.RegisterArchives(true);CHECK(files.MatchFinder("d\\*")==std::vector<std::string>({"\\d\\b","\\d\\a"}));
    CHECK(files.Match("d/*")==std::vector<std::string>({"d/a","d/b"})); //후보 중복은 첫 이름만 남기고 기존 정렬 API는 정렬을 유지한다.
    std::filesystem::create_directory(korean/L"d");WriteByte(korean/L"d/유니코드😀.english",'Q'); //실제 Unicode 파일 이름도 같은 코드 페이지 독립 경계로 왕복한다.
    CHECK(files.MatchFinder("d\\*.english")==std::vector<std::string>{"\\d\\유니코드😀.english"});CHECK(files.Match("d/*.english")==std::vector<std::string>{"d/유니코드😀.english"});CHECK(files.Read("d/유니코드😀.english")==std::vector<std::uint8_t>{'Q'});
}

// Run/창/외부 실행 없이 실제 Client의 기본/보조 등록과 Help source4의 이름 판정을 연결한다.
TEST_CASE(BaseFile_SecondaryClientUsesActualPoolForTechnicalHelp) {
    TaffTemporary base,secondary;WriteTaff(base.path/"extra.tarc","valid");auto unique=TaffInput("other");unique[0x3f]='c';unique[0x4f]='d'; //보조에만 있는 이름으로 source4를 구별한다.
    { std::ofstream file(secondary.path/"only.tarc",std::ios::binary);file.write(reinterpret_cast<const char*>(unique.data()),static_cast<std::streamsize>(unique.size()));if (!file) throw std::runtime_error("Help 보조 TAFF 입력 쓰기 실패"); } //등록 전에 실제 파일을 닫는다.
    netstorm::client::ClientOptions options;options.gameDirectory=base.path;options.noAudio=true;netstorm::client::Client client(std::move(options));
    CHECK(client.Files().ContainsArchiveOnly("d/a") && !client.Files().ContainsArchiveOnly("d/a",true));client.InitializeNativeSecondaryFiles(TaffUtf8(secondary.path));
    std::array<wchar_t,32768> program{};const auto length=GetModuleFileNameW(nullptr,program.data(),static_cast<DWORD>(program.size()));if (!length || length>=program.size()) throw std::runtime_error("Help 검사 실행기 경로 조회 실패"); //필수 실행기 경로는 현재 검사 EXE를 쓰되 실행하지 않는다.
    client.Configuration().Set("CDDir",TaffUtf8(secondary.path));client.Configuration().Set("techHelpFile","d/c");client.Configuration().Set("techHelpExe",TaffUtf8(std::filesystem::path(std::wstring_view(program.data(),length))));
    CHECK(!client.Files().ContainsArchiveOnly("d/c") && client.Files().ContainsArchiveOnly("d/c",true) && netstorm::client::ClientTechnicalHelpAvailable(client));
    client.Configuration().Set("techHelpFile","d/d");CHECK(netstorm::client::ClientTechnicalHelpAvailable(client)); //가용성 조회만 하며 파일 실행/추출은 하지 않는다.
}

// 실제 한글 설치 경로의 직접 파일 부재 뒤 기본 TAFF/보조 디스크/보조 TAFF를 차례로 사용한다.
TEST_CASE(BaseFile_AbsoluteMissingRequeriesActualPrimaryAndSecondarySources) {
    TaffTemporary base,secondary;const auto install=base.path/L"게임😀";std::filesystem::create_directory(install);std::filesystem::create_directory(install/L"d");std::filesystem::create_directory(secondary.path/L"d");
    WriteTaff(install/L"one.tarc","valid");WriteTaff(secondary.path/L"one.tarc","other");BaseFileSystem primary(install,secondary.path),fallback(install,secondary.path);
    CHECK(primary.RegisterArchive(install/L"one.tarc"));auto requested=std::filesystem::absolute(install/L"d/a");requested.make_preferred();const auto name=TaffUtf8(requested); //원문이 보관한 getcwd와 같은 native 구분자로 요청한다.
    CHECK(primary.Read(name)==std::vector<std::uint8_t>{'A'});WriteByte(secondary.path/L"d/a",'S');CHECK(fallback.Read(name)==std::vector<std::uint8_t>{'S'});
    std::filesystem::remove(secondary.path/L"d/a");CHECK(fallback.RegisterArchive(secondary.path/L"one.tarc",true));CHECK(fallback.Read(name)==std::vector<std::uint8_t>{'X'});
    WriteByte(requested,'P');CHECK(primary.Read(name)==std::vector<std::uint8_t>{'P'});
    const auto driveRelative=TaffUtf8(requested.root_name())+"d\\a";CHECK(primary.Read(driveRelative)==std::vector<std::uint8_t>{'P'}); //프로세스CWD가검사실행폴더여도원본게임CWD의같은드라이브파일이TAFF보다먼저읽힌다.
    { std::ofstream empty(requested,std::ios::binary|std::ios::trunc);if (!empty) throw std::runtime_error("빈 파일 검사 입력 쓰기 실패"); } //빈 직접 파일도 아카이브보다 먼저 성공해야 한다.
    CHECK(primary.Read(name).empty());CHECK(!fallback.TryRead(TaffUtf8(install/L"d/missing")));
    auto outside=std::filesystem::absolute(base.path/L"겨울/d/a");outside.make_preferred();CHECK(!primary.TryRead(TaffUtf8(outside))); //게임/겨울의공통UTF-8앞2BYTE를절삭해도없는파일은W변환예외대신미발견이어야한다.
}
