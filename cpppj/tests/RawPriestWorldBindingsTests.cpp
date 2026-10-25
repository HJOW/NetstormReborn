// 사제와 보호막의 실제 풀/공간/Regular/삭제를 하나의 월드에서 검사한다.
#include "RawSceneSupport.h"
#include "client/RawPriestWorldBindings.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidFormDispatch.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/RawDamageableRelease.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 프로세스의 클라이언트 번호까지 signed SID 범위 안에 확보한다.
constexpr std::uint32_t kWorldCapacity=32768;
// 사제의 받침은 실제 일반 Pop으로 hash0/spot을 등록하는 합성 base 자산이다.
constexpr std::uint32_t kSupportType=100;
// 외부 표시 요청을 기록하되 실제 표시/공간 계산은 SquidDisplay에 맡긴다.
struct DisplaySink final:SquidDisplaySink {
    // updates: 갱신. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::size_t updates{};
    // 콘솔에서도 표시 계산을 실행한다.
    bool Suppressed() const override { return false; }
    // Renderer 효과 경계의 호출 횟수만 관찰한다.
    void Invalidate(SquidDisplayRect,std::uint32_t) override { ++updates; }
};
// 신규 바인딩 자체의 계산을 복제하지 않고 원본 복원 모듈들을 실제로 조합한다.
struct PriestWorld {
    // edition: 판본별 필드 배치와 동작을 선택하는 원본 버전. 입력 구성 또는 실행 결과 대조에 사용한다.
    OriginalEdition edition;
    // patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool patch;
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;
    // hash: 좌표별 객체 후보를 찾는 공간 해시. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidHash hash;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<RiftTypeRecord> types;
    // frames: 타입별 프레임 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<RiftTypeFrames> frames;
    // shapes: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<SquidDisplayShape> shapes;
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    // random: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom).
    GameRandom random;
    // store: 패치판이 플레이어별 SP를 32개의 9칸 DWORD 배열에 흩뿌려 보관하는 방식이다 (ScrambledSpStore).
    ScrambledSpStore store;
    // book: 등록 목록과 개수의 장부. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidPostPopState book;
    // rewardState: 보상 계산이 읽거나 갱신하는 판본별 전역과 AI 지갑이다. 패치 주소/CD 주소는 괄호에 둔다 (SquidRewardState).
    SquidRewardState rewardState;
    // ownerMode: 판본·서버·로딩 상태에 따른 소유권 처리 정책. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidOwnerMode ownerMode;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    RawPriestWorldState state;
    // deletion: 삭제. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidDeletionState deletion;
    // clock: 프레임 판정에 현재 게임 시각을 공급하는 시계. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidProcessState clock;
    // damageableState: 피해가능객체 상태. 입력 구성 또는 실행 결과 대조에 사용한다.
    DamageablePreDestroyState damageableState;
    // releaseState: 해제 상태. 입력 구성 또는 실행 결과 대조에 사용한다.
    DamageableReleaseState releaseState;
    // placementState: 배치 상태. 입력 구성 또는 실행 결과 대조에 사용한다.
    PriestPlacementState placementState;
    // effectInput: 공간 효과는 월드의 실제 가상 Pop/Unpop/Destroy를 받는다. 계산을 임의 기본값으로 대체하지 않는다 (RawPriestWorldEffects).
    RawPriestWorldEffects effectInput;
    // sink: 계산한 그림 요청을 실제 출력에 전달하는 대상. 입력 구성 또는 실행 결과 대조에 사용한다.
    DisplaySink sink;
    // ownerNotices: 소유자 통지. 입력 구성 또는 실행 결과 대조에 사용한다.
    // claims: 점유. 입력 구성 또는 실행 결과 대조에 사용한다.
    // soundLoads: 소리 로딩. 입력 구성 또는 실행 결과 대조에 사용한다.
    // soundAttachments: 소리 부착. 입력 구성 또는 실행 결과 대조에 사용한다.
    // notices: 통지. 입력 구성 또는 실행 결과 대조에 사용한다.
    // falls: 추락. 입력 구성 또는 실행 결과 대조에 사용한다.
    // audits: 감사. 입력 구성 또는 실행 결과 대조에 사용한다.
    // damageRequests: 피해 요청. 입력 구성 또는 실행 결과 대조에 사용한다.
    // healthRequests: 체력 요청. 입력 구성 또는 실행 결과 대조에 사용한다.
    // fallbackEvents: 대체처리 이벤트. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::size_t ownerNotices{},claims{},soundLoads{},soundAttachments{},notices{},falls{},audits{},damageRequests{},healthRequests{},fallbackEvents{};
    // reward: HP·SP·지갑 보상을 실제 장부에 적용하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidReward> reward;
    // display: 객체·프레임에서 실제 화면 출력 요청을 만드는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidDisplay> display;
    // postPop: 객체 Pop 이후 목록·공간 후처리를 수행하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidPostPop> postPop;
    // unpop: 객체의 공간 등록을 해제하는 Unpop 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidUnpop> unpop;
    // pop: 객체를 공간 지도에 등록하는 Pop 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidPop> pop;
    // factory: 타입에 맞는 객체를 생성하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidFactory> factory;
    // owner: 소유 플레이어 번호 또는 소유권 처리 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidOwner> owner;
    // frame: 사용할 그림 프레임 또는 프레임 처리 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidFrame> frame;
    // destroy: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<RawSquidDestroy> destroy;
    // common: 파생 타입 사이에 공유하는 기본 처리기 또는 상태. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidDestroyLifecycle> common;
    // bindings: 사용 순서: 생성→ProcessHost(RegularHandler/DestroyHooks) 생성→BindProcesses→후속 모듈 Bind다 (std::unique_ptr<RawPriestWorldBindings>).
    std::unique_ptr<RawPriestWorldBindings> bindings;
    // release: 해제. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<RawDamageableRelease> release;
    // damageable: 피해 가능한 객체의 HP·삭제·후처리 연결. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<RawDamageablePreDestroy> damageable;
    // placement: 최종 배치 허용 여부를 판정하는 처리기 또는 상태. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<RawPriestPlacement> placement;
    // dispatch: 타입별 서로 다른 fallback 사슬을 같은 분배기로 모아 혼합 종속 체인의 중첩 삭제를 지원한다 (std::unique_ptr<SquidFormDispatch>).
    std::unique_ptr<SquidFormDispatch> dispatch;
    // host: Form이나 프로세스의 등록·통지·수명을 연결하는 호스트. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidProcessHost> host;
    // kernel: 프로세스 등록과 프레임 실행을 담당하는 커널. 입력 구성 또는 실행 결과 대조에 사용한다.
    Kernel kernel;
    // 두 판본의 실제 생성자/타입 분류와 작은 SHP 입력을 사용한다.
    explicit PriestWorld(OriginalEdition chosen):edition(chosen),patch(chosen==OriginalEdition::Patch1078),pool(chosen,kWorldCapacity,true),
        types(patch ? 188 : 171),frames(types.size(),RiftTypeFrames({})),shapes(types.size()),store(random,[] { return 1U; }) {
        auto& priest=types[kPriestType];priest.maxHitPoints=200;priest.flags1=0x69012;priest.flags2=0x210000;
        priest.footX=priest.footY=1;priest.constructorAddress=TypeConstructorAddress(chosen,kPriestType);
        auto& field=types[kForcefieldType];field.footX=field.footY=1;field.constructorAddress=TypeConstructorAddress(chosen,kForcefieldType);
        types[kSupportType].flags1=TypeFlag1::kSurface;types[kSupportType].flags2=TypeFlag2::kIsland;
        types[kSupportType].footX=types[kSupportType].footY=1;
        frames[kSupportType]=RiftTypeFrames({{'A','P',1,0}});
        frames[kPriestType]=RiftTypeFrames({{'A','P',1,0},{'E','P',1,0},{'J','P',1,0},{'J','P',2,0}});
        frames[kForcefieldType]=RiftTypeFrames({{'A','P',1,0},{'A','P',2,0},{'A','P',3,0}});
        // 원본 프레임 메타 자료와 같은 타입 인덱스를 공유한다.
        for (const auto type:{kPriestType,static_cast<std::uint32_t>(kForcefieldType),kSupportType}) {
            shapes[type].loaded=true;shapes[type].frameCount=static_cast<int>(frames[type].Codes().size());
            const auto layers=(types[type].flags1&0x40000) ? 2U : 1U;
            shapes[type].frames.assign(frames[type].Codes().size()*layers,SquidDisplayFrame{16,11,8,11,1,1});
        }
        state.priests.priests.entries.resize(100);ownerMode.battle=true;book.localOwner=1;state.contained.boss=true;
        if (patch) {
            store.Initialize();store.Set(1,std::bit_cast<std::uint32_t>(1000.5f));
            state.regen.patch.sequence=std::bit_cast<std::int32_t>(store.Offset());
            state.regen.patch.observedSequence=state.regen.patch.sequence;
        }
        reward=std::make_unique<SquidReward>(pool,types,book,rewardState,patch ? &store : nullptr);
        display=std::make_unique<SquidDisplay>(chosen,types,shapes,sink,SquidDisplayView{0,0,65536,{0,0,640,480}});
        postPop=std::make_unique<SquidPostPop>(pool,types,book);
        unpop=std::make_unique<SquidUnpop>(pool,hash,spots,display.get());pop=std::make_unique<SquidPop>(pool,hash,spots,display.get(),postPop.get());
        factory=std::make_unique<SquidFactory>(pool,types,false,unpop.get());owner=std::make_unique<SquidOwner>(pool,types,book,ownerMode);
        frame=std::make_unique<SquidFrame>(pool,types,MakeSquidFrameHooks(pool,types,*display,*unpop,*pop));
        // 두 위치의 받침도 실제 Pop이 hash0/spot을 함께 쓴다. 별도 표면 지도 사본을 만들지 않는다.
        for (const auto point:{std::array<float,2>{30,31},std::array<float,2>{40,41}}) {
            const auto support=factory->Create(kSupportType);owner->Set(support,0);Pop(support,point[0],point[1]);
        }
        destroy=std::make_unique<RawSquidDestroy>(pool,*unpop,types);
        common=std::make_unique<SquidDestroyLifecycle>(pool,types,book,deletion,*destroy,SquidDeletionHooks{
            [this](const SquidDeletionEvent&) { ++damageRequests; },
            [](const SquidDestroyEvent& event) { CHECK(event.effect==SquidDestroyEffect::Transmit); }},nullptr,reward.get());
        RawPriestWorldEffects effects;
        effects.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Pop(sid,x,y,flags); };
        effects.unpop=[this](Sid sid,std::uint32_t flags) { unpop->Unpop(sid,types[pool.Slot(sid)[10]],flags); };
        effects.repop=[this](Sid sid,std::uint32_t flags) { const auto raw=pool.Slot(sid);Pop(sid,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),flags); };
        effects.destroy=[this](Sid sid,std::uint32_t flags) { CHECK(dispatch->Destroy(sid,flags)); };
        effects.claimGround=[this](float,float,std::uint8_t) { ++claims; };
        effects.ownerNotice=[this](Sid) { ++ownerNotices; };effects.notifySurface=[](Sid) { throw std::logic_error("사제 검사에서 다리 표면 통지는 미도달"); };
        effects.dependentFound=[] { throw std::logic_error("사제 검사에서 생산 종속 후처리는 미도달"); };effects.reserveRegular=[] { return true; };
        effects.shield.reserveSound=[] { return true; };
        effects.shield.loadSound=[this](std::string_view name) { CHECK(name=="priestForceField.wav");++soundLoads;return 17U; };
        effects.shield.attachSound=[this](Sid sid,std::uint32_t sound,std::uint32_t flags,std::uint32_t mode) {
            CHECK(pool.Slot(sid)[10]==kForcefieldType && sound==17 && flags==0 && mode==1);++soundAttachments;
        };
        effects.shield.noticeSound=[this](const PriestShieldNotice&) { ++notices; };
        effects.shield.translate=[](std::string_view key) { CHECK(key=="PriestImmobile");return std::string(key); };
        effects.shield.tell=[](std::string_view text) { CHECK(text=="PriestImmobile"); };
        effects.fallSound=[this](Sid,float,float) { ++falls; };
        effects.regen.refresh=[this](Sid sid) { display->Update(pool.Slot(sid)); };
        effects.regen.damageable=[this](Sid) { ++healthRequests; };
        effects.regen.shape=[](std::uint32_t) -> std::uint32_t { throw std::logic_error("소유 사제의 중립 모양 검사는 미도달"); };
        effects.regen.occupied=[](Sid,std::uint32_t) -> bool { throw std::logic_error("소유 사제의 중립 점유 검사는 미도달"); };
        effects.regen.neutralAction=[](Sid) { throw std::logic_error("소유 사제의 중립 동작은 미도달"); };
        effects.regen.patchAudit=[this] {
            store.Rotate();state.regen.patch.observedSequence=std::bit_cast<std::int32_t>(store.Offset());++audits;
        };
        effects.regen.measurement=[]() -> double { throw std::logic_error("측정 remaining 0 경계는 미도달"); };
        effectInput=effects;
        bindings=std::make_unique<RawPriestWorldBindings>(RawPriestWorldCore{pool,hash,types,frames,spots,hash.Entries(0),*reward,random,*factory,*owner,ownerMode,*postPop,*frame},state,std::move(effects));
        bindings->SetLoadingDepth(1);bindings->SetLocalPlayer(1);bindings->SetAuthority(true);
        dispatch=std::make_unique<SquidFormDispatch>(pool,*destroy,bindings->DestroyHooks(common->Hooks()));
        host=std::make_unique<SquidProcessHost>(pool,types,kernel,*destroy,clock,bindings->RegularHandler(
            [this](Sid,std::uint32_t,std::uint32_t,float payload) { ++fallbackEvents;return payload; }),dispatch->Hooks());
        dispatch->Bind(*host);bindings->BindProcesses(*host);
        releaseState.battle=true;
        release=std::make_unique<RawDamageableRelease>(pool,types,spots,state.contained,releaseState,
            MakePriestSpawnHooks(pool,bindings->Spawn(),DamageableReleaseHooks{{},
                [this](std::uint32_t type,std::uint32_t flags) { return factory->Create(type,flags); },
                [this](Sid sid,std::uint32_t player) { bindings->SetOwner(sid,player); },
                [this](Sid sid,float x,float y,std::uint32_t flags) { Pop(sid,x,y,flags); },
                [](Sid) { throw std::logic_error("사제 해방에서 다리 표면 통지는 미도달"); }}));
        damageable=std::make_unique<RawDamageablePreDestroy>(pool,spots,state.contained,damageableState,
            MakeDamageableReleaseHooks(pool,*release,DamageablePreDestroyHooks{
                [this](Sid) { ++damageRequests; },[this](Sid) { ++damageRequests; },[this](const DamageableSoundEvent&) { ++damageRequests; },{},
                [this](Sid sid,std::uint32_t flags) { common->ValidatePre(sid,flags); },
                [this](Sid sid,std::uint32_t flags) { common->PreDestroy(sid,flags); }}));
        bindings->BindDamageable(*damageable);
        placementState.forcePlacement=1;
        placement=std::make_unique<RawPriestPlacement>(pool,types,placementState,PriestPlacementHooks{
            [](std::uint32_t,std::uint32_t,std::uint32_t) -> PriestPlacementRect { throw std::logic_error("강제 배치의 모양 조회는 미도달"); },
            [](const PriestPlacementQuery&,bool) -> bool { throw std::logic_error("강제 배치의 지형 조회는 미도달"); }});
        bindings->BindPlacement(*placement);bindings->ValidateReady();
    }
    // 현재 프레임의 SHP 크기로 실제 raw Pop을 호출한다.
    void Pop(Sid sid,float x,float y,std::uint32_t flags=0) {
        const auto raw=pool.Slot(sid);const auto current=static_cast<int>(Get(raw,patch ? 36 : 34,patch ? 4 : 1));
        const auto size=display->FrameSize(edition,raw,current);
        CHECK(pop->Pop(sid,types[raw[10]],size[0],size[1],x,y,flags)==RawPopResult::Registered);
    }
    // 실제 factory/가상 소유자/Pop으로 사제를 생성한다. HP/좌표는 생성 body와 Pop만 쓴다.
    Sid Add(float x=30,float y=31) {
        const auto sid=factory->Create(kPriestType);bindings->SetOwner(sid,1);Pop(sid,x,y);return sid;
    }
};
}
// 실제 fullHP 사제 로딩이 단일 목록/소유자 상태/Regular/Form을 등록하고 재Pop도 중복하지 않는다.
TEST_CASE(RawPriestWorldBindings_LoadsFullHealthAndRunsActualRegular) {
    // 두 원본 판본의 필드 폭과 생성자 분기를 각각 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestWorld world(edition);const auto sid=world.Add();
        CHECK(world.bindings->PriestState().CurrentHitPoints(sid)==200);
        CHECK(world.bindings->Priests().priests.count==1 && world.bindings->Priests().priests.Items()[0]==sid.value);
        CHECK(&world.bindings->Priests()==&world.state.priests && world.state.priests.ownerState[1]==0);
        const auto regen=world.host->FindEvent(sid,kPriestRegenEvent);CHECK(regen && regen->Parent()==sid && world.kernel.Size()==1);
        CHECK(regen->Payload()==8.25f && world.bindings->Forcefield().Find(sid).value==0 && !world.bindings->CheckCarrier(sid));
        world.unpop->Unpop(sid,world.types[kPriestType],0);world.Pop(sid,30,31);
        CHECK(world.host->FindEvent(sid,kPriestRegenEvent)==regen && world.kernel.Size()==1 && world.state.priests.priests.count==1);
        world.bindings->SetLoadingDepth(0);world.clock.now=9;world.kernel.RunFrame();
        CHECK(regen->Count()==1 && regen->Time()==17.25 && world.bindings->PriestState().CurrentHitPoints(sid)==200);
        CHECK(world.audits==(world.patch ? 1U : 0U) && world.fallbackEvents==0 && world.book.depth==0);
        if (world.patch) CHECK(world.state.regen.patch.sequence==world.state.regen.patch.observedSequence &&
            world.store.Get(1)==std::bit_cast<std::uint32_t>(1000.5f));
    }
}
// 이동 불가/낙하/보호막이 실제 factory·finder·프레임·공간·Regular를 공유하고 삭제도 같은 분배기를 사용한다.
TEST_CASE(RawPriestWorldBindings_ShieldFallAndDestructionShareWorld) {
    // 패치와 CD 각각 같은 실제 연결 경로를 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestWorld world(edition);const auto sid=world.Add();world.bindings->SetLoadingDepth(0);
        world.pool.AllocatedBytes(sid)[world.patch ? 40 : 35]|=0x20;
        world.unpop->Unpop(sid,world.types[kPriestType],0);world.Pop(sid,30,31);
        const auto shield=world.bindings->Forcefield().Find(sid);CHECK(shield.value && world.kernel.Size()==3);
        CHECK(world.soundLoads==1 && world.soundAttachments==1 && world.notices==1);
        CHECK(world.bindings->FallWalker(sid) && world.falls==1 && world.kernel.Size()==4);
        CHECK(world.bindings->Forcefield().Find(sid)==shield && world.soundLoads==1);
        const auto support=world.hash.Cell(0,30,31);CHECK(support!=0);
        world.hash.Cell(0,30,31)=0;
        world.clock.now=0.02;world.kernel.RunFrame();
        CHECK(Get(world.pool.Slot(sid),world.patch ? 36 : 34,world.patch ? 4 : 1)==2);
        CHECK(world.host->FindEvent(sid,kPriestFallEvent)!=nullptr && world.kernel.Size()==4);
        // 같은 실제 hash0 복구를 낙하가 다음 이벤트에서 관찰하여 사본 캐시와 구별한다.
        world.hash.Cell(0,30,31)=support;world.clock.now=0.13;world.kernel.RunFrame();
        CHECK(Get(world.pool.Slot(sid),world.patch ? 36 : 34,world.patch ? 4 : 1)==1);
        CHECK(world.host->FindEvent(sid,kPriestFallEvent)==nullptr && world.kernel.Size()==3);
        CHECK(world.dispatch->Destroy(sid,0));
        CHECK(world.pool.Slot(sid)[11]&1);CHECK(world.pool.Slot(shield)[11]&1);
        CHECK(world.kernel.Size()==0 && world.state.priests.priests.count==0 && world.book.depth==0);
        CHECK(world.destroy->PreDepth()==0 && world.destroy->PostDepth()==0 && world.fallbackEvents==0);
    }
}
// 연결 누락/다른 월드의 호스트 및 미복원 사제 사건은 명시 진단하고 이후 정상 흐름을 유지한다.
TEST_CASE(RawPriestWorldBindings_RejectsWrongBindingAndUnknownPriestEvent) {
    PriestWorld world(OriginalEdition::Patch1078),other(OriginalEdition::Patch1078);
    const auto surfaces=world.hash.Entries(0);const std::vector<std::uint16_t> copy(surfaces.begin(),surfaces.end());
    CHECK(Throws([&] {
        RawPriestWorldBindings invalid({world.pool,world.hash,world.types,world.frames,world.spots,copy,*world.reward,
            world.random,*world.factory,*world.owner,world.ownerMode,*world.postPop,*world.frame},world.state,world.effectInput);
    }));
    CHECK(Throws([&] { world.bindings->BindProcesses(*other.host); }));
    CHECK(Throws([&] { world.bindings->BindDamageable(*other.damageable); }));
    CHECK(Throws([&] { world.bindings->BindPlacement(*other.placement); }));
    const auto sid=world.Add();CHECK(Throws([&] { world.bindings->RegularHandler()(sid,0x25c,0,1); }));
    CHECK(world.bindings->RegularHandler([&](Sid parent,std::uint32_t event,std::uint32_t count,float payload) {
        CHECK(parent==sid && event==0x25c && count==3);return payload;
    })(sid,0x25c,3,2.5f)==2.5f);
    world.bindings->ValidateReady();CHECK(world.state.priests.priests.count==1 && world.kernel.Size()==1);
}
// 해방 생성이 실제 나선/배치 접두/factory/WORD/가상 소유자/Pop/사제 목록/회복 예약을 연결한다.
TEST_CASE(RawPriestWorldBindings_SpawnUsesOriginalOwnerWordAndActualFactory) {
    // 두 판본의 사제 생성자·HP 폭을 각각 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestWorld world(edition);world.bindings->SetLoadingDepth(0);
        world.bindings->Spawn().Spawn(40,41,kPriestType,0x2082);
        CHECK(world.state.priests.priests.count==1);
        const Sid sid{static_cast<std::uint16_t>(world.state.priests.priests.Items()[0])};const auto raw=world.pool.Slot(sid);
        CHECK(raw[10]==kPriestType && raw[world.patch ? 34 : 32]==2 && Get(raw,12,2)==0x2002);
        CHECK(std::bit_cast<float>(Get(raw,14))==40 && std::bit_cast<float>(Get(raw,18))==41);
        CHECK(world.bindings->PriestState().CurrentHitPoints(sid)==200 && world.state.priests.ownerState[2]==0);
        CHECK(world.host->FindEvent(sid,kPriestRegenEvent)!=nullptr && world.kernel.Size()==1 && world.book.depth==0);
    }
}
// 전체 타입/balloon 우선순위/현재 이동 전역을 두 실제 PE의 독립 결과와 대조한다.
TEST_CASE(RawPriestWorldBindings_ReplaysActualMovementSelection) {
    const auto fixture=LoadFixture(NETSTORM_PRIESTWORLDEVENTS_FIXTURE);
    CHECK(fixture.rows.size()==(188+171)*8);
    // 판본당 하나의 실제 월드에서 mutable 선택 전역과 현재 타입 표를 공급한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestWorld world(edition);std::size_t count=0;
        // 기대 이동 프로세스는 C++ 입력 생성기가 계산하지 않고 fixture에서 읽는다.
        for (const auto& row:fixture.rows) {
            if (row[0]!=(world.patch ? "originals" : "originalCD")) continue;
            CHECK(row.size()==7);world.state.movement=RawMovementProcessTypes{};
            if (Number(row[1])) world.state.movement.priest=world.state.movement.windWalker;
            const auto shift=Number(row[2]);auto& movement=world.state.movement;
            movement.wormProcess+=shift;movement.walkerProcess+=shift;movement.balloonProcess+=shift;
            movement.teleportProcess+=shift;movement.bulfProcess+=shift;movement.dudeProcess+=shift;
            movement.priestProcess+=shift;movement.flyProcess+=shift;
            const auto type=Number(row[4]);world.types[type].flags2=Number(row[3]);
            if (Number(row[6])) CHECK(Throws([&] { world.bindings->MovementProcessType(type); }));
            else CHECK(world.bindings->MovementProcessType(type)==Number(row[5]));
            ++count;
        }
        CHECK(count==world.types.size()*8);
    }
}
// 회복의 절반 HP 공간 전환이 실제 보호막/예약을 정리하며 damageEffect 0 하위 효과를 호출하지 않는다.
TEST_CASE(RawPriestWorldBindings_HealingCrossesHalfHealthAndZeroDamageEffectReturns) {
    // 두 원본 HP 저장 폭을 각각 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestWorld world(edition);const auto sid=world.Add();world.bindings->SetLoadingDepth(0);
        world.state.hitPoints.recoveryAllowed[1]=true;world.bindings->SetHitPoints(sid,90);
        const auto shield=world.bindings->Forcefield().Find(sid);
        CHECK(shield.value && world.bindings->PriestState().Immobile(sid) && world.kernel.Size()==3);
        world.clock.now=9;world.kernel.RunFrame();
        CHECK(world.bindings->PriestState().CurrentHitPoints(sid)==123 && !world.bindings->PriestState().Immobile(sid));
        CHECK(world.bindings->Forcefield().Find(sid).value==0 && (world.pool.Slot(shield)[11]&1));
        CHECK(world.kernel.Size()==1 && world.healthRequests==0 && world.book.depth==0);
        world.types[kPriestType].damageEffect=1;world.bindings->DamageableRefresh(sid);CHECK(world.healthRequests==1);
        world.types[kPriestType].damageEffect=0;world.bindings->DamageableRefresh(sid);CHECK(world.healthRequests==1);
    }
}
// 실제 타입 속성 AST의 숫자 폭·절삭·대소문자·마지막 지정값을 damageEffect 필드에 전달한다.
TEST_CASE(RawPriestWorldBindings_DamageEffectMetadataPreservesNumericProperty) {
    // 두 판본 전체 로더가 같은 새 필드를 채운다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto definition=RiftTypeDefinition::Parse("typename test { damageEffect=-3.75; DAMAGEEFFECT=1.99; } A00:default:\"a.gif\"#0;");
        std::vector<RiftTypeSource> sources;
        // 원본 타입 로딩 순서만 공급한다. 기대 필드 값은 알려진 절삭 입력이다.
        for (const auto name:TypeLoadOrder(edition)) sources.push_back({name,&definition});
        const RiftTypeTable table(edition,sources);CHECK(table.Types()[70].damageEffect==1 && table.Types()[0].damageEffect==0);
        const auto invalid=RiftTypeDefinition::Parse("typename test { damageEffect=\"1\"; } A00:default:\"a.gif\"#0;");
        // 원본 숫자 assert 경로를 현재 입력 계약의 예외로 확인한다.
        for (auto& source:sources) source.definition=&invalid;
        CHECK(Throws([&] { RiftTypeTable checked(edition,sources); }));
    }
}

// 실제 네 소비자가 같은 boss 주소를 빌리고 다른 owner나 값형 setter로 대여를 교체하지 못한다.
TEST_CASE(RawPriestWorldBindings_SharedAuthorityAliasesAllLeavesAndRejectsForeignOwner) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 독립 월드의 두 판본도 같은 명시 바인딩을 사용한다.
        RawAuthorityState authority,foreign;authority.server=false;PriestWorld world(edition);world.bindings->BindAuthority(authority);world.bindings->BindAuthority(authority);
        CHECK(world.state.hitPoints.sharedAuthority==&authority.boss && world.state.fall.sharedAuthority==&authority.boss && world.state.carrier.sharedBoss==&authority.boss && world.state.contained.sharedBoss==&authority.boss);
        CHECK(Throws([&] { world.bindings->BindAuthority(foreign); }) && Throws([&] { world.bindings->SetAuthority(false); }) && authority.boss && !authority.server);
        const auto sid=world.Add();authority.boss=false;world.bindings->SetHitPoints(sid,1);CHECK(world.bindings->PriestState().CurrentHitPoints(sid)==100);authority.boss=true;world.bindings->SetHitPoints(sid,1);CHECK(world.bindings->PriestState().CurrentHitPoints(sid)==1);
    }
}

// 실제 Player9의 동일 주소만 연결하고 빈 입력·다른 폭·다른 owner의 재연결은 기존 대여를 보존한 채 거부한다.
TEST_CASE(RawPriestWorldBindings_PlayerRecoveryRejectsForeignRecords) {
    // 두 판본은 서로 다른 stride의 정확한 기록 폭을 요구한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestWorld world(edition);const std::size_t stride=world.patch ? 0xb4U : 0xacU;std::vector<std::uint8_t> players(9*stride),foreign(players.size());CHECK(Throws([&] { world.bindings->BindPlayerRecords({}); }) && world.state.hitPoints.playerRecords.empty());world.bindings->BindPlayerRecords(players);world.bindings->BindPlayerRecords(players);CHECK(Throws([&] { world.bindings->BindPlayerRecords(foreign); }) && Throws([&] { world.bindings->BindPlayerRecords(std::span<const std::uint8_t>(players).first(players.size()-1)); }) && world.state.hitPoints.playerRecords.data()==players.data() && world.state.hitPoints.playerRecords.size()==players.size());
    }
}

// 실제 Regular·Kernel의 회복이 신전 필드의 새 값을 읽고 보호막을 같은 삭제기로 정리한다.
TEST_CASE(RawPriestWorldBindings_PlayerRecoveryRunsActualRegenAndShieldCleanup) {
    // 원본 두 판본의 HP setter·공간·Form 폭을 그대로 조합한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestWorld world(edition);const std::size_t stride=world.patch ? 0xb4U : 0xacU,field=world.patch ? 0x7cU : 0x74U;std::vector<std::uint8_t> players(9*stride);world.bindings->BindPlayerRecords(players);const auto sid=world.Add();world.bindings->SetLoadingDepth(0);world.state.hitPoints.recoveryAllowed[1]=true;world.bindings->SetHitPoints(sid,90);const auto shield=world.bindings->Forcefield().Find(sid);CHECK(shield.value && world.kernel.Size()==3);world.bindings->SetHitPoints(sid,200);CHECK(world.bindings->PriestState().CurrentHitPoints(sid)==90 && world.bindings->Forcefield().Find(sid)==shield);Put(players,stride+field,0x10000);world.state.hitPoints.recoveryAllowed[1]=false;world.clock.now=9;world.kernel.RunFrame();CHECK(world.bindings->PriestState().CurrentHitPoints(sid)==123 && !world.bindings->PriestState().Immobile(sid) && !world.bindings->Forcefield().Find(sid).value && (world.pool.Slot(shield)[11]&1) && world.kernel.Size()==1 && world.healthRequests==0 && world.book.depth==0 && world.destroy->PreDepth()==0 && world.destroy->PostDepth()==0);
    }
}
