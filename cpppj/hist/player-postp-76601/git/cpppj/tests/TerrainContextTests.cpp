// 실제 두 PE의 초기 배열·문자 대응·청크/칸bounds와 문자 변환 결과를 그대로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawTerrainContext.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 현재 고유입력의 완전한 DWORD 목록만 문자열로 바꾼다. 기대 결과를 계산하지 않는다.
template<class Values> std::string ContextValues(const Values& values) {
    std::string result;
    // 원본 순서를 유지하며 signed 값을 실제 DWORD 비트로 비교한다.
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(static_cast<std::uint32_t>(value)); }
    return result;
}
// 검증 입력의 X열 우선16×16 창을 읽는다. 원본의 바깥 배열 전체는 constructor uniform으로 별도 확인한다.
std::string ContextWindow(std::span<const std::uint8_t> bytes) {
    std::string result;
    // native가 관찰한 X열 순서로 같은 창을 이어 붙인다.
    for (std::size_t x=48;x<64;++x) result+=Hex(bytes.subspan(x*256+48,16));
    return result;
}
}

// 실제 constructor 모든 고유입력과 앞뒤문자변환·정상/assert 계속반환을 양판본에서 전수 대조한다.
TEST_CASE(TerrainContext_ReplaysNativeConstructorAndCharacters) {
    const auto fixture=LoadFixture(NETSTORM_TERRAINCONTEXT_FIXTURE);std::size_t checked=0;
    // native와 같은 값형입력만 준비하며 출력·branch·지도값을 기대와 함께 다시 계산하지 않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==13);const auto fields=Split(row[2],',');CHECK(fields.size()==10 || fields.size()==11);
        std::vector<std::uint32_t> input;
        // 원본 입력의 실제 DWORD 비트를 유지한다.
        for (const auto& field:fields) input.push_back(Number(field));
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        TerrainContextGlobals globals;globals.battle=input[1]!=0;globals.noRegion=static_cast<std::int32_t>(input[2]);globals.islandType=input[8];globals.edgeType=input[9];std::uint32_t assertions=0;
        RawTerrainContext context(edition,input[0],static_cast<std::int32_t>(input[7]),
            {static_cast<int>(input[3]),static_cast<int>(input[4]),static_cast<int>(input[5]),static_cast<int>(input[6])},globals,
            [&assertions](TerrainContextAssertion) { ++assertions; });
        if (row[1]!="Ctor") {
            const auto profile=input[10];
            // 같은5×5의 문자/지역 입력만 넣고 실제 모듈에 변환을 맡긴다.
            for (std::size_t x=49;x<54;++x)
                // 각 X열은 Y순서로 원본 입력과 같은 byte를 쓴다.
                for (std::size_t y=51;y<56;++y) {
                    std::uint8_t value='x';if (profile==1 && x==49) value='X';if (profile==2 && y==51) value='o';
                    if (profile==3 && x==52) value='=';if (profile==4 && x==51 && y==53) value='.';
                    context.Codes()[x*256+y]=value;context.Regions()[x*256+y]=static_cast<std::uint8_t>(profile==3 && y==54 ? 6 : 5);
                }
            if (row[1]=="Edge") context.MarkEdges('x','=');else context.NormalizeAfterStamp();
        }
        const auto& map=context.Map();const auto chunks=context.Chunks();const auto codes=context.Codes(),regions=context.Regions();
        const std::array<std::uint32_t,4> uniform{static_cast<std::uint32_t>(std::all_of(codes.begin(),codes.end(),[&](auto value) { return value==codes.front(); })),codes.front(),
            static_cast<std::uint32_t>(std::all_of(regions.begin(),regions.end(),[&](auto value) { return value==regions.front(); })),regions.front()};
        CHECK(context.Owner()==Number(row[3]));CHECK(ContextValues(std::array<std::uint32_t,4>{map.makePredictable,map.lastSid.value,map.excludedFlags,static_cast<std::uint32_t>(context.Region())})==row[4]);
        CHECK(ContextValues(std::array<int,4>{chunks.left,chunks.top,chunks.right,chunks.bottom})==row[5]);CHECK(ContextValues(map.bounds)==row[6]);
        CHECK(ContextValues(map.typeMapping)==row[7]);CHECK(ContextValues(map.aliases)==row[8]);CHECK(ContextValues(uniform)==row[9]);
        CHECK(ContextWindow(codes)==row[10] && ContextWindow(regions)==row[11] && assertions==Number(row[12]));++checked;
    }
    CHECK(checked==224);
}

// 초기 도장은 같은 owned배열을 참조하며 잘못된 배열 밖 범위를 묵시clamp하지 않는다.
TEST_CASE(TerrainContext_SharesArraysAndRejectsMissingDiagnostics) {
    TerrainContextGlobals globals;RawTerrainContext context(OriginalEdition::Patch1078,1,17,{1,1,15,15},globals,[](TerrainContextAssertion) {});
    CHECK(context.Map().codes.data()==context.Codes().data() && context.Map().regions.data()==context.Regions().data());
    CHECK(context.Map().aliases.data()==context.Aliases().data());context.Aliases()['h']='x';CHECK(context.Map().aliases['h']=='x');
    CHECK(Throws([&] { RawTerrainContext missing(OriginalEdition::Patch1078,1,17,{1,1,15,15},globals,{}); }));
    context.SetBounds({-1,0,1,1});CHECK(context.Map().bounds[0]==-16);CHECK(Throws([&] { context.MarkEdges('x','='); }));
}
