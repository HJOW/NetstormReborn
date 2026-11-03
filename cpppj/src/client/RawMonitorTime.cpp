// 원문 guard/현재시각·임시서식의 부분효과와 정상 복귀 시점만 보존한다.
#include "client/RawMonitorTime.h"
#include "o/RawChatManagerPrefix.h"
#include <algorithm>
#include <array>
#include <ctime>
#include <stdexcept>
namespace netstorm::client { // Client/독립 CPU 대조가 같은 부모를 사용한다.
namespace { // 선택된 실제 경계와 고정 물리의 내부 장치다.
constexpr double kMonitorInterval=180.0; // 양판본 PE005019e0/005074e0의 double180이다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("시간 알림의 실제 장치 미연결");return function; } // 미도달 장치는 요구하지 않는다.
// 원문 NUL 문자열을 실제 manager 범위에서 읽고 지역80BYTE 초과는 명시 거부한다.
std::string Front(std::span<std::uint8_t> bytes,std::size_t offset) {
    if (offset>=bytes.size()) throw std::out_of_range("임시 Chat front 물리 부족");const auto view=bytes.subspan(offset);const auto end=std::find(view.begin(),view.end(),0);if (end==view.end() || end-view.begin()>=80) throw std::out_of_range("임시 Chat front 지역80BYTE 범위");return {view.begin(),end};
}
// 실제 strncpy60/16의 padding과 마지막BYTE0을 같은 물리에만 적용한다.
void SetFront(std::span<std::uint8_t> bytes,std::size_t offset,std::size_t count,std::string_view text) {
    if (offset>bytes.size() || count>bytes.size()-offset) throw std::out_of_range("임시 Chat front setter 범위");o::CopyChatManagerFormat(bytes.subspan(offset),text,static_cast<std::uint32_t>(count));bytes[offset+count-1]=0;
}
}
// 달력 문자열의 기존 개행과 추가 개행을 모두 보존하며 NUL 이후는 CRT처럼 읽지 않는다.
std::string FormatMonitorTimeLine(std::string_view handle,std::string_view calendar) {
    std::string line(handle.substr(0,handle.find('\0')));line+=": !";line+=calendar.substr(0,calendar.find('\0'));line+='\n';if (line.size()>=256) throw std::out_of_range("시간 알림 원문 지역256BYTE 범위");return line;
}
// 시계/달력/Config/submit callback 뒤의 현재 필드를 원문 위치에서 다시 읽는다.
void StepMonitorTime(MonitorTimeState& state,const MonitorTimeHooks& hooks) {
    if (!(state.initialized&1U)) { state.initialized|=1U;state.deadline=0.0; }
    if (!state.monitor && !state.alert) { if (state.deadline==0.0) state.deadline=0.0;return; }
    if (state.deadline==0.0) state.deadline=Need(hooks.wallSeconds)()+kMonitorInterval;
    const auto now=Need(hooks.wallSeconds)();if (!(state.deadline<=now)) return;
    const auto calendar=Need(hooks.calendar)();const auto handle=Need(hooks.handle)();const auto line=Need(hooks.line)(handle,calendar);const auto format=Need(hooks.format)();Need(hooks.submit)(format,line);
    state.deadline=Need(hooks.wallSeconds)()+kMonitorInterval;
}
// 원문과 같은32비트 시간 범위/영문 요일·월·끝개행을 사용하고 CRT 실패를 빈 성공으로 바꾸지 않는다.
std::string MonitorCalendarText() {
    __time32_t now{};if (_time32(&now)==-1) throw std::runtime_error("시간 알림 time32 실패");std::tm local{};if (_localtime32_s(&local,&now)!=0) throw std::runtime_error("시간 알림 localtime32 실패");std::array<char,26> text{};if (asctime_s(text.data(),text.size(),&local)!=0) throw std::runtime_error("시간 알림 asctime 실패");return text.data();
}
// submit 예외에서는 원문처럼 앞선 서식 변경을 남긴다. 정상 복귀에서만 현재 manager를 다시 읽어 복원한다.
void SubmitTemporaryChatFormat(std::uint32_t& current,std::string_view format,std::string_view line,o::OriginalEdition edition,const TemporaryChatFormatHooks& hooks) {
    const bool patch=edition==o::OriginalEdition::Patch1078;const std::size_t offset=patch ? 0x14U : 0x10U,count=patch ? 60U : 16U;const auto initial=current;const auto saved=Front(Need(hooks.memory)(initial),offset);
    SetFront(Need(hooks.memory)(initial),offset,count,format);Need(hooks.submit)(current,line);SetFront(Need(hooks.memory)(current),offset,count,saved);
}
}
