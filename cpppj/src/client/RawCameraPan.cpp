// 프레임 delta나 임시 속도 대신 원문 signed 좌표/FTOL/8과 Shift 축 선택을 사용한다.
#include "client/RawCameraPan.h"
#include "client/RawCameraView.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 입력/GUI 경계와 독립인 원문 pan 본체다.
namespace { // signed overflow 없이 x86의 하위32비트 산술을 수행한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("카메라 pan 현재 장치 미연결");return callback; } //도달한 장치만 요구한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b)); } //ADD의 감긴 결과다.
std::int32_t Sub(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b)); } //SUB의 감긴 결과다.
std::int32_t Abs(std::int32_t value) { return value<0 ? Sub(0,value) : value; } //INT_MIN의 원문 ABS는 여전히 signed 음수다.
std::int32_t Axis(std::int32_t pointer,std::int32_t origin,std::int32_t extent,std::int32_t client) {
    const auto center=Add(origin,extent/2);const auto bounded=std::min(std::max(origin,pointer),Add(origin,extent)); //뒤집힌 폭도 원문 CMP 순서의 상한 선택을 유지한다.
    const double ratio=static_cast<double>(client)/static_cast<double>(extent); //FILD/FDIV를 먼저53bit로 저장하며 뒤 곱과 합치지 않는다.
    return CameraFtolLow(ratio*static_cast<double>(Sub(bounded,center))); //FIMUL 뒤 실제 FTOL 하위 DWORD다.
} //축의 원문 signed 크기/비정상 FTOL 결과를 보존한다.
}
bool ProcessCameraPan(InputEvent pointer,const CameraPanHooks& hooks) {
    const auto middle=Need(hooks.poll)(InputCode::kMiddleButton);const auto gate=Need(hooks.globals)();
    if ((!gate.scrollWhileMouseOut && (pointer.code&InputCode::kOutside)) ||
        ((pointer.code&InputCode::kRelease) && (middle.code&InputCode::kRelease)) || gate.dialog) return false;
    Need(hooks.closePopups)();const auto current=hooks.globals();
    if (pointer.x==-1 || pointer.y==-1 || !current.active) return true;
    auto x=Axis(pointer.x,current.viewX,current.viewWidth,current.clientWidth);auto y=Axis(pointer.y,current.viewY,current.viewHeight,current.clientHeight);
    if (!(hooks.poll(0x00100000U).code&InputCode::kRelease)) { if (Abs(x)>Abs(y)) y=0;else x=0; } //같은 크기면 X를 지우며 각각 /8 이전 값을 비교한다.
    Need(hooks.request)(x/8,y/8);return true; //signed 나눗셈은0 방향 절삭이며 화면 갱신 말미가 현재 view를 적용한다.
}
}
