// DWORD 후보의 높은 바이트를 색 번호에 섞지 않고 원본 아홉 표를 생성한다.
#include "client/OwnerColorMaps.h"
#include "o/RawColorSortPrimary.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 보호10.78 PE의005319e8..00531b07 원문이다. 첫8개0은 색0 후보이며 변환표0 자체는 항등이다.
constexpr std::array<std::uint32_t,72> kCandidates{
    0,0,0,0,0,0,0,0,81,80,76,145,146,147,4,10,94,100,170,169,204,249,217,227,
    195,93,185,35,31,149,70,15,207,131,133,128,136,2,142,143,189,190,253,150,148,70,14,10,
    113,218,219,3,52,59,67,213,9,85,155,156,6,72,144,143,220,221,222,223,224,63,227,213};
}
// 원본 정적 DWORD 표의 읽기 전용 view다.
std::span<const std::uint32_t,72> OriginalOwnerColorCandidates() { return kCandidates; }
// 원본 가변 인자의 실제 DWORD 순서를 생성·수명 모듈이 함께 사용한다.
std::span<const std::uint32_t> SpecialOwnerColorSources(std::uint32_t type) {
    static constexpr std::array<std::uint32_t,33> surface{44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,3,177,200,203,224,220,221,222,223,114,201,226}; // 94/97의 지면 소스다.
    static constexpr std::array<std::uint32_t,20> first{86,188,85,247,84,31,34,75,82,74,83,156,153,70,72,71,73,152,13,144}; // 111의 소스다.
    static constexpr std::array<std::uint32_t,21> second{87,88,85,86,84,156,75,82,153,74,73,83,155,72,71,44,152,154,70,13,144}; // 141의 소스다.
    static constexpr std::array<std::uint32_t,19> third{218,219,220,52,53,54,55,56,57,58,59,66,161,210,19,21,214,141,18}; // 137의 소스다.
    if (type==111) return first;if (type==141) return second;if (type==137) return third;if (type==94 || type==97) return surface;
    throw std::invalid_argument("전용 소유자 색표 TYPE 범위");
}
// 원문에서 색0 행은 변경하지 않고1..8행만 실제 띠를 바꾼다.
OwnerColorMaps BuildStandardOwnerColorMaps(std::uint32_t type,std::uint32_t bridgeType,std::span<const std::uint32_t,72> candidates) {
    OwnerColorMaps result;
    // 모든 비대상 색/색0의 원래 번호를 유지한다.
    for (auto& row:result) {
        // 원본256바이트 항등 표 하나다.
        for (std::size_t color=0;color<row.size();++color) row[color]=static_cast<std::uint8_t>(color);
    }
    // 플레이어 색 번호1..8 각각의 명도 후보를 사용한다.
    for (std::size_t owner=1;owner<result.size();++owner) {
        //228..237은 밝은0,238..245는 역순7..0이며 다리의 색2/7은 후반 명도를 고정한다.
        for (std::size_t color=228;color<=245;++color) {
            auto shade=color<238 ? std::size_t{} : 245-color;
            if (color>=238 && type==bridgeType && (owner==2 || owner==7)) shade=owner==2 ? 4 : 3;
            result[owner][color]=static_cast<std::uint8_t>(candidates[owner*8+shade]);
        }
    }
    return result;
}
// 실제 지정 소스만 바꾸며 등록되지 않은 소스/중립0표는 항등을 유지한다.
namespace {
// 기존소스/후보/보정본문을공유하며명도읽기장치만실제raw와독립adapter로구별한다.
template<class Lightness>
OwnerColorMaps BuildSpecialOwnerFromLightness(const Lightness& light,std::uint32_t type,std::span<const std::uint32_t,72> candidates) {
    constexpr std::array<double,8> offsets{-0.1,-0.17,0.15,0,0,0.05,-0.2,0}; // 실제43b750의 소유자별 double 명도 보정이다.
    constexpr double otherOffset=-0.15000000596046448; // 원본 ColorSort 정규화 및141의 저장 double 값이다.
    const auto sources=SpecialOwnerColorSources(type); // 같은 Native 호출자가 선택한 DWORD 소스 순서다.
    OwnerColorMaps result; // 실제항등9표이며명도는현재읽기장치에서필요할때직접조회한다.
    // 중립을포함한9표를원본43b950처럼항등으로시작한다.
    for (std::size_t index=0;index<256;++index) { // 소스색순서를유지한다.
        for (auto& map:result) map[index]=static_cast<std::uint8_t>(index); // 현재원래번호를아홉행에쓴다.
    }
    // 실제 변환은 소유자1..8에만 쓰고 동일 거리 후보는 앞의 번호를 유지한다.
    for (std::size_t owner=1;owner<result.size();++owner) {
        const double offset=type==97 ? offsets[owner-1] : (type==141 ? otherOffset : 0.0);
        // 실제 호출자의 지정 소스 순서대로 같은 표를 수정한다.
        for (const auto source:sources) {
            const double target=light(source)+offset;std::size_t selected{};double minimum=std::min(99.0,std::abs(target-light(candidates[owner*8])));
            // 후보1..7은 엄격히 더 가까울 때만 교체하며 float로 최소값을 좁히지 않는다.
            for (std::size_t shade=1;shade<8;++shade) { const double distance=std::abs(target-light(candidates[owner*8+shade]));if (distance<minimum) { minimum=distance;selected=shade; } }
            result[owner][source]=static_cast<std::uint8_t>(candidates[owner*8+selected]);
        }
    }
    return result;
}
}
// 기존Native15관측과장치없는독립adapter는같은logical계산을유지한다.
OwnerColorMaps BuildSpecialOwnerColorMaps(std::span<const std::uint32_t,256> logical,std::uint32_t type,std::span<const std::uint32_t,72> candidates) {
    constexpr double normalize=1.0/255.0; // 기존adapter의binary64상수다.
    std::array<double,256> light; // 실제Client의raw소유자로세지않는명시독립입력이다.
    for (std::size_t index=0;index<light.size();++index) { // 기존RGB→명도계산순서를그대로유지한다.
        const double red=(logical[index]&255)*normalize,green=((logical[index]>>8)&255)*normalize,blue=((logical[index]>>16)&255)*normalize;
        light[index]=(std::max({red,green,blue})+std::min({red,green,blue}))*0.5;
    }
    return BuildSpecialOwnerFromLightness([&light](std::size_t index) { return light[index]; },type,candidates);
}
// 실제소비자는같은primary의현재+8double을읽고RGB에서명도를재계산하지않는다.
OwnerColorMaps BuildSpecialOwnerColorMaps(const o::RawColorSortPrimary& primary,std::uint32_t type,std::span<const std::uint32_t,72> candidates) {
    return BuildSpecialOwnerFromLightness([&primary](std::size_t index) { return primary.Lightness(index); },type,candidates);
}
}
