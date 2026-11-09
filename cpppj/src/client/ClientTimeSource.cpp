// Client 시각 공급원(기본 OS 시계, 검사 전용 고정 시각 모드). 2026-11-09 webpj/dotnetpj L2·L3 결정론 대조용으로 추가했다.
#include "client/ClientTimeSource.h"
#include <ctime>
#include <limits>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

namespace netstorm::client {
namespace {
// 프로세스 하나의 고정 시각 상태다. 실행 파일은 Client를 하나만 만들므로 전역으로 둔다. (2026-11-09 추가)
struct FixedClientTimeState {
    FixedClientTime setting; // 켠 설정이다. frameMilliseconds 0이면 꺼짐이다.
    std::uint64_t elapsed{}; // 원점 뒤 가상 경과 밀리초다.
};
// 고정 시각 상태의 단일 저장소를 돌려준다. (2026-11-09 추가)
FixedClientTimeState& State() { static FixedClientTimeState state;return state; }
}

void EnableFixedClientTime(FixedClientTime setting) {
    if (setting.frameMilliseconds==0 || setting.frameMilliseconds>60000) throw std::invalid_argument("고정 시각 프레임 밀리초는 1~60000");
    if (setting.unixSeconds<0) throw std::invalid_argument("고정 시각 유닉스 초는 0 이상");
    if (FixedClientTimeEnabled()) throw std::logic_error("고정 시각 모드는 한 번만 켤 수 있음");
    State()=FixedClientTimeState{setting,0};
}

void DisableFixedClientTimeForTests() { State()=FixedClientTimeState{}; }

bool FixedClientTimeEnabled() { return State().setting.frameMilliseconds!=0; }

std::uint32_t ClientMilliseconds() {
    const auto& state=State();
    if (state.setting.frameMilliseconds) return static_cast<std::uint32_t>(state.setting.originMilliseconds+state.elapsed); // 원본 timeGetTime과 같은 unsigned 32비트 감김이다.
#ifdef _WIN32
    return timeGetTime();
#else
    throw std::logic_error("Client OS 밀리초 시계 미연결");
#endif
}

std::int32_t ClientUnixSeconds() {
    const auto& state=State();
    const auto now=state.setting.frameMilliseconds ? static_cast<long long>(state.setting.unixSeconds)+static_cast<long long>(state.elapsed/1000U) : static_cast<long long>(std::time(nullptr));
    if (now<0 || now>(std::numeric_limits<std::int32_t>::max)()) throw std::out_of_range("실제 time32 지원 날짜 범위 초과");
    return static_cast<std::int32_t>(now);
}

void AdvanceFixedClientTime() { auto& state=State();if (state.setting.frameMilliseconds) state.elapsed+=state.setting.frameMilliseconds; }
}
