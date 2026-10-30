// 원본 ANSI 실행 대신 Windows10/11의 UTF-8 경로와 W API를 사용한다.
#define NOMINMAX
#include "client/ExternalHelpClient.h"
#include "client/HelpLinkPolicy.h"
#include "client/ClientMain.h"
#include "o/OriginalText.h"
#include <windows.h>
#include <shellapi.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
namespace netstorm::client { // 원문 실행 정책은 유지하고 OS 문자/핸들 경계만 현대화한다.
namespace { // UTF-8 현재 설정과 원본 CP1252 링크를 별개로 변환한다.
std::wstring Wide(std::string_view value) { //원본 A API와 달리 한글 설치/설정 경로를 손실 없이 W API에 전달한다.
    value=value.substr(0,value.find('\0'));if (value.empty()) return {};if (value.size()>static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("Help 실행 UTF-8 길이 범위");
    const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);if (!count) throw std::invalid_argument("Help 실행 경로의 UTF-8 형식");
    std::wstring result(static_cast<std::size_t>(count),L'\0');if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),result.data(),count)!=count) throw std::runtime_error("Help 실행 경로 W 변환 실패");return result;
}
std::string Decode(std::string_view value) { return o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // 자산 링크만 원본 CP1252에서 변환하며 설정 명령은 이미 UTF-8이다.
}
// desktop 부모와 open/SHOW5 인수는 원문과 같고 URL의 성공 판정도32초과다.
bool OpenWindowsHelpUrl(std::string_view utf8) { const auto value=Wide(utf8);return reinterpret_cast<std::uintptr_t>(ShellExecuteW(GetDesktopWindow(),L"open",value.c_str(),nullptr,nullptr,5))>32U; }
// 원본의 비대기 CreateProcess와 같은 현재 작업 폴더/환경을 쓰며 숨김/추가 인용 규칙을 넣지 않는다.
bool StartWindowsHelpCommand(std::string_view utf8) {
    const auto value=Wide(utf8);std::vector<wchar_t> line(value.begin(),value.end());line.push_back(L'\0');STARTUPINFOW startup{};startup.cb=static_cast<DWORD>(sizeof(startup));PROCESS_INFORMATION process{};
    const bool created=CreateProcessW(nullptr,line.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process)!=FALSE;
    if (created) { CloseHandle(process.hThread);CloseHandle(process.hProcess); } //원본이 남기던 핸들은 Win10/11 반복 Help 실행의 자원 누적을 막기 위해 즉시 반납한다. 자식은 종료시키지 않는다.
    return created;
}
// RawMenuControls의 동일 순수 파싱을 재사용하며 외부 정책 경계에서만 문자열을 UTF-8로 바꾼다.
void OpenClientExternalHelp(Client& client,std::string_view originalBytes) {
    originalBytes=originalBytes.substr(0,originalBytes.find('\0'));const auto address=ParseHelpLinkAddress(originalBytes);ExternalHelpHooks hooks;
    hooks.pathSpec=[&client](auto key) { return client.Configuration().PathSpec(key); };hooks.readString=[&client](auto key,auto& value) { return client.Configuration().ReadString(key,value); };
    hooks.shellOpen=OpenWindowsHelpUrl;hooks.launch=[](auto command) { static_cast<void>(StartWindowsHelpCommand(command)); }; //원본 void 실행 부모처럼 OS 시작 실패도 다른 성공 경로로 바꾸지 않는다.
    OpenExternalHelpLink(client.Edition(),Decode(originalBytes),Decode(address.protocol),Decode(address.body),hooks); //이 정책은 자산이 필요 없으므로 창/자산 초기화 전에도 같은 실행 판본을 읽는다.
}
}
