// 실제 일반 지형을 문자/지역 배열에 생성하며 게임 객체 생성은 뒤의 RawTerrainStamp에 맡긴다.
#include "o/RawTerrainGrowth.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 패치 로컬 난수의 청크 방문 번호 증가량과 성장 연속 실패 한계다.
constexpr std::uint32_t kRawChunkSeedStep=0x10e3;
constexpr std::int32_t kRawGrowthFailures=1000;
}
// 실제 조회 자료의 수명만 빌리며 constructor는 지도/난수를 변경하지 않는다.
RawTerrainGrowth::RawTerrainGrowth(RawTerrainContext& context,const ChunkMap& chunks,const TerrainContextGlobals& globals,
    TerrainGrowthDiagnostics diagnostics)
    :context_(context),chunks_(chunks),globals_(globals),diagnostics_(std::move(diagnostics)) {}
// 일반 성장과 다른 첫 시드 경계에서만 원문 전체 진단 인자를 전달한다.
void RawTerrainGrowth::Assert(std::string_view expression,std::string_view file,std::uint32_t line) const {
    if (!diagnostics_.assertion) throw std::logic_error("초기 지형 시드 진단 장치 누락");
    diagnostics_.assertion(expression,file,line);
}
// 첫 진단의 효과로 mode/청크가 바뀌어도 뒤의 검사와 getter는 현재 저장소를 다시 읽는다.
const ChunkEntry& RawTerrainGrowth::SeedChunkAt(std::int32_t x,std::int32_t y) const {
    if (context_.Edition()==OriginalEdition::Patch1078) {
        if (!chunks_.Mode()) Assert("isValid()","\\Ns\\O\\Chunkmap.cpp",0xb7);
        if (x<0 || y<0 || x>=chunks_.XLength() || y>=chunks_.YLength())
            Assert("cc.x>=0 && cc.y>=0 && cc.x<xLen && cc.y<yLen","\\Ns\\O\\Chunkmap.cpp",0xb8);
    }
    return chunks_.At(x,y);
}
// CD만 noRegion을 진입 때 캐시한다. 패치는 각 getter의 효과 뒤 현재 전체 DWORD와 비교한다.
std::uint32_t RawTerrainGrowth::FirstChunkSeed() const {
    const auto cdNoRegion=globals_.noRegion;
    // 원본 청크 범위는 Y행이 외부이며, 진단 복귀 뒤 right/bottom을 다시 읽는다.
    for (auto y=context_.Chunks().top;y<context_.Chunks().bottom;++y)
        // 현재 행의 left에서 시작하고 첫 비어 있지 않은 raw 청크만 반환한다.
        for (auto x=context_.Chunks().left;x<context_.Chunks().right;++x) {
            const auto& chunk=SeedChunkAt(x,y);
            const auto noRegion=context_.Edition()==OriginalEdition::Patch1078 ? globals_.noRegion : cdNoRegion;
            if (std::bit_cast<std::int8_t>(chunk.island)!=noRegion) return chunk.record[3];
        }
    if (context_.Edition()==OriginalEdition::Patch1078 && diagnostics_.checking)
        Assert("0","\\Ns\\O\\Terrainbuilder.cpp",0x25f);
    return 0x2333;
}
// limit<=0에서 시드도 그대로인 원문 range helper를 기존 양수 limit helper와 구별한다.
std::int32_t RawTerrainGrowth::NextRange(std::uint32_t& seed,std::int32_t low,std::int32_t high) {
    const auto width=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(high)-static_cast<std::uint32_t>(low));
    if (width<1) return low;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(low)+static_cast<std::uint32_t>(Next(seed,static_cast<std::uint32_t>(width))));
}
// 일반 RunNormal에는 호출을 추가하지 않는다. 실제 군도 상위가 같은 context에서 호출하는 별도 몸체다.
void RawTerrainGrowth::Scatter(std::int32_t count) {
    auto seed=FirstChunkSeed()*0x5dU;auto remaining=static_cast<std::uint32_t>(count);
    // 거부된 칸은 count를 줄이지 않는다. 원문에 없는 재시도 제한이나 중복 제거는 넣지 않는다.
    while (remaining!=0) {
        const auto& bounds=context_.Map().bounds;
        const auto x=NextRange(seed,std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(bounds[0])+2U),
            std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(bounds[2])-2U));
        const auto y=NextRange(seed,std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(bounds[1])+2U),
            std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(bounds[3])-2U));
        const auto coordinate=[this](std::int32_t value) { // 패치 float32→ftol 하위DWORD와 CD 정수의 0방향 나눗셈이다.
            if (context_.Edition()==OriginalEdition::Cd1072) return value/16;
            const auto rounded=static_cast<std::int64_t>(static_cast<float>(value));
            return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(rounded))/16;
        };
        const auto chunkX=coordinate(x),chunkY=coordinate(y);
        const auto hasIsland=std::bit_cast<std::int8_t>(SeedChunkAt(chunkX,chunkY).island)!=globals_.noRegion;
        if (hasIsland && SeedChunkAt(chunkX,chunkY).island!=0) continue;
        const auto index=Index(x,y);context_.Codes()[index]='x';context_.Regions()[index]=0;--remaining;
    }
}
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
// 같은 현재 군도 DWORD가 비영일 때만 청크 지역 조회를 건너뛴다.
bool RawTerrainGrowth::Put(std::int32_t x,std::int32_t y,std::uint8_t region) {
    const auto& bounds=context_.Map().bounds;
    if (x<bounds[0] || y<bounds[1] || x>bounds[2] || y>bounds[3]) return false;
    const auto index=Index(x,y);
    if (context_.Codes()[index]=='x') return false;
    if (!context_.ArchipelagoMode() && ChunkRegion(x/16,y/16)!=std::bit_cast<std::int8_t>(region)) return false;
    context_.Regions()[index]=region;context_.Codes()[index]='x';return true;
}
// 실제 두 판본의 같은 caller 로컬 난수를 사용하며 게임 전역 난수를 소비하지 않는다.
std::int32_t RawTerrainGrowth::Next(std::uint32_t& seed,std::uint32_t limit) {
    return TerrainBuilder::Next(seed,static_cast<int>(limit));
}
// 원본은 X/Y 난수와 limit1 호출을 모두 소비한 뒤 현재x/지역을 읽는다.
void RawTerrainGrowth::Grow(std::int32_t island,std::int32_t x,std::int32_t y,std::int32_t target,std::uint32_t& seed) {
    GrowRectangle(island,{x,y,x+1,y+1},target,seed);
}
// 원문 bottom 인자는 난수폭 계산에 사용하지 않는다. X폭을 Y높이로 바꾸지 않는다.
void RawTerrainGrowth::GrowRectangle(std::int32_t island,ChunkRect bounds,std::int32_t target,std::uint32_t& seed) {
    const auto cell=[](std::int32_t value) { // 원본 signed DWORD 왼쪽 shift의 하위32비트를 보존한다.
        return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)<<4);
    };
    const auto width=std::bit_cast<std::int32_t>((static_cast<std::uint32_t>(bounds.right)-static_cast<std::uint32_t>(bounds.left))*16U-1U);
    const auto add=[](std::int32_t left,std::int32_t right) { // 좌표 덧셈도 호스트 signed overflow 없이 원본 감김을 보존한다.
        return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(left)+static_cast<std::uint32_t>(right));
    };
    auto failures=kRawGrowthFailures;
    // 성공이 없는 연속1000회 또는 목표 추가량 소진에서 종료한다.
    while (target>0 && failures!=0) {
        --failures;const auto cx=add(cell(bounds.left),NextRange(seed,0,width)),cy=add(cell(bounds.top),NextRange(seed,0,width));Next(seed,1);
        const auto index=Index(cx,cy);
        if (context_.Codes()[index]!='x' || (!context_.ArchipelagoMode() &&
            context_.Regions()[index]!=static_cast<std::uint8_t>(island))) continue;
        const auto before=target;
        // 현재 원본3×3은 Y행을 먼저 순회한다.
        for (auto ny=cy-1;ny<=cy+1;++ny)
            // 각 행의 X를 처음부터 다시 읽는다.
            for (auto nx=cx-1;nx<=cx+1;++nx)
                if (Put(nx,ny,static_cast<std::uint8_t>(island))) --target;
        if (target<before) failures=kRawGrowthFailures;
    }
}
// 현재 칸이 x일 때만 여덟 이웃을 읽으며 지역BYTE의 일치 여부는 점수 조건이 아니다.
bool RawTerrainGrowth::RemoveSparseCell(std::int32_t x,std::int32_t y,std::int32_t threshold) {
    const auto index=Index(x,y);if (context_.Codes()[index]!='x') return false;
    std::int32_t weight=0;
    // 북부터 시계방향으로 읽으며 짝수 방향의 직교 이웃에만 추가 가중치를 준다.
    for (std::size_t direction=0;direction<kNeighborCells.size();++direction) {
        const auto [dx,dy]=kNeighborCells[direction];
        if (context_.Codes()[Index(x+dx,y+dy)]=='x') weight+=(direction&1U) ? 1 : 2;
    }
    if (weight>threshold) return false;
    context_.Regions()[index]=static_cast<std::uint8_t>(globals_.noRegion);context_.Codes()[index]='.';return true;
}
// 제거된 앞 칸을 같은 pass의 뒤 칸이 즉시 읽는다. 원문에 없는 동시 갱신 버퍼를 만들지 않는다.
void RawTerrainGrowth::RemoveSparse() {
    bool changed;
    do {
        changed=false;
        // 원문 내부 Y행 끝은 bottom-1 직전이다.
        for (auto y=context_.Map().bounds[1]+1;y<context_.Map().bounds[3]-1;++y)
            // 현재 right를 각 칸 뒤 다시 읽고 임계값4로 지운다.
            for (auto x=context_.Map().bounds[0]+1;x<context_.Map().bounds[2]-1;++x)
                changed=RemoveSparseCell(x,y,4) || changed;
    } while (changed);
}
// 첫 좌표는 float32 저장 뒤 ftol 하위DWORD이며 반복 비교는 정확한 정수와 float32 끝값 사이에서 한다.
void RawTerrainGrowth::ClearPlacement(ChunkRect bounds) {
    const auto cell=[](std::int32_t value) { // 원본32비트 shift 후 float32 저장 지점을 보존한다.
        return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)<<4));
    };
    const auto first=[](float value) { // 이 범위의 float32는 int64 안에 들어가며 ftol의 하위DWORD만 사용한다.
        return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
    };
    const auto left=cell(bounds.left),top=cell(bounds.top),right=cell(bounds.right),bottom=cell(bounds.bottom);
    // Y행을 먼저 진행하며 원문 context의 현재 bounds로 잘라내지 않는다.
    for (auto y=first(top);static_cast<double>(y)<bottom;++y)
        // 각 행의 시작X는 변환된 같은 left이며 오른쪽 경계는 포함하지 않는다.
        for (auto x=first(left);static_cast<double>(x)<right;++x) {
            const auto index=Index(x,y);context_.Regions()[index]=static_cast<std::uint8_t>(globals_.noRegion);
            context_.Codes()[index]='.';
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
