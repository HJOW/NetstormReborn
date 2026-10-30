// 메타 배율 표시와 고정 추상색 추가 Draw의 원문 필드/출력 순서를 공유한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o { // 원문479340/485f90의 마지막 VFX 경계다.
struct AuxiliaryShape { // SHP는 같은 TYPE 자산 테이블의 호스트 식별자다.
    std::uint32_t shape{},frame{}; // 현재 TYPE+dc SHP와 출력 직전에 읽은+24 프레임이다.
    std::int32_t x{},y{}; // 원문이 넘긴 기준점이며 임의 여백/화면 보정을 넣지 않는다.
    bool abstract{}; // true는 원문의556238 고정 소스 치환표, false는 원색 배율 출력이다.
    std::int32_t scale{}; // 0은 비배율 추상색 출력이며 나머지는 두 축에 같은 signed16.16 값이다.
};
struct AuxiliaryDrawHooks { // 같은 raw/TYPE 저장소와 현재 VFX 출력기를 빌린다.
    std::function<std::uint32_t(std::uint32_t)> shape; // 현재 TYPE+dc SHP 토큰을 조회한다.
    std::function<void(AuxiliaryShape)> draw; // 마지막 배율/치환 출력 요청을 실제 표면에 전달한다.
};
// 00479340: hidden4000 조기 반환→현재16.16 배율(0은1배)→현재 프레임/TYPE/SHP→원색 배율 출력이다.
void DrawScaledMetadisplay(std::span<const std::uint8_t> raw,std::int32_t x,std::int32_t y,const AuxiliaryDrawHooks& hooks);
// 00485f90: 현재 프레임/TYPE/SHP를 읽어556238 고정 치환표로 한 번 출력한다. 소유자 관문은 없다.
void DrawAbstractAdditional(std::span<const std::uint8_t> raw,std::int32_t x,std::int32_t y,const AuxiliaryDrawHooks& hooks);
}
