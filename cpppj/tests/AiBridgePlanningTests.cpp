// 실제 네 원본 하위의 두 정밀도 결과·전체 본체·난수와 현재 graph 조립을 대조한다.
#include "o/RawAiBridgePlanning.h"
#include "o/RawPlayerPlacementAnchor.h"
#include "RawSceneSupport.h"
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace {
// 현재 검사 파일 옆의 새 독립 원본 입력/출력만 읽는다.
FixtureData PlanningFixture(const char* name) { const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures"/(std::string("ai-bridge-planning-")+name+"-x86.tsv");return LoadFixture(path.string().c_str()); }
// 실제8BYTE 결과의 부호0/NaN도 원문 비트로 비교한다.
std::string PointHex(RawPathPoint point) { std::array<std::uint8_t,8> raw{};Put(raw,0,std::bit_cast<std::uint32_t>(point.x));Put(raw,4,std::bit_cast<std::uint32_t>(point.y));return Hex(raw); }
// 결과를 계산하지 않고 원본 caller와 동일한 현재 월드 입력만 조립한다.
struct PlanningScene {
    RawAiBrainMemory memory{0x55000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa5}); }}; //전체 실제본체의 유일 소유자다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //현재 살아 있는 본체다.
    SidPool pool{OriginalEdition::Patch1078,32768,false}; //source50/base51/target60을 가진 실제50BYTE 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //실제 현재TYPE flags입력이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); //source의 실제현재코드다.
    std::vector<std::uint16_t> surfaces=std::vector<std::uint16_t>(65536); //현재hash0과거의올림graph조회가공유하는입력이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); //현재실제spotBYTE입력이다.
    std::uint32_t bridge{82},fencemark{90},windArcher{140}; //같은현재전역이다.
    GameRandom random; //실제공용난수의입력상태다.
    double now{}; //현재QWORD게임시각이다.
    SquidPostPopState bookkeeping; //실제graph모듈이빌리는장부다.후보를합성하지않는다.
    SquidPostPopList additional; //graph조회에서사용하지않는현재목록이다.
    ContainedFinderState finder; //같은실제내용물권한입력이다.
    PlayerPlacementAnchorState anchorState{1,254,0,90,140}; //실제활성graph/무효값/특수TYPE다.
    // 실제 슬롯을 확보하고 caller 본체의 두 현재 필드만 설치한다.
    PlanningScene() { for (int index=5;index<=60;++index) CHECK(pool.Allocate(2).value==index);Put(memory.Bytes(token),0x298,7);Put(memory.Bytes(token),0x260,2); } //할당순서는원본격리입력과같은번호다.
    // 같은 현재 원문·지도·코드의 실제 모듈을 만든다.
    RawAiBridgeCandidates Candidates() { return {memory,pool,types,frames,surfaces,spots,{bridge,fencemark,windArcher}}; }
    // 같은 현재 graph 몸체이며 별도 기대값을 반환하지 않는다.
    std::uint32_t Graph(Sid sid) { RawPlayerPlacementAnchor anchor(pool,types,surfaces,bookkeeping,additional,finder,anchorState);return anchor.GraphFor(sid.value); }
    // 입력 지도 모드와TYPE/슬롯만 원본 도구와 같은 방식으로 구성한다.
    void World(std::uint32_t mode,std::uint32_t kind,std::uint8_t side) {
        for (std::uint32_t y=0;y<256;++y) { //flat 행 순서로 caller 입력만 설치한다.
            for (std::uint32_t x=0;x<256;++x) { const auto at=y*256+x;surfaces[at]=mode && (mode!=3 || (x+3*y)%5!=0) ? 50 : 0;spots[at]=static_cast<std::uint8_t>(mode==2 ? 0x80 : mode==3 && (x+3*y)%5==0 ? 0x40 : 0); } //비대칭 입력으로X/Y전도와전체BYTE검사를검증한다.
        }
        auto source=pool.AllocatedBytes(Sid{50});std::fill(source.begin(),source.end(),std::uint8_t{});source[10]=static_cast<std::uint8_t>(kind);source[30]=7;source[34]=2;types[kind].flags1=0x800;frames[kind]=RiftTypeFrames({FrameCode{side,65,0,0}});
    }
};
}
// 모든A~P/다리·육지/지도·난수와경계에서실제XY/공용난수/전체본문을대조한다.
TEST_CASE(AiBridgePlanning_Native261RandomStartsAndState) {
    std::size_t count{};
    for (const auto& row:PlanningFixture("random").rows) { //독립caller입력만제품에공급한다.
        CHECK(row.size()==12);if (row.size()!=12) return;PlanningScene scene;scene.World(Number(row[0]),Number(row[1]),static_cast<std::uint8_t>(Number(row[2])));Put(scene.memory.Bytes(scene.token),0x298,Number(row[7]));Put(scene.memory.Bytes(scene.token),0x260,Number(row[8]));scene.random.SetState(Number(row[9]));const auto before=Hex(scene.memory.Bytes(scene.token));auto candidates=scene.Candidates();RawAiBridgePlanning planning(candidates,scene.random,scene.now,{});
        const AiBuildSearchBounds bounds{std::bit_cast<float>(Number(row[3])),std::bit_cast<float>(Number(row[4])),std::bit_cast<float>(Number(row[5])),std::bit_cast<float>(Number(row[6]))};const auto point=planning.RandomStart(scene.token,bounds);const auto same=PointHex(point)==row[10] && scene.random.State()==Number(row[11]) && Hex(scene.memory.Bytes(scene.token))==before;
        if (!same) std::printf("AI 다리 무작위 원본 차이 행%zu XY%s/%s 난수%u/%s\n",count,PointHex(point).c_str(),row[10].c_str(),scene.random.State(),row[11].c_str());CHECK(same);++count;
    }
    CHECK(count==261);
}
// 실제00462bc0에대응하는현재anchor도조립해12칸과원본전체DWORD비교를검증한다.
TEST_CASE(AiBridgePlanning_Native85ConnectedEdgesWithActualGraph) {
    std::size_t count{};
    for (const auto& row:PlanningFixture("connected").rows) { //슬롯/지도입력만구성하며연결기대를만들지않는다.
        CHECK(row.size()==9);if (row.size()!=9) return;PlanningScene scene;scene.World(0,Number(row[1]),static_cast<std::uint8_t>(Number(row[2])));Put(scene.memory.Bytes(scene.token),0x298,Number(row[3]));Put(scene.memory.Bytes(scene.token),0x260,Number(row[4]));scene.bridge=Number(row[5]);const auto xbits=Number(row[6]),ybits=Number(row[7]);auto target=scene.pool.AllocatedBytes(Sid{60});std::fill(target.begin(),target.end(),std::uint8_t{});target[10]=98;Put(target,14,xbits);Put(target,18,ybits);
        scene.types[97].flags1=0x800;auto base=scene.pool.AllocatedBytes(Sid{51});std::fill(base.begin(),base.end(),std::uint8_t{});base[10]=97;base[30]=7;base[34]=2;const auto xf=std::bit_cast<float>(xbits),yf=std::bit_cast<float>(ybits);const auto x=static_cast<int>(xf),y=static_cast<int>(yf),gx=static_cast<int>(static_cast<double>(xf)+static_cast<double>(0.9999f)),gy=static_cast<int>(static_cast<double>(yf)+static_cast<double>(0.9999f));
        if (gx>=0 && gx<256 && gy>=0 && gy<256) scene.surfaces[static_cast<std::size_t>(gy)*256+gx]=51;const auto edge=std::bit_cast<std::int32_t>(Number(row[0]));
        const std::array<std::array<int,2>,12> edges{{{-2,-3},{-2,1},{-1,-3},{-1,1},{0,-3},{0,1},{-3,-2},{1,-2},{-3,-1},{1,-1},{-3,0},{1,0}}}; //실제caller의단일가장자리배치좌표다.
        if (edge>=0) { const auto cx=x+edges[static_cast<std::size_t>(edge)][0],cy=y+edges[static_cast<std::size_t>(edge)][1];if (cx>=0 && cx<256 && cy>=0 && cy<256) scene.surfaces[static_cast<std::size_t>(cy)*256+cx]=50; }
        const auto before=Hex(scene.memory.Bytes(scene.token));auto candidates=scene.Candidates();RawAiBridgePlanning planning(candidates,scene.random,scene.now,[&](Sid sid) { return scene.Graph(sid); });const auto result=planning.Connected(scene.token,Sid{60});const auto same=result==(Number(row[8])!=0) && Hex(scene.memory.Bytes(scene.token))==before;if (!same) std::printf("AI 간헐천 연결 원본 차이 행%zu edge%d 코드%s\n",count,edge,row[2].c_str());CHECK(same);++count;
    }
    CHECK(count==85);
}
// 단일칸·반전사각형·flat행경계의inclusive반환과본문보존을확인한다.
TEST_CASE(AiBridgePlanning_Native24InclusiveRectangles) {
    std::size_t count{};
    for (const auto& row:PlanningFixture("rectangle").rows) { //실제점유입력만제품에공급한다.
        CHECK(row.size()==6);if (row.size()!=6) return;PlanningScene scene;scene.World(Number(row[4]),82,'J');auto candidates=scene.Candidates();RawAiBridgePlanning planning(candidates,scene.random,scene.now,{});const auto before=Hex(scene.memory.Bytes(scene.token));CHECK(planning.RectangleFree(std::bit_cast<std::int32_t>(Number(row[0])),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2])),std::bit_cast<std::int32_t>(Number(row[3])))==(Number(row[5])!=0));CHECK(Hex(scene.memory.Bytes(scene.token))==before);++count;
    }
    CHECK(count==24);
}
// 현재QWORD시각과전체countDWORD로원본5d0만쓴전체본문을대조한다.
TEST_CASE(AiBridgePlanning_Native28RetryWholeBodies) {
    std::size_t count{};
    for (const auto& row:PlanningFixture("delay").rows) { //40/120초와큰정밀도/무한시각에서도독립원문만기대로쓴다.
        CHECK(row.size()==3);if (row.size()!=3) return;PlanningScene scene;Put(scene.memory.Bytes(scene.token),0x350,Number(row[0]));scene.now=std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(row[1])));auto candidates=scene.Candidates();RawAiBridgePlanning planning(candidates,scene.random,scene.now,{});planning.DelayRetry(scene.token);CHECK(Hex(scene.memory.Bytes(scene.token))==row[2]);++count;
    }
    CHECK(count==28);
}
// graph하위가현재owner/좌표를바꾸면변경후필드로연결을판정하고해제토큰은거부한다.
TEST_CASE(AiBridgePlanning_ConnectedReadsCurrentStateAfterGraph) {
    PlanningScene scene;scene.World(0,82,'J');auto raw=scene.pool.AllocatedBytes(Sid{60});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=98;Put(raw,14,std::bit_cast<std::uint32_t>(1.0f));Put(raw,18,std::bit_cast<std::uint32_t>(1.0f));scene.surfaces[60*256+48]=50;auto candidates=scene.Candidates();std::size_t calls{};
    RawAiBridgePlanning planning(candidates,scene.random,scene.now,[&](Sid sid) { CHECK(sid.value==60);++calls;Put(raw,14,std::bit_cast<std::uint32_t>(50.75f));Put(raw,18,std::bit_cast<std::uint32_t>(63.25f));Put(scene.memory.Bytes(scene.token),0x298,9);return 9U; });CHECK(planning.Connected(scene.token,Sid{60}) && calls==1);scene.memory.Free(scene.token);bool threw{};try { planning.DelayRetry(scene.token); }catch (const std::exception&) { threw=true; }CHECK(threw);
}
