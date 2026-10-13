// 두 원본 PE의 독립 Handle 관찰과 같은 풀의 실제 Frame/Finder/Advance·Regular 재예약을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawAssetRegular.h"
#include "o/SquidPostPop.h"
#include "o/RawSquidDestroy.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 복원한 부모의 실제 가상 표 기록값이다. 호스트 포인터로 호출하지 않는다.
constexpr std::array<std::uint32_t,3> kPatchTables{0x50f540,0x50da88,0x5196f0},kCdTables{0x501b90,0x500760,0x501d28};
// 원본 기계어와 동일한 6개 방향의 8프레임 코드 표를 준비한다.
RiftTypeFrames Codes(std::uint8_t flags=0) {
    std::vector<FrameCode> codes;
    // 각 방향의 실제 구간은 생산 FrameTable이 직접 계산한다.
    for (unsigned i=0;i<48;++i) codes.push_back({static_cast<std::uint8_t>('A'+i/8),'A',static_cast<std::uint8_t>(i),flags});
    return RiftTypeFrames(std::move(codes));
}
// 현재 원본 fixture를 기대값 재계산 없이 읽는다.
const std::vector<std::vector<std::string>>& Rows() {
    static const auto rows=LoadFixture(NETSTORM_ASSETREGULAR_FIXTURE).rows;return rows;
}
// 원본 입력의 단일 pool/hash/random와 현재 외부 효과를 구성한다.
struct AssetRegularScene {
    SidPool pool; // 동일 부모/이웃을 읽고 쓰는 raw 풀이다.
    SquidHash hash; // 실제 finder가 읽는 네 단계 버킷이다.
    std::vector<RiftTypeRecord> types; // 실제 비용/genus/발자국 입력 표다.
    std::vector<RiftTypeFrames> frames; // 실제 방향/코드 flags 입력 표다.
    std::vector<std::int32_t> elements; // 현재 신전 타입의 원소 +98 값이다.
    GameRandom random; // fixture가 공급한 공유 난수 전역이다.
    AssetPostPopState assets; // 원본과 같은 신전 목록 저장소다.
    AssetRegularState state; // 모드/사건/최대 조사 전역이다.
    std::vector<std::string> events; // 표시/명시 효과의 원본 호출 순서다.
    std::vector<std::uint32_t> input; // 25개 입력 필드이며 결과 계산에는 쓰지 않는다.
    bool patch; // 현재 raw 배치의 판본이다.
    SquidFrame setter; // 실제 프레임 지정/표시 순서를 실행한다.
    RawFrameAdvance advance; // 실제 방향별 한 번 감싸기를 실행한다.
    std::unique_ptr<RawAssetRegular> regular; // 이번 Scene 참조를 읽는 생산 모듈이다.
    // 실제 SID 50/60..62를 할당하고 frame/update를 같은 풀에 구성한다.
    explicit AssetRegularScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size(),Codes()),elements(types.size()),patch(edition==OriginalEdition::Patch1078),
        setter(pool,types,{[](Sid,std::int32_t) { return std::array<float,2>{1,1}; },
            [this](Sid sid,std::uint32_t flags) {
                Update(sid,flags);
                if (input[24]==4) Put(pool.AllocatedBytes(kSource),patch ? 36 : 34,7,patch ? 4 : 1);
            },[](Sid,std::uint32_t) { throw std::logic_error("합성 입력은 해시 단계1을 유지해야 합니다"); },
            [](Sid,float,float,std::uint32_t) { throw std::logic_error("합성 입력은 재Pop 경계에 도달하지 않습니다"); }}),
        advance(pool,frames,setter) {
        // native fixture의 단일 풀 번호를 확보한다.
        for (std::uint16_t sid=5;sid<=62;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 현재 판본의 frame 비트를 읽는다.
    std::uint32_t Frame(Sid sid) const { return Get(pool.Slot(sid),patch ? 36 : 34,patch ? 4 : 1); }
    // 원본 표시 경계에서 호출 당시의 프레임을 기록한다.
    void Update(Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)+':'+std::to_string(Frame(sid))); }
    // input만 raw 슬롯/타입/해시에 쓰며 Handle 판단을 미리 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        input.clear();events.clear();hash.Reset();
        // 저장된 원본 입력은 production 모듈과 명시 효과 경계에만 넘긴다.
        for (const auto& text:Split(row[1],',')) input.push_back(Number(text));
        CHECK(input.size()==25);random.SetState(input[7]);state={};assets={};
        state.fortMode=input[8]!=0;state.boss=input[9]!=0;state.suppressLeaveRequest=input[10]!=0;
        state.checkingForms=input[11]!=0;state.themeMode=input[13]!=0;state.decorationType=91;state.maxDecorationScan=input[16];
        state.regionGeneration=3;state.animationType=17;assets.temples.entries={62,0};assets.temples.count=1;
        const auto x=input[17]==4 ? 5.25f : 50.75f,y=input[17]==4 ? 4.75f : 52.5f;
        // 원본 입력과 같은 역순 머리 삽입으로 실제 finder의 반환 순서를 비교한다.
        for (const auto sid:{Sid{62},Sid{61},Sid{60},kSource}) {
            // 앞 관찰의 이탈 효과가 raw free를 켰어도 다음 독립 입력은 원본처럼 슬롯 자체를 다시 초기화한다.
            const auto previous=pool.Slot(sid); // 상태 검사 없는 읽기 전용 범위이며 fixture 초기화에서만 쓰기로 변환한다.
            std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(previous.data()),previous.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{});
            const auto xx=sid==kSource ? x : sid.value==60 ? 49.75f : sid.value==61 ? 50.25f : 51.75f;
            const auto yy=sid==kSource ? y : 50.25f;
            raw[10]=static_cast<std::uint8_t>(sid==kSource ? 89 : sid.value==62 ? 90 : 91);
            raw[patch ? 40 : 35]=static_cast<std::uint8_t>(sid==kSource ? input[22] : input[17]==3 ? 8 : 0);
            Put(raw,0,0x11018000);Put(raw,14,std::bit_cast<std::uint32_t>(xx));Put(raw,18,std::bit_cast<std::uint32_t>(yy));
            auto& bucket=hash.Bucket(1,xx,yy);Put(raw,4,bucket,2);bucket=sid.value;
        }
        const auto kind=input[0],number=kind==0 ? 89U : kind==1 ? 122U : 124U;
        auto raw=pool.AllocatedBytes(kSource);raw[10]=static_cast<std::uint8_t>(number);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(input[12]);
        Put(raw,12,input[4],2);Put(raw,patch ? 36 : 34,input[5],patch ? 4 : 1);raw[patch ? 33 : 31]=1;
        Put(raw,0,(patch ? kPatchTables : kCdTables)[std::min(kind,2U)]);if (patch) Put(raw,42,5);
        pool.AllocatedBytes(Sid{60})[patch ? 34 : 32]=static_cast<std::uint8_t>(input[15]);
        pool.AllocatedBytes(Sid{62})[patch ? 34 : 32]=static_cast<std::uint8_t>(input[12]);
        // 같은 원본 입력의 타입/코드를 구성하고 각 몸체가 후속 프레임을 결정하게 한다.
        for (const auto typ:{89U,90U,91U,122U,124U}) {
            types[typ].cost=std::bit_cast<float>(input[6]);types[typ].flags2=input[21];types[typ].footX=types[typ].footY=1;
            elements[typ]=static_cast<std::int32_t>(input[14]);frames[typ]=Codes(static_cast<std::uint8_t>(input[17]==1 ? 8 : input[17]==2 ? 16 : 0));
        }
        regular=std::make_unique<RawAssetRegular>(pool,hash,types,frames,elements,setter,advance,random,assets,state,Hooks());
    }
    // 원본이 관찰한 하위 효과의 인자/후속 raw 변화만 동일하게 공급한다.
    AssetRegularHooks Hooks() {
        AssetRegularHooks hooks;
        hooks.notifySurface=[this](Sid sid) { events.push_back("N:"+std::to_string(sid.value));if (input[24]==1) Put(pool.AllocatedBytes(sid),patch ? 36 : 34,23,patch ? 4 : 1); };
        hooks.surface=[this](float,float) { return Sid{static_cast<std::uint16_t>(input[15] ? 60 : 0)}; };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) { events.push_back("O:"+std::to_string(sid.value)+':'+std::to_string(owner));pool.AllocatedBytes(sid)[patch ? 34 : 32]=static_cast<std::uint8_t>(owner);if (input[24]==2) Put(pool.AllocatedBytes(sid),42,7); };
        hooks.residenceEvent2=[this](Sid sid) { events.push_back("E:"+std::to_string(sid.value)); };
        hooks.displayUpdate=[this](Sid sid,std::uint32_t flags) { Update(sid,flags);if (input[24]==3) Put(pool.AllocatedBytes(sid),12,1000,2); };
        hooks.ascendTick=[this](Sid sid,std::uint32_t count) { events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(count));return input[18]!=0; };
        hooks.playerLeavingNotice=[this](std::uint32_t owner) { events.push_back("L:"+std::to_string(owner)); };
        hooks.countForeignOwnedIslands=[this](std::uint32_t owner) { events.push_back("R:"+std::to_string(owner));return 0; };
        hooks.islandNumber=[this](float,float) { events.push_back("I");return 9; };
        hooks.requestLeave=[this](Sid sid,std::uint32_t island) { events.push_back("Q:9:0:"+std::to_string(island)+':'+std::to_string(sid.value)+":1");if (input[19] || input[24]==5) pool.AllocatedBytes(sid)[11]=1; };
        hooks.regionNumber=[this](float,float) { events.push_back("Z");return 11; };
        hooks.removeAnimation=[this](std::uint32_t type,std::uint32_t token) { events.push_back("A:"+std::to_string(type)+':'+std::to_string(token)+':'+std::to_string(token));if (input[24]==6) Put(pool.AllocatedBytes(kSource),12,99,2); };
        hooks.damageGround=[this](float x,float y) { events.push_back("G:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))); };
        hooks.hitPoints=[this](Sid sid,std::uint32_t value,std::int32_t source,std::uint32_t flags) { events.push_back("H:"+std::to_string(sid.value)+':'+std::to_string(value)+':'+std::to_string(static_cast<std::uint32_t>(source))+':'+std::to_string(flags));if (input[24]==7) types[pool.Slot(sid)[10]].flags2=0x200000;return static_cast<std::int32_t>(input[20]); };
        hooks.findAttached=[this](Sid sid,std::uint32_t type) { events.push_back("F:"+std::to_string(sid.value)+':'+std::to_string(type)+":0:0");return Sid{static_cast<std::uint16_t>(input[23])}; };
        hooks.startConstruction=[this](Sid sid,float duration) { events.push_back("X:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(duration))); };
        return hooks;
    }
};
// 한 판본의 독립 실제 반환/raw/난수/최대 조사·효과 순서를 production 모듈로 재생한다.
void Replay(const char* edition) {
    AssetRegularScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::size_t count=0;
    // 각 행의 결과는 원본이 정했으며 테스트가 재계산하지 않는다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);scene.Prepare(row);
        const auto& in=scene.input;
        const auto result=in[0]==3 ? scene.regular->DamageableHandle(kSource,in[1],in[2],std::bit_cast<float>(in[3])) :
            scene.regular->Handle(kSource,in[1],in[2],std::bit_cast<float>(in[3]));
        std::string events;
        // 원본과 같은 단일 순서 문자열을 만든다.
        for (const auto& event:scene.events) { if (!events.empty()) events+=';';events+=event; }
        if (events.empty()) events="-";
        const auto neighbors=std::to_string(scene.Frame(Sid{60}))+','+std::to_string(scene.Frame(Sid{61}))+','+std::to_string(scene.Frame(Sid{62}));
        const auto observed=Hex(scene.pool.Slot(kSource));
        const bool same=std::bit_cast<std::uint32_t>(result)==Number(row[2]) && scene.random.State()==Number(row[3]) &&
            scene.state.maxDecorationScan==Number(row[4]) && events==row[5] && observed==row[6] && neighbors==row[7];
        CHECK(same);++count;
        if (!same) { std::printf("%s Handle 행 %zu 입력 %s\n기대값 %s / 실제 %u\n기대효과 %s / 실제 %s\n기대raw %s / 실제 %s\n",
            edition,count,row[1].c_str(),row[2].c_str(),std::bit_cast<std::uint32_t>(result),row[5].c_str(),events.c_str(),row[6].c_str(),observed.c_str());break; }
    }
    CHECK(count>900);
}
}
// 패치10.78 실제 Handle/기계어 입력을 같은 pool/hash/Frame/Finder에서 대조한다.
TEST_CASE(AssetRegular_ReplaysPatch) { Replay("originals"); }
// CD10.72의 다른 가이저 주기·레지던스 분기와 같은 신전/공통 사건을 독립 대조한다.
TEST_CASE(AssetRegular_ReplaysCd) { Replay("originalCD"); }

// 시각/float payload를 혼동하지 않고 postPop 등록된 첫 Regular가 현재 Kernel에서 바로 호출되는지 검사한다.
TEST_CASE(AssetRegular_ComposesSharedKernelAndInitialTime) {
    AssetRegularScene scene(OriginalEdition::Patch1078);
    const auto found=std::ranges::find_if(Rows(),[](const auto& row) { return row[0]=="originals" && row[1].starts_with("2,1,"); });
    CHECK(found!=Rows().end());if (found==Rows().end()) return;scene.Prepare(*found);
    SquidPostPopState bookkeeping;bookkeeping.suppressed=true;SquidPostPop post(scene.pool,scene.types,bookkeeping);
    std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(scene.pool,scene.hash,spots);
    RawSquidDestroy destroy(scene.pool,unpop,scene.types);Kernel kernel;SquidProcessState processState;processState.now=17.25;
    SquidProcessHost host(scene.pool,scene.types,kernel,destroy,processState,scene.regular->Handler());
    const auto scheduled=host.AddRegular(kSource,1,0.08f);CHECK(scheduled!=nullptr);if (!scheduled) return;
    CHECK(scheduled->Time()==17.25 && scheduled->Count()==0 && scheduled->Payload()==0.08f);
    kernel.RunFrame();CHECK(scheduled->Count()==1 && scene.Frame(kSource)==1);
    CHECK(scheduled->Time()==17.25+static_cast<double>(0.08f));
    processState.now=17.26;kernel.RunFrame();CHECK(scheduled->Count()==1 && scene.Frame(kSource)==1);
    processState.now=scheduled->Time();kernel.RunFrame();CHECK(scheduled->Count()==2 && scene.Frame(kSource)==2);
}

// 미복원 효과/부모 및 다른 풀 지정기는 성공 처리 전에 진단한다.
TEST_CASE(AssetRegular_RejectsMissingEffectsAndForeignPool) {
    AssetRegularScene scene(OriginalEdition::Patch1078);scene.Prepare(Rows().front());
    RawAssetRegular missing(scene.pool,scene.hash,scene.types,scene.frames,scene.elements,scene.setter,scene.advance,scene.random,scene.assets,scene.state,{});
    bool failed=false;try { missing.Handle(kSource,0,0,0); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
    Put(scene.pool.AllocatedBytes(kSource),0,0x11018000);failed=false;
    try { missing.Handler()(kSource,0,0,0); } catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
    AssetRegularScene other(OriginalEdition::Patch1078);failed=false;
    try { RawAssetRegular foreign(scene.pool,scene.hash,scene.types,scene.frames,scene.elements,other.setter,other.advance,scene.random,scene.assets,scene.state,{}); }
    catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
}
