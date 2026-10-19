// 원본 가상 getter와 추가 Draw를 독립 캡처333행/전체 보호 픽셀로 검증한다.
#include "o/RawThunderCannonCapture.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint8_t kType=131; // 보호 TYPE 로딩 순서에서 썬더 캐논의 번호다.
constexpr std::uint32_t kTable=0x50bad0U; // 실제 생성자46ae50이 슬롯에 쓰는 가상 표다.
// 정렬되지 않은 원문 DWORD를 이웃 바이트와 섞지 않고 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // little endian의 낮은 바이트부터 네 바이트를 합친다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
// 원본이 확인되지 않은 타입/가상 표는 그림 요청 전에 거부한다.
void Validate(std::span<const std::uint8_t> raw) {
    if (!IsThunderCannonCapture(raw)) throw std::invalid_argument("썬더 캐논 캡처의 다른 TYPE/가상 표");
}
}
bool IsThunderCannonCapture(std::span<const std::uint8_t> raw) { return raw.size()>=50 && raw[10]==kType && Read(raw,0)==kTable; }
// 현재 프레임을 유효한 방향 코드로 조회하며 단일 TYPE 기본 프레임으로 바꾸지 않는다.
std::int32_t ThunderCannonCaptureMainFrame(std::span<const std::uint8_t> raw,const RiftTypeFrames& frames) {
    Validate(raw);const auto current=Read(raw,36);
    if (current>=frames.Codes().size()) throw std::out_of_range("썬더 캐논 캡처의 현재 프레임 범위");
    return frames.Run(frames.Codes()[current].side).first;
}
// 원래497280의 shape0/frame-1은 현재 raw 타입/프레임을 뜻한다.
void DrawThunderCannonCapture(std::span<const std::uint8_t> raw,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& draw,std::int32_t x,std::int32_t y) {
    Validate(raw);if (!draw) throw std::invalid_argument("썬더 캐논 캡처의 실제 그림 소비자 누락");
    draw(kType,Read(raw,36),x,y);
}
}
