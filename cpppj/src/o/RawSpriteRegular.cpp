// 다른 파생의 프레임 진행/방향 나머지와 구별하여 원문4794b0 한 번의 signed 나머지를 수행한다.
#include "o/RawSpriteRegular.h"
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::o { //원본 주소는 판본/원문 기록이며 호스트 함수 포인터로 실행하지 않는다.
float RunSpriteRegular(const SidPool& pool,std::span<const RiftTypeRecord> types,const SquidFrame& setter,Sid parent,float payload) { //4ad6c0 이전의 현재 프레임과 이후 실제 지정/표시 순서를 유지한다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || &pool!=&setter.Pool() || types.size()!=188) throw std::invalid_argument("Sprite Regular의 같은PATCH/188TYPE/Frame 계약 오류");
    const auto raw=pool.Slot(parent);std::uint32_t current{};
    for (std::size_t byte=0;byte<4;++byte) current|=std::uint32_t(raw[36+byte])<<(8*byte); //원문은 전체DWORD 프레임을 먼저 선저장한다.
    const auto type=raw[10];if (type>=types.size()) throw std::out_of_range("Sprite Regular의 현재TYPE 범위 오류");
    const auto divisor=types[type].frameCount,next=std::bit_cast<std::int32_t>(current+1U);
    if (!divisor || (next==std::numeric_limits<std::int32_t>::min() && divisor==-1)) throw std::domain_error("Sprite 원본 IDIV 오류");
    setter.Set(parent,next%divisor,0);return payload;
}
}
