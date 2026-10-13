// 양판본 실제 일반 성장의 전체 code/region 배열을 고유 원본 입력 그대로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawTerrainGrowth.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 전체 배열의 원본 RLE를 풀며 기대 지형을 새로 계산하지 않는다.
std::vector<std::uint8_t> GrowthBytes(const std::string& value) {
    std::vector<std::uint8_t> result;result.reserve(65536);
    // native의 연속값/길이를 원본 저장 순서로 그대로 복원한다.
    for (const auto& run:Split(value,',')) {
        const auto separator=run.find(':');if (separator==std::string::npos) throw std::logic_error("성장 RLE 필드 누락");
        const auto byte=static_cast<std::uint8_t>(std::stoul(run.substr(0,separator),nullptr,16));
        const auto count=static_cast<std::size_t>(std::stoul(run.substr(separator+1)));
        if (count>65536-result.size()) throw std::logic_error("성장 RLE 배열 범위 초과");
        result.insert(result.end(),count,byte);
    }
    if (result.size()!=65536) throw std::logic_error("성장 RLE 전체 배열 길이 오류");
    return result;
}
// 기대 지형 없이 입력 청크의 원본 섬BYTE/방향/seed만 만든다.
void GrowthChunks(ChunkMap& chunks,std::uint32_t profile,std::uint32_t seed) {
    // 각 profile의 명시 청크 좌표는 native 입력과 동일하며 출력 mask는 생성하지 않는다.
    const std::array<std::vector<std::pair<int,int>>,6> profiles{{{},{{3,3}},{{3,3},{4,3},{3,4},{4,4}},
        {{3,3},{4,3},{3,4},{4,4}},{{3,3},{4,3},{5,3},{3,4},{4,4},{5,4}},{{3,3},{4,4}}}};
    std::size_t index=0;
    // 원본 입력의 현재 청크 필드만 채운다.
    for (const auto [x,y]:profiles.at(profile)) {
        auto& chunk=chunks.At(x,y);chunk.island=static_cast<std::uint8_t>((profile==3 || profile==5) ? (x-3)&1 : 0);
        chunk.side=static_cast<char>('A'+(profile*3+index)%16);
        chunk.record[3]=static_cast<std::uint8_t>(seed+(x-3)*13+(y-3)*19);++index;
    }
}
}

// 정상108개 원본 입력은 각 판본54개이며 두x87 반복은 고유 검사 개수에 더하지 않는다.
TEST_CASE(TerrainGrowth_ReplaysBothNativeFullArrays) {
    const auto fixture=LoadFixture(NETSTORM_TERRAINGROWTH_FIXTURE);std::size_t checked=0;
    // 모든 기대 배열은 actual PE fixture에서 읽고 C++ actual 성장에 같은 입력만 공급한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==6);const auto input=Split(row[1],',');CHECK(input.size()==3);
        const auto profile=Number(input[0]),seed=Number(input[1]),selected=Number(input[2]);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        TerrainContextGlobals globals;std::uint32_t assertions=0;
        RawTerrainContext context(edition,1,static_cast<std::int32_t>(selected),{3,3,profile==4 ? 6 : 5,5},globals,
            [&assertions](TerrainContextAssertion) { ++assertions; });
        ChunkMap chunks;GrowthChunks(chunks,profile,seed);
        if (profile==5) { context.Codes()[56*256+56]='x';context.Regions()[56*256+56]=3;
            context.Codes()[57*256+56]='=';context.Regions()[57*256+56]=3; }
        RawTerrainGrowth growth(context,chunks,globals);growth.RunNormal();
        const auto codes=GrowthBytes(row[3]),regions=GrowthBytes(row[4]);
        CHECK(std::equal(codes.begin(),codes.end(),context.Codes().begin()));
        CHECK(std::equal(regions.begin(),regions.end(),context.Regions().begin()));
        CHECK(Number(row[2])==seed && assertions==Number(row[5]));++checked;
    }
    CHECK(checked==108);
}

// 성장은 같은 context를 갱신하고 mapping/alias/도장 마지막SID는 유지한다.
TEST_CASE(TerrainGrowth_UsesSameContextAndPreservesStampFields) {
    TerrainContextGlobals globals;RawTerrainContext context(OriginalEdition::Patch1078,1,127,{3,3,5,5},globals,
        [](TerrainContextAssertion) {});ChunkMap chunks;GrowthChunks(chunks,2,7);
    auto* codes=context.Codes().data();auto* regions=context.Regions().data();context.Map().lastSid=Sid{123};
    const auto mapping=std::vector<std::int32_t>(context.Map().typeMapping.begin(),context.Map().typeMapping.end());
    RawTerrainGrowth growth(context,chunks,globals);growth.RunNormal();
    CHECK(context.Codes().data()==codes && context.Map().codes.data()==codes);
    CHECK(context.Regions().data()==regions && context.Map().regions.data()==regions);
    CHECK(context.Map().lastSid==Sid{123} && context.Map().makePredictable==1);
    CHECK(std::equal(mapping.begin(),mapping.end(),context.Map().typeMapping.begin()));
}
