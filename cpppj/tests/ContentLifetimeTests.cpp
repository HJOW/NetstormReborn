// 특수 내용물 삭제의 실제 PE 출력과 공유 Form 분배/경제 저장소 연결을 검사한다.
#include "ConstructionSupport.h"
#include "o/SquidFormDispatch.h"

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 두 PE 관찰의 같은 풀 크기와 판본별 사례 수다.
constexpr std::uint32_t kPoolCapacity=32768,kCasesPerEdition=198;
// 모든 watched 슬롯의 명시 합성 입력만 복원한다. 출력 기대값은 fixture에서 읽는다.
void Input(SidPool& pool,const std::string& slots) {
    // 각 raw 슬롯의 모든 바이트를 손실 없이 입력한다.
    for (const auto& item:Split(slots,';')) {
        const auto field=Split(item,':');auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(Number(field[0]))});
        if (field[1].size()!=raw.size()*2) throw std::runtime_error("Content fixture 입력 폭 오류");
        // free/서버 번호도 포함한 합성 입력을 놓는다.
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(std::stoul(field[1].substr(index*2,2),nullptr,16));
    }
}
// watched 밖의 전체 풀은 zlib Adler-32와 비교한다. 큰 합을 블록별로 줄여 빠르게 계산한다.
std::uint32_t PoolAdler(std::span<const std::uint8_t> bytes) {
    // 원본 관찰과 같은 소수와 안전한 누적 블록 크기다.
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;std::size_t block=0;
    // 전체 raw 저장 순서대로 합산한다.
    for (const auto byte:bytes) { a+=byte;b+=a;if (++block==4096) { a%=prime;b%=prime;block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// 외부 효과 당시 전체 슬롯/전역/현재 타입 비용을 관찰한다.
std::string Snapshot(const SidPool& pool,Sid form,Sid parent,const ContentLifetimeState& state,std::span<const RiftTypeRecord> types) {
    std::array<std::uint8_t,4> cost{};Put(cost,0,Bits(types[state.nuggetType].cost));
    return Hex(pool.Slot(form))+':'+Hex(pool.Slot(parent))+':'+std::to_string(state.localOwner)+':'+std::to_string(state.boss ? 1 : 0)+':'+std::to_string(state.nuggetType)+':'+Hex(cost);
}
// 세미콜론 효과 열의 빈 첫 항목을 만들지 않는다.
void Add(std::string& events,std::string event) { if (!events.empty()) events+=';';events+=event; }
// 부모 삭제 중 자신의 Form을 같은 분배기로 지우는 프로세스다. 객체를 삭제한 뒤 멤버를 다시 읽지 않는다.
class SelfDeletingProcess final:public SquidProcess {
public:
    // 세 카운터는 이 객체보다 오래 살아야 한다.
    SelfDeletingProcess(SquidFormDispatch& dispatch,std::array<std::uint32_t,3>& counts):dispatch_(dispatch),counts_(counts) {}
    // Kernel의 실제 객체 소멸도 확인한다.
    ~SelfDeletingProcess() override { ++counts_[2]; }
    // 같은 Form의 가상 preDestroy로 들어온 통지를 센다.
    void OnFormDestroy(std::uint32_t) override { ++counts_[1]; }
    // 상대 부모 release 중 현재 프로세스만 중첩 삭제한다. 부모가 선저장한 다음 종속은 지우지 않는다.
    void OnParentDying(Sid) override { const auto form=Form();++counts_[0];dispatch_.Destroy(form,0x10); }
private:
    SquidFormDispatch& dispatch_;
    std::array<std::uint32_t,3>& counts_;
};
}

// 실제 두 PE의 효과/콜백 뒤 재조회·전체 슬롯·공통 카운터/풀 출력에 C++를 직접 대조한다.
TEST_CASE(ContentLifetime_IndependentPeFixture) {
    std::ifstream input(NETSTORM_CONTENTLIFETIME_FIXTURE);if (!input) throw std::runtime_error("Content 수명 fixture 없음");
    std::string line;std::array<std::uint32_t,2> counts{};
    // 각 루트는 독립적인 명시 입력 상태에서 시작한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=14) throw std::runtime_error("Content 수명 열 수 오류");
        const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");++counts[patch ? 0 : 1];
        SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kPoolCapacity,row[2]=="1");Input(pool,row[8]);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);types[154].cost=Float(Number(row[5]));types[90].footX=3;types[90].footY=5;
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
        const Sid form{80},parent{static_cast<std::uint16_t>(Get(pool.Slot(form),16,2))};
        ContentLifetimeState state{Sid{static_cast<std::uint16_t>(Number(row[6]))},154,Number(row[3]),0x1234,row[4]=="1"};
        // CD는 없는 너겟 전역의 합성 기본값만 관찰한다.
        if (!patch) { state.localOwner=1;state.boss=true; }
        const auto mutation=Number(row[7]);std::string effects,notices;
        ContentFormState formState;
        if (mutation>=100) { formState.inBattle=((mutation-100)&4)!=0;types[80].flags2=((mutation-100)&2) ? 0x200000U : 0U;types[90].flags2=((mutation-100)&1) ? 0x10000U : 0U; }
        ContentFormHost forms(pool,types,[&](Sid notifiedParent,Sid notifiedForm,std::uint32_t reason) {
            Add(notices,"content:"+std::to_string(notifiedParent.value)+':'+std::to_string(notifiedForm.value)+':'+std::to_string(reason)+':'+Hex(pool.Slot(notifiedParent))+':'+Hex(pool.Slot(notifiedForm)));
        },{},&formState,[&](Sid object,std::uint32_t previousOwner) { Add(effects,"capture:"+std::to_string(object.value)+':'+std::to_string(previousOwner)+':'+Snapshot(pool,form,parent,state,types)); });
        ContentLifetimeHooks hooks;
        // 취소 경계는 원본 관찰에서 명시한 현재 cursor0/kind 변경만 수행한다.
        hooks.cancelCursor=[&] { Add(effects,"cancel:"+Snapshot(pool,form,parent,state,types));state.cursor={};if (patch && mutation==1) Put(InputRaw(pool,form),18,154,2); };
        // 로컬 알림이 바꾼 paid/owner/dead/kind는 후속 실제 C++ 몸체가 재조회한다.
        hooks.localNotice=[&](std::uint32_t command) {
            Add(effects,"local:"+std::to_string(command)+':'+Snapshot(pool,form,parent,state,types));
            if (mutation==2) InputRaw(pool,form)[22]=1;
            else if (mutation==3) InputRaw(pool,parent)[34]=3;
            else if (mutation==7) InputRaw(pool,parent)[11]=2;
            else if (mutation==8) Put(InputRaw(pool,form),18,153,2);
        };
        // 원격 송신의 실제 인자와 당시 상태를 기록한다. 네트워크는 실행하지 않는다.
        hooks.remoteNotice=[&](std::uint32_t owner) { Add(effects,"remote:"+std::to_string(owner)+':'+Snapshot(pool,form,parent,state,types)); };
        // 선지급 지갑 경계는 cost/owner/localOwner만 합성 입력대로 변경한다.
        hooks.wallet=[&](std::uint32_t owner,std::int32_t amount) {
            Add(effects,"wallet:"+std::to_string(owner)+':'+std::to_string(std::bit_cast<std::uint32_t>(amount))+':'+Snapshot(pool,form,parent,state,types));
            if (mutation==4) { InputRaw(pool,parent)[34]=2;types[154].cost=125.75f; }
            else if (mutation==5) state.localOwner=2;
        };
        // SP 요청은 로컬/다른 소유자 경로를 구분하고 이후 중심 계산의 현재 좌표 입력을 바꾼다.
        hooks.addSp=[&](std::uint32_t owner,float amount,bool local) {
            Add(effects,"sp:"+std::to_string(owner)+':'+std::to_string(Bits(amount))+':'+std::to_string(local ? 1 : 0)+':'+Snapshot(pool,form,parent,state,types));
            if (mutation==6) { Put(InputRaw(pool,parent),14,Bits(200.5f));Put(InputRaw(pool,parent),18,Bits(60.25f)); }
        };
        // 실제 중심 함수의 결과 float 비트를 비교한다.
        hooks.sound=[&](float x,float y) { Add(effects,"sound:"+std::to_string(Bits(x))+':'+std::to_string(Bits(y))+':'+Snapshot(pool,form,parent,state,types)); };
        ContentFormLifetime lifetime(pool,types,forms,destroy,state,hooks);std::uint32_t result=0;
        if (row[1]=="Destroy") lifetime.Destroy(form,0x10);
        else if (row[1]=="Linked") result=forms.Linked(form) ? 1U : 0U;
        else if (row[1]=="Attach") forms.Attach(form,parent,0x10);
        else throw std::runtime_error("Content fixture 연산 오류");
        CHECK(result==Number(row[10]));CHECK(effects==row[12]);CHECK(notices==row[13]);
        // watched 슬롯은 체크섬이 아니라 전체 원본 바이트와 직접 비교한다.
        for (const auto& item:Split(row[9],';')) { const auto field=Split(item,':');CHECK(Hex(pool.Slot(Sid{static_cast<std::uint16_t>(Number(field[0]))}))==field[1]); }
        const auto meta=Inputs(row[11]);const std::array<std::uint32_t,11> actual{pool.Capacity(),pool.FreeCount(),pool.PredictableCursor(),pool.FirstFree(true).value,
            pool.Tail(true).value,pool.FirstFree(false).value,pool.Tail(false).value,PoolAdler(pool.Bytes()),destroy.PreDepth(),destroy.PostDepth(),0};
        // 정상 반환 후 세 공통 깊이도 별도로 대조한다.
        for (std::size_t index=0;index<actual.size();++index) CHECK(actual[index]==meta.at(index));
    }
    CHECK(counts[0]==kCasesPerEdition);CHECK(counts[1]==kCasesPerEdition);
}

// 너겟 삭제는 기존 실제 SP/AI 지갑 저장소를 사용하며 패치 선가산과 CD 무지급 차이를 유지한다.
TEST_CASE(ContentLifetime_ActualSpAndWalletIntegration) {
    // 같은 내용물 생성/삭제를 두 판본에서 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kPoolCapacity);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[154].cost=50.75f;types[90].footX=3;types[90].footY=5;SquidFactory factory(pool,types);
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
        GameRandom random(7);ScrambledSpStore store(random,[] { return 42U; });store.Initialize();store.Set(1,Bits(1000));
        SquidPostPopState books;books.localOwner=1;books.aiAttached[1]=true;SquidRewardState rewardState;rewardState.cdLocalSp=1000;
        std::uint32_t refreshed=0;SquidReward reward(pool,types,books,rewardState,edition==OriginalEdition::Patch1078 ? &store : nullptr,{{},[&] { ++refreshed; }});
        const auto parent=factory.Create(90,2);auto parentRaw=pool.AllocatedBytes(parent);parentRaw[edition==OriginalEdition::Patch1078 ? 34 : 32]=1;
        Put(parentRaw,14,Bits(20));Put(parentRaw,18,Bits(30));std::vector<std::uint32_t> reasons;
        ContentFormHost forms(pool,types,[&](Sid object,Sid,std::uint32_t reason) { CHECK(object==parent);reasons.push_back(reason); });
        const auto form=factory.CreateContained(2);Put(pool.AllocatedBytes(form),18,154,2);Put(pool.AllocatedBytes(form),20,0x20,2);forms.Attach(form,parent,0x10);
        ContentLifetimeState state;std::uint32_t notified=0,sounded=0;ContentLifetimeHooks hooks;
        hooks.localNotice=[&](std::uint32_t) { ++notified; };
        hooks.sound=[&](float x,float y) { ++sounded;CHECK(x==18.5f);CHECK(y==27.5f); };
        ContentFormLifetime lifetime(pool,types,forms,destroy,state,hooks,{},&reward);lifetime.Destroy(form,0x10);
        const bool patch=edition==OriginalEdition::Patch1078;CHECK((patch ? Float(store.Get(1)) : rewardState.cdLocalSp)==(patch ? 1050.75f : 1000));
        CHECK(rewardState.aiWallet[1]==(patch ? 100 : 0));CHECK(notified==(patch ? 1U : 0U));CHECK(sounded==(patch ? 1U : 0U));CHECK(refreshed==(patch ? 1U : 0U));
        CHECK(reasons==std::vector<std::uint32_t>({1,2}));CHECK(Get(pool.Slot(parent),6,2)==0);CHECK(pool.Deletions(true)[0].type==6);
    }
}

// 알림 뒤 체인에서 빠진 사제 내용물은 포획 알림을 생략한다. 목록 flags가 0이어도 정상 연결이면 알린다.
TEST_CASE(ContentLifetime_PriestNoticeUsesCurrentStructuralLinks) {
    SidPool pool(OriginalEdition::Patch1078,kPoolCapacity);std::vector<RiftTypeRecord> types(188);types[90].flags2=0x10000;types[80].flags2=0x200000;
    SquidFactory factory(pool,types);const auto parent=factory.Create(90,2);ContentFormState state{true,false};std::uint32_t notices=0;
    bool detach=true;ContentFormHost* binding=nullptr;
    ContentFormHost forms(pool,types,[&](Sid object,Sid form,std::uint32_t reason) { if (reason==1 && detach) { CHECK(object==parent);binding->Unpop(form); } },{},&state,
        [&](Sid object,std::uint32_t) { CHECK(object==parent);++notices; });binding=&forms;
    const auto form=factory.CreateContained(2);Put(pool.AllocatedBytes(form),18,80,2);forms.Attach(form,parent,0x10);
    CHECK(!forms.Linked(form));CHECK(notices==0);detach=false;forms.Attach(form,parent,0x10);CHECK(forms.Linked(form));CHECK(!forms.Robust(form));CHECK(notices==1);
}

// 한 부모의 감시·Gump·프로세스·Content가 자기 알림에서 삭제돼도 같은 저장소/깊이/선저장 next를 공유한다.
TEST_CASE(ContentLifetime_SharedMixedFormDispatch) {
    SidPool pool(OriginalEdition::Patch1078,kPoolCapacity);std::vector<RiftTypeRecord> types(188);SquidFactory factory(pool,types);
    SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
    const auto root=factory.Create(90,2),watcher=factory.Create(91,2);Sid selected=root;std::uint32_t assetRoots=0,dependNotices=0,gumpNotices=0,contentRemovals=0;
    SquidDestroyHooks assets;assets.selected=[&] { return selected; };assets.emit=[&](const SquidDestroyEvent& event) {
        if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();
        else if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();
        else if (event.effect==SquidDestroyEffect::ClearSelection) selected={};
        else CHECK(event.effect==SquidDestroyEffect::Transmit);
    };
    SquidFormDispatch dispatch(pool,destroy,assets,[&](Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) { ++assetRoots;destroy.Destroy(sid,flags,hooks);return true; });
    ContentFormHost content(pool,types,[&](Sid,Sid,std::uint32_t reason) { if (reason==2) ++contentRemovals; });ContentLifetimeState contentState;
    ContentFormLifetime lifetime(pool,types,content,destroy,contentState,{},dispatch.Hooks());dispatch.Bind(lifetime);
    std::array<std::uint32_t,3> counts{};Kernel kernel;SquidProcessState processState;SquidProcessHost processes(pool,types,kernel,destroy,processState,{},dispatch.Hooks());dispatch.Bind(processes);
    Sid gumpForm{};GumpFormHost gumps(pool,factory,types,destroy,[&](std::uint32_t owner,Sid object) { CHECK(owner==0xabcdef12);CHECK(object==root);++gumpNotices;dispatch.Destroy(gumpForm,0x10); },dispatch.Hooks());dispatch.Bind(gumps);
    DependFormHost depends(pool,destroy,[&](Sid notifiedWatcher,Sid watched,Sid form) { CHECK(notifiedWatcher==watcher);CHECK(watched==root);++dependNotices;dispatch.Destroy(form,0x10); },dispatch.Hooks());dispatch.Bind(depends);
    const auto value=factory.CreateContained(2);Put(pool.AllocatedBytes(value),18,80,2);content.Attach(value,root,0x10);
    auto* process=processes.Attach(std::make_unique<SelfDeletingProcess>(dispatch,counts),46,root,0x10);const auto processForm=process->Form();
    gumpForm=gumps.Create(2);gumps.SetOwner(gumpForm,0xabcdef12);gumps.Attach(gumpForm,root);const auto pair=depends.Create(root,watcher);
    CHECK(dispatch.Destroy(root,0x10));CHECK(assetRoots==1);CHECK(!selected.value);CHECK(kernel.Size()==0);CHECK((counts==std::array<std::uint32_t,3>{1,1,1}));
    CHECK(dependNotices==1);CHECK(gumpNotices==1);CHECK(contentRemovals==0);CHECK(Get(pool.Slot(watcher),6,2)==0);
    // 모든 Form과 루트가 같은 SID 반납/삭제 기록에 들어간다.
    for (const auto sid:{root,value,processForm,gumpForm,pair.watchedForm,pair.watcherForm}) CHECK((pool.Slot(sid)[11]&3)==3);
    CHECK(destroy.PreDepth()==0);CHECK(destroy.PostDepth()==0);CHECK(Throws([&] { dispatch.Bind(gumps); }));
}
