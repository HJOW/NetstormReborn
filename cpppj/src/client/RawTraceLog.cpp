// 원문 공유 버퍼를 콜백마다 다시 읽으며 출력 전후의 현재 전역을 복원하지 않는다.
#include "client/RawTraceLog.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string>

namespace netstorm::client { // Native 숫자/문자열 의미와 호스트 장치의 수명을 구별한다.
namespace { // 선택된 출력 장치와 현재 C문자열만 확인한다.
// 실제 도달한 장치 부재는 앞선 버퍼/출력 효과를 유지한 채 거부한다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("실제 trace 로그 장치 미연결");return value; }
// 같은 물리 버퍼의 첫NUL까지 빌리며 이전 출력 문자열의 사본을 재사용하지 않는다.
std::string_view Current(const TraceLogState& state) {
    const auto end=std::find(state.buffer.begin(),state.buffer.end(),'\0');
    if (end==state.buffer.end()) throw std::logic_error("trace 로그 공유 버퍼 NUL 없음");return {state.buffer.data(),static_cast<std::size_t>(end-state.buffer.begin())};
}
}
// 원문 입력이 이미 변환된 뒤의 같은1024BYTE 버퍼/상태 효과를 적용한다.
void WriteRawTraceLog(TraceLogState& state,std::int32_t level,std::string_view formatted,std::size_t prefixBytes,const TraceLogHooks& hooks) {
    if (level<0 || level>state.level) return;
    formatted=formatted.substr(0,formatted.find('\0'));
    if (prefixBytes>formatted.size() || (prefixBytes&1U)) throw std::invalid_argument("trace 로그 format 접두 범위");
    for (std::size_t i=0;i<prefixBytes;i+=2) if (formatted[i]!='~') throw std::invalid_argument("trace 로그 format 접두 쌍"); // 원문format의선행색상쌍만복사한다.
    const auto spaces=std::bit_cast<std::int32_t>(state.indent)>0 ? state.indent : 0U; // DWORD의음수값은들여쓰기를생략한다.
    if (spaces>=state.buffer.size() || formatted.size()>=state.buffer.size()-spaces) throw std::length_error("trace 로그 원본1024BYTE 범위");
    std::string line(formatted.substr(0,prefixBytes));line.append(spaces,' ');line.append(formatted.substr(prefixBytes)); // 입력이공유버퍼를가리켜도출력쓰기전내용을보존한다.
    std::copy(line.begin(),line.end(),state.buffer.begin());state.buffer[line.size()]=0;
    if (!state.stream) { if (!hooks.standardOut) throw std::logic_error("trace 로그 실제 stdout 미연결");state.stream=hooks.standardOut; }
    auto end=line.size();bool stripped=false; // 원문은실제formatter출력뒤모든끝CR/LF를NUL로쓴다.
    while (end && (state.buffer[end-1]=='\r' || state.buffer[end-1]=='\n')) { state.buffer[--end]=0;stripped=true; } // 끝개행전부를지운다.
    if (stripped) { state.buffer[end]='\n';state.buffer[end+1]=0; }
    Need(hooks.console)(Current(state));
    if (state.stream && state.mirror) Need(hooks.write)(hooks.standardError,Current(state));
    Need(hooks.write)(state.stream,Current(state));
    if (state.flush) Need(hooks.flush)(state.stream);
}
// 같은현재FILE이0이면장치없이복귀한다. 호출후상태변경을취소하지않는다.
void FlushRawTraceLog(TraceLogState& state,const TraceLogHooks& hooks) { if (state.stream) Need(hooks.flush)(state.stream); }
}
