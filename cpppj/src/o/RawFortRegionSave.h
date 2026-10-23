// 전체 Territory/Chaff의 현재 지도 크기 재조회와 같은 저장 scope를 보존한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::o {
// 원본 두 순회가 사용하는 실제 청크 지도·영역 판정·한 청크 저장 경계다.
struct FortRegionSaveHooks {
    std::function<std::int32_t()> chunksX,chunksY; //0054c8a4/0054c8a8의 현재 signed DWORD를 원본 읽기 위치에서 조회한다.
    std::function<std::uint8_t(std::int32_t,std::int32_t)> chunkRegion; //00432180의 실제 청크 조회이며 Chaff도 반환값만 버리고 효과는 실행한다.
    std::function<std::uint32_t(std::int32_t,std::uint32_t,std::int32_t)> allowed; //0046f200:청크의 signed BYTE와 고정 owner/region을 전달한다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> chunk; //004be020:고정 stream과 해당 청크 x/y를 저장한다.
};
//004be230:현재 height 읽기→공유 scope1→선택 영역의 모든 청크 저장이다. scope를 끝에서 복구하지 않는다.
void SaveFortTerritoryChunks(std::uint32_t stream,std::uint32_t owner,std::int32_t region,std::uint32_t& scope,const FortRegionSaveHooks& hooks);
//004be350:현재 height 읽기→공유 scope2→매 셀 청크 조회와 저장이다. 영역 판정 장치는 요구하지 않는다.
void SaveFortChaffChunks(std::uint32_t stream,std::uint32_t& scope,const FortRegionSaveHooks& hooks);
}
