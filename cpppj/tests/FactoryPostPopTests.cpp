// 두 PE의 Factory/아웃포스트·실제 finder·프레임/목록/현재 상태를 재생한다.
#include "RawSceneSupport.h"
#include "o/RawFactoryPostPop.h"
#include "o/SquidFrame.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 입력 글자 구간만 공급하며 프레임 보정 기대값은 native가 계산한다.
const std::vector<FrameCode> kFactoryCodes{{'A','A',0,0},{'A','A',1,0},{'B','A',0,0},{'B','A',1,0},{'B','A',2,0},{'H','A',0,0},{'H','A',1,0}};
// 실제 같은 풀·해시·프레임과 callback 입력을 묶은 검사 장면이다.
struct FactoryScene {
    bool patch; // raw 필드 폭과 vtable을 선택하는 판본이다.
    SidPool pool; // 관찰 대상 SID 슬롯을 확보·수정한다.
    SquidHash hash; // 일반 finder가 읽는 실제 네 공간 해시다.
    std::vector<RiftTypeRecord> types; // 현재 타입의 발자국/장부 입력이다.
    std::vector<RiftTypeFrames> frames; // 현재 상태별 첫 프레임/개수 구간이다.
    SquidPostPopState baseState; // 공통 깊이를 정확히 한 번 감소시키는 장부다.
    CarrierPostPopState carrierState; // 직접 Damageable의 권한/로딩 상태다.
    AssetPostPopState state; // 기존 거주 구역과 공유하는 목록/권한이다.
    std::vector<std::string> events; // 원본 명시 효과의 호출 순서다.
    std::uint32_t reserveMask{},alloc{},mutation{}; // 확보 성공 비트·현재 확보 위치·callback 변경 입력이다.
    // native의 client 번호50/60/61을 확보하며 미션 초기화 자체를 검증하지는 않는다.
    explicit FactoryScene(bool chosen):patch(chosen),pool(chosen ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false),
        types(chosen ? 188 : 171),frames(types.size(),RiftTypeFrames(kFactoryCodes)) {
        // 관찰 번호를 빠짐없이 확보한다.
        for (std::uint16_t sid=5;sid<=61;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        // finder의 현재 발자국을 입력하며 그림 크기와 구별한다.
        for (auto& type:types) { type.footX=1;type.footY=1; }
    }
    // extra는 패치40/CD35의 BYTE다.
    std::size_t Extra() const { return patch ? 40U : 35U; }
    // frame은 패치36 DWORD/CD34 BYTE다.
    std::size_t FrameOffset() const { return patch ? 36U : 34U; }
    // 생성자가 기록하는 vtable의 전체 시작 주소다.
    std::uint32_t Table(std::uint32_t kind) const { return patch ? (kind ? 0x509210U : 0x509148U) : (kind ? 0x503358U : 0x503298U); }
    // 입력 슬롯/체인/목록을 초기화하며 기대 결과는 만들지 않는다.
    void Prepare(std::span<const std::uint32_t> input) {
        CHECK(input.size()==11);events.clear();alloc=0;reserveMask=input[7];mutation=input[10];state={};baseState={};carrierState={};
        baseState.suppressed=true;carrierState.boss=true;state.boss=input[4]!=0;hash.Cell(1,25,26)=50;
        // 실제 finder의 next 저장 순서가 source→60→61이 되게 한다.
        for (const auto sid:{Sid{50},Sid{60},Sid{61}}) {
            auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            Put(raw,0,Table(input[0]));Put(raw,4,sid.value==50 ? 60U : sid.value==60 ? 61U : 0U,2);
            raw[10]=static_cast<std::uint8_t>(sid.value==61 ? (input[1]!=86 ? 86 : 87) : input[1]);
            raw[11]=static_cast<std::uint8_t>(sid.value==50 ? input[5]<<5 : 0);
            raw[Extra()]=static_cast<std::uint8_t>(sid.value==50 ? input[3] : sid.value==61 ? 8 : 0);
            Put(raw,14,std::bit_cast<std::uint32_t>(sid.value==50 ? 50.75f : sid.value==60 ? 50.25f : 50.5f));
            Put(raw,18,std::bit_cast<std::uint32_t>(sid.value==50 ? 52.5f : sid.value==60 ? 52.75f : 52.25f));
        }
        Put(pool.AllocatedBytes(kSource),FrameOffset(),input[6],patch ? 4U : 1U);
        // 중복 번호나 꽉 찬 capacity를 그대로 입력한다.
        for (auto* list:{&state.outposts,&state.residences}) { list->entries={input[9] ? 50U : 7U,8,9,10};list->count=input[8]; }
    }
    // 직접 Damageable가 실제 공통 장부를 호출하고 native 경계와 같은 사건을 남긴다.
    CarrierPostPopHooks DamageableHooks(SquidPostPop& base) {
        return {
            [](Sid) { throw std::logic_error("Factory는 Carrier 가상을 호출하지 않습니다"); },
            [](float,float,std::uint8_t) { throw std::logic_error("이 입력은 로딩 소유권을 호출하지 않습니다"); },
            [&base](Sid sid,std::uint32_t flags) { base.ValidateBase(sid,flags); },
            [this,&base](Sid sid,std::uint32_t flags) {
                events.push_back("D:"+std::to_string(flags));if (mutation==1) pool.AllocatedBytes(sid)[11]=7<<5;
                base.PostPopBase(sid,flags);
            }
        };
    }
    // native와 같은 프로세스·프레임/소유권·로그/삭제 관찰 경계를 제공한다.
    FactoryPostPopHooks Hooks() {
        FactoryPostPopHooks hooks;
        hooks.reserveRegular=[this] { events.push_back("A:40");return (reserveMask&(1U<<alloc++))!=0; };
        hooks.addRegular=[this](Sid sid,std::uint32_t event,float delay) {
            events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(delay)));
            if (mutation==2 && event==2) pool.AllocatedBytes(sid)[10]=87;
        };
        hooks.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) {
            events.push_back("F:"+std::to_string(static_cast<std::uint32_t>(frame))+':'+std::to_string(flags));
            Put(pool.AllocatedBytes(sid),FrameOffset(),static_cast<std::uint32_t>(frame),patch ? 4U : 1U);
        };
        hooks.regionOwnership=[this](float x,float y) {
            events.push_back("C:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y)));
            if (mutation==3) pool.AllocatedBytes(kSource)[10]=87;
        };
        hooks.warnDuplicate=[this](Sid,std::int32_t x,std::int32_t y) { events.push_back("W:"+std::to_string(static_cast<std::uint32_t>(x))+':'+std::to_string(static_cast<std::uint32_t>(y))); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) {
            events.push_back("X:"+std::to_string(sid.value)+':'+std::to_string(flags));pool.AllocatedBytes(sid)[11]|=2;
            if (mutation==4) { auto next=pool.AllocatedBytes(Sid{61});next[Extra()]=0;next[10]=pool.Slot(kSource)[10]; }
        };
        return hooks;
    }
    // 이전 목록 메모리와 callback 뒤 후보 상태까지 원본 형식으로 기록한다.
    std::vector<std::string> Output() const {
        std::string actions,lists,other;
        // 효과의 순서를 한 번에 대조한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        // capacity 전체의 stale 항목도 보존됐는지 확인한다.
        for (const auto* list:{&state.outposts,&state.residences}) {
            if (!lists.empty()) lists+='|';lists+=std::to_string(list->count)+':';
            // 활성 count 밖의 기존 항목도 기록한다.
            for (std::size_t i=0;i<list->entries.size();++i) { if (i) lists+=',';lists+=std::to_string(list->entries[i]); }
        }
        // 다음 후보는 삭제 뒤의 현재 타입/state/extra를 읽는다.
        for (const auto sid:{Sid{60},Sid{61}}) {
            if (!other.empty()) other+='|';const auto raw=pool.Slot(sid);
            other+=std::to_string(raw[10])+','+std::to_string(raw[11])+','+std::to_string(raw[Extra()]);
        }
        return {actions.empty() ? "-" : actions,Hex(pool.Slot(kSource)),lists,other};
    }
};
}
// 실제 기계어가 만든 독립 행을 같은 C++ 풀/해시의 전체 출력과 비교한다.
TEST_CASE(factoryPostPop_native_body_and_dynamic_finder) {
    const auto rows=LoadFixture(NETSTORM_FACTORYPOSTPOP_FIXTURE).rows;std::size_t checked=0;
    // 두 판본 모두 현재 입력 격자를 재생한다.
    for (const bool patch:{true,false}) {
        FactoryScene scene(patch);
        // 기대값을 구현으로 재계산하지 않고 fixture를 읽는다.
        for (const auto& row:rows) {
            if (row[0]!=(patch ? "originals" : "originalCD")) continue;
            CHECK(row.size()==6);std::vector<std::uint32_t> input;
            // native에 넘겼던 정수만 현재 장면에 공급한다.
            for (const auto& value:Split(row[1],',')) input.push_back(Number(value));
            scene.Prepare(input);SquidPostPop base(scene.pool,scene.types,scene.baseState);
            RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.DamageableHooks(base));
            RawFactoryPostPop factory(scene.pool,scene.hash,scene.types,scene.frames,base,damageable,scene.state,scene.Hooks());
            CHECK(!base.HandlesFactory(kSource));base.SetFactoryPostPop(&factory);CHECK(base.HandlesFactory(kSource));
            base.Activate(kSource,input[2]);CHECK(scene.baseState.depth==0);
            const auto output=scene.Output();const std::vector<std::string> expected(row.begin()+2,row.end());
            if (output!=expected) {
                // 최초 차이를 남기고 원본 기대값은 변경하지 않는다.
                for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i])
                    std::printf("Factory %s 입력%s 결과%zu 기대%s / 실제%s\n",row[0].c_str(),row[1].c_str(),i,expected[i].c_str(),output[i].c_str());
                CHECK(output==expected);return;
            }
            ++checked;
        }
    }
    CHECK(checked==rows.size());CHECK(checked>500);
}
// 다른 풀과 필요한 가상 효과 누락은 쓰기 전에 거부한다.
TEST_CASE(factoryPostPop_requires_effects_and_same_pool) {
    FactoryScene scene(true);const std::array<std::uint32_t,11> input{1,85,1,0,1,0,7,7,0,0,0};scene.Prepare(input);
    SquidPostPop base(scene.pool,scene.types,scene.baseState);
    RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.DamageableHooks(base));
    auto hooks=scene.Hooks();hooks.regionOwnership={};
    RawFactoryPostPop factory(scene.pool,scene.hash,scene.types,scene.frames,base,damageable,scene.state,hooks);
    base.SetFactoryPostPop(&factory);const auto before=Hex(scene.pool.Slot(kSource));bool rejected=false;
    try { base.Activate(kSource,1); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected);CHECK(Hex(scene.pool.Slot(kSource))==before);CHECK(scene.events.empty());CHECK(scene.baseState.depth==0);
    FactoryScene other(true);SquidPostPop foreign(other.pool,other.types,other.baseState);rejected=false;
    try { foreign.SetFactoryPostPop(&factory); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected);base.SetFactoryPostPop(nullptr);CHECK(!base.HandlesFactory(kSource));
}
// Factory의 구간 판단에서 실제 SquidFrame의 두 표시 갱신과 raw 프레임 쓰기까지 연결한다.
TEST_CASE(factoryPostPop_actual_frame_effect_and_pool_contract) {
    // 판본별 frame 폭과 단계 바이트를 동시에 확인한다.
    for (const bool patch:{true,false}) {
        FactoryScene scene(patch);const std::array<std::uint32_t,11> input{0,86,0,0,0,1,7,7,0,0,0};scene.Prepare(input);
        scene.pool.AllocatedBytes(kSource)[patch ? 33 : 31]=1;std::vector<std::uint32_t> observed;
        SquidFrameHooks frameHooks;
        frameHooks.frameSize=[](Sid,std::int32_t) { return std::array<float,2>{1,1}; };
        frameHooks.update=[&](Sid sid,std::uint32_t flags) { CHECK(sid==kSource);CHECK(flags==0);observed.push_back(Get(scene.pool.Slot(sid),scene.FrameOffset(),patch ? 4U : 1U)); };
        frameHooks.unpop=[](Sid,std::uint32_t) { throw std::logic_error("같은 해시 단계는 Unpop하지 않습니다"); };
        frameHooks.pop=[](Sid,float,float,std::uint32_t) { throw std::logic_error("같은 해시 단계는 Pop하지 않습니다"); };
        SquidFrame frame(scene.pool,scene.types,frameHooks);SquidPostPop base(scene.pool,scene.types,scene.baseState);
        RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.DamageableHooks(base));
        RawFactoryPostPop factory(scene.pool,scene.hash,scene.types,scene.frames,base,damageable,scene.state,MakeFactoryPostPopHooks(scene.pool,frame,scene.Hooks()));
        base.SetFactoryPostPop(&factory);base.Activate(kSource,0);CHECK(Get(scene.pool.Slot(kSource),scene.FrameOffset(),patch ? 4U : 1U)==2);
        CHECK(observed==std::vector<std::uint32_t>({7,2}));CHECK(scene.baseState.depth==0);
        FactoryScene other(patch);bool rejected=false;
        try { static_cast<void>(MakeFactoryPostPopHooks(other.pool,frame,{})); } catch (const std::invalid_argument&) { rejected=true; }
        CHECK(rejected);
    }
}
