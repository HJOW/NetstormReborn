// 원본 ShapeToBuffer의384개 입력/전체 보호 픽셀과 대조한72 전용 가상 그림이다.
#include "o/RawSunAviaryCapture.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint8_t kType=72; // 원본 TYPE 로딩 순서의 훨리베이스 번호다.
constexpr std::uint32_t kTable=0x509930U; // 원본 생성자와 홈 통지가 공유하는 실제 가상 표다.
// 정렬되지 않은 DWORD를 해당 네 바이트에서만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // little endian의 낮은 바이트부터 합치며 주변 필드는 읽지 않는다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
// 확인되지 않은 타입/가상 표는 실제 그림 요청 전에 거부한다.
void Validate(std::span<const std::uint8_t> raw) {
    if (!IsSunAviaryCapture(raw)) throw std::invalid_argument("훨리베이스 캡처의 다른 TYPE/가상 표");
}
}
// 판본의 실제50바이트 슬롯과 두 식별자를 함께 확인한다.
bool IsSunAviaryCapture(std::span<const std::uint8_t> raw) { return raw.size()>=50 && raw[10]==kType && Read(raw,0)==kTable; }
// 생산 앞구간은 이동한 기본 그림, 뒷구간은10번이며 extra 관문은 현재 그림을 반환한다.
std::int32_t SunAviaryCaptureMainFrame(std::span<const std::uint8_t> raw) {
    Validate(raw);const auto current=std::bit_cast<std::int32_t>(Read(raw,36));
    if (raw[40]&9U) return current;
    if (current>=20 && current<=26) return 10;
    return current>=11 && current<=19 ? current-9 : -1;
}
// 생략 경로는 소비자를 요구하지 않으며 실제497280의 shape0/frame-1을 현재 값으로 전달한다.
void DrawSunAviaryCapture(std::span<const std::uint8_t> raw,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& draw,std::int32_t x,std::int32_t y) {
    Validate(raw);if (raw[40]&9U) return;
    if (!draw) throw std::invalid_argument("훨리베이스 캡처의 실제 그림 소비자 누락");
    draw(kType,Read(raw,36),x,y);
}
}
