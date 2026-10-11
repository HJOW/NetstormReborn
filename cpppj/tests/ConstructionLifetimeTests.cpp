// 세 PE의 표시 등록/삭제 관찰과 실제 생성→프레임 Kill→버퍼/목록 해제를 검사한다.
#include "ConstructionSupport.h"
#include "o/RawConstructionLifetime.h"
#include "o/RawConstructionFrame.h"
#include <memory>

namespace {
using namespace netstorm::test::construction;
// 독립 관찰의 입력 열 순서와 호스트 정규화 객체/부모 토큰이다.
enum Input:std::size_t { Capacity,Pattern,Parent,Z0,Z,Mutate,Buffers,Flags,Slot };
constexpr std::uint32_t kToken=1;
constexpr Sid kParent{50};
// 저장 fixture만 읽으며 원본 PE/Python/게임을 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONLIFETIME_FIXTURE).rows;return rows; }
// 입력/출력의 순서를 유지하며 빈 자료는 실행기와 같은 '-'로 표현한다.
std::string Join(const std::vector<std::string>& values,char delimiter) {
    std::string text;
    // 각 값을 입력 순서대로 이어 붙인다.
    for (const auto& value:values) { if (!text.empty()) text+=delimiter;text+=value; }return text.empty() ? "-" : text;
}
// 원본 포인터 목록을 토큰으로 정규화한 입력이다. 빈 칸 검색/확장을 미리 계산하지 않는다.
std::vector<std::uint32_t> Initial(const std::vector<std::uint32_t>& input) {
    std::vector<std::uint32_t> slots(input[Capacity]);
    // 외부 객체마다 서로 다른 토큰만 공급한다.
    for (std::size_t i=0;i<slots.size();++i) slots[i]=static_cast<std::uint32_t>(100+i);
    if (!slots.empty() && input[Pattern]) slots[input[Pattern]==1 ? 0 : input[Pattern]==2 ? slots.size()/2 : slots.size()-1]=0;
    return slots;
}
// 원본 필드/전체 목록을 관찰 순서로 직렬화한다. 기대 상태는 계산하지 않는다.
std::vector<std::string> Snapshot(const RawRenderRegistry& registry,const RenderProcessFields& fields) {
    std::vector<std::string> slots,values;
    // 빈 칸을 포함한 전체 배열의 순서를 비교한다.
    for (const auto token:registry.Slots()) slots.push_back(std::to_string(token));
    // 원본 signed 필드도 DWORD 비트 그대로 비교한다.
    for (const auto value:{static_cast<std::uint32_t>(fields.form),static_cast<std::uint32_t>(fields.parent),
        Bits(fields.left),Bits(fields.top),Bits(fields.right),Bits(fields.bottom),static_cast<std::uint32_t>(fields.slot),static_cast<std::uint32_t>(fields.zOrder)})
        values.push_back(std::to_string(value));
    return {Join(slots,':'),Join(values,':')};
}
// 실제 등록 목록·입력 raw 자료와 외부 사건을 소유한다.
struct LifetimeScene {
    SidPool pool; // 실제 판본 raw 부모 자료다.
    std::vector<std::uint32_t> input; // 관찰의 입력 열이다.
    std::vector<std::string> events; // 확보/반납/부착의 원본 호출 순서다.
    std::unique_ptr<RawRenderRegistry> registry; // 각 행의 초기 목록이다.
    // raw 깊이의 BYTE 위치가 다른 두 판본을 같은 검사에 사용한다.
    explicit LifetimeScene(OriginalEdition edition):pool(edition,32768,false) {}
    // 호출 인자와 등록이 이미 끝난 부착 시점을 관찰한다.
    RenderRegistryHooks Hooks() {
        RenderRegistryHooks hooks;
        // 원본 32비트 포인터 배열의 확보 바이트다.
        hooks.reserve=[this](std::uint32_t amount) { events.push_back("A:"+std::to_string(amount));return true; };
        // 원본의 기존 배열 반납은 확장했을 때만 발생한다.
        hooks.releaseArray=[this] { events.push_back("F:T"); };
        // 실제 BaseProcess 대신 명시 입력으로 부모/form을 공급한다. 원본과 같은 슬롯/초기 깊이를 기록한다.
        hooks.attach=[this](RenderProcessFields& fields,std::uint32_t type,Sid parent,std::uint32_t flags) {
            CHECK(registry->Slots()[fields.slot]==kToken && type==17 && flags==16);
            events.push_back("B:"+std::to_string(type)+':'+std::to_string(parent.value)+":0:"+std::to_string(flags)+':'+std::to_string(fields.slot)+':'+std::to_string(static_cast<std::uint32_t>(fields.zOrder)));
            fields.form=7;fields.parent=input[Mutate]==2 ? 51 : parent.value;
            if (input[Mutate]==1) InputRaw(pool,parent)[pool.Edition()==OriginalEdition::Patch1078 ? 35 : 33]=128;
        };return hooks;
    }
    // 기대 결과 없이 합성 슬롯/부모 깊이와 포인터가 있는 버퍼만 준비한다.
    RenderProcessFields Prepare(std::string_view operation,const std::string& values) {
        events.clear();input=Inputs(values);const auto z=pool.Edition()==OriginalEdition::Patch1078 ? 35U : 33U;
        InputRaw(pool,{})[z]=static_cast<std::uint8_t>(input[Z0]);InputRaw(pool,kParent)[z]=static_cast<std::uint8_t>(input[Z]);InputRaw(pool,{51})[z]=127;
        auto slots=Initial(input);RenderProcessFields fields;
        if (operation!="Construct" && operation!="Sequence") {
            slots.at(input[Slot])=kToken;fields.token=kToken;fields.slot=static_cast<std::int32_t>(input[Slot]);fields.form=7;fields.parent=50;
            fields.zOrder=-128;fields.left=fields.top=fields.right=fields.bottom=Float(0xa5a5a5a5);
        }
        registry=std::make_unique<RawRenderRegistry>(pool,Hooks(),std::move(slots));return fields;
    }
    // 반납 경계에서 아직 등록이 살아 있는지, 객체 반납 때 이미 제거되었는지도 확인한다.
    void Release(ConstructionRelease kind,const RenderProcessFields& fields) {
        const auto label=kind==ConstructionRelease::Source ? "S" : kind==ConstructionRelease::Output ? "O" : kind==ConstructionRelease::Particles ? "P" : "B";
        CHECK(registry->Slots()[fields.slot]==(kind==ConstructionRelease::Object ? 0U : kToken));events.push_back(std::string("F:")+label);
    }
};
// 한 판본의 전체 등록/삭제/연속 재사용 관찰을 C++ 몸체와 대조한다.
void Replay(std::string_view edition) {
    LifetimeScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 기대 필드/목록/호출 순서는 독립 PE fixture에서 읽는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);auto fields=scene.Prepare(row[1],row[2]);std::vector<std::string> history;std::uint32_t result=0;
        if (row[1]=="Construct" || row[1]=="Sequence") {
            scene.registry->Construct(fields,kToken,{static_cast<std::uint16_t>(scene.input[Parent])},17);result=1;
            if (row[1]=="Sequence") {
                history.push_back(Join(Snapshot(*scene.registry,fields),'|'));scene.registry->Destroy(fields);history.push_back(Join(Snapshot(*scene.registry,fields),'|'));
                scene.registry->Construct(fields,kToken,{static_cast<std::uint16_t>(scene.input[Parent])},17);history.push_back(Join(Snapshot(*scene.registry,fields),'|'));
            }
        } else if (row[1]=="Render") scene.registry->Destroy(fields);
        else {
            ConstructionVisualFields visual;
            if (scene.input[Buffers]&1) visual.image.pixels={17};if (scene.input[Buffers]&2) visual.output={31};if (scene.input[Buffers]&4) visual.particles.resize(1);
            DestroyConstructionVisual(visual,fields,*scene.registry,row[1]=="Build" ? 0 : scene.input[Flags],
                [&](ConstructionRelease kind) { scene.Release(kind,fields); });
            CHECK(visual.image.pixels.capacity()==0 && visual.output.capacity()==0 && visual.particles.capacity()==0);
            if (row[1]!="Build") result=1;
        }
        const auto observed=Snapshot(*scene.registry,fields);
        const bool same=Join(scene.events,';')==row[3] && observed[0]==row[4] && observed[1]==row[5] && Join(history,'~')==row[6] && result==Number(row[7]);CHECK(same);
        if (!same) { std::printf("건설 표시 수명 %s 행 %zu %s 불일치\n",std::string(edition).c_str(),count,row[1].c_str());break; }++count;
    }
    CHECK(count==459);
}
}
// 패치판의 실제 등록/삭제 명령 관찰이다.
TEST_CASE(ConstructionLifetime_PatchNative) { Replay("originals"); }
// CD판의 내부 helper/파생 소멸자를 포함한 독립 관찰이다.
TEST_CASE(ConstructionLifetime_CdNative) { Replay("originalCD"); }
// 추가 판본도 별도 PE 관찰로 비교한다.
TEST_CASE(ConstructionLifetime_AdditionalNative) { Replay("original1037"); }

// 잘못된 등록·0 토큰·경계 누락은 정상 목록을 손상시키지 않고 거부한다.
TEST_CASE(ConstructionLifetime_RejectsInvalidRegistration) {
    LifetimeScene scene(OriginalEdition::Patch1078);auto fields=scene.Prepare("Construct","40,1,50,128,255,0,7,0,19");
    CHECK(Throws([&] { scene.registry->Construct(fields,0,kParent,17); }));scene.registry->Construct(fields,kToken,kParent,17);
    const auto before=Snapshot(*scene.registry,fields);CHECK(Throws([&] { scene.registry->Construct(fields,kToken,kParent,17); }));CHECK(Snapshot(*scene.registry,fields)==before);
    CHECK(Throws([&] { scene.registry->Construct(fields,999,kParent,17); }));CHECK(Snapshot(*scene.registry,fields)==before);
    auto invalid=fields;invalid.slot=-1;CHECK(Throws([&] { scene.registry->Destroy(invalid); }));invalid=fields;invalid.token=999;
    CHECK(Throws([&] { scene.registry->Destroy(invalid); }));CHECK(Snapshot(*scene.registry,fields)==before);
    scene.registry->Destroy(fields);CHECK(Throws([&] { scene.registry->Destroy(fields); }));
    auto hooks=scene.Hooks();hooks.attach={};CHECK(Throws([&] { RawRenderRegistry registry(scene.pool,hooks); }));
    CHECK(Throws([&] { MakeConstructionRegistryHooks(*scene.registry,fields,0,{}); }));
}

// 확보가 실패하면 기존 배열/토큰을 보존하고 Kernel 부착을 요청하지 않는다.
TEST_CASE(ConstructionLifetime_AllocationFailureKeepsSlots) {
    LifetimeScene scene(OriginalEdition::Patch1078);scene.input={1,0,50,128,255,0,7,0,0};auto hooks=scene.Hooks();hooks.reserve=[](std::uint32_t) { return false; };
    RawRenderRegistry registry(scene.pool,hooks,{99});RenderProcessFields fields;
    CHECK(Throws([&] { registry.Construct(fields,kToken,kParent,17); }));CHECK(registry.Slots().size()==1 && registry.Slots()[0]==99 && scene.events.empty());
}

// 실제 Create/Run/Draw에 같은 등록을 연결하고 abstract 종료의 Kill로 버퍼·목록을 함께 해제한다.
TEST_CASE(ConstructionLifetime_CreateRunKillAndReuse) {
    // 두 판본과 두 생성 방식에서 동일한 소유권 흐름을 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        LifetimeScene scene(edition);scene.input={0,0,50,128,255,0,7,0,0};scene.registry=std::make_unique<RawRenderRegistry>(scene.pool,scene.Hooks());
        auto raw=InputRaw(scene.pool,kParent);Put(raw,0,edition==OriginalEdition::Patch1078 ? 0x501dd0 : 0x5058d8);raw[10]=100;raw[11]=0;raw[edition==OriginalEdition::Patch1078 ? 34 : 32]=1;
        Put(raw,14,Bits(10.0F));Put(raw,18,Bits(20.0F));Put(raw,26,0);
        GameRandom random;ConstructionVisualState visualState;visualState.palette=71;ConstructionFrameState state;
        const std::array<std::uint32_t,12> colors{0,1,2,3,4,5,6,7,8,9,10,11};ConstructionFramePalette palette{71,11,colors};
        // 빈 저장소를 처음 만들 때도 최소 40칸을 확보한다. 같은 토큰은 삭제 후 다시 쓸 수 있다.
        for (const bool instant:{false,true}) {
            RenderProcessFields registration;ConstructionVisualHooks hooks;
            // 생성의 확보/팔레트 경계는 입력만 공급하며 등록 경계는 실제 몸체로 대체한다.
            hooks.reserve=[](std::uint32_t) { return true; };hooks.createPalette=[] { return 71U; };
            hooks.gradient=[](std::uint32_t,std::array<double,3>,std::array<double,3>,double) {};hooks.buildPalette=[](std::uint32_t) {};
            hooks.capture=[](Sid) { return ConstructionVisualImage{5,7,2,3,std::vector<std::uint8_t>(35,17)}; };hooks.releaseParticles=[] {};
            RawConstructionVisual visual(scene.pool,visualState,random,MakeConstructionRegistryHooks(*scene.registry,registration,kToken,hooks));
            ConstructionFrameFields fields;fields.visual=visual.Create(kParent,instant);CHECK(registration.slot==0 && scene.registry->Slots()[0]==kToken && fields.visual.parent==kParent);
            bool destroyed=false;std::size_t draws=0;ConstructionFrameHooks frameHooks;
            // 부착 뒤 프레임의 HP/dirty/Draw는 실제 몸체를 거친다. 장치 출력은 검사 경계다.
            frameHooks.maxHitPoints=[](Sid) { return 101; };frameHooks.invalidate=[](ConstructionFrameRect) {};
            frameHooks.draw=[&](const ConstructionFrameDraw& request) { CHECK(request.pixels.size()==35);++draws; };
            // Run의 마지막 Kill을 실제 소유 버퍼/등록 해제에 연결한다. 원본과 같이 flags는 0이다.
            frameHooks.kill=[&](std::uint32_t flags) { CHECK(flags==0);DestroyConstructionVisual(fields.visual,registration,*scene.registry,flags,
                [&](ConstructionRelease kind) { scene.Release(kind,registration); });destroyed=true; };
            RawConstructionFrame frame(scene.pool,visualState,state,palette,random,frameHooks);
            raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=1;frame.Run(fields);frame.Draw(fields);CHECK(draws==1 && !destroyed);
            raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=0;scene.events.clear();frame.Run(fields);
            CHECK(destroyed && scene.registry->Slots()[0]==0 && registration.form==-12345 && registration.parent==-12345);
            CHECK(Join(scene.events,';')=="F:S;F:O;F:P" && fields.visual.image.pixels.capacity()==0 && fields.visual.output.capacity()==0 && fields.visual.particles.capacity()==0);
            CHECK(visualState.palette==71 && fields.visual.palette==71 && scene.registry->Slots().size()==40);
        }
    }
}
