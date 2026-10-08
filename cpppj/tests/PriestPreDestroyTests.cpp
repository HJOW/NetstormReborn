// 독립 원본 preDestroy 관찰과 실제 공통 삭제·회복 form/Kernel 정리를 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPreDestroy.h"
#include "o/RawPriestForcefield.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidFactory.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 각 실제 PE에서 얻은 관찰 수다. Python/원본 없이 CTest에서 읽는다.
constexpr std::size_t kRowsPerEdition=2184;
// 공간/파생 보호막 효과를 대체하는 합성 자산의 원본 타입 번호다.
constexpr std::uint8_t kForcefieldType=167;
// 독립 fixture는 입력과 기대 출력을 분리해 그대로 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_PRIESTDESTROY_FIXTURE);return data.rows; }
struct PriestScene {
    SidPool pool;
    PriestPostPopState state;
    std::string events,after;
    Sid found{};
    bool patch;
    int validations{};
    // 실제 client 할당으로 관찰 대상 번호를 확보한다.
    explicit PriestScene(OriginalEdition edition):pool(edition,kCapacity,false),patch(edition==OriginalEdition::Patch1078) {
        // 각 행에서 같은 번호를 재사용한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 현재 목록/extra는 외부 호출 시점마다 새로 읽는다.
    void Record(char marker,Sid sid,std::string flags={}) {
        if (!events.empty()) events+=';';
        events+=std::string(1,marker)+':'+std::to_string(sid.value)+flags+':'+std::to_string(state.priests.count)+':'+
            std::to_string(pool.Slot(kSource)[patch ? 40 : 35]);
    }
    // lookup/destroy/Carrier 몸체만 원본 실행기와 같은 명시적 대체로 공급한다.
    PriestPreDestroyHooks Hooks() {
        return {[this](Sid sid) {
            Record('F',sid);
            if (after!="-") pool.AllocatedBytes(sid)[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(after));
            return found;
        },[this](Sid sid,std::uint32_t flags) { Record('D',sid,':'+std::to_string(flags)); },
        [this](Sid,std::uint32_t) { ++validations; },
        [this](Sid sid,std::uint32_t flags) { Record('C',sid,':'+std::to_string(flags)); }};
    }
    // 입력 칸만 raw/목록에 넣는다. 기대 압축·분기는 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==12);events.clear();validations=0;
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{0xab});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;
        raw[11]=static_cast<std::uint8_t>(Number(row[2]));raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[1]));
        state.priests.entries.clear();state.priests.count=Number(row[4]);
        // 비활성 꼬리를 포함한 목록 저장소를 보존한다.
        for (const auto& item:Split(row[5],',')) state.priests.entries.push_back(Number(item));
        found={static_cast<std::uint16_t>(Number(row[6]))};after=row[7];
    }
    // 목록 저장소 전체를 원본 관찰과 비교할 문자열로 만든다.
    std::string Entries() const {
        std::string result;
        // 개수 밖 값도 원본에서 남는 stale 메모리로 검사한다.
        for (const auto item:state.priests.entries) { if (!result.empty()) result+=',';result+=std::to_string(item); }
        return result;
    }
};
// 같은 클래스의 두 raw 배치로 실제 세 PE의 독립 출력을 모두 재생한다.
void Replay(std::string_view edition) {
    PriestScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==3*kRowsPerEdition);
    // 각 행의 목록/가상 호출/raw 슬롯 전체를 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);RawPriestPreDestroy priest(scene.pool,scene.state,scene.Hooks());priest.PreDestroy(kSource,Number(row[3]));
        const bool same=scene.state.priests.count==Number(row[8]) && scene.Entries()==row[9] && scene.events==row[10] && Hex(scene.pool.Slot(kSource))==row[11];
        CHECK(same && scene.validations==1);
        if (!same) { std::printf("사제 pre %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[10].c_str(),scene.events.c_str());break; }
        ++count;
    }
    CHECK(count==kRowsPerEdition);
}
}
// 실제 patch preDestroy와 Array::remove의 전체 결과다.
TEST_CASE(priest_predestroy_patch_x86) { Replay("originals"); }
// CD는 inline 압축 뒤 보호막 삭제 helper를 실제 실행한다.
TEST_CASE(priest_predestroy_cd_x86) { Replay("originalCD"); }
// 추가 10.37 실제 PE 관찰은 별도 근거로 재생한다.
TEST_CASE(priest_predestroy_1037_x86) { Replay("original1037"); }

// 잘못된 raw/목록/미연결 하위 경계는 목록 변경과 외부 호출 전에 거부한다.
TEST_CASE(priest_predestroy_guards_and_dispatch) {
    // 판본별 raw 위치와 같은 풀 계약을 함께 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestScene scene(edition);scene.Prepare(Fixture().front());auto hooks=scene.Hooks();hooks.carrierPre={};
        CHECK(Throws([&] { RawPriestPreDestroy invalid(scene.pool,scene.state,hooks); }));
        RawPriestPreDestroy priest(scene.pool,scene.state,scene.Hooks());
        // 거부 전에 변경된 슬롯/목록이 없어야 한다.
        const auto reject=[&](Sid sid) {
            const auto before=Hex(scene.pool.Slot(kSource));const auto entries=scene.Entries();const auto count=scene.state.priests.count;
            CHECK(Throws([&] { priest.PreDestroy(sid,0); }));
            CHECK(scene.events.empty() && scene.state.priests.count==count && scene.Entries()==entries && Hex(scene.pool.Slot(kSource))==before);
        };
        reject({0});reject({51});reject({24000});reject({65535});
        // free를 만든 뒤 AllocatedBytes로 다시 쓰지 않도록 기존 확보 범위에서 복구한다.
        auto raw=scene.pool.AllocatedBytes(kSource);raw[11]=1;reject(kSource);raw[11]=8;reject(kSource);
        scene.pool.AllocatedBytes(kSource)[11]=0;Put(scene.pool.AllocatedBytes(kSource),0,kVtable);reject(kSource);
        Put(scene.pool.AllocatedBytes(kSource),0,scene.patch ? kPatchPriestVtable : kCdPriestVtable);
        scene.state.priests.count=7;reject(kSource);scene.state.priests.count=6;
        hooks=scene.Hooks();hooks.validateCarrier=[](Sid,std::uint32_t) { throw std::logic_error("미복원 Carrier 거부"); };
        const auto entries=scene.Entries();
        RawPriestPreDestroy blocked(scene.pool,scene.state,hooks);CHECK(Throws([&] { blocked.PreDestroy(kSource,0); }));
        CHECK(scene.events.empty() && scene.state.priests.count==6 && scene.Entries()==entries);
        // 추상 사제는 목록을 읽지 않으므로 손상된 활성 개수도 이 접두에서 접근하지 않는다.
        scene.state.priests.count=7;scene.pool.AllocatedBytes(kSource)[scene.patch ? 40 : 35]=9;
        priest.PreDestroy(kSource,123);CHECK(scene.events=="C:50:123:7:9" && scene.state.priests.count==7 && scene.Entries()==entries);
        SidPool other(edition,kCapacity,false);int forwarded=0;
        SquidDestroyHooks fallback{[&](const SquidDestroyEvent&) { ++forwarded; },[] { return Sid{}; }};
        CHECK(Throws([&] { MakePriestPreDestroyHooks(other,priest,fallback); }));
        auto dispatch=MakePriestPreDestroyHooks(scene.pool,priest,fallback);
        dispatch.emit({SquidDestroyEffect::PostDestroy,kSource,{},1});CHECK(forwarded==1);
        dispatch.emit({SquidDestroyEffect::PreDestroy,Sid{51},{},1});CHECK(forwarded==2 && dispatch.selected()==Sid{});
        CHECK(Throws([&] { MakePriestPreDestroyHooks(scene.pool,priest,{}); }));
    }
}

// void 자산의 삭제에 실제 장부·종속 form·Kernel·SID 반납을 연결한다. 공간 및 파생 Carrier/보호막 효과는 대체다.
TEST_CASE(priest_predestroy_cleans_real_regen_form_kernel_and_common_bookkeeping) {
    // 두 판본과 server/client SID의 깊이/전파 분기를 각각 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) for (bool server:{false,true}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,32768,server);const auto initialFree=pool.FreeCount();
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);auto& type=types[kPriestType];
        type.constructorAddress=TypeConstructorAddress(edition,kPriestType);type.flags1=0x69012;type.flags2=0x210000;type.maxHitPoints=200;type.cost=123;
        type.footX=type.footY=1;types[kForcefieldType].cost=20;types[kForcefieldType].footX=types[kForcefieldType].footY=1;
        SquidFactory factory(pool,types);const auto root=factory.Create(kPriestType,server ? 0U : 2U);
        const auto child=pool.Allocate(server ? 0U : 2U);pool.AllocatedBytes(child)[10]=kForcefieldType;
        pool.AllocatedBytes(root)[patch ? 34 : 32]=1;pool.AllocatedBytes(child)[patch ? 34 : 32]=1;
        // 일반 Pop 미복원 범위는 유지하며 합성 void 자산의 실제 공간 조회 입력을 등록한다.
        for (auto sid:{root,child}) { Put(pool.AllocatedBytes(sid),14,std::bit_cast<std::uint32_t>(20.75f));Put(pool.AllocatedBytes(sid),18,std::bit_cast<std::uint32_t>(21.9f)); }
        SquidPostPopState book;book.localOwner=1;book.totalCost=10;book.depth=2;
        SquidPostPop post(pool,types,book);post.PostPopBase(root,1);post.PostPopBase(child,1);
        CHECK(book.totalCost==153 && book.globalCounts[kPriestType]==1 && book.globalCounts[kForcefieldType]==1);
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
        SquidDeletionState deletion;int transmitted=0,forceRequests=0,carrierCalls=0;
        SquidDestroyLifecycle lifecycle(pool,types,book,deletion,destroy,{
            [](const SquidDeletionEvent&) {},[&](const SquidDestroyEvent& event) { CHECK(event.effect==SquidDestroyEffect::Transmit);++transmitted; }});
        hash.Bucket(1,20.75f,21.9f)=child.value;
        PriestForcefieldState forcefieldState;RawPriestForcefield lookup(pool,hash,types,forcefieldState);
        PriestPostPopState priests;priests.priests.entries.resize(3);SquidProcessHost* hostPointer=nullptr;
        RawPriestPreDestroy pre(pool,priests,MakePriestForcefieldHooks(pool,lookup,{
            {},[&](Sid sid,std::uint32_t flags) {
                CHECK(sid==child && flags==0 && priests.priests.count==0 && (pool.Slot(root)[11]&2));
                // 같은 dead 사제를 중첩 삭제해도 새 pre/종속/장부 효과가 생기지 않는다.
                destroy.Destroy(root,0,hostPointer->Hooks());++forceRequests;destroy.Destroy(sid,flags,hostPointer->Hooks());
            },
            [&](Sid sid,std::uint32_t flags) { lifecycle.ValidatePre(sid,flags);lifecycle.ValidatePost(sid,flags); },
            [&](Sid sid,std::uint32_t flags) {
                CHECK(sid==root && (pool.Slot(child)[11]&1) && destroy.PreDepth()==1);++carrierCalls;
                // Carrier/Damageable의 미복원 파생 효과를 대체하고 실제 공통 장부만 호출한다.
                lifecycle.PreDestroy(sid,flags);
            }}));
        Kernel kernel;SquidProcessState processState;
        SquidProcessHost host(pool,types,kernel,destroy,processState,{},MakePriestPreDestroyHooks(pool,pre,lifecycle.Hooks()));hostPointer=&host;
        GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidRewardState hpState;
        SquidReward hp(pool,types,book,hpState,patch ? &store : nullptr);
        RawPriestPostPop prefix(pool,hp,priests,MakePriestPostPopProcessHooks(host));prefix.Prefix(root,1);
        auto* regular=host.FindEvent(root,kPriestRegenEvent);CHECK(regular && kernel.Size()==1 && priests.priests.count==1);
        const Sid form=regular->Form();CHECK(pool.FreeCount()==initialFree-3);
        destroy.Destroy(root,0x200000,host.Hooks());
        CHECK(forceRequests==1 && carrierCalls==1 && priests.priests.count==0 && kernel.Size()==0);
        CHECK((pool.Slot(root)[11]&1) && (pool.Slot(child)[11]&1) && (pool.Slot(form)[11]&1));
        CHECK(pool.FreeCount()==initialFree && destroy.PreDepth()==0 && destroy.PostDepth()==0);
        CHECK(book.totalCost==10 && book.localCounts[kPriestType]==0 && book.globalCounts[kPriestType]==0 && book.globalCounts[kForcefieldType]==0);
        CHECK(book.localSecondaryCounts[kPriestType]==1 && transmitted==(server ? 2 : 0) && !store.Initialized());
    }
}
