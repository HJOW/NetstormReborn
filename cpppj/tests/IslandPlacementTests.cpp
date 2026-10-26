// 편집기 섬 충돌 검사에 두 보호 PE의 독립 반환·진단을 같은 입력으로 재생한다.
#include "RawSceneSupport.h"
#include "o/RawIslandPlacement.h"
#include <cstring>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 새 배치 검사만 소유하며 기존 IslandBuilder fixture와 출처는 바꾸지 않는다.
// 원문 puzzlePiece 입력에 필요한 A~P/번호0~10 프레임을 제공한다.
RiftTypeFrames PlacementFrames(std::uint32_t profile) {
    std::vector<FrameCode> codes; // 원본 실행기와 같은 합성 프레임 입력이며 출력은 fixture에서 읽는다.
    // 빈 profile 외에는 현재 디코더가 방향과 번호로 검색할 모든 프레임을 넣는다.
    for (std::uint8_t side='A';side<='P';++side)
        // 각 방향의 원본 variation 번호 범위를 보존한다.
        for (std::uint8_t number=0;number<11;++number) if (profile!=2) codes.push_back({side,'P',number,0});
    if (profile==1) std::reverse(codes.begin(),codes.end());
    if (profile!=2) codes.push_back(codes.front());
    if (profile==3) std::erase_if(codes,[](const FrameCode& code) { return code.side=='G'; });
    return RiftTypeFrames(std::move(codes));
}
// 새 모듈이 사용하는 실제 세 진단과 decoder 진단을 native 문자 순서로 기록한다.
char PlacementDiagnostic(RawIslandBuilderAssertion value) {
    switch (value) {
    case RawIslandBuilderAssertion::InvalidBounds:return 'B';
    case RawIslandBuilderAssertion::OddDirection:return 'O';
    case RawIslandBuilderAssertion::MissingFrame:return 'F';
    case RawIslandBuilderAssertion::ChunkRange:return 'R';
    default:throw std::logic_error("CanPlace의 원문 범위 밖 진단");
    }
}
}
// 실제 모양64개·방향4개·경계·충돌·빈/누락 프레임의 양판본 결과를 직접 대조한다.
TEST_CASE(IslandPlacement_ReplaysNativeReadOnlyCollisionAndBounds) {
    const auto fixture=LoadFixture(NETSTORM_ISLAND_PLACEMENT_FIXTURE);std::size_t checked=0;
    // 원본 각 고유 입력을 한 번 재생하며 두 x87 반복 관찰은 중복 검사 수로 세지 않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==4);const auto raw=Split(row[1],',');CHECK(raw.size()==8);std::array<std::uint32_t,8> input{};
        // signed bounds도 DWORD 원문으로 읽어 호스트 부호 변환을 분리한다.
        for (std::size_t i=0;i<input.size();++i) input[i]=Number(raw[i]);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        ChunkMap chunks;std::uint32_t piece=107;std::vector<RiftTypeFrames> frames(edition==OriginalEdition::Patch1078 ? 188U : 171U,RiftTypeFrames({}));
        frames[piece]=PlacementFrames(input[2]);
        // 실제 고정 지도 전체의 임의 기존 record·ordinal을 원본 입력과 동일하게 준비한다.
        for (std::int32_t x=0;x<16;++x)
            // 한 점 또는 전체 점유는 현재 패턴을 고려하지 않은 독립 입력이다.
            for (std::int32_t y=0;y<16;++y) {
                auto& entry=chunks.At(x,y);entry.island=static_cast<std::uint8_t>(input[3]==2 || (input[3]==1 && x==1 && y==1) ? 0 : 127);
                entry.side='.';entry.record.fill(0xa5);entry.ordinal=0x12345678;
            }
        const auto before=chunks;RawIslandBuilderState state{{std::bit_cast<std::int32_t>(input[4]),std::bit_cast<std::int32_t>(input[5]),
            std::bit_cast<std::int32_t>(input[6]),std::bit_cast<std::int32_t>(input[7])},1};
        const auto beforeState=state;const std::array<std::uint8_t,6> record{static_cast<std::uint8_t>(input[0]),1,static_cast<std::uint8_t>(input[1]),0x55,0xaa,0xbb};
        std::string events; // 진단 없는 반환과 원문 중복 보고를 구별한다.
        const auto result=CanPlaceIsland(edition,chunks,frames,piece,state,record,[&](auto diagnostic) { if (!events.empty()) events+=';';events+=PlacementDiagnostic(diagnostic); });
        if (events.empty()) events="-";
        const bool same=result==(Number(row[2])!=0) && events==row[3];CHECK(same);
        if (!same) { std::fprintf(stderr,"CanPlace %s input=%s result=%u/%s diagnostic=%s/%s\n",row[0].c_str(),row[1].c_str(),result ? 1U : 0U,row[2].c_str(),events.c_str(),row[3].c_str());break; }
        CHECK(std::memcmp(chunks.Entries().data(),before.Entries().data(),chunks.Entries().size_bytes())==0);
        CHECK(state.bounds.left==beforeState.bounds.left && state.bounds.top==beforeState.bounds.top && state.bounds.right==beforeState.bounds.right && state.bounds.bottom==beforeState.bounds.bottom && state.owner==beforeState.owner);
        ++checked;
    }
    CHECK(checked==fixture.rows.size());CHECK(checked==4896);
}
// 판정에서 우측/하단을 제외하고 원본에 없는 하한 거부나 청크 쓰기를 추가하지 않는다.
TEST_CASE(IslandPlacement_CurrentMapAndBoundsAreBorrowed) {
    ChunkMap chunks;RawIslandBuilderState state{{0,0,16,16},1};std::uint32_t piece=107,diagnostics=0;
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));frames[piece]=PlacementFrames(0);
    const std::array<std::uint8_t,6> record{3,1,0,0,0,0};const auto diagnostic=[&](auto) { ++diagnostics; };
    CHECK(CanPlaceIsland(OriginalEdition::Patch1078,chunks,frames,piece,state,record,diagnostic));
    chunks.At(0,0).island=0;CHECK(!CanPlaceIsland(OriginalEdition::Patch1078,chunks,frames,piece,state,record,diagnostic));
    chunks.At(0,0).island=127;state.bounds.right=0;CHECK(!CanPlaceIsland(OriginalEdition::Patch1078,chunks,frames,piece,state,record,diagnostic));
    CHECK(diagnostics==0);
}
