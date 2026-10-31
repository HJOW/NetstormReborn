// 원본 ANSI 파일/프로세스 대신 Win10/11 UTF-8 경로를 W API/동적 문자열로 보존한다.
#define NOMINMAX
#include "client/TechnicalHelp.h"
#include "client/ExternalHelpClient.h"
#include "client/ClientMain.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
namespace netstorm::client { //가용성 조회는 실행/브라우저/창을 만들지 않는다.
namespace { //현재 설치 경로와 원문 Filespec 장치 판정을 함께 연결한다.
std::filesystem::path Wide(std::string_view value) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(value.data()),value.size())); } //한글 경로를 ANSI 코드 페이지로 손실시키지 않는다.
std::string Utf8(const std::filesystem::path& value) { const auto text=value.u8string();return {text.begin(),text.end()}; } //현재 Config의 내부 UTF-8과 같은 인코딩이다.
struct FileSpec { //원문 FileSpec의 Find 핸들은 경로 선택이 끝날 때까지 유지한다.
    HANDLE handle{}; //패턴이 없는 일반 입력은 원래NULL이다.
    std::string value; //장치 확인에만 쓰는 원래 FileSpec 내용이다.
    explicit FileSpec(std::string_view name):value(name) { //원문459610의 기본 .\와 구분자/패턴 분기를 보존한다.
        if (name.find_first_of("\\:")==name.npos) value=".\\"+value;std::replace(value.begin(),value.end(),'/','\\');
        if (name.find_first_of("*?")!=name.npos) { WIN32_FIND_DATAW data{};handle=FindFirstFileW(Wide(name).c_str(),&data);if (handle==INVALID_HANDLE_VALUE) value.clear();else { const auto slash=name.find_last_of('\\');value=slash==name.npos ? std::string{} : std::string(name.substr(0,slash+1));value+=Utf8(data.cFileName); } }
    }
    ~FileSpec() { if (handle) FindClose(handle); } //원문4595f0처럼 INVALID_HANDLE_VALUE도 비NULL이면 닫기를 요청한다.
    bool Device() const { return value.starts_with("\\\\") || value.find(':')!=value.npos; } //459900의 UNC/콜론 접두이며 실제 파일 존재 여부를 대신 묻지 않는다.
};
std::optional<std::string> Resolve(Client& client,std::string_view name) { //원문의 원래 이름과 실제 선택한 디스크 경로를 구별한다.
    FileSpec spec(name);ExternalFileHooks h;h.device=[&spec](auto) { return spec.Device(); };
    h.path=[&client](bool secondary,auto file) { return JoinExternalFileRoot(secondary ? client.Configuration().PathSpec("CDDir") : Utf8(client.GameDirectory()),file); };
    h.readable=[](auto path) { return std::ifstream(Wide(path),std::ios::binary).is_open(); }; //원본 rb 열기/닫기와 같은 존재 판정이며 Win10/11 한글 경로를 지원한다.
    h.archive=[&client](bool secondary,auto file) { return client.Files().ContainsArchiveOnly(file,secondary); }; //원문처럼 기본/보조 등록 목록을 구별하며 파일을 추출하거나 실행하지 않는다.
    return ResolveExternalFilePath(name,nullptr,h);
}
TechnicalHelpHooks Hooks(Client& client) { //현재 설정/파일을 매 호출 다시 읽으며 이전 가용성을 캐시하지 않는다.
    TechnicalHelpHooks h;h.readString=[&client](auto key,auto& value) { return client.Configuration().ReadString(key,value) ? 1U : 0U; };h.resolve=[&client](auto path) { return Resolve(client,path); };
    h.launch=[](auto exe,auto file) { std::string command;if (!exe.empty()) { command=exe;command+=' '; }command+=file;static_cast<void>(StartWindowsHelpCommand(command)); }; //원본은 실행 파일/본문을 추가 인용 없이 공백 하나로 연결하고 OS 실패를 가용성 반환에 반영하지 않는다.
    return h;
}
}
bool ClientTechnicalHelpAvailable(Client& client) { return TechnicalHelpAvailable(client.Edition(),Hooks(client)); } //실제 메뉴의435ef0 경계다.
bool LaunchClientTechnicalHelp(Client& client) { return LaunchTechnicalHelp(client.Edition(),Hooks(client)); } //실제 명령의435f10 경계다.
}
