// 현재 Client의 Technical Help 가용성/설정 재조회와 실제 로컬 W API 실행을 확인한다.
#define NOMINMAX
#include "TestSupport.h"
#include "client/TechnicalHelp.h"
#include "client/ClientMain.h"
#include <windows.h>
#include <chrono>
#include <fstream>
#include <optional>
#include <thread>
using namespace netstorm::client; //같은 제품 Client와 기술 도움말 장치를 검사에서 직접 사용한다.
namespace { //검사는 새 작업 경로와 현재 프로세스 환경만 사용한다.
constexpr const wchar_t* kProof=L"NETSTORM_HELP_PROOF_PATH"; //GUI 도우미에만 전달하는 실제 기록 경로다.
std::string Utf8(const std::filesystem::path& path) { const auto value=path.u8string();return {value.begin(),value.end()}; } //실제 경로의 UTF-8 바이트다.
std::string Literal(std::string_view value) { std::string result;for (const char byte:value) { if (byte=='"' || byte=='`') result+='`';result+=byte; }return result; } //Config의 인용 제거/백틱 치환 뒤 원래 명령 인용을 유지한다.
struct Environment { //다른 프로세스/시스템 설정은 바꾸지 않고 원래 상태를 복구한다.
    std::optional<std::wstring> previous; //미정의와 정의된 빈 값을 구별한다.
    explicit Environment(const std::filesystem::path& path) { //다음 로컬 자식의 환경 복사에만 사용한다.
        SetLastError(ERROR_SUCCESS);const auto count=GetEnvironmentVariableW(kProof,nullptr,0);if (count) { std::wstring value(count,L'\0');GetEnvironmentVariableW(kProof,value.data(),count);value.pop_back();previous=std::move(value); }else if (GetLastError()==ERROR_SUCCESS) previous=std::wstring{};CHECK(SetEnvironmentVariableW(kProof,path.c_str())!=FALSE);
    }
    ~Environment() { SetEnvironmentVariableW(kProof,previous ? previous->c_str() : nullptr); } //현재 검사 프로세스의 이전값만 복구한다.
};
}
// 가용성 조회는 실행하지 않고 선택할 때 현재 파일 설정을 다시 읽는다.
TEST_CASE(TechnicalHelpClient_CurrentPathsMetadataAndUnicodeExecution) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto work=root/"extracted/cpp-technical-help-602-20261030/os-tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/std::filesystem::path(u8"한글 기술 도움말");std::filesystem::create_directories(work);
    const auto helper=work/"proof.exe",proof=work/"received.txt",first=work/"first.hlp",second=work/"second.hlp";std::filesystem::copy_file(std::filesystem::path(NETSTORM_TECHNICAL_PROBE),helper);std::ofstream(first)<<"first";std::ofstream(second)<<"second";Environment environment(proof);
    ClientOptions options;options.gameDirectory=root/"originals";Client client(std::move(options));client.Configuration().Set("CDDir",Utf8(work)); //창/Run 없이 실제 Config/보호 TAFF 메타데이터만 사용한다.
    client.Configuration().Set("techHelpExe",Literal('"'+Utf8(helper)+'"'));client.Configuration().Set("techHelpFile",Literal('"'+Utf8(first)+'"'));CHECK(ClientTechnicalHelpAvailable(client));CHECK(!std::filesystem::exists(proof));
    client.Configuration().Set("techHelpFile",Literal('"'+Utf8(second)+'"'));CHECK(LaunchClientTechnicalHelp(client));const auto expected='"'+Utf8(helper)+"\" \""+Utf8(second)+'"';std::string actual;
    for (int attempt=0;attempt<200;++attempt) { //비대기 제품의 기록 완료만 최대5초 관측한다.
        std::ifstream input(proof,std::ios::binary);actual.assign(std::istreambuf_iterator<char>(input),{});const auto newline=actual.find('\n');if (newline!=actual.npos && actual.substr(newline+1)==expected) break;std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    const auto newline=actual.find('\n');CHECK(newline!=actual.npos && actual.substr(newline+1)==expected);
    client.Configuration().Set("techHelpFile","this-file-is-absent-602.hlp");CHECK(!ClientTechnicalHelpAvailable(client)); //기본/TAFF/CD에도 없으면 메뉴에서 제외한다.
    client.Configuration().Set("techHelpFile",Utf8(work/"absolute-absent.hlp"));CHECK(ClientTechnicalHelpAvailable(client)); //장치 접두가 있으면 원문대로 실제 존재를 확인하지 않으며 이 분기에서는 실행하지 않는다.
    client.Configuration().Set("techHelpFile","first.hlp");CHECK(ClientTechnicalHelpAvailable(client)); //실제 보조 CD 경로의 rb 열기/닫기를 확인한다.
    CHECK(!client.Files().ContainsArchiveOnly("this-file-is-absent-602.hlp")); //존재 메타데이터 조회는 파일 내용을 열지 않는다.
    CHECK(client.Files().ContainsArchiveOnly("\\d\\priest.type")); //보호 TAFF의 실제 이름 메타데이터를 같은 등록 집합에서 찾는다.
}
