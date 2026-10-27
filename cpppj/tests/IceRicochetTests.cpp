// 원본10.78의 반사점·후보 선택·후속 Missile 인자를 같은 입력으로 비트 대조한다.
#include "RawSceneSupport.h"
#include "o/RawIceRicochet.h"
#include "o/SquidReward.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원문 도구 KEYS와 같은 독립 입력 열이다.
enum IceColumn : std::size_t { Kind,Seed,OriginX,OriginY,X,Y,Range,Target,Speed,Nodes,Mutation,Output };
// 보호PE에서 생성한 자료만 읽으며 제품 출력으로 기대값을 만들지 않는다.
const std::vector<std::vector<std::string>>& IceRows() {
    static const auto rows=LoadFixture(NETSTORM_ICE_RICOCHET_FIXTURE).rows;return rows;
}
// 같은128BYTE/표시 두 슬롯/일반 Finder 후보와 공유 난수를 가진 대조 장면이다.
struct IceRicochetScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 실제 저번호/서버번호 후보를 읽는 저장소다.
    std::vector<RiftTypeRecord> types{188}; // 현재 raw TYPE/HP/비행 플래그 입력이다.
    std::vector<MissileTypeData> missileTypes{188}; // 현재 표시130의 반사 사거리다.
    GameRandom random; // 제품 공유 난수 몸체의 전진을 원문 최종 상태와 대조한다.
    MissileFields fields; // padding을 포함한 원문128BYTE 필드다.
    std::vector<std::string> events; // Finder/후속 ctor의 순서·인자를 기록한다.
    std::vector<std::uint16_t> candidates; // 원문 외부 Finder에 공급한 동일 후보 순서다.
    std::size_t index{}; // 현재 Finder 후보 위치다.
    const std::vector<std::string>* input{}; // 이번 독립 입력 행이다.
    // free/저번호 후보의 물리 바이트까지 검사 입력으로 직접 구성한다.
    std::span<std::uint8_t> Slot(Sid sid) {
        const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
    }
    // 입력행마다 원문과 동일한 현재 객체/TYPE/난수를 설치한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==Output+4);input=&row;events.clear();candidates.clear();index=0;random.SetState(Number(row[Seed]));
        // 생성자가 쓰지 않은 padding도 같은 비트로 대조한다.
        for (std::size_t i=0;i<fields.bytes.size();++i) fields.bytes[i]=static_cast<std::uint8_t>(i*13+7);
        Put(fields.bytes,4,81);Put(fields.bytes,8,90);Put(fields.bytes,24,Number(row[Target]));Put(fields.bytes,48,Number(row[Speed]));Put(fields.bytes,56,Number(row[OriginX]));Put(fields.bytes,60,Number(row[OriginY]));
        std::fill(types.begin(),types.end(),RiftTypeRecord{});std::fill(missileTypes.begin(),missileTypes.end(),MissileTypeData{});missileTypes[130].range=std::bit_cast<float>(Number(row[Range]));
        // 부모가 순회 중 바뀌는 입력도 처음 캡처한90 슬롯과 구분한다.
        for (const auto sid:{Sid{90},Sid{91}}) {
            auto raw=Slot(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,6,81,2);raw[10]=130;Put(raw,14,Number(row[X]));Put(raw,18,Number(row[Y]));
        }
        if (row[Nodes]=="-") return;
        // 후보별로 다른 TYPE를 써 플래그가 서로 덮이지 않게 한다.
        for (const auto& text:Split(row[Nodes],';')) {
            const auto node=Split(text,',');CHECK(node.size()==7);const auto type=70+candidates.size();const auto sid=Sid{static_cast<std::uint16_t>(Number(node[0]))};candidates.push_back(sid.value);
            auto raw=Slot(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(Number(node[6]));raw[40]=static_cast<std::uint8_t>(Number(node[3]));Put(raw,14,Number(node[4]));Put(raw,18,Number(node[5]));types[type].flags1=Number(node[1]);types[type].flags2=Number(node[2]);
        }
    }
    // 원문 경계의 동일 시점에 현재 필드를 바꾸고 다음 후보를 제공한다.
    Sid Next() {
        events.push_back("N");++index;
        if (index==1) {
            const auto mutation=Number(input->at(Mutation));
            if (mutation==1) Put(fields.bytes,24,15001);
            if (mutation==2) Put(fields.bytes,48,std::bit_cast<std::uint32_t>(37.25F));
            if (mutation==3) Put(fields.bytes,8,91);
            if (mutation==4) { Slot(Sid{90})[10]=132;Put(Slot(Sid{90}),14,std::bit_cast<std::uint32_t>(81.25F));Put(Slot(Sid{90}),18,std::bit_cast<std::uint32_t>(82.5F)); }
        }
        return Sid{index<candidates.size() ? candidates[index] : std::uint16_t{0}};
    }
    // 원본이 넘긴12개 DWORD 인자의 실제 비트를 순서대로 기록한다.
    void Create(const GunProjectileRequest& request) {
        const std::array<std::uint32_t,12> args{request.firing.value,request.target.value,std::bit_cast<std::uint32_t>(request.speed),request.displayType,request.effectType,std::bit_cast<std::uint32_t>(request.targetPosition[0]),std::bit_cast<std::uint32_t>(request.targetPosition[1]),std::bit_cast<std::uint32_t>(request.targetCenter[0]),std::bit_cast<std::uint32_t>(request.targetCenter[1]),request.ricochet,std::bit_cast<std::uint32_t>(request.firingPosition[0]),std::bit_cast<std::uint32_t>(request.firingPosition[1])};std::string event="J:";
        // 문자열 표기만 변환하며 기대 인자를 재계산하지 않는다.
        for (std::size_t i=0;i<args.size();++i) { if (i) event+=',';event+=std::to_string(args[i]); }
        events.push_back(std::move(event));
    }
    // 실제 제품 Point/Generate와 같은 원본 HP 몸체를 실행한다.
    std::vector<std::string> Run() {
        SquidPostPopState bookkeeping;SquidRewardState rewardState;ScrambledSpStore store(random,[] { return 1U; });SquidReward reward(pool,types,bookkeeping,rewardState,&store);IceRicochetHooks hooks;
        hooks.begin=[this](SquidSearchArea area,std::uint32_t flags) { CHECK(flags==0);index=0;events.push_back("B:"+std::to_string(area.left)+','+std::to_string(area.top)+','+std::to_string(area.right)+','+std::to_string(area.bottom)+":0:0");return Sid{candidates.empty() ? std::uint16_t{0} : candidates[0]}; };
        hooks.next=[this] { return Next(); };hooks.hasHitPoints=[&reward](Sid sid) { return reward.HasHitPoints(sid); };hooks.createProjectile=[this](const GunProjectileRequest& request) { Create(request); };
        RawIceRicochet logic(pool,types,missileTypes,random,std::move(hooks));std::array<float,2> point{123.25F,124.5F};
        if (Number(input->at(Kind))==0) logic.Point(fields,point);else logic.Generate(fields);
        std::array<std::uint8_t,8> pointBytes{};Put(pointBytes,0,std::bit_cast<std::uint32_t>(point[0]));Put(pointBytes,4,std::bit_cast<std::uint32_t>(point[1]));std::string trace;
        // 관찰 순서와 모든 원문 필드/난수를 그대로 직렬화한다.
        for (const auto& event:events) { if (!trace.empty()) trace+=';';trace+=event; }
        return {trace,Hex(pointBytes),Hex(fields.bytes),std::to_string(random.State())};
    }
};
// 첫 차이를 원문 행/열과 함께 출력하며 기존 기대값을 수정하지 않는다.
void ReplayIce(unsigned group) {
    IceRicochetScene scene;const auto& rows=IceRows();std::size_t count{};
    // 점 계산·선택·재조회 입력을 서로 별도 검사로 집계한다.
    for (std::size_t i=0;i<rows.size();++i) {
        const auto& row=rows[i];const auto kind=Number(row[Kind]);const auto mutation=Number(row[Mutation]);const bool equal=row[X]==row[OriginX] && row[Y]==row[OriginY];
        if ((group==0 && (kind!=0 || equal)) || (group==1 && (kind!=0 || !equal)) || (group==2 && (kind!=1 || mutation)) || (group==3 && (kind!=1 || !mutation))) continue;
        scene.Prepare(row);const auto actual=scene.Run();++count;
        // 관찰4열의 전체 문자열/비트를 원문에 대조한다.
        for (std::size_t column=0;column<4;++column) {
            if (actual[column]!=row[Output+column]) { std::fprintf(stderr,"아이스 반사 차이 row=%zu column=%zu actual=%s expected=%s\n",i,column,actual[column].c_str(),row[Output+column].c_str());CHECK(false);return; }
        }
    }
    CHECK(count>0);
}
}
TEST_CASE(IceRicochet_ReplaysNativePointAndSharedRandom) { ReplayIce(0); } // 방향/직접 삼각함수/클램프/두 난수 상태를 대조한다.
TEST_CASE(IceRicochet_ReplaysNativeZeroDisplacementPreservesOutput) { ReplayIce(1); } // 변위0의 출력/난수 보존을 대조한다.
TEST_CASE(IceRicochet_ReplaysNativeChanceAndCandidateSelection) { ReplayIce(2); } //19/20·서버SID·HP/비행/추상·동률과 target0을 대조한다.
TEST_CASE(IceRicochet_ReplaysNativeCurrentFieldsAfterFinder) { ReplayIce(3); } // 현재 표적/속도/부모와 처음 잡은 표시 슬롯의 재조회를 대조한다.

TEST_CASE(IceRicochet_SelectedMissingLifetimeIsRejected) { // 새 탄의 수명이 없으면 빈 성공으로 처리하지 않는다.
    IceRicochetScene scene;auto row=IceRows().front();row[Kind]="1";row[Seed]="20";row[Range]="0";scene.Prepare(row);IceRicochetHooks hooks;hooks.begin=[](SquidSearchArea,std::uint32_t) { return Sid{}; };hooks.next=[] { return Sid{}; };hooks.hasHitPoints=[](Sid) { return false; };RawIceRicochet logic(scene.pool,scene.types,scene.missileTypes,scene.random,std::move(hooks));CHECK(Throws([&] { logic.Generate(scene.fields); }));CHECK(scene.events.empty());
}

TEST_CASE(IceRicochet_ActualWorldFinderAndRewardShareNativeRequest) { // 실제 해시 체인/HP가 원문의 같은 후보에 같은 후속 인자를 내는지 검사한다.
    const auto& rows=IceRows();const auto found=std::find_if(rows.begin(),rows.end(),[](const auto& row) { return row[Kind]=="1" && row[Seed]=="20" && row[Range]=="0" && row[Target]=="60" && row[Mutation]=="0" && row[Nodes]=="15000,16,0,0,1114636288,1112014848,0"; });CHECK(found!=rows.end());if (found==rows.end()) return;
    IceRicochetScene scene;scene.Prepare(*found);SquidHash hash;hash.Bucket(0,60,50)=15000;Put(scene.Slot(Sid{15000}),4,0,2);
    SquidPostPopState bookkeeping;SquidRewardState rewardState;ScrambledSpStore store(scene.random,[] { return 1U; });SquidReward reward(scene.pool,scene.types,bookkeeping,rewardState,&store);IceRicochetHooks hooks;hooks.createProjectile=[&scene](const GunProjectileRequest& request) { scene.Create(request); };
    RawIceRicochet logic(scene.pool,scene.types,scene.missileTypes,scene.random,MakeIceRicochetWorldHooks(scene.pool,hash,scene.types,reward,std::move(hooks)));logic.Generate(scene.fields);
    CHECK(scene.events.size()==1);if (!scene.events.empty()) { const auto& expected=found->at(Output);const auto position=expected.rfind("J:");CHECK(position!=std::string::npos);if (position!=std::string::npos) CHECK(scene.events[0]==expected.substr(position)); }
    CHECK(Hex(scene.fields.bytes)==found->at(Output+2));CHECK(scene.random.State()==Number(found->at(Output+3)));CHECK(!store.Initialized());
}
