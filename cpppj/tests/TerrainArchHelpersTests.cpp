// 군도 공유 성장/제거 helper의 전체 지도와 caller 난수를 양판본 PE 기록에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawTerrainGrowth.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 원문 출력 계산 없이 입력과 RLE 기록을 복원하는 검사 도구다.
// 원본 관측의 연속 BYTE/길이를 전체65536BYTE 배열로 되돌린다.
std::vector<std::uint8_t> ArchBytes(const std::string& value) {
    std::vector<std::uint8_t> result;result.reserve(65536);
    for (const auto& run:Split(value,',')) { // 기록된 길이만 펼치고 기대 지형은 계산하지 않는다.
        const auto separator=run.find(':');if (separator==std::string::npos) throw std::logic_error("군도 RLE 필드 누락");
        const auto byte=static_cast<std::uint8_t>(std::stoul(run.substr(0,separator),nullptr,16));
        const auto count=static_cast<std::size_t>(std::stoul(run.substr(separator+1)));
        if (count>65536-result.size()) throw std::logic_error("군도 RLE 길이 초과");
        result.insert(result.end(),count,byte);
    }
    if (result.size()!=65536) throw std::logic_error("군도 전체 배열 길이 오류");
    return result;
}
// 실제 helper가 읽을 명시 문자/지역만 놓고 결과 성장/가중치/제거는 계산하지 않는다.
void ArchInput(RawTerrainContext& context,std::uint32_t profile) {
    std::fill(context.Regions().begin(),context.Regions().end(),std::uint8_t{9});
    for (std::int32_t x=48;x<112;++x) { // Native와 같은 넓은 입력 칸 영역이다.
        for (std::int32_t y=48;y<112;++y) { // 입력 공식은 호출할 제거 helper와 독립이다.
            std::uint8_t code='.';
            if (profile==0) code=(x==70 && y==70) ? 'x' : '.';
            else if (profile==1) code=(x*3+y)%7==0 ? 'x' : (x+y)%3==0 ? '=' : '.';
            else if (profile==2) code='x';
            else if (profile==3) code=(x==70 || y==70) ? 'x' : '.';
            else if (profile==4) code=(x/3+y/3)%2==0 ? 'x' : '.';
            else code=(x+y)%2==0 ? 'x' : '.';
            const auto index=static_cast<std::size_t>(x)*256+static_cast<std::size_t>(y);
            context.Codes()[index]=code;context.Regions()[index]=static_cast<std::uint8_t>((x+y)%256);
        }
    }
}
// 입력의 signed DWORD를 같은 하위 비트로 읽는다.
std::int32_t ArchSigned(const std::string& value) { return std::bit_cast<std::int32_t>(Number(value)); }
// 선택 helper의 양판본 모든 고유 행을 재생하며 두 배열과 caller 시드를 원문 기대값과 비교한다.
void ReplayArch(std::string_view kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_TERRAIN_ARCH_HELPERS_FIXTURE);std::size_t checked=0;
    for (const auto& row:fixture.rows) { // 선택된 helper의 입력만 실행한다. MP/게임/전투 효과는 없다.
        CHECK(row.size()==7);if (row[1]!=kind) continue;
        const auto input=Split(row[2],',');const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        TerrainContextGlobals globals;RawTerrainContext context(edition,1,127,{4,4,6,5},globals,[](TerrainContextAssertion) {});
        ChunkMap chunks;RawTerrainGrowth growth(context,chunks,globals);std::uint32_t seed=0;bool result=false;
        chunks.Mode()=1;
        for (auto& chunk:chunks.Entries()) chunk.island=0; // Native의 명시 청크0 입력을 기본 noRegion127과 구별한다.
        ArchInput(context,kind=="Cell" ? 0 : Number(input[0]));
        if (kind=="Grow") {
            CHECK(input.size()==5);const auto profile=Number(input[0]);context.ArchipelagoMode()=Number(input[1]);
            const auto region=ArchSigned(input[2]);seed=Number(input[3]);
            std::fill(context.Regions().begin(),context.Regions().end(),static_cast<std::uint8_t>(region));
            growth.GrowRectangle(region,profile==1 ? ChunkRect{4,4,5,6} : ChunkRect{4,4,6,5},ArchSigned(input[4]),seed);
        } else if (kind=="Cell") {
            CHECK(input.size()==4);const auto mask=Number(input[0]);globals.noRegion=ArchSigned(input[2]);
            context.Codes()[70*256+70]=static_cast<std::uint8_t>(Number(input[3]));
            for (std::size_t direction=0;direction<kNeighborCells.size();++direction) { // 명시 mask를 고정 방향의 이웃 입력으로만 변환한다.
                const auto [dx,dy]=kNeighborCells[direction];
                context.Codes()[static_cast<std::size_t>(70+dx)*256+static_cast<std::size_t>(70+dy)]=(mask&(1U<<direction)) ? 'x' : '.';
            }
            result=growth.RemoveSparseCell(70,70,ArchSigned(input[1]));
        } else if (kind=="Sparse") {
            CHECK(input.size()==2);globals.noRegion=ArchSigned(input[1]);growth.RemoveSparse();
        } else {
            CHECK(kind=="Clear" && input.size()==2);globals.noRegion=ArchSigned(input[1]);
            const std::array<ChunkRect,6> bounds{{{4,4,6,5},{3,3,4,5},{6,5,4,4},{4,4,4,5},{4,5,6,4},{0x10000004,4,0x10000005,5}}};
            growth.ClearPlacement(bounds.at(Number(input[0])));
        }
        const auto codes=ArchBytes(row[5]),regions=ArchBytes(row[6]);
        CHECK(std::equal(codes.begin(),codes.end(),context.Codes().begin()));
        CHECK(std::equal(regions.begin(),regions.end(),context.Regions().begin()));
        CHECK(static_cast<std::uint32_t>(result)==Number(row[3]) && seed==Number(row[4]));++checked;
    }
    CHECK(checked==expected);
}
}

// 지역BYTE 감김/군도 비영 DWORD·목표 비양수와 비정사각형 X폭/최종 시드를 대조한다.
TEST_CASE(TerrainArchHelpers_GrowRectangleNative) { ReplayArch("Grow",144); }
// 전체256이웃 mask와 -1/4/12 임계값·비x/현재 noRegion BYTE를 대조한다.
TEST_CASE(TerrainArchHelpers_SparseCellNative) { ReplayArch("Cell",1542); }
// 같은 pass의 즉시 제거와 재순회·내부 경계 제외를 전체 배열에서 대조한다.
TEST_CASE(TerrainArchHelpers_RemoveSparseNative) { ReplayArch("Sparse",24); }
// context bounds 밖의 배치·역전/빈 영역·DWORD shift 감김을 실제 양판본 출력과 대조한다.
TEST_CASE(TerrainArchHelpers_ClearPlacementNative) { ReplayArch("Clear",24); }
