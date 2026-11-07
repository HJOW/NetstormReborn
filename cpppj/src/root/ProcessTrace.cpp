// 원문 로그·ZAssertion의 현재 옵션/실행 순서와 현대 Windows의 실제 진단 장치를 연결한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Windows 구 선언 충돌을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 함수와 Windows 매크로 충돌을 피한다.
#endif
#include <windows.h>
#include "root/ProcessTrace.h"
#include "client/ProductVersion.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <bit>
#include <cerrno>
#include <ctime>
#include <limits>
#include <memory>
#include <stdexcept>
#include <system_error>

namespace netstorm::root { // 실제 파일/대화상자/시각은 해당 로그·진단 호출이 선택됐을 때만 실행한다.
namespace { // 원문 첫NUL 계약과 Windows10/11 UTF-8 진단 문자열 경계다.
std::string_view CString(std::string_view text) { return text.substr(0,text.find('\0')); } // caller의 완성 메시지/표현식도 실제 C문자열처럼 첫NUL까지만 사용한다.
std::wstring Wide(std::string_view text) { // 파일 경로가 아닌 실제 대화상자 문자열을 UTF-16으로 변환한다.
    if (text.empty()) return {};if (text.size()>static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("R 진단 UTF-8 문자열 범위");const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0);if (!count) throw std::system_error(static_cast<int>(GetLastError()),std::system_category(),"R 진단 UTF-8 변환 실패");std::wstring result(static_cast<std::size_t>(count),L'\0');if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),count)!=count) throw std::runtime_error("R 진단 UTF-8 변환 길이 불일치");return result;
}
}
ProcessTrace::ProcessTrace(ConnectionState& connections,Options& options,platform::UserDataRoot user):connections_(connections),options_(options),user_(std::move(user)) {}
std::string ProcessTrace::Timestamp() {
    const auto now=::_time32(nullptr);std::tm local{};std::array<char,26> text{};if (::_localtime32_s(&local,&now) || ::asctime_s(text.data(),text.size(),&local)) throw std::runtime_error("R 로그의 실제 OS 현지시각 변환 실패");return {text.data(),19};
}
void ProcessTrace::Log(std::int32_t level,std::string_view message) {
    if (level<0 || level>options_.traceLevel) return;message=CString(message);std::size_t codes{};
    while (codes<message.size() && message[codes]=='~') { // 첫 연속 ~두BYTE 코드는 들여쓰기 앞에 그대로 보관한다.
        if (message.size()-codes<2) throw std::out_of_range("R 로그의 불완전한 선두 색상 코드");codes+=2;
    }
    const auto spaces=indentation_>0 ? static_cast<std::size_t>(indentation_):0U;if (message.size()>=buffer_.size() || spaces>=buffer_.size()-message.size()) throw std::length_error("R 로그의 실제1024BYTE 물리 범위");
    std::copy_n(message.data(),codes,buffer_.data());std::fill_n(buffer_.data()+codes,spaces,' ');std::copy(message.begin()+static_cast<std::ptrdiff_t>(codes),message.end(),buffer_.data()+codes+spaces);auto length=message.size()+spaces;buffer_[length]=0;bool newline=false;
    while (length && (buffer_[length-1]=='\r' || buffer_[length-1]=='\n')) { buffer_[--length]=0;newline=true; } // 모든 끝CR/LF를 제거한 뒤 원문의LF 한 개만 복구한다.
    if (newline) { buffer_[length++]='\n';buffer_[length]=0; }if (!output_) output_=stdout;
    std::fprintf(output_,"%s %s %s ",(connections_.modeFlags&0x11U) ? "diag":"",(connections_.modeFlags&kRootMode) ? "root":"",(connections_.modeFlags&kChallengeMode) ? "chal":"");const auto time=Timestamp();std::fprintf(output_,"%.19s: ",time.c_str());
    if (output_ && options_.traceToStdErr) std::fprintf(stderr,"%s",buffer_.data());std::fprintf(output_,"%s",buffer_.data());if (options_.flushAfterTrace) std::fflush(output_); // stderr에는 접두가 아닌 본문만 복제하며 원문 fwrite/flush 실패를 되돌리지 않는다.
}
void ProcessTrace::Flush() { if (output_) std::fflush(output_); }
void ProcessTrace::Report(std::string_view diagnostic) {
    platform::UserDataRoot::EnsureDirectory(user_.Diagnostics());const auto path=user_.Diagnostics()/L"assert.html";auto* raw=::_wfopen(path.c_str(),L"w+b");if (!raw) throw std::system_error(errno,std::generic_category(),"R 실제 진단 HTML 파일 열기 실패");const std::unique_ptr<std::FILE,decltype(&std::fclose)> file(raw,&std::fclose);std::setvbuf(file.get(),nullptr,_IONBF,0);
    std::fprintf(file.get(),"<html>\n<title>Netstorm Crash</title>\n<body>\n<h2>Crash Results</h2>\n");const std::string text(diagnostic);std::fprintf(file.get(),"%s",text.c_str());std::fprintf(file.get(),"</body></html>\n"); // 원문은 HTML 이스케이프/새스택 본문을 추가하지 않으며 실제 fclose는 이 함수 정상/예외 복귀에서 수행한다.
}
std::string ProcessTrace::Stack() {
    std::array<void*,20> frames{};const auto count=CaptureStackBackTrace(0,static_cast<DWORD>(frames.size()),frames.data(),nullptr);std::string result;
    for (std::uint32_t index=0;index<count;++index) { // 실제 캡처된 반환주소만 포인터 전체 폭/대문자16진수로 쓰고 네 개마다 원문CRLF를 넣는다.
        std::array<char,80> row{};std::snprintf(row.data(),row.size(),"%u.%0*llX ",static_cast<unsigned>(index),static_cast<int>(sizeof(std::uintptr_t)*2),static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(frames[index])));result+=row.data();if ((index&3U)==3U) result+="\r\n";
    }
    return result;
}
void ProcessTrace::Assertion(std::string_view expression,std::string_view file,std::uint32_t line) {
    if (asserting_) { extra_.reset();return; }asserting_=1;const auto condition=std::string(CString(expression)),source=std::string(CString(file));const auto number=std::to_string(std::bit_cast<std::int32_t>(line));const auto diagnostic="ZAssertion failed ("+condition+") at "+source+" line "+number+'\n';std::fprintf(stderr,"%s",diagnostic.c_str());Report(diagnostic);Flush();const auto stack=Stack();
    const auto message="An assertion has failed:\n'"+condition+"' in file "+source+", line "+number+" version "+std::string(client::kProductVersion)+".\nExtra info: "+(extra_ ? std::string(CString(*extra_)):std::string{})+"\nStack:\r\n"+stack+"\n진단 보고서는 사용자 Netstorm/Diagnostics/assert.html에 저장됩니다.";const auto wide=Wide(message);MessageBoxW(nullptr,wide.c_str(),L"Assertion failed.",MB_OK|MB_ICONWARNING);asserting_=0;extra_.reset(); // 원문처럼 정상 대화상자 복귀 뒤에만 guard/추가정보를 반납한다. throwOnAssert의 별도 예외를 합성하지 않는다.
}
ProcessLogHook ProcessTrace::LogDevice() { return [this](auto level,auto message) { Log(level,message); }; } // 같은 현재 옵션/버퍼를 빌리는 실제 로그 함수다.
ProcessAssertionHook ProcessTrace::AssertionDevice() { return [this](auto expression,auto file,auto line) { Assertion(expression,file,line); }; } // 같은 진단 재진입 상태/사용자 경로를 빌린다.
}
