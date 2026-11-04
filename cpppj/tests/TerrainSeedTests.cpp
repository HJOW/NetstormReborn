// 실제 양판본 raw+5 시드와 공유 지형 흩뿌리기의 전체 배열/진단만 대조한다. MP/게임은 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawTerrainGrowth.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 기대 결과 계산 없이 원본 입력과 RLE 배열만 준비하는 검사 도구다.
// 원본 code/region 전체65536BYTE를 손실 없이 복원한다.
std::vector<std::uint8_t> SeedBytes(const std::string& value) {
    std::vector<std::uint8_t> result;result.reserve(65536);
    for (const auto& run:Split(value,',')) { // native가 기록한 연속 길이를 그대로 펼친다.
        const auto separator=run.find(':');if (separator==std::string::npos) throw std::logic_error("시드 RLE 필드 누락");
        const auto byte=static_cast<std::uint8_t>(std::stoul(run.substr(0,separator),nullptr,16));
        const auto count=static_cast<std::size_t>(std::stoul(run.substr(separator+1)));
        if (count>65536-result.size()) throw std::logic_error("시드 RLE 배열 초과");
        result.insert(result.end(),count,byte);
    }
    if (result.size()!=65536) throw std::logic_error("시드 RLE 전체 배열 길이 오류");
    return result;
}
// 입력 profile의 청크 경계이며 성공/실패 순서를 계산하지 않는다.
ChunkRect SeedBounds(std::int32_t profile) {
    if (profile==4) return {-1,-1,1,1};
    return {3,3,profile==0 ? 3 : 5,5};
}
// Native 입력과 같은 청크 원시 바이트만 채우고 출력 지형은 생성하지 않는다.
void SeedChunks(ChunkMap& chunks,std::int32_t profile,std::uint32_t seed) {
    const auto add=[&](int x,int y,std::uint8_t island,std::uint32_t value) { // 원본 +0/+5만 변경한다.
        auto& chunk=chunks.At(x,y);chunk.island=island;chunk.record[3]=static_cast<std::uint8_t>(value);
    };
    if (profile==2) { add(4,3,0,seed);add(3,4,1,seed+19); }
    if (profile==3) { add(3,3,255,seed);add(4,3,128,seed+19);add(3,4,0,seed+37); }
    if (profile==4) add(0,0,0,seed);
    if (profile>=5 && profile<=9) { add(3,3,0,seed);add(4,3,1,seed+13);add(3,4,127,seed+19);add(4,4,255,seed+37); }
}
// 선택한 원본 몸체만 실행하며 fixture 기대값은 getter/난수 구현으로 다시 계산하지 않는다.
void ReplaySeed(std::string_view kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_TERRAINSEED_FIXTURE);std::size_t checked{};
    for (const auto& row:fixture.rows) { // 첫 조회와 흩뿌리기 사례를 별도 검사로 분리한다.
        CHECK(row.size()==10);if (row[1]!=kind) continue;
        const auto fields=Split(row[2],',');CHECK(fields.size()==7);
        std::array<std::int32_t,7> input{};
        for (std::size_t i=0;i<input.size();++i) input[i]=static_cast<std::int32_t>(std::stoll(fields[i])); // 전체 signed DWORD를 유지한다.
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        TerrainContextGlobals globals;globals.noRegion=input[2];
        RawTerrainContext context(edition,1,127,SeedBounds(input[0]),globals,[](TerrainContextAssertion) {});
        ChunkMap chunks;SeedChunks(chunks,input[0],static_cast<std::uint32_t>(input[1]));chunks.Mode()=static_cast<std::uint32_t>(input[4]);
        if (input[0]==8) context.Map().bounds={50,50,53,53};
        if (input[0]==9) { std::fill(context.Codes().begin(),context.Codes().end(),std::uint8_t{'x'});std::fill(context.Regions().begin(),context.Regions().end(),std::uint8_t{9}); }
        std::string assertions;auto checking=input[3];
        RawTerrainGrowth growth(context,chunks,globals,{RawCheckingFlag(checking),
            [&](std::string_view expression,std::string_view file,std::uint32_t line) { // 원문 진단 복귀의 명시 효과만 적용한다.
                if (!assertions.empty()) assertions+=';';assertions+=std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line);
                if (input[5]==1 && line==0xb7) { chunks.Mode()=1;globals.noRegion=0; }
                if (input[5]==2 && line==0xb8) { auto bounds=context.Chunks();bounds.right=0;context.SetBounds(bounds);chunks.At(0,0).island=127; }
                if (input[5]==3 && line==0xb7) { chunks.Mode()=1;checking=0;chunks.At(0,0).island=127; }
            }});
        if (kind=="First") CHECK(growth.FirstChunkSeed()==Number(row[3]));else growth.Scatter(input[6]);
        CHECK((assertions.empty() ? "-" : assertions)==row[4]);
        const auto codes=SeedBytes(row[8]),regions=SeedBytes(row[9]);
        CHECK(std::equal(codes.begin(),codes.end(),context.Codes().begin()));
        CHECK(std::equal(regions.begin(),regions.end(),context.Regions().begin()));
        CHECK(Number(row[7])==(kind=="First" ? 0U : static_cast<std::uint32_t>(input[6])*2U));
        ++checked;
    }
    CHECK(checked==expected);
}
}

// 두 판본111개씩의 signed BYTE/전체 noRegion·순회·진단 복귀/고정 fallback을 대조한다.
TEST_CASE(TerrainSeed_ReplaysNativeFirstChunkAndDiagnostics) { ReplaySeed("First",222); }
// 중복/기존x의 지역 덮어쓰기·다른 섬 거부·빈 지도 fallback·0회 호출을 원문 배열 전체로 대조한다.
TEST_CASE(TerrainSeed_ReplaysNativeScatterFullArrays) { ReplaySeed("Scatter",156); }
// 같은 장기 checking은 호출 때 다시 읽으며 진단 없는 정상 호출에 장치를 미리 강제하지 않는다.
TEST_CASE(TerrainSeed_LiveCheckingAndSelectedMissingDevice) {
    TerrainContextGlobals globals;RawTerrainContext context(OriginalEdition::Patch1078,1,127,{3,3,5,5},globals,[](TerrainContextAssertion) {});
    ChunkMap chunks;std::int32_t checking{};TerrainGrowthDiagnostics diagnostics;diagnostics.checking.Bind(checking);
    RawTerrainGrowth growth(context,chunks,globals,diagnostics);CHECK(growth.FirstChunkSeed()==0x2333);
    checking=static_cast<std::int32_t>(0x80000000U);CHECK(Throws([&] { growth.FirstChunkSeed(); }));
    chunks.At(4,3).island=0;chunks.At(4,3).record[3]=255;CHECK(growth.FirstChunkSeed()==255);
    chunks.Mode()=0;checking=0;CHECK(Throws([&] { growth.FirstChunkSeed(); }));
}
// 일반 분기는 첫 시드 실패 진단을 만들지 않는다. 기존 실제 일반 성장 계약을 보존한다.
TEST_CASE(TerrainSeed_NormalBranchDoesNotQueryFirstSeed) {
    TerrainContextGlobals globals;RawTerrainContext context(OriginalEdition::Patch1078,1,127,{3,3,5,5},globals,[](TerrainContextAssertion) {});
    ChunkMap chunks;std::uint32_t checking=0xffffffff;RawTerrainGrowth growth(context,chunks,globals,{RawCheckingFlag(checking),{}});
    growth.RunNormal();CHECK(std::all_of(context.Codes().begin(),context.Codes().end(),[](auto value) { return value=='.'; }));
}
