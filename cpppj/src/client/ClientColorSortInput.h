// 실제ColorSort가최초논리RGB를복사하는수명을소유하고기존색표계산을같은고정입력에합성한다.
#pragma once
#include "client/Renderer.h"
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace netstorm::client {
// 실제Client는고정초기입력을필수로주며소유자없는기존독립표시검사만명시current fallback을사용한다.
inline std::span<const std::uint32_t,256> ColorSortLogicalInput(std::span<const std::uint32_t> fixed,std::span<const std::uint32_t,256> independentCurrent) {
    if (fixed.empty()) return independentCurrent;
    if (fixed.size()!=256) throw std::invalid_argument("ColorSort 고정 초기RGB256색 계약");
    return std::span<const std::uint32_t,256>(fixed.data(),256);
}
// raw0x3800/0x5008전체객체를가장하지않는고정RGB입력과43c010의전체9표소유자다.
class ClientColorSortInput {
public:
    ClientColorSortInput()=default; //실제439de0입력캡처전에는현재ScreenRGB로빈성공을만들지않는다.
    ClientColorSortInput(const ClientColorSortInput&)=delete; //같은입력/표주소를빌린소비자에게사본을주지않는다.
    ClientColorSortInput& operator=(const ClientColorSortInput&)=delete; //현재주소를대입으로바꾸지않는다.
    ClientColorSortInput(ClientColorSortInput&&)=delete; //Client수명의고정주소를유지한다.
    ClientColorSortInput& operator=(ClientColorSortInput&&)=delete; //이미빌린span을이전소유자에남기지않는다.
    void Capture(std::span<const std::uint32_t,256> logical); //439de0의명시생성위치에서만전체RGBX입력을복사한다.일반palette변경은호출하지않는다.
    bool Captured() const; //호스트입력계약의준비상태이며원문paletteLoaded와별개다.
    std::span<const std::uint32_t,256> Logical() const; //초기입력의같은주소를빌린다.미캡처는명시오류다.
    const std::array<ColorMap,9>& Rebuild(std::uint32_t currentRed); //43c010의현재빨강과같은초기입력으로기본53비트9표를갱신한다.64비트별도출력은혼합하지않는다.
    const std::array<ColorMap,9>& Maps() const; //명시Rebuild뒤같은현재표를빌린다.
private:
    std::array<std::uint32_t,256> logical_{}; //주소가변하지않는최초ColorSort입력이다.미캡처0은유효입력이아니다.
    std::array<ColorMap,9> maps_{}; //555e38/55f38/56038/56138/56238/56338/56438/57738/57838순서다.
    bool captured_{},built_{}; //실제원문DWORD를대신하지않는host물리준비계약이다.
};
}
