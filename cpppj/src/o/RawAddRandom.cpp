// 실제 지역 난수 seed 뒤의 소유자 조회 순서를 유지한다.
#include "o/RawAddRandom.h"
#include "o/ChunkMap.h"
#include <stdexcept>

namespace netstorm::o { // 카메라/글로벌 난수를 이 부모의 범위/seed로 대체하지 않는다.
namespace { // 선택된 실제 장치만 호출 위치에서 요구한다.
// 앞선 효과가 실행된 뒤 장치 누락이 발생하면 그 효과를 임의로 되돌리지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AddRandom 부모의 실제 장치 미연결");return function; }
}
// 기본 범위 변환은 원문00431fc0/0040d900의 고정1/15를 각각16배한 값이다.
void AddRandomObjects(std::uint32_t type,std::uint32_t quantity,const AddRandomHooks& hooks) {
    GameRandom random(RawFortFinishOwner::TimeSeed(Need(hooks.wallSeconds)()));
    const auto chunk=DefaultChunkRect();const FortRandomRectangle rectangle{static_cast<float>(chunk.left*16),static_cast<float>(chunk.top*16),static_cast<float>(chunk.right*16),static_cast<float>(chunk.bottom*16)};
    const auto owner=Need(hooks.local)();const auto region=Need(hooks.island)(owner,0);
    static_cast<void>(Need(hooks.place)(random,rectangle,region,quantity,type,0));
}
}
