// 세 원본 PE의 건설 프로세스 시작/진행/완료/form 삭제 통지 관찰과 실제 host·Kernel·보상 모듈 연결을 검사한다.
// 이력: 2026-10-11 추가.
#include "ConstructionNoticeSupport.h"
#include "o/RawConstructionProcess.h"
#include "o/SpStore.h"
#include <cstdio>

namespace {
using namespace netstorm::test::construction;
// 실행기 입력 열의 순서다. 기대 계산에 사용하지 않고 장면 자료와 경계 응답만 공급한다.
enum Input : std::size_t { Parent,Duration,NowLow,NowHigh,Allocation,Mutate,Type,State,Extra,Hp,Owner,X,Y,Genus,Ready,Max,
    StartLow,StartHigh,EndLow,EndHigh,Local,Boss,PushGenus,Dais,FinishAll,FinishGenus,ProcessType,SoundType,Flags,InputCount };
// 각 PE의 독립 원본 행 수와 실행기의 고정 입력값(부착된 form 번호, 소리 항목, 경계 변동이 쓰는 값)이다.
constexpr std::size_t kRows=956;
constexpr std::uint32_t kForm=90,kSound=0x15000040,kChangedSound=0x15000200,kChangedReady=0x15000400;
// 저장 관찰만 읽는다. CTest에서 원본 게임·PE·Python은 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONPROCESS_FIXTURE).rows;return rows; }
// 시각 입력 비트를 손실 없이 double로 조합한다.
double Time(std::uint32_t low,std::uint32_t high) { return std::bit_cast<double>((static_cast<std::uint64_t>(high)<<32)|low); }
// 실제 풀과 입력 타입, 동작 밖 명시 효과의 관찰 장면이다.
struct ProcessScene {
    SidPool pool; // 부모 raw를 공급하는 월드다.
    std::vector<RiftTypeRecord> types; // 부모 현재 타입의 genus 입력이다.
    ConstructionProcessState state; // 전역 입력과 경계 변동 대상이다.
    ConstructionProcessFields fields; // attach 경계가 보관하는 프로세스 필드다.
    std::vector<std::uint32_t> input; // fixture의 29개 입력 DWORD다.
    std::vector<std::string> events; // 기대 순서를 만들지 않고 실제 훅 호출을 저장한다.
    std::uint32_t ready{}; // 타입 +0x90 준비 소리 입력이다.
    std::uint32_t hpCalls{}; // 최대 HP 경계를 부른 횟수다.
    bool patch; // 판본별 raw 위치와 밀어내기 인자 수를 고른다.
    // 부모 슬롯은 행마다 raw 입력으로 공급하므로 미리 할당하지 않는다.
    explicit ProcessScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        patch(edition==OriginalEdition::Patch1078) {}
    // 판본별 소유자/extra(abstract) 바이트와 HP 폭이다.
    std::size_t OwnerOffset() const { return patch ? 34 : 32; }
    std::size_t ExtraOffset() const { return patch ? 40 : 35; }
    std::size_t HpWidth() const { return patch ? 4 : 2; }
    // 이 행의 부모 번호와 그 raw 슬롯이다.
    Sid ParentSid() const { return Sid{static_cast<std::uint16_t>(input[Parent])}; }
    std::span<std::uint8_t> Raw() { return InputRaw(pool,ParentSid()); }
    // 실제 호출 인자들을 실행기와 같은 10진 문자열로 기록한다.
    void Event(char letter,std::initializer_list<std::uint32_t> values) {
        std::string text(1,letter);
        // 인자 순서와 DWORD 폭을 그대로 보존한다.
        for (auto value:values) text+=':'+std::to_string(value);
        events.push_back(std::move(text));
    }
    // 이전 행의 상태를 제거하고 실제 PE에 공급했던 입력 자료로 채운다. 시작 연산만 필드를 비워 둔다.
    void Prepare(std::vector<std::uint32_t> values,bool start) {
        input=std::move(values);CHECK(input.size()==InputCount);events.clear();hpCalls=0;ready=input[Ready];
        state={};state.now=Time(input[NowLow],input[NowHigh]);state.processType=input[ProcessType];state.soundProcessType=input[SoundType];
        state.localPlayer=input[Local];state.boss=input[Boss]!=0;state.pushGenus=input[PushGenus];state.daisType=input[Dais];
        state.quickBuild=input[FinishAll]!=0;state.quickVortex=input[FinishGenus]!=0;
        auto& record=types.at(input[Type]);record={};record.flags2=input[Genus];
        auto raw=Raw();
        // 실행기와 같은 표식 바이트로 주변 필드 보존을 비교한다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*29+7);
        Put(raw,0,kVtable);Put(raw,6,kForm,2);raw[10]=static_cast<std::uint8_t>(input[Type]);raw[11]=static_cast<std::uint8_t>(input[State]);
        Put(raw,14,input[X]);Put(raw,18,input[Y]);Put(raw,26,input[Hp],HpWidth());
        raw[OwnerOffset()]=static_cast<std::uint8_t>(input[Owner]);raw[ExtraOffset()]=static_cast<std::uint8_t>(input[Extra]);
        fields={};
        if (!start) fields={0,Time(input[StartLow],input[StartHigh]),Time(input[EndLow],input[EndHigh]),kSound};
    }
    // 시각/비율/조건 계산을 대체하지 않고 효과 응답과 실행기와 같은 경계 변동만 공급한다.
    ConstructionProcessHooks Hooks() {
        ConstructionProcessHooks hooks;
        // 객체 확보: 실패하면 생성자를 부르지 않는다.
        hooks.reserve=[this] { Event('A',{56});return (input[Allocation]&1)!=0; };
        // 표시 효과 경계 뒤 현재 시각/프로세스 타입이 바뀌는 입력을 공급한다.
        hooks.effect=[this](Sid parent,bool instant) {
            Event(instant ? 'I' : 'E',{parent.value});
            if (input[Mutate]==1) state.now=7.75;
            if (input[Mutate]==2) state.processType=44;
        };
        // 부착은 받은 필드를 보관하고 그 저장소를 돌려준다. 부착 뒤의 시각 변경은 필드에 영향이 없어야 한다.
        hooks.attach=[this](const ConstructionProcessFields& initial,std::uint32_t type,Sid parent,std::uint32_t flags) {
            Event('J',{type,parent.value,0,flags});fields=initial;
            if (input[Mutate]==5) state.now=99.5;
            return &fields;
        };
        // 소리 조회는 이름을 그대로 기록한다.
        hooks.loadSound=[this](std::string_view name) { events.push_back("S:"+std::string(name));return input[Mutate]==4 ? 0U : kSound; };
        // 소리 프로세스 확보 경계가 현재 소리 필드를 바꾸는 입력이다.
        hooks.reserveSound=[this] {
            Event('A',{40});
            if (input[Mutate]==3) fields.sound=kChangedSound;
            return (input[Allocation]&2)!=0;
        };
        hooks.attachSound=[this](Sid parent,std::uint32_t sound,std::uint32_t alternate,std::uint32_t flags) { Event('P',{parent.value,sound,alternate,flags}); };
        // 갱신 요청 뒤 로컬 플레이어 전역이 바뀌는 입력이다.
        hooks.refresh=[this] { events.push_back("R");if (input[Mutate]==10) state.localPlayer=2; };
        // 최대 HP 경계: 현재 시각/부모 HP 변경과 둘째 호출의 다른 반환을 공급한다.
        hooks.maxHitPoints=[this](Sid sid) {
            Event('H',{sid.value});++hpCalls;auto result=input[Max];
            if (input[Mutate]==6) state.now=6.0;
            if (input[Mutate]==9) Put(Raw(),26,input[Max],HpWidth());
            if (input[Mutate]==14 && hpCalls>1) result+=7;
            return std::bit_cast<std::int32_t>(result);
        };
        // 표시 경계가 abstract 비트를 뒤집는 입력이다.
        hooks.updateDisplay=[this](Sid sid) { Event('D',{sid.value});if (input[Mutate]==7) Raw()[ExtraOffset()]^=1; };
        // contained 삭제 경계가 부모 소유자를 바꾸는 입력이다.
        hooks.removeContained=[this](Sid sid,std::uint32_t type) { Event('X',{sid.value,type,0,1});if (input[Mutate]==15) Raw()[OwnerOffset()]=2; };
        // 완료음 경계가 권한/준비 소리를 바꾸는 입력이다.
        hooks.playAt=[this](float x,float y,std::string_view name,std::uint32_t loop) {
            events.push_back("W:"+std::to_string(Bits(x))+':'+std::to_string(Bits(y))+':'+std::string(name)+':'+std::to_string(loop));
            if (input[Mutate]==11) state.boss=!state.boss;
            if (input[Mutate]==16) ready=kChangedReady;
        };
        hooks.readySound=[this](std::uint32_t type) { CHECK(type==input[Type]);return ready; };
        hooks.play=[this](std::uint32_t sound) { Event('G',{sound,0,0,0,0,0}); };
        // 실제 Unpop/재등록처럼 void를 켜고 끈다. Unpop은 abstract 바이트의 다른 비트도 바꿀 수 있다.
        hooks.unpop=[this](Sid sid,std::uint32_t flags) {
            Event('U',{sid.value,flags});Raw()[11]|=4;
            if (input[Mutate]==13) Raw()[ExtraOffset()]|=0x80;
        };
        hooks.repop=[this](Sid sid,std::uint32_t flags) {
            Event('Q',{sid.value,flags});Raw()[11]&=0xfb;
            if (input[Mutate]==12) { Put(Raw(),14,Bits(40.5F));Put(Raw(),18,Bits(41.25F));state.pushGenus=0x200; }
        };
        // CD판 원본은 소유자를 넘기지 않으므로 세 인자만 기록한다.
        hooks.push=[this](float x,float y,std::uint32_t type,std::uint32_t owner) {
            if (patch) Event('B',{Bits(x),Bits(y),type,owner});else Event('B',{Bits(x),Bits(y),type});
        };
        return hooks;
    }
    // 실행기의 Kill 경계는 프로세스를 없애지 않으므로 항상 남아 있다고 답한다. 권한 전역 변경 입력도 공급한다.
    ConstructionProcessKill Kill() {
        return [this](std::uint32_t flags) { Event('K',{flags});if (input[Mutate]==8) state.boss=!state.boss;return true; };
    }
    // 관찰 문자열을 구분자로 묶는다. 빈 관찰은 fixture의 '-'로 표시한다.
    std::string Events() const {
        std::string out;
        // 실제 호출 순서대로 직렬화한다.
        for (const auto& event:events) { if (!out.empty()) out+=';';out+=event; }
        return out.empty() ? "-" : out;
    }
    // dirty와 두 시각의 비트, 소리 항목을 실행기와 같은 순서로 적는다.
    std::string Fields() const {
        const auto start=std::bit_cast<std::uint64_t>(fields.start),end=std::bit_cast<std::uint64_t>(fields.end);std::string out;
        // 여섯 값을 ':'로 잇는다.
        for (auto value:{static_cast<std::uint32_t>(fields.dirty),static_cast<std::uint32_t>(start),static_cast<std::uint32_t>(start>>32),
                static_cast<std::uint32_t>(end),static_cast<std::uint32_t>(end>>32),fields.sound}) { if (!out.empty()) out+=':';out+=std::to_string(value); }
        return out;
    }
};
// 같은 입력의 사건 순서/인자·프로세스 필드·부모 raw 전체를 독립 PE 관찰과 비교한다.
void Replay(std::string_view edition) {
    ProcessScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    RawConstructionProcess process(scene.pool,scene.types,scene.state,scene.Hooks());std::size_t count=0;
    CHECK(Fixture().size()==3*kRows);
    // 저장 행마다 장면만 공급하며 기대 시각/HP/조건은 재계산하지 않는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==6);const bool start=row[1]=="Start";scene.Prepare(Inputs(row[2]),start);
        const auto parent=scene.ParentSid();
        if (start) process.Start(parent,Float(scene.input[Duration]));
        else if (row[1]=="Run") process.Run(scene.fields,parent,scene.Kill());
        else if (row[1]=="Complete") process.Complete(parent);
        else process.FormDestroyed(parent);
        const auto fields=start && !(scene.input[Allocation]&1) ? std::string("-") : scene.Fields();
        const bool same=scene.Events()==row[3] && fields==row[4] && Hex(scene.pool.Slot(parent))==row[5];
        CHECK(same);if (!same) { std::printf("건설 프로세스 %s 행 %zu 불일치\n",std::string(edition).c_str(),count);break; }++count;
    }
    CHECK(count==kRows);
}
// 기본 장면의 입력만 제공한다: 서버 번호의 abstract 부모, HP가 최대와 같고 로컬 소유이며 권한이 있다.
std::vector<std::uint32_t> DefaultInput() {
    std::vector<std::uint32_t> input(InputCount);
    input[Parent]=20000;input[Duration]=Bits(2.5F);input[NowHigh]=0x40000000;input[Allocation]=3;input[Type]=100;input[Extra]=1;input[Hp]=100;
    input[Owner]=1;input[X]=Bits(20.5F);input[Y]=Bits(21.25F);input[Genus]=0x4000;input[Max]=100;input[EndHigh]=0x40200000;input[Local]=1;
    input[Boss]=1;input[PushGenus]=0x10444200;input[Dais]=165;input[ProcessType]=30;input[SoundType]=47;
    return input;
}
// 실제 모듈 장면의 타입 표다. 타입 100은 HP 300의 factory genus다. host와 보상 모듈이 생성 때 표를 복사하므로 먼저 채운다.
std::vector<RiftTypeRecord> RealTypes(bool patch) {
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);
    types[100].flags1=kHasHp;types[100].flags2=0x4000;types[100].maxHitPoints=300;types[100].footX=types[100].footY=1;
    return types;
}
// 실제 프로세스 host/Kernel/공통 삭제/보상 모듈 위에 건설 프로세스를 올리고 나머지 효과만 기록하는 장면이다.
struct RealScene {
    // patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool patch;/* pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다. */ SidPool pool;/* hash: 좌표별 객체 후보를 찾는 공간 해시. 입력 구성 또는 실행 결과 대조에 사용한다. */ SquidHash hash;/* spots: 칸별 점유·통행 비트를 읽는 spot 지도. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);/* types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RiftTypeRecord> types;
    // unpop: 객체의 공간 등록을 해제하는 Unpop 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidUnpop unpop;/* destroy: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다. */ RawSquidDestroy destroy;/* kernel: 프로세스 등록과 프레임 실행을 담당하는 커널. 입력 구성 또는 실행 결과 대조에 사용한다. */ Kernel kernel;/* clock: 프레임 판정에 현재 게임 시각을 공급하는 시계. 입력 구성 또는 실행 결과 대조에 사용한다. */ SquidProcessState clock;/* host: Form이나 프로세스의 등록·통지·수명을 연결하는 호스트. 입력 구성 또는 실행 결과 대조에 사용한다. */ SquidProcessHost host;
    // books: 장부. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidPostPopState books;/* rewardState: 보상 계산이 읽거나 갱신하는 판본별 전역과 AI 지갑이다. 패치 주소/CD 주소는 괄호에 둔다 (SquidRewardState). */ SquidRewardState rewardState;/* rng: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom). */ GameRandom rng;/* store: 패치판이 플레이어별 SP를 32개의 9칸 DWORD 배열에 흩뿌려 보관하는 방식이다 (ScrambledSpStore). */ ScrambledSpStore store;/* reward: HP·SP·지갑 보상을 실제 장부에 적용하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다. */ SquidReward reward;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    ConstructionProcessState state;/* events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<std::string> events;/* parent: 부착·통지 대상인 부모 객체 또는 부모 SID. 입력 구성 또는 실행 결과 대조에 사용한다. */ Sid parent{};
    // 서버 권한 풀에서 부모를 서버 번호로 확보한다. SP 저장소는 보상 모듈 생성 조건만 채운다.
    explicit RealScene(OriginalEdition edition):patch(edition==OriginalEdition::Patch1078),pool(edition,kCapacity,true),types(RealTypes(patch)),
        unpop(pool,hash,spots),destroy(pool,unpop,types),host(pool,types,kernel,destroy,clock),store(rng,[] { return 1U; }),reward(pool,types,books,rewardState,patch ? &store : nullptr) {
        parent=pool.Allocate(0);CHECK(parent.value>=pool.Layout().serverFirst);
        auto raw=pool.AllocatedBytes(parent);raw[10]=100;raw[11]=0;raw[patch ? 34 : 32]=1;raw[patch ? 40 : 35]=1;
        Put(raw,14,Bits(20.5F));Put(raw,18,Bits(21.25F));state.now=2.0;
    }
    // 보상 모듈의 실제 최대 HP 외의 효과는 이름만 기록한다. Unpop/재등록은 실제처럼 void를 켜고 끈다.
    ConstructionProcessHooks Hooks() {
        ConstructionProcessHooks hooks;
        hooks.reserve=[] { return true; };hooks.effect=[this](Sid,bool instant) { events.push_back(instant ? "I" : "E"); };
        hooks.loadSound=[](std::string_view) { return 77U; };hooks.reserveSound=[] { return true; };
        hooks.attachSound=[this](Sid,std::uint32_t sound,std::uint32_t,std::uint32_t flags) { events.push_back("P:"+std::to_string(sound)+':'+std::to_string(flags)); };
        hooks.refresh=[this] { events.push_back("R"); };hooks.updateDisplay=[this](Sid) { events.push_back("D"); };
        hooks.removeContained=[this](Sid,std::uint32_t type) { events.push_back("X:"+std::to_string(type)); };
        hooks.playAt=[this](float,float,std::string_view name,std::uint32_t) { events.push_back("W:"+std::string(name)); };
        hooks.readySound=[](std::uint32_t) { return 85U; };hooks.play=[this](std::uint32_t sound) { events.push_back("G:"+std::to_string(sound)); };
        hooks.unpop=[this](Sid sid,std::uint32_t) { events.push_back("U");pool.AllocatedBytes(sid)[11]|=4; };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { events.push_back("Q:"+std::to_string(flags));pool.AllocatedBytes(sid)[11]&=0xfb; };
        hooks.push=[this](float,float,std::uint32_t type,std::uint32_t owner) { events.push_back("B:"+std::to_string(type)+':'+std::to_string(owner)); };
        return MakeConstructionProcessHooks(pool,reward,nullptr,std::move(hooks));
    }
    // 부모의 현재 HP를 판본의 폭으로 읽는다.
    std::uint32_t Hp() const { return Get(pool.Slot(parent),26,patch ? 4 : 2); }
    // 기록한 사건을 ';'로 잇는다.
    std::string Events() const {
        std::string out;
        // 호출 순서를 보존한다.
        for (const auto& event:events) { if (!out.empty()) out+=';';out+=event; }
        return out;
    }
};
}

// 패치 전체 원본 명령의 관찰을 재생한다.
TEST_CASE(ConstructionProcess_ReplaysOriginals) { Replay("originals"); }
// CD 전체 원본 명령의 관찰을 재생한다.
TEST_CASE(ConstructionProcess_ReplaysCd) { Replay("originalCD"); }
// 별도 10.37 PE의 독립 관찰을 재생한다.
TEST_CASE(ConstructionProcess_ReplaysExtra1037) { Replay("original1037"); }

// 필수 효과가 비었거나 타입 표/host 풀이 맞지 않으면 첫 효과 전에 거부한다. attach는 host가 있으면 비워도 된다.
TEST_CASE(ConstructionProcess_RejectsMissingHooksAndForeignHost) {
    ProcessScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput(),true);auto hooks=scene.Hooks();
    hooks.push={};CHECK(Throws([&] { RawConstructionProcess invalid(scene.pool,scene.types,scene.state,hooks); }));
    hooks=scene.Hooks();hooks.attach={};CHECK(Throws([&] { RawConstructionProcess invalid(scene.pool,scene.types,scene.state,hooks); }));
    std::vector<RiftTypeRecord> cdTypes(171);CHECK(Throws([&] { RawConstructionProcess invalid(scene.pool,cdTypes,scene.state,scene.Hooks()); }));
    RealScene real(OriginalEdition::Patch1078);
    CHECK(Throws([&] { RawConstructionProcess invalid(scene.pool,scene.types,scene.state,hooks,&real.host); }));
    CHECK(!Throws([&] { RawConstructionProcess valid(real.pool,real.types,real.state,hooks,&real.host); }));
}

// 객체 확보 실패는 효과가 없고, 소리 프로세스 확보 실패는 소리 항목을 남긴 채 갱신까지 진행한다.
TEST_CASE(ConstructionProcess_AllocationFailuresKeepOriginalEffects) {
    ProcessScene scene(OriginalEdition::Patch1078);RawConstructionProcess process(scene.pool,scene.types,scene.state,scene.Hooks());
    auto input=DefaultInput();input[Allocation]=0;scene.Prepare(input,true);process.Start(scene.ParentSid(),2.5F);CHECK(scene.Events()=="A:56");
    input[Allocation]=1;scene.Prepare(input,true);process.Start(scene.ParentSid(),2.5F);
    CHECK(scene.Events()=="A:56;E:20000;J:30:20000:0:16;S:jimBuild.wav;A:40;R" && scene.fields.sound==kSound);
    CHECK(scene.fields.dirty==0xff && scene.fields.start==2.0 && scene.fields.end==4.5);
}

// 지속 시간 0은 10초 건설이 되고, 1초 이하는 진행음이 없다. 양의 NaN은 CD판에서만 진행음을 붙인다.
TEST_CASE(ConstructionProcess_DurationSelectsDefaultTimeAndSound) {
    // 두 판본의 같은 입력에서 진행음 조건만 다르다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        ProcessScene scene(edition);RawConstructionProcess process(scene.pool,scene.types,scene.state,scene.Hooks());
        scene.Prepare(DefaultInput(),true);process.Start(scene.ParentSid(),-0.0F);
        CHECK(scene.events.at(1)=="I:20000" && scene.fields.end==12.0 && scene.fields.sound==kSound);
        scene.Prepare(DefaultInput(),true);process.Start(scene.ParentSid(),1.0F);
        CHECK(scene.events.at(1)=="E:20000" && scene.fields.end==3.0 && scene.fields.sound==0 && scene.Events().find("S:")==std::string::npos);
        scene.Prepare(DefaultInput(),true);process.Start(scene.ParentSid(),std::bit_cast<float>(0x7fc00000U));
        CHECK((scene.fields.sound==kSound)==!scene.patch);
    }
}

// Unpop 경계가 void를 켜지 않으면 원본 assert에 해당하므로 abstract를 풀지 않고 거부한다.
TEST_CASE(ConstructionProcess_CompleteRequiresVoidBeforeClearingAbstract) {
    ProcessScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput(),false);auto hooks=scene.Hooks();
    hooks.unpop=[](Sid,std::uint32_t) {};RawConstructionProcess process(scene.pool,scene.types,scene.state,hooks);
    CHECK(Throws([&] { process.Complete(scene.ParentSid()); }) && (scene.Raw()[scene.ExtraOffset()]&1));
}

// 통지의 건설 시작 경계는 단정도 비트 0을 지속 시간 0으로 넘겨 기본 10초 건설을 시작한다.
TEST_CASE(ConstructionProcess_NoticeAdapterStartsDefaultConstruction) {
    ProcessScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput(),true);
    RawConstructionProcess process(scene.pool,scene.types,scene.state,scene.Hooks());
    auto notice=MakeConstructionProcessNoticeHooks(scene.pool,process,SilentNoticeHooks());notice.start(scene.ParentSid(),0);
    CHECK(scene.Events()=="A:56;I:20000;J:30:20000:0:16;S:jimBuild.wav;A:40;P:20000:"+std::to_string(kSound)+":0:1;R" && scene.fields.end==12.0);
    SidPool other(scene.pool.Edition(),kCapacity,false);
    CHECK(Throws([&] { MakeConstructionProcessNoticeHooks(other,process,SilentNoticeHooks()); }));
}

// 실제 host에 붙은 프로세스가 Kernel 프레임마다 HP를 올리고, 다 차면 스스로 끝나며 abstract를 푼다.
TEST_CASE(ConstructionProcess_ActualHostRunsToCompletion) {
    // 두 판본 모두 실제 부착·Kill·form 삭제 통지·보상 모듈의 최대 HP를 거친다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        RealScene scene(edition);RawConstructionProcess process(scene.pool,scene.types,scene.state,scene.Hooks(),&scene.host);
        const std::size_t extra=scene.patch ? 40 : 35;
        process.Start(scene.parent,8.0F);CHECK(scene.Events()=="E;P:77:1;R" && scene.kernel.Size()==1 && Get(scene.pool.Slot(scene.parent),6,2)!=0);
        scene.events.clear();scene.state.now=4.0;scene.kernel.RunFrame();
        CHECK(scene.Hp()==75 && scene.Events()=="D" && scene.kernel.Size()==1 && (scene.pool.Slot(scene.parent)[extra]&1));
        scene.events.clear();scene.state.now=10.0;scene.kernel.RunFrame();
        // 진행의 표시 → Kill → form 삭제 통지의 완료 처리 순서다.
        CHECK(scene.Events()=="D;X:47;R;W:buildDone.wav;G:85;U;Q:9;B:100:1");
        CHECK(scene.kernel.Size()==0 && scene.Hp()==300 && !(scene.pool.Slot(scene.parent)[extra]&1) && !(scene.pool.Slot(scene.parent)[11]&4));
        CHECK(Get(scene.pool.Slot(scene.parent),6,2)==0);
    }
}

// abstract가 아닌 부모는 첫 Kill에서 프로세스가 사라지므로 HP가 최대여도 둘째 Kill을 하지 않는다.
TEST_CASE(ConstructionProcess_ActualHostSkipsSecondKillAfterRemoval) {
    RealScene scene(OriginalEdition::Patch1078);RawConstructionProcess process(scene.pool,scene.types,scene.state,scene.Hooks(),&scene.host);
    process.Start(scene.parent,8.0F);scene.pool.AllocatedBytes(scene.parent)[40]=0;scene.events.clear();scene.state.now=10.0;
    scene.kernel.RunFrame();
    // 완료 처리는 한 번만 실행되며 abstract가 아니므로 Unpop/재등록은 없다.
    CHECK(scene.Events()=="D;X:47;R;W:buildDone.wav;G:85" && scene.kernel.Size()==0 && scene.Hp()==300);
}

// 권한이 없으면 HP가 최대여도 Kill을 요청하지 않아 프로세스가 남고, quickbuild는 시각과 무관하게 한 프레임에 끝낸다.
TEST_CASE(ConstructionProcess_ActualHostQuickBuildAndNonBossKeepsProcess) {
    RealScene scene(OriginalEdition::Patch1078);RawConstructionProcess process(scene.pool,scene.types,scene.state,scene.Hooks(),&scene.host);
    process.Start(scene.parent,8.0F);scene.state.boss=false;scene.clock.boss=false;scene.state.now=10.0;scene.events.clear();scene.kernel.RunFrame();
    CHECK(scene.Events()=="D" && scene.kernel.Size()==1 && scene.Hp()==300);
    scene.state.boss=true;scene.clock.boss=true;scene.state.now=2.0;scene.state.quickBuild=true;scene.events.clear();scene.kernel.RunFrame();
    CHECK(scene.kernel.Size()==0 && scene.Hp()==300 && !(scene.pool.Slot(scene.parent)[40]&1));
}

// 패치판은 free/dead 부모에 붙지 않고 남은 효과를 계속하며, 다른 풀의 보상/밀어내기 모듈은 거부한다.
TEST_CASE(ConstructionProcess_DeadParentAndForeignPoolAdapters) {
    RealScene scene(OriginalEdition::Patch1078);RawConstructionProcess process(scene.pool,scene.types,scene.state,scene.Hooks(),&scene.host);
    scene.pool.AllocatedBytes(scene.parent)[11]=2;process.Start(scene.parent,8.0F);
    CHECK(scene.Events()=="E;P:77:1;R" && scene.kernel.Size()==0);
    RealScene other(OriginalEdition::Patch1078);
    CHECK(Throws([&] { MakeConstructionProcessHooks(scene.pool,other.reward,nullptr,{}); }));
}

// 실제 Kernel의 진행과 완료가 서로 다른 시점의 동일 boss를 읽는다.
TEST_CASE(ConstructionProcess_SharedBossRecheckedDuringActualCompletion) {
    RealScene scene(OriginalEdition::Patch1078);bool boss=false,flipOnRefresh=false;scene.state.sharedBoss=&boss;auto hooks=scene.Hooks();const auto refresh=hooks.refresh;hooks.refresh=[&] { refresh();if (flipOnRefresh) boss=false; };RawConstructionProcess process(scene.pool,scene.types,scene.state,std::move(hooks),&scene.host);process.Start(scene.parent,8.F);scene.state.quickBuild=true;scene.state.now=10;scene.kernel.RunFrame();CHECK(scene.kernel.Size()==1 && scene.Hp()==300 && (scene.pool.Slot(scene.parent)[40]&1));boss=true;scene.state.boss=false;flipOnRefresh=true;scene.kernel.RunFrame();CHECK(scene.kernel.Size()==0 && scene.Hp()==300 && (scene.pool.Slot(scene.parent)[40]&1) && !boss);
}
