// 추가 그림 함수에 없는 dead/추상/소유자 관문을 새로 추가하지 않는다.
#include "o/RawDaisDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 정렬되지 않은 원본 DWORD를 필요한 위치에서만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset+4>raw.size()) throw std::out_of_range("다이스 그림 raw 범위 오류");std::uint32_t value{};
    // 원본 little endian 네 바이트만 조합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
}
// 생성자 주소가 확인된 실제 다이스만 별도 그림으로 분배한다.
bool IsPatchDais(std::span<const std::uint8_t> raw) { return Read(raw,0)==0x507950U && raw.size()>10 && raw[10]==165; }
// TYPE169의 현재 전체 프레임 수를 사용하며 다이스165의 프레임 수로 대체하지 않는다.
void DrawDaisAdditional(std::span<const std::uint8_t> raw,std::int32_t extraFrameCount,const std::function<void(std::uint32_t,std::int32_t)>& draw) {
    if (!IsPatchDais(raw) || extraFrameCount<=0 || !draw) throw std::invalid_argument("다이스 추가 그림 TYPE/프레임 수/소비자 오류");
    const auto frame=std::bit_cast<std::int32_t>(Read(raw,36)); // 원본 idiv의 signed DWORD 프레임이다.
    draw(kPatchDaisExtraType,frame%extraFrameCount);
}
}
