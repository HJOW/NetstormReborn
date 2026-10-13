// 실제 Fort owner 후처리/시각 seed/AddRandom의 정상 반환과 local PRNG를 두 PE 기록에 대조한다.
#include "ConstructionSupport.h"
#include "o/RawFortFinishOwner.h"

namespace {
using namespace netstorm::test::construction;
// 고유 fixture 입력 순서와 각 PE의 저장 행 수다. 기존 rawscene 이름과 섞이지 않게 전용 이름을 쓴다.
enum FinishField : std::size_t { FOwner,FMode,FFort,FSpecial,FRegion,FList,FTimeLow,FTimeHigh,FRect,FSuccessAt,FPeriod,FSeed,FType,FQuantity,FForce,FMutation };
constexpr std::size_t kFinishCases=719;
// 일반 CTest는 보존된 기대값만 읽으며 PE/Python/게임을 실행하지 않는다.
const auto& FinishFixture() { static const auto rows=LoadFixture(NETSTORM_FORTFINISHOWNER_FIXTURE).rows;return rows; }
// 실제 native의 지역 입력 사각형이다. 기대 배치 좌표는 계산하지 않는다.
FortRandomRectangle FinishRectangle(std::uint32_t profile) {
    return std::array<FortRandomRectangle,6>{{{32,48,96,112},{15.25F,21.5F,16.1F,23.9F},{-15.25F,-3.5F,4.5F,9.5F},
        {20,21,20,21},{20,21,19,19},{1,2,255,254}}}.at(profile);
}
// 원본 residence 목록의 타입/상태/소유자/genus 자료만 공급한다.
struct FinishNode { /* sid: 원본 풀에서 객체를 찾는 SID 번호. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::uint16_t sid;/* type: 타입 표에서 객체 종류를 찾는 번호. 입력 구성 또는 실행 결과 대조에 사용한다. */ /* state: 검사 입력과 실행 뒤 결과를 보관할 상태. */ /* owner: 객체나 지역을 소유한 플레이어 번호. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::uint8_t type,state,owner;/* genus: genus. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::uint32_t genus; };
// 원본과 동일한 중복/후행/틀린 소유자/flags/free·dead·void 입력이다.
std::vector<FinishNode> FinishNodes(std::uint32_t profile,std::uint8_t owner) {
    const auto other=static_cast<std::uint8_t>((owner+1)%9);
    return std::array<std::vector<FinishNode>,6>{{{},{{50,89,0,owner,0x40000000}},{{50,89,0,other,0x40000000}},{{50,100,0,owner,0}},
        {{50,100,0,owner,0},{51,89,0,other,0x40000000},{52,89,0,owner,0x40000000},{52,89,0,owner,0x40000000}},
        {{50,89,15,owner,0x40000000}}}}.at(profile);
}
// 현재 타입/목록·모드·호출별 시각·배치 정책 응답을 같은 actual pool에 공급하는 장면이다.
struct FortFinishScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;/* types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RiftTypeRecord> types;/* residences: 거주건물. 입력 구성 또는 실행 결과 대조에 사용한다. */ SquidPostPopList residences;/* state: 검사 입력과 실행 뒤 결과를 보관할 상태. */ FortFinishOwnerState state;
    // values: 값. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint32_t> values;/* events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<std::string> events;/* attempts: 시도. 입력 구성 또는 실행 결과 대조에 사용한다. */ /* rounds: 회차. 입력 구성 또는 실행 결과 대조에 사용한다. */ /* lastSeed: 마지막 시드. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::uint32_t attempts{},rounds{},lastSeed{};/* added: 추가된 여부. 입력 구성 또는 실행 결과 대조에 사용한다. */ /* patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다. */ bool added{},patch;
    // 가상 배치 경계가 반환한 성공의 실제 목록 변경을 관찰할 번호를 미리 확보한다.
    FortFinishScene(OriginalEdition edition,std::vector<std::uint32_t> input):pool(edition,kCapacity,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        values(std::move(input)),patch(edition==OriginalEdition::Patch1078) {
        // raw native 입력 SID를 실제로 할당한다.
        for (std::uint16_t sid=5;sid<=100;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        types[89].flags2=types[90].flags2=0x40000000;residences.entries.resize(8);
        state.chaffMode=values[FMode]!=0;state.fort=values[FFort]!=0;state.special=values[FSpecial]!=0;state.residenceType=values[FType];
        state.playerRegions.fill(std::bit_cast<std::int32_t>(values[FRegion]));
        // 각 목록 항목의 원본 raw 상태만 공급하며 같은 SID의 중복도 그대로 둔다.
        for (const auto node:FinishNodes(values[FList],static_cast<std::uint8_t>(values[FOwner]))) {
            auto raw=pool.AllocatedBytes(Sid{node.sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=node.type;raw[patch ? 34 : 32]=node.owner;raw[11]=node.state;
            types[node.type].flags2=node.genus;residences.entries[residences.count++]=node.sid;
        }
    }
    // 실제 효과/인자의 원시 DWORD를 기록한다.
    void Event(char letter,std::initializer_list<std::uint32_t> args) {
        std::string text(1,letter);
        // native 관찰의 값 순서/폭을 유지한다.
        for (const auto value:args) text+=':'+std::to_string(value);events.push_back(std::move(text));
    }
    // 매 시각 호출마다 전달했던 독립 double 표본을 반환한다. callback 횟수도 검증한다.
    double Clock() {
        const double initial=std::bit_cast<double>((static_cast<std::uint64_t>(values[FTimeHigh])<<32)|values[FTimeLow]);
        const auto time=initial+static_cast<double>(rounds++)*0.125;const auto bits=std::bit_cast<std::uint64_t>(time);
        Event('T',{static_cast<std::uint32_t>(bits),static_cast<std::uint32_t>(bits>>32)});return time;
    }
    // native의 실제 외부 경계만 공급한다. 생성 성공은 같은 residence 목록에 등록하므로 Finish가 다시 읽는다.
    FortFinishOwnerHooks Hooks(bool finishMode) {
        FortFinishOwnerHooks h;
        h.regionBounds=[&](std::int32_t region) { const auto rect=FinishRectangle(values[FRect]);Event('R',{static_cast<std::uint32_t>(region),Bits(rect.left),Bits(rect.top),Bits(rect.right),Bits(rect.bottom)});
            if (values[FMutation]==1) state.residenceType=90;return rect; };
        h.wallSeconds=[&] { return Clock(); };
        h.place=[&,finishMode](GameRandom& random,const FortRandomPlaceRequest& request) {
            ++attempts;Event('P',{request.type,Bits(request.x),Bits(request.y),static_cast<std::uint32_t>(request.region),request.mode,request.force});
            if (values[FMutation]==2 && attempts==1) random.SetState(0x98765432);lastSeed=random.State();
            const bool success=attempts>=values[FSuccessAt] && (attempts-values[FSuccessAt])%values[FPeriod]==0;
            if (success && finishMode && !added) {
                added=true;auto raw=pool.AllocatedBytes(Sid{80});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(request.type);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(values[FOwner]);
                residences.entries.at(residences.count++)=80;
            }
            return success ? 7U : 0U;
        };
        h.invalidType=[&] { Event('D',{29}); };return h;
    }
    // 관찰 문자열을 fixture와 같은 형식으로 묶는다.
    std::string Events() const {
        std::string result;
        // 실제 callback 순서를 유지한다.
        for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result.empty() ? "-" : result;
    }
    // 같은 실제 목록의 중복/최종 순서를 그대로 비교한다.
    std::string List() const {
        std::string result;
        // 활성 count까지만 직렬화하며 native와 같은 후행 저장소는 읽지 않는다.
        for (std::uint32_t index=0;index<residences.count;++index) { if (!result.empty()) result+=',';result+=std::to_string(residences.entries.at(index)); }
        return result.empty() ? "-" : result;
    }
};
// entry/standalone batch/실제 seed 생성 관찰을 동일 입력의 C++ 몸체와 대조한다.
void ReplayFinish(std::string_view edition) {
    CHECK(FinishFixture().size()==2*kFinishCases);std::size_t count=0;
    // 한 행마다 다른 actual pool/목록을 구성해 이전 callback 상태가 섞이지 않게 한다.
    for (const auto& row:FinishFixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);FortFinishScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,Inputs(row[2]));
        RawFortFinishOwner finish(scene.pool,scene.types,scene.residences,scene.state,scene.Hooks(row[1]=="F"));std::uint32_t result=0;
        if (row[1]=="F") finish.Finish(scene.values[FOwner]);
        else if (row[1]=="B") {
            GameRandom random(scene.values[FSeed]);result=finish.PlaceRandom(random,FinishRectangle(scene.values[FRect]),std::bit_cast<std::int32_t>(scene.values[FRegion]),
                scene.values[FQuantity],scene.values[FType],scene.values[FForce]);scene.lastSeed=random.State();
        } else { result=finish.TimeSeed(scene.Clock());scene.lastSeed=result; }
        const bool same=scene.Events()==row[3] && std::to_string(result)==row[4] && std::to_string(scene.lastSeed)==row[5] && scene.List()==row[6] && std::to_string(scene.state.residenceType)==row[7];
        CHECK(same);if (!same) { std::printf("Fort finish actual %s %s 행%zu 불일치\n",row[0].c_str(),row[1].c_str(),count);break; }++count;
    }
    CHECK(count==kFinishCases);
}
}
// 원본 전체 entry/seed/batch의 관찰을 재생한다.
TEST_CASE(FortFinishOwner_ReplaysPatch) { ReplayFinish("originals"); }
// CD의 사각형 점 helper/local PRNG 및 정수 HP와 별도의 후처리 관찰이다.
TEST_CASE(FortFinishOwner_ReplaysCd) { ReplayFinish("originalCD"); }

// loader 후처리는 실제 같은 월드 모듈을 호출하고 residence가 있으면 지역/시각/배치를 하지 않는다.
TEST_CASE(FortFinishOwner_ConnectsTheSameResidenceWorld) {
    auto input=Inputs(FinishFixture().front()[2]);input[FList]=1;input[FRegion]=3;input[FMode]=input[FFort]=input[FSpecial]=0;input[FOwner]=1;
    FortFinishScene scene(OriginalEdition::Patch1078,input);RawFortFinishOwner finish(scene.pool,scene.types,scene.residences,scene.state,scene.Hooks(true));
    const auto hooks=MakeFortFinishOwnerHooks(scene.pool,finish,{});hooks.finishOwner(1);CHECK(scene.events.empty() && scene.rounds==0 && scene.attempts==0);
    SidPool foreign(OriginalEdition::Patch1078,kCapacity,true);CHECK(Throws([&] { MakeFortFinishOwnerHooks(foreign,finish,{}); }));
    scene.state.chaffMode=true;finish.Finish(0xffffffffU);CHECK(scene.events.empty()); // 초기 모드 반환은 소유자 범위 조회보다 앞선다.
}

// by-value rectangle snapshot·quantity/실제 callback과 손상된 현재 목록·누락 효과 거부를 검사한다.
TEST_CASE(FortFinishOwner_PreservesRectangleAndRequiresRealEffects) {
    auto input=Inputs(FinishFixture().front()[2]);FortFinishScene scene(OriginalEdition::Patch1078,input);auto hooks=scene.Hooks(false);hooks.place={};
    CHECK(Throws([&] { RawFortFinishOwner bad(scene.pool,scene.types,scene.residences,scene.state,hooks); }));
    FortRandomRectangle rect{1,2,1,2};std::uint32_t attempts=0;hooks=scene.Hooks(false);
    hooks.place=[&](GameRandom&,const FortRandomPlaceRequest& request) { CHECK(request.x==1 && request.y==2);++attempts;rect={100,101,150,151};return 7U; };
    RawFortFinishOwner finish(scene.pool,scene.types,scene.residences,scene.state,hooks);GameRandom random(0);
    CHECK(finish.PlaceRandom(random,rect,3,2,89,0)==7 && attempts==2 && random.State()==0);
    scene.state.playerRegions[1]=3;scene.state.chaffMode=scene.state.special=scene.state.fort=false;scene.residences.count=9;
    CHECK(Throws([&] { finish.Finish(1); }));
}
