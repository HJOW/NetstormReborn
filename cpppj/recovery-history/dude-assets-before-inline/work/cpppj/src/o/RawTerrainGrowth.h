// 실제 일반 초기 지형의 통로·청크별 성장·빈틈 보정을 현재 owned context에 연결한다.
#pragma once
#include "o/RawTerrainContext.h"

namespace netstorm::o {
// 패치004c10a0/CD0049e160의 일반 생성 분기다. archipelago 상위 분기는 별도다.
class RawTerrainGrowth {
public:
    // 지도/청크/현재 전역은 이 객체보다 오래 살아야 한다. 두 판본 모두 로컬 난수만 소비한다.
    RawTerrainGrowth(RawTerrainContext& context,const ChunkMap& chunks,const TerrainContextGlobals& globals);
    // 원본 Y외부/X내부 통로→현재 전체 청크 성장→두 번 빈틈 보정을 같은 배열에 수행한다.
    void RunNormal();
private:
    // 실제 지도 저장 범위를 검사하고 X열 우선 주소를 얻는다. 손상 좌표를 clamp하지 않는다.
    static std::size_t Index(std::int32_t x,std::int32_t y);
    // 원본 청크 BYTE를 signed로 읽는다. 현재 섬 목록의 존재 조회와 별개다.
    std::int32_t ChunkRegion(std::int32_t x,std::int32_t y) const;
    // 004c0930/CD0049d810: X외부/Y내부 사각형에서 code가 다를 때만 code/region을 쓴다.
    std::int32_t Fill(std::int32_t left,std::int32_t top,std::int32_t right,std::int32_t bottom,std::uint8_t region);
    // 004c0780/CD0049d5b0: inclusive bounds/현재 청크 지역을 만족하는 비x 칸만 추가한다.
    bool Put(std::int32_t x,std::int32_t y,std::uint8_t region);
    // 004c0800/CD0049d690: 1000회 연속 실패와 Y외부/X내부3×3 성장을 원본 순서로 수행한다.
    void Grow(std::int32_t island,std::int32_t x,std::int32_t y,std::int32_t target,std::uint32_t& seed);
    // 두 판본의 실제 caller 로컬 시드를 소비한다. CD 디컴파일의 생략된 this도 기계어로 확인했다.
    std::int32_t Next(std::uint32_t& seed,std::uint32_t limit);
    // 004c0c30/CD0049da90: 원래 이웃13개를 읽어 같은 지역 연속5개/대각선 끝에서 빈 칸을 메운다.
    void FixCell(std::int32_t x,std::int32_t y);
    // 004c0cf0/CD0049db10: Y범위와 X범위의 서로 다른 끝 조건을 보존한다.
    void FixGaps(std::int32_t passes);
    // context_: 새 지도 사본 없이 실제 code/region/현재 bounds를 쓰는 owned context다.
    RawTerrainContext& context_;
    // chunks_: 원본12바이트 항목의 섬번호/방향/seed/ordinal을 읽는 현재 청크 지도다.
    const ChunkMap& chunks_;
    // globals_: 선택 지역/빈 청크 비교가 읽는 현재 noRegion 전역이다. constructor 초기값으로 복제하지 않는다.
    const TerrainContextGlobals& globals_;
};
}
