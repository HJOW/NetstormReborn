// 원본 Userinput.cpp의 강조 모드004d5bb0와 마우스 타입 초기화004d5480를 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>

namespace netstorm::client {
// 실제 선택과 강조는 다른 전역이다. 모든 참조 대상은 이 처리기보다 오래 살아야 한다.
struct UserHighlightState {
    std::uint32_t& mode; // 005cae90의 현재 강조 모드이며 선택 SID와 합치지 않는다.
    const std::uint32_t& selectedSid; // 005caea0의 현재 선택 SID다. Highlight는 이 값을 변경하지 않는다.
    std::uint32_t& mouseType; // 005caea4의 현재 마우스 대상 TYPE이며 ResetMouse가 이 DWORD만0으로 쓴다.
    const std::int32_t& mouseX; // 005cae94의 현재 정수 화면 X이며 모드5에서만 읽는다.
    const std::int32_t& mouseY; // 005cae98의 현재 정수 화면 Y이며 모드5에서만 읽는다.
};
// 모드5의 좌표 반환은 두 float의 원본 비트이며 임의 반올림이나 위치 계산을 추가하지 않는다.
struct UserHighlightHooks {
    std::function<void(std::uint32_t)> selectCursor; // 004a4410의 실제 하드웨어/소프트웨어 커서 선택이다.
    std::function<std::array<std::uint32_t,2>(std::int32_t,std::int32_t)> project; // 004971e0의 현재 카메라 역투영 결과 두 DWORD다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> move; // 004284a0(selectedSid,xBits,yBits,0)의 실제 이동 강조 경계다.
};
// 강조 모드 변경만 처리한다. 선택 해제004d5c90/004d5490와 별개이며 selectedSid를 지우지 않는다.
class RawUserHighlight {
public:
    // 같은 장기 선택/마우스 전역을 빌린다. 도달하지 않은 외부 효과는 생성 시 요구하지 않는다.
    RawUserHighlight(UserHighlightState state,UserHighlightHooks hooks);
    // 004d5480: 마우스 TYPE만0으로 쓰며 강조 모드/선택/화면 좌표는 보존한다.
    void ResetMouse() const;
    // 004d5bb0: 같은 모드는 효과 없이 반환하며 변경 때만 초기화와 현재 모드의 효과를 실행한다.
    void Highlight(std::uint32_t mode) const;
private:
    UserHighlightState state_; // 원본 다섯 전역의 동일 저장소 참조다. 별도 선택 사본을 소유하지 않는다.
    UserHighlightHooks hooks_; // 실제 커서/역투영/이동 강조의 선택된 외부 소비자다.
};
}
