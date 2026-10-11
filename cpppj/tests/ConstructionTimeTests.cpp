// 끝 시각 재설정의 세 PE 관찰과 실제 form→Kernel 조회·건설 진행 연결을 검사한다.
#include "ConstructionNoticeSupport.h"
#include "o/RawConstructionProcess.h"

namespace {
using namespace netstorm::test::construction;
// 독립 원본 fixture의 입력 열 순서다. 시각 결과를 여기서 계산하지 않는다.
enum Input : std::size_t { NowLow,NowHigh,StartLow,StartHigh,EndLow,EndHigh,Maximum,Damage,Dirty,Mutate,Parent };
// 판본별 독립 원본 행 수와 실행기의 form/Kernel/소리 입력 값이다.
constexpr std::size_t kRows=423;
constexpr std::uint32_t kPid=7,kSound=0x15000040;
constexpr Sid kForm{5};
// 저장 관찰만 읽으므로 CTest에서 게임/PE/Python을 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONTIME_FIXTURE).rows;return rows; }
// 두 DWORD의 double 비트를 손실 없이 조합한다.
double Time(std::uint32_t low,std::uint32_t high) { return std::bit_cast<double>((static_cast<std::uint64_t>(high)<<32)|low); }
// 실제 프로세스 host와 Kernel 위에 원본 실행기의 부모/필드를 공급하는 장면이다.
struct TimeScene {
    SidPool pool; // 모든 모듈이 공유하는 입력 월드다.
    std::vector<RiftTypeRecord> types; // 부모 타입 100과 프로세스 타입 30의 자료다.
    SquidHash hash; // host가 쓰는 공통 삭제 계층의 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 공간 해제용 지도다.
    SquidUnpop unpop; // 실제 form 삭제가 쓰는 공간 해제 계층이다.
    RawSquidDestroy destroy; // Kernel 완료 때 form을 실제로 지운다.
    Kernel kernel; // 슬롯 7에 실제 건설 프로세스를 붙인다.
    SquidProcessState clock; // 실제 host의 권한/게임 시각 상태다.
    SquidProcessHost host; // form→현재 DWORD 번호→프로세스를 조회한다.
    ConstructionProcessState state; // 끝 시각 계산이 읽는 현재 전역이다.
    std::vector<std::uint32_t> input; // 원본 실행기에 공급한 입력 DWORD다.
    std::vector<std::string> events; // 실제 최대 HP 경계 호출을 기록한다.
    ConstructionProcess* process{}; // host가 소유한 실제 건설 프로세스다.
    std::unique_ptr<RawConstructionProcess> logic; // 실제 Kernel 프레임을 실행하는 동안 유지하는 복원 몸체다.
    // 입력 타입 수와 실제 삭제/host를 판본에 맞춰 연결한다.
    explicit TimeScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        unpop(pool,hash,spots),destroy(pool,unpop,types),host(pool,types,kernel,destroy,clock) {}
    // 현재 부모 SID다. fixture의 실제 원본 입력 범위 안에서만 사용한다.
    Sid ParentSid() const { return Sid{static_cast<std::uint16_t>(input[Parent])}; }
    // 기본 경계 중 실제 최대 HP 호출/현재 자료 변경만 관찰한다.
    ConstructionProcessHooks Hooks() {
        ConstructionProcessHooks hooks;
        // 이번 비교는 부착 후 재설정이며 시작 효과·소리 경계는 실행되지 않는다.
        hooks.reserve=[] { return true; };hooks.effect=[](Sid,bool) {};hooks.attach=[](const ConstructionProcessFields&,std::uint32_t,Sid,std::uint32_t) { return static_cast<ConstructionProcessFields*>(nullptr); };
        hooks.loadSound=[](std::string_view) { return 0U; };hooks.reserveSound=[] { return false; };hooks.attachSound=[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) {};
        hooks.refresh=[] {};hooks.updateDisplay=[](Sid) {};hooks.removeContained=[](Sid,std::uint32_t) {};
        hooks.playAt=[](float,float,std::string_view,std::uint32_t) {};hooks.readySound=[](std::uint32_t) { return 0U; };hooks.play=[](std::uint32_t) {};
        // 실제 진행/완료 연결 검사에서는 원본 공간 계약처럼 void를 켜고 끈다.
        hooks.unpop=[this](Sid sid,std::uint32_t) { pool.AllocatedBytes(sid)[11]|=4; };
        hooks.repop=[this](Sid sid,std::uint32_t) { pool.AllocatedBytes(sid)[11]&=0xfb; };hooks.push=[](float,float,std::uint32_t,std::uint32_t) {};
        // 원본과 같은 경계 입력으로 현재 시각/필드가 바뀌게 한다.
        hooks.maxHitPoints=[this](Sid sid) {
            events.push_back("H:"+std::to_string(sid.value));
            if (input[Mutate]==1) state.now=7.75;
            if (input[Mutate]==2) { process->Fields().start=-1.5;process->Fields().end=4.0; }
            if (input[Mutate]==3) { process->Fields().dirty=0x81;process->Fields().sound=0x12345678; }
            return std::bit_cast<std::int32_t>(input[Maximum]);
        };
        return hooks;
    }
    // 동일한 입력의 부모 raw·시각과 실제 부착된 프로세스를 준비한다.
    void Prepare(std::vector<std::uint32_t> values) {
        input=std::move(values);CHECK(input.size()==11);state.now=Time(input[NowLow],input[NowHigh]);
        auto raw=InputRaw(pool,ParentSid());
        // raw 전체 표식은 원본 출력의 주변 필드 불변을 검사하는 입력이다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*29+7);
        raw[10]=100;raw[11]=0;Put(raw,6,0,2);
        // 앞 여섯 슬롯을 채워 원본 실행기의 Kernel 번호 7과 맞춘다.
        for (std::uint32_t pid=1;pid<kPid;++pid) CHECK(kernel.Add(std::make_unique<RegularProcess>(1,0.0F))==pid);
        ConstructionProcessFields fields{static_cast<std::uint8_t>(input[Dirty]),Time(input[StartLow],input[StartHigh]),Time(input[EndLow],input[EndHigh]),kSound};
        logic=std::make_unique<RawConstructionProcess>(pool,types,state,Hooks(),&host);
        process=static_cast<ConstructionProcess*>(host.Attach(std::make_unique<ConstructionProcess>(*logic,fields),30,ParentSid(),0x10));
        CHECK(process && kernel.Find(*process)==kPid && process->Form()==kForm);
    }
    // 실제 프로세스의 필드를 독립 관찰과 같은 여섯 비트 값으로 직렬화한다.
    std::string Fields() const {
        const auto& fields=process->Fields();const auto start=std::bit_cast<std::uint64_t>(fields.start),end=std::bit_cast<std::uint64_t>(fields.end);std::string result;
        // dirty/start/end/sound만 관찰하며 타입/포인터 기대값을 재계산하지 않는다.
        for (auto value:{static_cast<std::uint32_t>(fields.dirty),static_cast<std::uint32_t>(start),static_cast<std::uint32_t>(start>>32),
                static_cast<std::uint32_t>(end),static_cast<std::uint32_t>(end>>32),fields.sound}) { if (!result.empty()) result+=':';result+=std::to_string(value); }
        return result;
    }
};
// 실제 host로 재설정 루트를 호출한 결과를 세 PE에서 얻은 시각/비트/호출/raw와 비교한다.
void Replay(std::string_view edition) {
    std::size_t count=0;CHECK(Fixture().size()==3*kRows);
    // Kernel/form/필드 상태는 행마다 새로 만들어 이전 입력이 남지 않게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==5);TimeScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
        scene.Prepare(Inputs(row[1]));const auto form=Hex(scene.pool.Slot(kForm));scene.logic->ResetEndFromForm(kForm,scene.input[Damage]);
        const bool same=scene.events.size()==1 && scene.events.front()==row[2] && scene.Fields()==row[3] &&
            Hex(scene.pool.Slot(scene.ParentSid()))==row[4] && Hex(scene.pool.Slot(kForm))==form;
        CHECK(same);if (!same) { std::printf("건설 끝 시각 %s 행 %zu 불일치\n",std::string(edition).c_str(),count);break; }++count;
    }
    CHECK(count==kRows);
}
// 실제 연결 검사에 쓰는 기본 입력이다. 건설 시작/끝은 0/8, 최대 HP는 100이다.
std::vector<std::uint32_t> DefaultInput() { return {0,0x40140000,0,0,0,0x40200000,100,5,8,0,20000}; }
}

// 패치의 전체 재설정/Getter 관찰을 실제 host 연결로 재생한다.
TEST_CASE(ConstructionTime_ReplaysOriginals) { Replay("originals"); }
// CD의 진행 계산/기본 helper를 포함한 전체 관찰을 재생한다.
TEST_CASE(ConstructionTime_ReplaysCd) { Replay("originalCD"); }
// 별도 10.37 PE의 독립 관찰을 재생한다.
TEST_CASE(ConstructionTime_ReplaysExtra1037) { Replay("original1037"); }

// 재설정 뒤 원래 완료 시각에는 남아 있고 새 완료 시각에 HP 채움/abstract 해제를 끝낸다.
TEST_CASE(ConstructionTime_ActualHostDelaysCompletion) {
    // 두 판본 모두 실제 Kernel 프레임·form 삭제와 완료를 실행한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        TimeScene scene(edition);scene.Prepare(DefaultInput());const auto extra=edition==OriginalEdition::Patch1078 ? 40U : 35U;
        scene.pool.AllocatedBytes(scene.ParentSid())[extra]=1;scene.logic->ResetEndFromForm(kForm,5);
        CHECK(scene.process->Fields().start==0 && scene.process->Fields().end==15 && scene.process->Fields().sound==kSound);
        scene.state.now=8;scene.kernel.RunFrame();CHECK(scene.kernel.Size()==kPid && (scene.pool.Slot(scene.ParentSid())[extra]&1));
        scene.state.now=15;scene.kernel.RunFrame();CHECK(scene.kernel.Size()==kPid-1 && !(scene.pool.Slot(scene.ParentSid())[extra]&1));
        CHECK(Get(scene.pool.Slot(scene.ParentSid()),26,edition==OriginalEdition::Patch1078 ? 4 : 2)==100);
    }
}

// 최대 HP 경계 뒤 현재 시각과 변경 비트를 쓰고 시작/소리의 경계 변경을 보존한다.
TEST_CASE(ConstructionTime_UsesCurrentClockAndPreservesOtherFields) {
    TimeScene scene(OriginalEdition::Patch1078);auto input=DefaultInput();input[Mutate]=1;scene.Prepare(input);
    scene.logic->ResetEndFromForm(kForm,0xffffffff);CHECK(scene.process->Fields().end==17.75 && scene.process->Fields().dirty==12);
    scene.input[Mutate]=3;scene.logic->ResetEndFromForm(kForm,0);CHECK(scene.process->Fields().dirty==0x85 && scene.process->Fields().sound==0x12345678);
    CHECK(scene.process->Fields().start==0 && scene.events.size()==2);
}

// 빈/오래된/다른 종류의 form·상위 비트 Kernel 번호는 실제 건설 필드를 쓰기 전에 거부한다.
TEST_CASE(ConstructionTime_RejectsInvalidFormsAndKeepsDwordPid) {
    TimeScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput());const auto before=scene.Fields();
    Put(scene.pool.AllocatedBytes(kForm),18,0x10007);CHECK(scene.host.FromForm(kForm)==nullptr);
    CHECK(Throws([&] { scene.logic->ResetEndFromForm(kForm,0); }));CHECK(scene.Fields()==before && scene.events.empty());
    Put(scene.pool.AllocatedBytes(kForm),18,kPid);const auto regular=scene.host.AddRegular(scene.ParentSid(),1,0);CHECK(regular);
    if (regular) CHECK(Throws([&] { scene.logic->ResetEndFromForm(regular->Form(),0); }));
    auto stale=InputRaw(scene.pool,Sid{100});std::copy(scene.pool.Slot(kForm).begin(),scene.pool.Slot(kForm).end(),stale.begin());
    CHECK(Throws([&] { scene.logic->ResetEndFromForm(Sid{100},0); }));
    RawConstructionProcess noHost(scene.pool,scene.types,scene.state,scene.Hooks());
    CHECK(Throws([&] { noHost.ResetEndFromForm(kForm,0); }));CHECK(scene.Fields()==before && scene.events.empty());
}
