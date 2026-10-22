// 보호PE의 +78=4acfe0/+a0=41c0f0/+a4=41c100에 연결되는 현재 슬롯만 허용한다.
#include "o/RawPlainWorkshopCapture.h"
#include <array>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,5> kPlainWorkshops{{
    {129,0x50b9b8},{136,0x509470},{140,0x50b6e8},{143,0x5099e8},{148,0x50b7c0}
}}; // 아이스 캐논/썬 바리케이드/크로스보우/맨오워 풀/반더 타워의 정확한 쌍이다.
}
// 같은VT를 공유하는128을 이름이나 주소 하나만으로 새 분배에 포함하지 않는다.
bool IsPlainWorkshopCapture(std::span<const std::uint8_t> raw) {
    if (raw.size()<50) return false;std::uint32_t table{}; // 비정렬 little endian의 현재 가상 표다.
    for (std::size_t index=0;index<4;++index) table|=std::uint32_t(raw[index])<<(8*index); // 네 바이트의 원래 폭만 읽는다.
    for (const auto& [type,value]:kPlainWorkshops) if (raw[10]==type && table==value) return true; // 현재TYPE도 함께 대조한다.
    return false;
}
}
