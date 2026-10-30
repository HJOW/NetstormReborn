// 실제 로컬 GUI 도우미로 UTF-8/긴 명령과 현재 Client 프로토콜 연결을 확인한다. 게임/브라우저는 열지 않는다.
#include "TestSupport.h"
#include "client/ExternalHelpClient.h"
#include "client/ClientMain.h"
#define NOMINMAX
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
namespace { // 각 검사는 고유 작업 폴더와 현재 프로세스 환경만 사용한다.
using namespace netstorm::client; // 같은 제품의 실제 OS/Client 경계다.
constexpr const wchar_t* kProof=L"NETSTORM_HELP_PROOF_PATH"; // 자식 전용 기록 경로이며 시스템 환경에는 저장하지 않는다.
std::string Utf8(const std::filesystem::path& path) { const auto value=path.u8string();return {value.begin(),value.end()}; } // 원래 실제 경로를 UTF-8 명령에 사용한다.
std::string ConfigLiteral(std::string_view value) { // Config의 원래 인용 제거/백틱 치환을 거친 뒤에도 실제 명령의 인용을 유지한다.
    std::string result;for (const char byte:value) { if (byte=='"' || byte=='`') result+='`';result+=byte; }return result; //설정 문법 문자만 원래 이스케이프로 기록한다.
}
std::filesystem::path Work() { // 기존 파일을 덮지 않는 매 실행 고유 검사 경로다.
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();const auto path=root/"extracted/cpp-help-links-601-20261030/os-tests"/std::to_string(stamp)/std::filesystem::path(u8"한글 프로토콜 도우미");std::filesystem::create_directories(path);return path;
}
struct Environment { // 현재 검사 프로세스의 이전값만 복구하며 다른 앱/시스템 설정은 바꾸지 않는다.
    std::optional<std::wstring> previous; // 이전 미정의/정의 상태를 구별한다.
    explicit Environment(const std::filesystem::path& path) { // 이 프로세스의 다음 CreateProcess 자식에만 전달한다.
        SetLastError(ERROR_SUCCESS);const auto count=GetEnvironmentVariableW(kProof,nullptr,0);if (count) { std::wstring value(count,L'\0');GetEnvironmentVariableW(kProof,value.data(),count);value.pop_back();previous=std::move(value); }else if (GetLastError()==ERROR_SUCCESS) previous=std::wstring{};
        if (!SetEnvironmentVariableW(kProof,path.c_str())) throw std::runtime_error("Help 도우미의 검사 환경 설정 실패");
    }
    ~Environment() { SetEnvironmentVariableW(kProof,previous ? previous->c_str() : nullptr); } // 자식 환경 복사 뒤 현재값을 원래대로 되돌린다.
};
std::string WaitProof(const std::filesystem::path& path,std::string_view expected) { // 파일 존재만으로 쓰기 완료를 가정하지 않는다.
    for (int attempt=0;attempt<200;++attempt) { // 최대5초 안에서 완성된 명령을 기다리며 제품 실행은 비대기 그대로다.
        std::ifstream input(path,std::ios::binary);const std::string text((std::istreambuf_iterator<char>(input)),{});const auto newline=text.find('\n');if (newline!=text.npos && text.substr(newline+1)==expected) return text;std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }return {};
}
}
// 실제 W CreateProcess가 한글 실행 파일 경로와512BYTE 초과 명령을 손실 없이 전달한다.
TEST_CASE(ExternalHelpClient_WindowsUnicodeLongCommandAndCurrentConfig) {
    const auto work=Work();const auto helper=work/"proof.exe";std::filesystem::copy_file(std::filesystem::path(NETSTORM_HELP_PROBE),helper);const auto proof=work/"received.txt";Environment environment(proof);
    const std::string prefix='"'+Utf8(helper)+"\" \""+std::string(600,'x')+"\" ";const auto expected=prefix+"  payload"; // 원문 첫%1 대체의 공백 둘과 기존 구분 공백 하나를 그대로 기대한다.
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";Client client(std::move(options)); // 창/Run 없이 현재 설정만 만든다.
    client.Configuration().Set("autoOpen","");client.Configuration().Set("probe:",ConfigLiteral(prefix+"%1"));std::string configured;CHECK(client.Configuration().ReadString("probe:",configured) && configured==prefix+"%1"); //원래 Config 문법에서 인용을 보존한 명시 입력이다.
    OpenClientExternalHelp(client,"probe:payload");const auto observed=WaitProof(proof,expected);CHECK(!observed.empty());CHECK(expected.size()>512); // 현재 Config/정책과 실제 자식의 결과다.
    const auto missing=work/"does-not-exist.exe";CHECK(!StartWindowsHelpCommand('"'+Utf8(missing)+"\" payload")); // 원래 OS 실패 반환도 다른 프로그램의 실행으로 바꾸지 않는다.
}
