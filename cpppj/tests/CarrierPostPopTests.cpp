// 실제 세 PE의 carrier/damageable 전체 몸체와 사제 예약·공통 장부의 합성을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierPostPop.h"
#include "o/RawPriestPostPop.h"
#include "o/SquidFactory.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 판본별 전체 몸체 독립 관찰 수다.
constexpr std::size_t kRowsPerEdition=2448;
// 실행 때 원본 PE/Ghidra/Python 없이 저장된 fixture만 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto fixture=LoadFixture(NETSTORM_CARRIERPOSTPOP_FIXTURE);return fixture.rows;
}
struct CarrierScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    CarrierPostPopState state;
    std::vector<std::string> events;
    int afterType{},afterExtra{},afterOwner{},groundOwner{};
    std::uint32_t validations{};
    bool patch{};
    // 관찰 SID 50을 확보한다. 실제 client 번호 영역을 사용한다.
    explicit CarrierScene(OriginalEdition edition):pool(edition,kCapacity,false),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 최초 client 번호부터 대상까지 확보한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 판본별 extra와 현재 소유자 필드 위치다.
    std::size_t Extra() const { return patch ? 40 : 35; }
    std::size_t Owner() const { return patch ? 34 : 32; }
    // 입력만 준비하며 기대 분기/호출 순서는 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==19);events.clear();validations=0;
        state.boss=Number(row[3])!=0;state.loadingDepth=Number(row[4]);state.geyserType=Number(row[6]);
        // 가상 효과 뒤 타입 재조회에서도 같은 입력 flags를 사용한다.
        for (const auto number:{82U,122U,158U}) { types[number].flags1=Number(row[8]);types[number].flags2=0x00210000; }
        afterType=std::stoi(row[13]);afterExtra=std::stoi(row[14]);afterOwner=std::stoi(row[15]);groundOwner=std::stoi(row[16]);
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=static_cast<std::uint8_t>(Number(row[5]));
        raw[11]=static_cast<std::uint8_t>(Number(row[9]));raw[Extra()]=static_cast<std::uint8_t>(Number(row[7]));
        raw[Owner()]=static_cast<std::uint8_t>(Number(row[10]));Put(raw,12,0xab84,2);Put(raw,14,Number(row[11]));Put(raw,18,Number(row[12]));
    }
    // 원본의 외부 경계와 같은 위치에서 사건/필드 쓰기를 주입한다. 보호 검사는 사건으로 세지 않는다.
    CarrierPostPopHooks Hooks() {
        return {
            [this](Sid sid) {
                CHECK(validations==1);events.push_back("C:"+std::to_string(sid.value));
                if (afterType>=0) {
                    auto raw=pool.AllocatedBytes(sid);raw[10]=static_cast<std::uint8_t>(afterType);raw[Extra()]=static_cast<std::uint8_t>(afterExtra);
                    raw[Owner()]=static_cast<std::uint8_t>(afterOwner);Put(raw,14,0x41f20000);Put(raw,18,0x41fc0000);
                }
            },
            [this](float x,float y,std::uint8_t owner) {
                CHECK(validations==1);
                events.push_back("G:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(owner));
                if (groundOwner>=0) pool.AllocatedBytes(kSource)[Owner()]=static_cast<std::uint8_t>(groundOwner);
            },
            [this](Sid sid,std::uint32_t) { CHECK(sid==kSource);++validations; },
            [this](Sid sid,std::uint32_t flags) {
                CHECK(validations==1);const auto raw=pool.Slot(sid);
                events.push_back("B:"+std::to_string(sid.value)+':'+std::to_string(flags)+':'+std::to_string(raw[10])+':'+
                    std::to_string(raw[Extra()])+':'+std::to_string(raw[Owner()]));
            }
        };
    }
    // 원본 경계의 사건을 같은 순서/인자로 기록한다.
    std::string Events() const {
        std::string result;
        // 사건 사이에만 구분자를 넣는다.
        for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }
        return result;
    }
};
// 각 판본의 사건·좌표 비트·효과 뒤 raw 슬롯 전체를 독립 원본과 비교한다.
void Replay(const char* edition) {
    CarrierScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    CHECK(Fixture().size()==3*kRowsPerEdition);std::size_t count=0;int failures=0;
    // fixture가 정한 직접 호출 종류를 그대로 실행한다.
    for (const auto& row:Fixture()) {
        if (row[1]!=edition) continue;
        scene.Prepare(row);RawCarrierPostPop carrier(scene.pool,scene.types,scene.state,scene.Hooks());
        if (row[0]=="Carrier") carrier.PostPop(kSource,Number(row[2]));else carrier.DamageablePostPop(kSource,Number(row[2]));
        const bool same=scene.Events()==row[17] && Hex(scene.pool.Slot(kSource))==row[18];CHECK(same);
        if (!same) {
            std::printf("  %s carrier 행 %zu: 기대 %s / 실제 %s\n",edition,count,row[17].c_str(),scene.Events().c_str());
            if (++failures>=6) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==kRowsPerEdition);
}
}
// 10.78의 전체 carrier/damageable 흐름을 비교한다.
TEST_CASE(carrier_postpop_patch_x86_full_body) { Replay("originals"); }
// CD의 별도 타입 getter 및 전체 호출 흐름을 비교한다.
TEST_CASE(carrier_postpop_cd_x86_full_body) { Replay("originalCD"); }
// 추가 10.37 PE 관찰은 별도로 재생한다.
TEST_CASE(carrier_postpop_1037_x86_full_body) { Replay("original1037"); }

// 손상 raw/타입/누락 경계와 공통 사전 검사의 실패가 첫 효과 전에 거부되는지 확인한다.
TEST_CASE(carrier_postpop_rejects_invalid_inputs_before_external_effects) {
    // 두 판본의 같은 보호 계약을 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        CarrierScene scene(edition);scene.Prepare(Fixture().front());
        CHECK(Throws([&] { RawCarrierPostPop invalid(scene.pool,scene.types,scene.state,{}); }));
        CHECK(Throws([&] { RawCarrierPostPop invalid(scene.pool,std::span(scene.types).first(100),scene.state,scene.Hooks()); }));
        auto hooks=scene.Hooks();hooks.validateBase={};CHECK(Throws([&] { RawCarrierPostPop invalid(scene.pool,scene.types,scene.state,hooks); }));
        RawCarrierPostPop carrier(scene.pool,scene.types,scene.state,scene.Hooks());
        // 실패 전후 raw 슬롯/기록을 비교한다.
        const auto reject=[&](Sid sid) {
            const auto before=Hex(scene.pool.Slot(kSource));CHECK(Throws([&] { carrier.PostPop(sid,0); }));
            CHECK(Hex(scene.pool.Slot(kSource))==before && scene.events.empty() && scene.validations==0);
        };
        reject(Sid{0});reject(Sid{51});scene.pool.AllocatedBytes(kSource)[10]=255;reject(kSource);
        scene.pool.AllocatedBytes(kSource)[10]=158;scene.pool.AllocatedBytes(kSource)[11]=8;reject(kSource);
        scene.pool.AllocatedBytes(kSource)[11]=0;
        hooks=scene.Hooks();hooks.validateBase=[](Sid,std::uint32_t) { throw std::logic_error("공통 경계 거부 입력"); };
        RawCarrierPostPop rejected(scene.pool,scene.types,scene.state,hooks);const auto before=Hex(scene.pool.Slot(kSource));
        CHECK(Throws([&] { rejected.PostPop(kSource,0); }));CHECK(Hex(scene.pool.Slot(kSource))==before && scene.events.empty());
    }
}

// 직접 공통 호출은 기존 장부를 사용하지만 일반 가상 Pop/Activate의 사제 지원 범위는 늘어나지 않는다.
TEST_CASE(carrier_postpop_direct_base_preserves_virtual_guards_and_bookkeeping) {
    // 실제 판본별 비용 인코딩과 상태 칸을 합성한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        CarrierScene scene(edition);auto row=Fixture().front();row[3]="1";row[4]="1";row[7]="0";row[8]="431122";row[10]="1";
        scene.Prepare(row);scene.types[kPriestType].cost=123.75f;
        SquidPostPopState state;state.localOwner=1;state.totalCost=10;state.depth=1;SquidPostPop base(scene.pool,scene.types,state);
        CHECK(Throws([&] { base.Validate(kSource,1); }));CHECK(Throws([&] { base.Activate(kSource,1); }));
        CHECK(Throws([&] { base.PostPop(kSource,1); }));CHECK(state.depth==1 && state.totalCost==10);
        int claimed=0;std::uint32_t xbits=0,ybits=0;
        auto hooks=MakeCarrierPostPopHooks(scene.pool,base,[](Sid) {},[&](float x,float y,std::uint8_t owner) {
            ++claimed;xbits=std::bit_cast<std::uint32_t>(x);ybits=std::bit_cast<std::uint32_t>(y);CHECK(owner==1);
        });
        RawCarrierPostPop carrier(scene.pool,scene.types,scene.state,hooks);carrier.PostPop(kSource,1);
        CHECK(claimed==1 && xbits==Number(row[11]) && ybits==Number(row[12]));
        CHECK(state.depth==0 && state.totalCost==133 && state.localCounts[kPriestType]==1 && state.localSecondaryCounts[kPriestType]==1);
        CHECK(state.globalCounts[kPriestType]==1 && state.providers.count==0 && state.factories.count==0);
        // AI 미복원 보호는 지면/가상 효과 전에 적용한다.
        state.aiAttached[1]=true;state.depth=1;CHECK(Throws([&] { carrier.PostPop(kSource,1); }));
        CHECK(claimed==1 && state.depth==1 && state.totalCost==133 && state.globalCounts[kPriestType]==1);
        // 억제된 공통 몸체도 한 번만 깊이를 줄인다. 억제는 damageable 지면 효과를 끄지 않는다.
        state.suppressed=true;carrier.DamageablePostPop(kSource,1);CHECK(claimed==2 && state.depth==0 && state.totalCost==133);
        SidPool other(edition,kCapacity,false);
        CHECK(Throws([&] { MakeCarrierPostPopHooks(other,base,[](Sid) {},[](float,float,std::uint8_t) {}); }));
    }
}

// 실제 사제 생성자/소유자/회복 Form과 carrier→공통 장부를 순서대로 연결한다.
TEST_CASE(carrier_postpop_composes_priest_prefix_process_and_real_common_body) {
    // 두 슬롯 배치의 server/client 생성 번호 경로를 모두 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) for (const bool server:{false,true}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,32768,server);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);auto& type=types[kPriestType];
        type.constructorAddress=TypeConstructorAddress(edition,kPriestType);type.flags1=0x00069012;type.flags2=0x00210000;
        type.maxHitPoints=200;type.cost=123.75f;SquidFactory factory(pool,types);const auto priest=factory.Create(kPriestType,server ? 0U : 2U);
        SquidPostPopState book;book.localOwner=1;book.totalCost=10;book.depth=1;
        SquidOwnerMode mode;mode.battle=true;PriestOwnerState ownerState{1,1,1};SquidOwner baseOwner(pool,types,book,mode);
        RawPriestOwner owner(pool,mode,ownerState,{[&](Sid sid,std::uint32_t player) { baseOwner.Set(sid,player); },[](Sid) {}});
        owner.Set(priest,1);pool.AllocatedBytes(priest)[11]=0;
        GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidRewardState hpState;
        SquidReward hp(pool,types,book,hpState,patch ? &store : nullptr);
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
        Kernel kernel;SquidProcessState processState;processState.now=12.5;SquidProcessHost host(pool,types,kernel,destroy,processState);
        PriestPostPopState priestState;priestState.priests.entries.resize(2);priestState.ownerState.fill(8);
        RawPriestPostPop prefix(pool,hp,priestState,MakePriestPostPopProcessHooks(host));
        SquidPostPop base(pool,types,book);CarrierPostPopState carrierState;carrierState.loadingDepth=1;int checks=0,claims=0;
        RawCarrierPostPop carrier(pool,types,carrierState,MakeCarrierPostPopHooks(pool,base,[&](Sid sid) { CHECK(sid==priest);++checks; },
            [&](float,float,std::uint8_t) { ++claims; }));
        // 일반 사제 Pop은 아직 지원하지 않으므로 명시적인 복원 몸체 합성만 진행한다.
        CHECK(Throws([&] { base.Validate(priest,1); }));prefix.Prefix(priest,1);carrier.PostPop(priest,1);
        auto* regen=host.FindEvent(priest,kPriestRegenEvent);CHECK(regen && regen->Payload()==8.25f && regen->Time()==12.5);
        CHECK(kernel.Size()==1 && priestState.priests.count==1 && priestState.ownerState[1]==0 && checks==0 && claims==0);
        CHECK(book.depth==0 && book.totalCost==133 && book.localCounts[kPriestType]==1 && book.globalCounts[kPriestType]==1);
        // 두 번째 비최초 호출은 비권한 carrier 확인→억제된 공통 몸체로 이어지고 기존 회복을 유지한다.
        carrierState.boss=false;book.suppressed=true;book.depth=8;prefix.Prefix(priest,0);carrier.PostPop(priest,0);
        CHECK(checks==1 && claims==0 && book.depth==7 && book.totalCost==133 && book.globalCounts[kPriestType]==1);
        CHECK(host.FindEvent(priest,kPriestRegenEvent)==regen && kernel.Size()==1 && !store.Initialized());
        host.Kill(*regen,0x10);CHECK(kernel.Size()==0);
    }
}
