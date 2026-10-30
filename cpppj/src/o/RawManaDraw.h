// 마나103의 조건부 그림자/현재 프레임 출력을 같은 Native 화면·픽셀 선택에서 사용한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o { // 원본473710의 raw 필드와 출력 순서를 복원한다.
struct ManaAdditionalShape { // shape는 현재 TYPE+dc의 호스트 SHP 식별자이며 실제 주소로 실행하지 않는다.
    std::uint32_t shape{},frame{}; // 현재 SHP 토큰과 각 출력 직전에 읽은+24 프레임이다.
    std::int32_t x{},y{}; // 원문 하위 DWORD 좌표이며 그림자는 두 축에10을 더한다.
    bool shadow{}; // true는 원문의557738 치환 그림자, false는 원색 출력이다.
};
struct ManaAdditionalHooks { // 고정 TYPE의 현재 SHP 조회와 실제 VFX 요청을 공급한다.
    std::function<std::uint32_t(std::uint32_t)> shape; // 처음 선택한 TYPE의 현재+dc를 매 출력 직전에 읽는다.
    std::function<void(ManaAdditionalShape)> draw; // 출력 뒤 raw/TYPE가 바뀌어도 다음 출력은 현재 값을 다시 읽는다.
};
// 00473710: TYPE를 한 번 선택하고 short+c가 비영이면 그림자, 이어 현재 프레임/현재 SHP의 본체를 그린다.
void DrawManaAdditional(std::span<const std::uint8_t> raw,std::int32_t x,std::int32_t y,const ManaAdditionalHooks& hooks);
}
