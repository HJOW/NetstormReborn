// 원본459입력/918반환의 전체 픽셀 fixture가 확인한 간헐천 추가 그림만 제출한다.
#include "o/RawGeyserCapture.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint8_t kType=122; // 보호10.78 TYPE 로딩 순서의 간헐천 번호다.
constexpr std::uint32_t kTable=0x50da88U; // 실제4857a0 생성자가 슬롯에 설치하는 가상 표다.
// 정렬되지 않은 원문 DWORD를 낮은 바이트부터 정확히 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // 호스트 정렬/엔디언과 관계없이 원본 little endian 네 바이트를 조합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
}
// 가상 표와 타입 둘 다 일치해야 원본 고정 본체/현재 그림 조합을 적용한다.
bool IsGeyserCapture(std::span<const std::uint8_t> raw) { return raw.size()>=50 && raw[10]==kType && Read(raw,0)==kTable; }
// 현재 프레임이 본체와 같아도 원본처럼 두 번째 그림을 제출한다.
void DrawGeyserCapture(std::span<const std::uint8_t> raw,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& draw,std::int32_t x,std::int32_t y) {
    if (!IsGeyserCapture(raw)) throw std::invalid_argument("간헐천 캡처의 다른 TYPE/가상 표");
    if (!draw) throw std::invalid_argument("간헐천 캡처의 실제 그림 소비자 누락");
    draw(kType,Read(raw,36),x,y);
}
}
