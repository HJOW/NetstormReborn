// 원본은 행 시작의 폭을 직전 셀 끝에서 읽은 값으로 유지하며 최초 height도 scope 쓰기보다 먼저 읽는다.
#include "o/RawFortRegionSave.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 실제 선택한 원본 경계만 요구하여 빈 지도에서 도달하지 않는 장치를 강제로 호출하지 않는다.
template<class Function> const Function& RegionSaveEffect(const Function& function) {
    if (!function) throw std::logic_error("전체 Fort 청크 저장의 실제 경계 누락");return function;
}
// 원본 INC의 signed DWORD 감김을 C++ signed overflow 없이 보존한다.
std::int32_t NextRegionCoordinate(std::int32_t coordinate) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(coordinate)+1U); }
// 두 순회의 같은 폭/높이 읽기 위치를 유지하고 Territory만 signed BYTE 영역 판정을 실행한다.
void SaveRegionChunks(std::uint32_t stream,std::uint32_t owner,std::int32_t region,std::uint32_t& scope,const FortRegionSaveHooks& hooks,bool territory) {
    auto height=RegionSaveEffect(hooks.chunksY)();scope=territory ? 1U : 2U; // 최초 MOV height가 scope 쓰기보다 먼저 실행된다.
    if (height<=0) return;
    auto width=RegionSaveEffect(hooks.chunksX)();std::int32_t y{}; // 원본 최초 폭은 양수 height 관문 뒤에만 읽는다.
    do { // 매 행의 시작은 마지막 셀 끝에서 읽은 width를 사용하며 getter를 추가하지 않는다.
        std::int32_t x{};
        if (width>0) do { // 현재 셀의 영역 조회를 마친 뒤에만 판정/저장과 다음 폭 조회가 진행된다.
            const auto byte=RegionSaveEffect(hooks.chunkRegion)(x,y);
            if (!territory || RegionSaveEffect(hooks.allowed)(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(byte)),owner,region))
                RegionSaveEffect(hooks.chunk)(stream,x,y);
            width=RegionSaveEffect(hooks.chunksX)();x=NextRegionCoordinate(x);
        } while (x<width);
        height=RegionSaveEffect(hooks.chunksY)();y=NextRegionCoordinate(y);
    } while (y<height);
}
}
// 선택한 청크만 같은 하위 writer에 전달하며 소유자/영역 인자는 함수 수명 동안 고정한다.
void SaveFortTerritoryChunks(std::uint32_t stream,std::uint32_t owner,std::int32_t region,std::uint32_t& scope,const FortRegionSaveHooks& hooks) {
    SaveRegionChunks(stream,owner,region,scope,hooks,true);
}
// Chaff는 실제 청크 getter의 반환을 사용하지 않지만 호출/변경 효과는 Territory와 같은 위치에서 유지한다.
void SaveFortChaffChunks(std::uint32_t stream,std::uint32_t& scope,const FortRegionSaveHooks& hooks) {
    SaveRegionChunks(stream,0,0,scope,hooks,false);
}
}
