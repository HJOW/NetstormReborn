// 전체 MayPlace 독립 원본 관찰을 저장소를 공유하는 실제 C++ 파이프라인과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPlacementPipeline.h"
#include <type_traits>

namespace {
using namespace netstorm::test::rawscene;
// 독립 관찰의 판본별 행 수와 checksum 대상 슬롯 수다.
constexpr std::size_t kRows=144,kSlots=128;
// 내부 훅의 참조를 무효화하는 복사/이동을 컴파일 시점에 차단한다.
static_assert(!std::is_copy_constructible_v<RawCanonPlacementPipeline> && !std::is_move_constructible_v<RawCanonPlacementPipeline>);
// 도구/PE/실제 자산 없이 저장된 전체 함수 관찰을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_CANONMAYPLACE_FIXTURE).rows;return rows;
}
// 반환·표시 배열·지역 배열과 원본 입력 불변성을 대조한다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);SquidHash hash;
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
    std::vector<SquidDisplayShape> shapes(types.size());std::vector<PriestTypeHotspot> hotspots(types.size());
    std::vector<std::uint16_t> islands(65536);std::vector<std::uint8_t> spots(65536,6);std::vector<CanonPlacementIslandRegion> regions(128,{0,1});
    CanonPlacementPipelineState state;state.geometry.patternTypes={107,82,94,157,131,129,140,142};state.relations.islandCount=128;
    SquidPostPopState books;SquidPostPopList additional;ContainedFinderState contained;
    RawCanonPlacementPipeline pipeline(pool,hash,{types,frames,shapes,hotspots,spots,islands,regions},books,additional,contained,state);
    auto bytes=pool.Bytes().first(kSlots*pool.Layout().stride);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
    std::vector<std::uint8_t> hashBytes;std::array<std::uint8_t,131072> fixedBytes{};CHECK(Fixture().size()==3*kRows);std::size_t count=0;
    // 같은 슬롯/타입/목록/공간 자료를 판본별 원본 입력 순서로 재생한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==26);std::fill(raw.begin(),raw.end(),std::uint8_t{});hash.Reset();std::fill(islands.begin(),islands.end(),std::uint16_t{});
        const bool pattern=Number(row[1])!=0;const std::uint32_t placing=pattern ? 157U : 80U;
        types[placing].group=0;types[placing].flags1=Number(row[7]);types[placing].flags2=Number(row[6]);types[placing].footX=types[placing].footY=1;
        types[83].flags1=0x800;types[83].flags2=2;types[83].footX=types[83].footY=1;
        types[84].flags1=0x800;types[84].flags2=6;types[84].footX=types[84].footY=1;types[86].flags1=2;types[86].flags2=0;types[86].footX=types[86].footY=1;
        std::vector<FrameCode> codes;
        // 정상 원본 받침/일반 자산과 동일한 논리 프레임 코드를 넣는다.
        for (std::uint8_t side='A';side<=(pattern ? 'I' : 'A');++side) codes.push_back({side,'P',1,0});
        frames[placing].frames=RiftTypeFrames(std::move(codes));frames[83].frames=RiftTypeFrames(std::vector<FrameCode>{{'P','P',1,0}});
        shapes[placing]={static_cast<int>(frames[placing].frames.Codes().size()),true,std::vector<SquidDisplayFrame>(frames[placing].frames.Codes().size(),{16,11,8,5,0,0})};
        const auto scene=Number(row[8]),owner=Number(row[9]);std::uint16_t sid=50;
        // 원본 장면과 같은 6×6 칸의 raw/고정 섬/현재 해시를 준비한다.
        for (int y=19;y<25;++y) {
            // 현재 표면과 finder의 0단계는 같은 hash 저장소에 쓴다.
            for (int x=18;x<24;++x) {
                auto slot=raw.subspan(sid*pool.Layout().stride,pool.Layout().stride);slot[10]=83;slot[patch ? 34 : 32]=scene==2 ? 2 : 1;slot[patch ? 30 : 28]=7;slot[patch ? 40 : 35]=1;
                Put(slot,8,9,2);Put(slot,14,std::bit_cast<std::uint32_t>(static_cast<float>(x)));Put(slot,18,std::bit_cast<std::uint32_t>(static_cast<float>(y)));
                if (scene!=3 || x!=20 || y!=21) { hash.Bucket(0,static_cast<float>(x),static_cast<float>(y))=sid;islands[y*256+x]=sid; }++sid;
            }
        }
        auto work=raw.subspan(90*pool.Layout().stride,pool.Layout().stride);work[10]=84;work[patch ? 34 : 32]=1;work[patch ? 30 : 28]=7;Put(work,14,std::bit_cast<std::uint32_t>(18.0F));Put(work,18,std::bit_cast<std::uint32_t>(20.0F));
        books.ownerFactories[owner].entries={90};books.ownerFactories[owner].count=scene==1 ? 0 : 1;
        if (scene==4) {
            auto obstacle=raw.subspan(91*pool.Layout().stride,pool.Layout().stride);obstacle[10]=86;Put(obstacle,14,std::bit_cast<std::uint32_t>(20.0F));Put(obstacle,18,std::bit_cast<std::uint32_t>(21.0F));hash.Bucket(1,20,21)=91;
        }
        if (scene==5) { hash.Reset();std::fill(islands.begin(),islands.end(),std::uint16_t{}); }
        state.placement.forcePlacement=Number(row[18]);state.placement.localPlayer=static_cast<std::int32_t>(Number(row[10]));state.placement.blockedRelation=73;
        state.permission.graphReady=Number(row[11]);state.permission.editor=Number(row[12]);state.permission.useAlliances=Number(row[13]);state.permission.alliances.fill(0);
        state.permission.alliances[9+owner]=state.permission.alliances[18+owner]=Number(row[14]);state.relations.bypassGroundPermission=Number(row[15]);state.preview.blocked.fill(0x55);state.terrain.regions.fill(0x55);
        // 패치 전용 목록의 제한 값과 정상 존재 비트도 원본과 같다.
        for (auto& region:regions) region={Number(row[16]),1};
        const CanonPlacementQuery query{placing,Number(row[2]),std::bit_cast<float>(Number(row[4])),std::bit_cast<float>(Number(row[5])),Number(row[3]),owner,Number(row[17])};
        const bool allowed=pipeline.MayPlace(query);hashBytes.clear();
        // 네 단계 배열을 원본 메모리 순서로 직렬화한다.
        for (int level=0;level<4;++level) {
            // 각 머리 WORD의 물리 바이트를 보존한다.
            for (const auto head:hash.Entries(level)) { hashBytes.push_back(static_cast<std::uint8_t>(head));hashBytes.push_back(static_cast<std::uint8_t>(head>>8)); }
        }
        // 별도 고정 섬 지도도 little endian checksum으로 검사한다.
        for (std::size_t i=0;i<islands.size();++i) Put(fixedBytes,2*i,islands[i],2);
        const bool same=allowed==(Number(row[19])!=0) && state.placement.blockedRelation==Number(row[20]) && Hex(state.preview.blocked)==row[21] && Hex(state.terrain.regions)==row[22] &&
            Adler(raw)==Number(row[23]) && Adler(hashBytes)==Number(row[24]) && Adler(fixedBytes)==Number(row[25]);
        CHECK(same);if (!same) { std::printf("전체 MayPlace %s 행 %zu 불일치: 허용 %d/%u, 차단 %u/%u, 미리보기 %d, 지역 %d\n",std::string(edition).c_str(),count,allowed,Number(row[19]),state.placement.blockedRelation,Number(row[20]),Hex(state.preview.blocked)==row[21],Hex(state.terrain.regions)==row[22]);break; }++count;
    }
    CHECK(count==kRows);
}
}
// 세 판본 전체 진입/실제 helper/정상 반환의 독립 관찰을 비교한다.
TEST_CASE(canon_mayplace_patch_complete_x86) { Replay("originals"); }
// CD의 미리보기/지역/반환 차이를 전체 호출 결과로 검사한다.
TEST_CASE(canon_mayplace_cd_complete_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 전체 함수 관찰도 따로 재생한다.
TEST_CASE(canon_mayplace_1037_complete_x86) { Replay("original1037"); }
