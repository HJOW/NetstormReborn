// 실제 후보 준비 함수가 동결한 전체 본체와 입력 지도 보존을 대조한다.
#include "o/RawAiBridgeCandidates.h"
#include "o/RawAiBridgePath.h"
#include "RawSceneSupport.h"
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace {
// 원본에 공급한 지도/TYPE/슬롯 입력만 만들고 후보 결과는 제품 몸체가 계산한다.
struct CandidateScene {
    RawAiBrainMemory memory{0x55000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //실제 Brain 소유 저장소다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //현재 본체 핸들이다.
    SidPool pool{OriginalEdition::Patch1078,32768,false}; //명시source50의 실제50BYTE 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //원본 helper가 읽은surface flag 입력이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); //실제 현재 코드 입력이다.
    std::vector<std::uint16_t> surfaces=std::vector<std::uint16_t>(65536); //명시hash0 입력이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); //명시spot 입력이다.
    std::uint32_t bridge{82},fencemark{90},windArcher{140}; //같은 현재 원본 TYPE 전역이다.
    // 실제 슬롯을 확보하고 caller 입력을 각 필드에만 적용한다.
    explicit CandidateScene(const std::vector<std::string>& row) {
        for (std::uint16_t number=5;number<=50;++number) CHECK(pool.Allocate(2).value==number); //원본 격리 입력과 같은source50을 확보한다.
        const auto mode=Number(row[0]),kind=Number(row[1]);const auto x=std::bit_cast<std::int32_t>(Number(row[3])),y=std::bit_cast<std::int32_t>(Number(row[4])); //지도 생성에 쓰는 caller의 현재 시작 칸이다.
        std::fill(memory.Bytes(token).begin(),memory.Bytes(token).end(),static_cast<std::uint8_t>(Number(row[12])));
        Put(memory.Bytes(token),0x5f8,Number(row[9]));Put(memory.Bytes(token),0x5fc,Number(row[10]));Put(memory.Bytes(token),0x690,Number(row[6]));Put(memory.Bytes(token),0x694,Number(row[7]));Put(memory.Bytes(token),0x698,Number(row[5]));Put(memory.Bytes(token),0x69c,Number(row[8]));Put(memory.Bytes(token),0x6a0,Number(row[3]));Put(memory.Bytes(token),0x6a4,Number(row[4]));
        for (int cy=0;cy<256;++cy) { //원본 관측 도구와 같은 입력 지도만 행·열 순서로 구성한다.
            for (int cx=0;cx<256;++cx) { //각 칸의 지형/점유 입력만 채운다.
                bool present=mode!=0 && (mode!=4 || (cx==x && cy==y));if (mode==3) present=(cx+cy)%3!=0;const auto at=static_cast<std::size_t>(cy)*256+static_cast<std::size_t>(cx);
                if (present) surfaces[at]=50;if (mode==2) spots[at]=1;else if (mode==3) spots[at]=(cx+cy)%3==0 ? 0x80 : 0;else if (mode==5) spots[at]=((cx==x && cy==y-1) || (cx==x+1 && cy==y)) ? 0x40 : 0;
            }
        }
        auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(kind);raw[30]=7;raw[34]=2;types[kind].flags1=0x800;frames[kind]=RiftTypeFrames({FrameCode{static_cast<std::uint8_t>(Number(row[2])),'A',0,0}});
    }
    // 같은 현재 월드/본체를 참조하는 실제 준비 모듈이다.
    RawAiBridgeCandidates Candidates() { return {memory,pool,types,frames,surfaces,spots,{bridge,fencemark,windArcher}}; }
};
}
// 원본210장면/220반환의24칸 예산·최대6개·연결비트·재개·전체 바이트를 대조한다.
TEST_CASE(AiBridgeCandidates_Native210FullBodies) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/ai-bridge-candidates-x86.tsv").string();std::size_t count{},returns{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { //독립 caller 입력만 설치하고 원문 결과를 재계산하지 않는다.
        CHECK(row.size()==16);if (row.size()!=16) return;CandidateScene scene(row);auto candidates=scene.Candidates();bool result{};
        for (std::uint32_t repeat=0;repeat<Number(row[11]);++repeat) { result=candidates.Prepare(scene.token);++returns; } //같은 상태를 실제 다음 호출에 이어 재개한다.
        const auto same=result==(Number(row[13])!=0) && Hex(scene.memory.Bytes(scene.token))==row[14];if (!same) std::printf("AI 다리 후보 전체 본체 차이 행%zu 모드%s TYPE%s 코드%s\n",count,row[0].c_str(),row[1].c_str(),row[2].c_str());CHECK(same);++count;
    }
    CHECK(count==210 && returns==220);
}
// 실제 초기화와 준비를 Step에 연결하면 예산 중단 동안 상태2를 유지한 뒤에만 검색3으로 진행한다.
TEST_CASE(AiBridgeCandidates_ComposesInitializationAndBudgetedStep) {
    CandidateScene scene({"0","97","65","50","50","2","0","0","1","7","2","1","165","0","","0"});ContainedFinderState finder;double now{};std::int32_t limit{256};std::uint32_t logKind{},marker{};AiBridgePathHooks hooks;
    hooks.initialize=[&](auto token,auto source,auto target) { RawAiBridgePath path(scene.memory,scene.pool,finder,{now,limit,scene.bridge,logKind,marker,marker,marker},{});CHECK(path.Initialize(token,source,target)); };
    hooks.prepare=[&](auto token) { return scene.Candidates().Prepare(token); };RawAiBridgePath path(scene.memory,scene.pool,finder,{now,limit,scene.bridge,logKind,marker,marker,marker},std::move(hooks));Put(scene.memory.Bytes(scene.token),0x5f0,1);
    RawPathPoint source{50,50};CHECK(!path.Step(scene.token,source,{80,80}) && Get(scene.memory.Bytes(scene.token),0x5f0)==2);CHECK(!path.Step(scene.token,source,{80,80}) && Get(scene.memory.Bytes(scene.token),0x5f0)==2);
    for (int call=0;call<16 && Get(scene.memory.Bytes(scene.token),0x5f0)==2;++call) CHECK(!path.Step(scene.token,source,{80,80})); //실제 나선 상태가 완료할 때까지 동일 Step을 호출한다.
    CHECK(Get(scene.memory.Bytes(scene.token),0x5f0)==3 && source.x==50 && source.y==50 && &scene.Candidates().Pool()==&scene.pool);
}
