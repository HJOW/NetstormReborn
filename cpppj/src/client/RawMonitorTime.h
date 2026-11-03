// Client의180초 시간 알림과 Chat front 서식의 임시 교체를 원문 순서로 복원한다.
#pragma once
#include "o/SidPool.h"
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
namespace netstorm::client { // OS 달력/현재 UI와 분리한 동일 Client 전역/작은 부모다.
struct MonitorTimeState { //00544d68/60/0054db9c/dba0 또는 CD 대응의 장기 현재값이다.
    std::uint32_t initialized{}; // 첫 bit0만 켜며 다른 비트는 보존한다.
    double deadline{}; // 최초 guard에서0을 쓰고 비활성 때 비영 기한을 지우지 않는다.
    std::int32_t monitor{},alert{}; // 두 ReadInt 현재 DWORD이며 비영 관문을 bool로 정규화하지 않는다.
};
std::string FormatMonitorTimeLine(std::string_view handle,std::string_view calendar); // 원문 %s: !%s\n의 지역256BYTE 경계와 두 끝개행을 보존한다.
struct MonitorTimeHooks { // 도달한 장치만 조회한다. 매 clock 호출은 새로운 현재 벽시각이다.
    std::function<double()> wallSeconds; // 초기 예약/비교/전달 뒤 재예약의 서로 다른 호출이다.
    std::function<std::string()> calendar; // 실제 time32→localtime→asctime의 끝개행 포함 문자열이다.
    std::function<std::string_view()> handle; // 달력 변환 뒤 현재 이름을 읽으며 사본 캐시를 만들지 않는다.
    std::function<std::string(std::string_view,std::string_view)> line{FormatMonitorTimeLine}; // 고정 CRT 서식 경계이며 현재 이름/달력만 받는다.
    std::function<std::string()> format; //4410f0("monitorFormat",0,0,0)의 현재 Config 서식이다.
    std::function<void(std::string_view,std::string_view)> submit; //42f430(format,line)의 현재 Chat manager 부모다.
};
void StepMonitorTime(MonitorTimeState& state,const MonitorTimeHooks& hooks); // Wind 뒤 Movie 갱신 잠금 없는 가지에서 실행한다.
std::string MonitorCalendarText(); // Windows 원문32비트 달력의 표준26BYTE 출력을 실제 CRT로 얻는다.
struct TemporaryChatFormatHooks { // 같은 manager의 메모리는 순수 조회이며 submit만 외부 효과다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> memory; // 현재 토큰에 대응하는 실제 물리를 읽고 쓴다.
    std::function<void(std::uint32_t,std::string_view)> submit; //42da00의 실제 일반 제출 경계다. MP 검사는 별도다.
};
void SubmitTemporaryChatFormat(std::uint32_t& current,std::string_view format,std::string_view line,o::OriginalEdition edition,const TemporaryChatFormatHooks& hooks); // 최초 front를 저장하고 정상 submit 복귀 뒤 현재 manager에만 복원한다.
}
