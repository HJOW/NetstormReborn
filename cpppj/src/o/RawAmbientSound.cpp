// 두 현재 시계 읽기·기한 저장·signed 강제 분기·같은 난수·음향 뒤 모달 조회 순서를 보존한다.
#include "o/RawAmbientSound.h"
#include <stdexcept>
namespace netstorm::o { // 실제 장치가 없는 독립 원문 대조도 동일 부모를 사용한다.
namespace { // 선택한 경계에서만 미연결을 거부하며 선행 효과는 되돌리지 않는다.
constexpr double kAmbientInterval=120.0,kStateDelay=10.0; // 두 판본의 실제 PE double 상수다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("주변음 현재 장치 미연결");return function; } // 도달하지 않은 장치는 요구하지 않는다.
}
void ResetAmbientSoundDeadline(double& deadline,const std::function<double()>& wallSeconds) { deadline=Need(wallSeconds)()+kStateDelay; } // 게임 시계/이전 프레임 캐시를 쓰지 않는다.
// 미래/비순서 기한이면 첫 시계 조회만 수행한다. 재예약은 두 번째 조회 뒤 난수보다 먼저 저장한다.
void StepAmbientSound(AmbientSoundGlobals globals,GameRandom& random,const AmbientSoundHooks& hooks) {
    const auto now=Need(hooks.wallSeconds)(); // 시계 callback이 바꾼 현재 기한을 그 뒤에 조회한다.
    if (!(globals.deadline<=now)) return;
    globals.deadline=Need(hooks.wallSeconds)()+kAmbientInterval;
    if (globals.forced<=0) globals.kind=static_cast<std::int32_t>(random.Next(3000)/1000);
    if (globals.kind==0) {
        Need(hooks.sound)("thunderQuietDistant.wav",0,0,0,0,0);
        if (!Need(hooks.currentDialog)()) Need(hooks.flash)(0);
    } else if (globals.kind==1 || globals.kind==2) {
        const auto pan=static_cast<std::int32_t>(random.Next(20000))-10000;
        Need(hooks.sound)(globals.kind==1 ? "eagleScream.wav" : "whale.wav",0,0,pan,0,0);
    }
}
}
