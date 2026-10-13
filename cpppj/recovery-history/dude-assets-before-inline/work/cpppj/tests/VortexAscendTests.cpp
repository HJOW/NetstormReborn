// 실제 두 PE의 Ascend 후보/반경 삭제/섬 소유자 계수를 같은 raw pool/hash/spot/난수에서 대조한다.
#include "RawSceneSupport.h"
#include "o/RawVortexAscend.h"
#include "o/RawAssetRegular.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 저장된 독립 native 입력/결과를 기대값 재계산 없이 읽는다.
const std::vector<std::vector<std::string>>& AscendRows() {
    static const auto rows=LoadFixture(NETSTORM_VORTEXASCEND_FIXTURE).rows;return rows;
}
// 단일 pool/hash/spot와 실제 타이머/삭제 효과의 입력 저장소다.
struct VortexAscendScene {
    SidPool pool; // 실제 SID50/60..127의 단일 raw 풀이다.
    SquidHash hash; // 현재 네 단계 버킷과 표면0단계다.
    std::vector<std::uint8_t> spots; // 원본 spot 비트 입력 지도다.
    std::vector<RiftTypeRecord> types; // 현재 genus/flags1/발자국 입력 표다.
    std::array<VortexAscendIsland,6> islands; // 원본32바이트 섬 목록의 존재/owner 입력이다.
    GameRandom random; // 삭제 선택이 소비하는 실제 공유 난수 전역이다.
    VortexAscendState state; // 현재 noIsland 타입 전역이다.
    std::vector<std::uint32_t> input; // 저장된 입력 격자이며 반환 판단을 계산하지 않는다.
    std::vector<std::string> events; // 외부 효과에 전달된 현재 인자와 순서다.
    std::string kind; // 현재 Candidate/Tick/Count 진입점 이름이다.
    bool patch; // 원본 raw 배치가 패치판인지 나타낸다.
    std::int32_t island{9}; // 실제 섬 조회 경계가 반환할 명시 입력이다.
    bool allocated{}; // 원본 new0x50 성공을 나타내며free 인자를 관찰한다.
    std::unique_ptr<RawVortexAscend> ascend; // 이 Scene의 공유 자료를 참조하는 생산 모듈이다.
    // native 입력의 모든 번호를 실제 client 정책으로 확보한다.
    explicit VortexAscendScene(OriginalEdition edition):pool(edition,kCapacity,false),spots(65536),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 원본 입력이 읽는 공간 번호를 모두 확보한다.
        for (std::uint16_t sid=5;sid<=127;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 현재 판본의 owner 필드 위치다.
    std::size_t Owner() const { return patch ? 34U : 32U; }
    // 실제 입력의 버킷 머리 삽입·raw 좌표/타입을 준비한다.
    void Add(Sid sid,std::uint8_t type,std::uint8_t owner) {
        auto raw=pool.AllocatedBytes(sid);raw[10]=type;raw[Owner()]=owner;
        Put(raw,0,0x11018000);Put(raw,14,std::bit_cast<std::uint32_t>(50.75f));Put(raw,18,std::bit_cast<std::uint32_t>(52.5f));
        auto& bucket=hash.Bucket(1,50.75f,52.5f);Put(raw,4,bucket,2);bucket=sid.value;
    }
    // fixture 입력만 구성한다. 결과는 실제 모듈 Handle/Finder/난수가 정한다.
    void Prepare(const std::vector<std::string>& row) {
        input.clear();events.clear();hash.Reset();std::fill(spots.begin(),spots.end(),std::uint8_t{});
        kind=row[1];island=9;random.SetState(0);allocated=false;
        // 저장된 숫자 입력을 원본 DWORD 비트로 읽는다.
        for (const auto& text:Split(row[2],',')) input.push_back(Number(text));
        // native의 재사용 메모리도 case마다 전 슬롯을0으로 초기화한다.
        for (std::uint16_t sid=5;sid<=127;++sid) {
            auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});
        }
        // 원본에 넣은 발자국/타입/genus 초기값을 같은 표에 준비한다.
        for (const auto type:{89U,94U,124U,157U}) { types[type]={};types[type].footX=types[type].footY=1; }
        if (kind=="Candidate") {
            CHECK(input.size()==8);Add(Sid{60},89,static_cast<std::uint8_t>(input[5]));
            types[89].flags2=input[0];types[89].flags1=input[1];spots[52*256+50]=static_cast<std::uint8_t>(input[2]);
            if (input[3]) { hash.Cell(0,50,52)=127;pool.AllocatedBytes(Sid{127})[10]=static_cast<std::uint8_t>(input[3]); }
            island=static_cast<std::int32_t>(input[4]);
        } else if (kind=="Tick") {
            CHECK(input.size()==6);const auto size=input[1];
            // native 입력과 같은 역순의 머리 삽입이다. 현재 next는 실제 finder가 소비한다.
            for (std::uint32_t i=size;i>0;--i) Add(Sid{static_cast<std::uint16_t>(59+i)},89,2);
            Add(kSource,124,static_cast<std::uint8_t>(input[3]));types[89].flags1=0x200000;random.SetState(input[2]);
        } else {
            CHECK(input.size()==2);const auto variant=input[1];
            constexpr std::array<std::uint32_t,6> owners{0,1,2,8,0xffffffff,1}; // native 섬 기록에 넣은 소유자DWORD다.
            // 각 현재 섬 항목의 존재와 owner만 입력한다.
            for (std::size_t i=0;i<islands.size();++i) islands[i]={static_cast<std::uint32_t>((i+variant)%3!=0),owners[(i+variant)%owners.size()]};
        }
        VortexAscendHooks hooks;
        hooks.islandNumber=[this](float x,float y) {
            events.push_back("I:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y)));
            if (kind=="Candidate") {
                if (input[7]==1) pool.AllocatedBytes(Sid{60})[Owner()]=1;
                if (input[7]==2) types[89].flags1=0;
                if (input[7]==3) spots[52*256+50]=2;
            }
            return island;
        };
        hooks.reserveCandidateBuffer=[this] {
            events.push_back("N:80");allocated=input[4]!=0;
            if (input[5]==1) pool.AllocatedBytes(kSource)[Owner()]=2;
            if (input[5]==2) pool.AllocatedBytes(Sid{60})[patch ? 40 : 35]=8;
            return allocated;
        };
        hooks.releaseCandidateBuffer=[this] { events.push_back(std::string("F:")+(allocated ? "1" : "0")); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("D:"+std::to_string(sid.value)+':'+std::to_string(flags));pool.AllocatedBytes(sid)[11]|=2; };
        ascend=std::make_unique<RawVortexAscend>(pool,hash,spots,types,islands,random,state,std::move(hooks));
    }
};
// actual 원본 반환·난수·삭제/효과 순서·콜백 후 owner를 전수 대조한다.
void ReplayAscend(const char* edition) {
    VortexAscendScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::size_t count=0;
    // 각 native 결과를 생산 API와 비교한다.
    for (const auto& row:AscendRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);scene.Prepare(row);
        std::uint32_t result;
        if (scene.kind=="Candidate") result=scene.ascend->Candidate(Sid{60},9,scene.input[6]);
        else if (scene.kind=="Tick") result=scene.ascend->Tick(kSource,scene.input[0]);
        else result=scene.ascend->CountForeignOwnedIslands(scene.input[0]);
        std::string events,states;
        // 입력 행의 명시 효과 순서와 현재 raw 삭제 비트를 관찰한다.
        for (const auto& event:scene.events) { if (!events.empty()) events+=';';events+=event; }
        if (events.empty()) events="-";
        // native가 저장한40개 공간 후보의 상태BYTE를 비교한다.
        for (std::uint16_t sid=60;sid<100;++sid) { if (!states.empty()) states+=',';states+=std::to_string(scene.pool.Slot(Sid{sid})[11]); }
        const auto owner=scene.pool.Slot(kSource)[scene.Owner()];
        const bool same=result==Number(row[3]) && scene.random.State()==Number(row[4]) && events==row[5] && states==row[6] && owner==Number(row[7]);
        CHECK(same);++count;
        if (!same) { std::printf("%s Ascend 행 %zu %s %s\n기대 %s/%s/%s / 실제 %u/%u/%s\n",
            edition,count,row[1].c_str(),row[2].c_str(),row[3].c_str(),row[4].c_str(),row[5].c_str(),result,scene.random.State(),events.c_str());break; }
    }
    CHECK(count>700);
}
}
// 패치판의 실제 후보/원본 반경finder/20개 제한과 게임 난수/후속 삭제를 전수 대조한다.
TEST_CASE(VortexAscend_ReplaysPatch) { ReplayAscend("originals"); }
// CD판의 후보/반경/버퍼free·섬 조회를 독립 원본 실행과 대조한다.
TEST_CASE(VortexAscend_ReplaysCd) { ReplayAscend("originalCD"); }

// 실제 후보0/32→64 끝과 미연결 하위 효과 거부를 동일 모듈에서 확인한다.
TEST_CASE(VortexAscend_RejectsMissingEffectsAndPreservesEmptyAllocationFailure) {
    VortexAscendScene scene(OriginalEdition::Patch1078);
    const auto found=std::ranges::find_if(AscendRows(),[](const auto& row) { return row[0]=="originals" && row[1]=="Tick" && row[2]=="33,0,0,1,0,0"; });
    CHECK(found!=AscendRows().end());if (found==AscendRows().end()) return;scene.Prepare(*found);
    CHECK(!scene.ascend->Tick(kSource,33));CHECK(scene.random.State()==0);
    bool failed=false;
    try { RawVortexAscend missing(scene.pool,scene.hash,scene.spots,scene.types,scene.islands,scene.random,scene.state,{}); }
    catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
}
