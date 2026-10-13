// 실제 일반 지형을 문자/지역 배열에 생성하며 게임 객체 생성은 뒤의 RawTerrainStamp에 맡긴다.
#include "o/RawTerrainGrowth.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 패치 로컬 난수의 청크 방문 번호 증가량과 성장 연속 실패 한계다.
constexpr std::uint32_t kRawChunkSeedStep=0x10e3;
constexpr std::int32_t kRawGrowthFailures=1000;
}
// 실제 조회 자료의 수명만 빌리며 constructor는 지도/난수를 변경하지 않는다.
RawTerrainGrowth::RawTerrainGrowth(RawTerrainContext& context,const ChunkMap& chunks,const TerrainContextGlobals& globals)
    :context_(context),chunks_(chunks),globals_(globals) {}
// 현재 배열 바깥의 원본 손상 접근을 정상 칸으로 바꾸지 않는다.
std::size_t RawTerrainGrowth::Index(std::int32_t x,std::int32_t y) {
    if (x<0 || y<0 || x>=256 || y>=256) throw std::out_of_range("초기 성장 지도 범위 오류");
    return static_cast<std::size_t>(x)*256+static_cast<std::size_t>(y);
}
// 기존 청크 At의 원본 끝 clamp 규칙과 signed BYTE를 사용한다.
std::int32_t RawTerrainGrowth::ChunkRegion(std::int32_t x,std::int32_t y) const {
    return std::bit_cast<std::int8_t>(chunks_.At(x,y).island);
}
// 호출자가 지정한 사각형을 반열린 순서로 채우며 이미x인 칸의 지역은 유지한다.
std::int32_t RawTerrainGrowth::Fill(std::int32_t left,std::int32_t top,std::int32_t right,std::int32_t bottom,std::uint8_t region) {
    std::int32_t added=0;
    // 원본은 X열을 먼저 진행한다.
    for (auto x=left;x<right;++x)
        // 각 X열의 Y를 현재 끝까지 진행한다.
        for (auto y=top;y<bottom;++y) {
            const auto index=Index(x,y);
            if (context_.Codes()[index]=='x') continue;
            context_.Codes()[index]='x';context_.Regions()[index]=region;++added;
        }
    return added;
}
// 일반 분기는 원본 archipelago flag0이므로 현재 청크 지역이 요청과 같아야 한다.
bool RawTerrainGrowth::Put(std::int32_t x,std::int32_t y,std::uint8_t region) {
    const auto& bounds=context_.Map().bounds;
    if (x<bounds[0] || y<bounds[1] || x>bounds[2] || y>bounds[3]) return false;
    const auto index=Index(x,y);
    if (context_.Codes()[index]=='x' || ChunkRegion(x/16,y/16)!=std::bit_cast<std::int8_t>(region)) return false;
    context_.Regions()[index]=region;context_.Codes()[index]='x';return true;
}
// 실제 두 판본의 같은 caller 로컬 난수를 사용하며 게임 전역 난수를 소비하지 않는다.
std::int32_t RawTerrainGrowth::Next(std::uint32_t& seed,std::uint32_t limit) {
    return TerrainBuilder::Next(seed,static_cast<int>(limit));
}
// 원본은 X/Y 난수와 limit1 호출을 모두 소비한 뒤 현재x/지역을 읽는다.
void RawTerrainGrowth::Grow(std::int32_t island,std::int32_t x,std::int32_t y,std::int32_t target,std::uint32_t& seed) {
    auto failures=kRawGrowthFailures;
    // 성공이 없는 연속1000회 또는 목표 추가량 소진에서 종료한다.
    while (target>0 && failures!=0) {
        --failures;const auto cx=x*16+Next(seed,15),cy=y*16+Next(seed,15);Next(seed,1);
        const auto index=Index(cx,cy);
        if (context_.Codes()[index]!='x' || std::bit_cast<std::int8_t>(context_.Regions()[index])!=island) continue;
        const auto before=target;
        // 현재 원본3×3은 Y행을 먼저 순회한다.
        for (auto ny=cy-1;ny<=cy+1;++ny)
            // 각 행의 X를 처음부터 다시 읽는다.
            for (auto nx=cx-1;nx<=cx+1;++nx)
                if (Put(nx,ny,static_cast<std::uint8_t>(island))) --target;
        if (target<before) failures=kRawGrowthFailures;
    }
}
// 방향0부터12까지 같은8이웃을 순환하며 변경된 앞선 칸도 현재 배열에서 읽는다.
void RawTerrainGrowth::FixCell(std::int32_t x,std::int32_t y) {
    std::int32_t run=0,region=globals_.noRegion;
    // 북부터13번의 원본 조회 순서를 유지한다.
    for (std::uint32_t direction=0;direction<=12;++direction) {
        const auto [dx,dy]=kNeighborCells[direction&7];const auto index=Index(x+dx,y+dy);
        if (context_.Codes()[index]!='x') { run=0;continue; }
        const auto current=static_cast<std::int32_t>(std::bit_cast<std::int8_t>(context_.Regions()[index]));
        if (!run) region=current;
        if (region!=current) { run=0;continue; }
        if (++run>4 && (direction&1)) {
            const auto target=Index(x,y);context_.Regions()[target]=static_cast<std::uint8_t>(region);
            context_.Codes()[target]='x';return;
        }
    }
}
// 원본의 bottom은 포함 직전까지, right는 한 칸 더 제외한다.
void RawTerrainGrowth::FixGaps(std::int32_t passes) {
    // 각 pass의 현재 문자 변경을 즉시 다음 칸에 반영한다.
    for (std::int32_t pass=0;pass<passes;++pass)
        // Y는 현재 top+1부터 bottom직전까지다.
        for (auto y=context_.Map().bounds[1]+1;y<context_.Map().bounds[3];++y)
            // X는 현재 left+1부터 right-1직전까지다.
            for (auto x=context_.Map().bounds[0]+1;x<context_.Map().bounds[2]-1;++x) {
                if (context_.Codes()[Index(x,y)]!='.') continue;
                if (context_.Codes()[Index(x-1,y)]=='x' || context_.Codes()[Index(x+1,y)]=='x' ||
                    context_.Codes()[Index(x,y-1)]=='x' || context_.Codes()[Index(x,y+1)]=='x') FixCell(x,y);
            }
}
// 일반 분기의 로컬counts/targets만 임시이며 결과 code/region은 실제context 배열이다.
void RawTerrainGrowth::RunNormal() {
    std::array<std::int32_t,256> targets{};std::array<std::uint32_t,100> visits{};
    // 전체 통로를 Y청크 순서로 먼저 구성한다.
    for (auto y=context_.Chunks().top;y<context_.Chunks().bottom;++y)
        // 각 Y청크 행의 X청크를 현재 bounds에서 다시 읽는다.
        for (auto x=context_.Chunks().left;x<context_.Chunks().right;++x) {
            const auto& chunk=chunks_.At(x,y);const auto island=ChunkRegion(x,y);
            if (island==globals_.noRegion || (context_.Region()!=globals_.noRegion && island!=context_.Region())) continue;
            if (island<0 || island>=100) throw std::out_of_range("초기 성장 섬 방문 배열 범위 오류");
            auto seed=visits[island]*kRawChunkSeedStep+chunk.record[3];++visits[island];
            const auto left=x*16,top=y*16,cx=left+4+(chunk.record[3]&7),cy=top+4+((9999-chunk.record[3])&7);
            const auto mask=TerrainBuilder::Connection(chunk.side);
            auto added=Fill(cx-1,cy-1,cx+3,cy+3,static_cast<std::uint8_t>(island));
            added+=Fill(mask&8 ? left : cx,mask&1 ? top : cy,mask&2 ? left+16 : cx+2,
                mask&4 ? top+16 : cy+2,static_cast<std::uint8_t>(island));
            if (visits[island]==2) added+=Fill(left+3,top+3,left+13,top+13,static_cast<std::uint8_t>(island));
            const auto density=std::max(20,Next(seed,30)+50-added*100/256);
            targets[static_cast<std::size_t>(x)*16+y]=density*256/100;
        }
    // 통로가 모두 만들어진 뒤 모든 비noRegion 청크의 실제 성장helper를 호출한다.
    for (auto y=context_.Chunks().top;y<context_.Chunks().bottom;++y)
        // 현재 청크를 같은 X순서로 읽고 방문 수 증가를 원본처럼 유지한다.
        for (auto x=context_.Chunks().left;x<context_.Chunks().right;++x) {
            const auto& chunk=chunks_.At(x,y);const auto island=ChunkRegion(x,y);
            if (island==globals_.noRegion) continue;
            if (island<0 || island>=100) throw std::out_of_range("초기 성장 현재 섬 배열 범위 오류");
            auto seed=visits[island]*kRawChunkSeedStep+chunk.record[3];++visits[island];
            Grow(island,x,y,targets[static_cast<std::size_t>(x)*16+y],seed);
        }
    FixGaps(2);
}
}
