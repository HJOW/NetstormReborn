// 단위867: 투사체의 현재 checking/항상참 Baseprocess 진단과 같은 Host 자기 삭제를 게임 없이 검사한다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "RawSceneSupport.h"
#include "o/RawMissile.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 합성 입력과 명시 효과의 수명을 이 검사 파일 안에 제한한다.
struct MissileCheckingScene { // 비충돌 직선 이동과 사거리 종료만 준비하며 실제 공격/게임 틱은 실행하지 않는다.
    SidPool pool; // 모든 진단이 현재 용량24000을 직접 읽는 같은 저장소다.
    std::vector<RiftTypeRecord> types; // 현재 group을 읽는 판본별 TYPE 표다.
    std::vector<MissileTypeData> supplement; // 표시/발사자 사거리와 명시 DPS 입력이다.
    MissileState state; // 현재 시각과 외부 checking DWORD를 빌린다.
    MissileFields fields; // 두 진단과 이동 본체가 공유하는 raw 필드다.
    std::uint32_t checking{}; // 상위 비트까지 유지하는 실제 관문 입력이다.
    MissileHooks hooks; // 선택된 공간/표적/종료만 사건으로 관찰한다.
    std::vector<std::string> events; // 순서와 SID를 포함한 실제 외부 요청이다.
    bool patch; // CD의 다른 필드 위치와 진단 부재를 구별한다.
    explicit MissileCheckingScene(OriginalEdition edition=OriginalEdition::Patch1078)
        :pool(edition,kCapacity,false),types(::netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171)),supplement(types.size()),patch(edition==OriginalEdition::Patch1078) {
        for (std::uint16_t id=5;id<=16;++id) { CHECK(pool.Allocate(2)==Sid{id});auto raw=pool.AllocatedBytes(Sid{id});std::fill(raw.begin(),raw.end(),std::uint8_t{}); } // 원문 입력 세 자산과 후반 재조회 대상을 실제 할당한다.
        pool.AllocatedBytes(Sid{10})[10]=128;pool.AllocatedBytes(Sid{11})[10]=71;pool.AllocatedBytes(Sid{12})[10]=80;pool.AllocatedBytes(Sid{12})[34]=3;
        types[128].group=1;types[0].group=7;supplement[0].damagePerSecond=10;state.now=1;state.checking.Bind(checking);
        Put(fields.bytes,4,13);Put(fields.bytes,8,12);Put(fields.bytes,20,10);Put(fields.bytes,24,11);
        Point(patch ? 56 : 40,10,10);Point(patch ? 72 : 56,20,10);Point(patch ? 64 : 48,20,10);Point(Previous(),-10000,-10000);
        Double(patch ? 104 : 80,2);Double(patch ? 112 : 88,0);
        hooks.unpop=[this](Sid sid,auto flags) { CHECK(flags==0);events.push_back("U"+std::to_string(sid.value)); };
        hooks.pop=[this](Sid sid,float x,float y,auto flags) { CHECK(sid==Sid{12} && y==10 && flags==0);events.push_back("P"+std::to_string(std::bit_cast<std::uint32_t>(x))); };
        hooks.targetBounds=[this](Sid sid,auto) { CHECK(sid==Sid{11});events.push_back("B");return std::array<float,4>{100,100,101,101}; };
        hooks.center=[this](Sid sid) { CHECK(sid==Sid{10});events.push_back("C");return std::array<float,2>{10,10}; };
        hooks.refresh=[this](Sid sid) { events.push_back("Q"+std::to_string(sid.value)); };
        hooks.destroy=[this](Sid sid,auto flags) { CHECK(flags==0);events.push_back("X"+std::to_string(sid.value)); };
        hooks.hit=[](Sid,Sid) { throw std::logic_error("투사체 진단 검사의 미선택 직접 피해"); };
        hooks.damage=[this](Sid sid,auto amount,auto owner,Sid attacker) { CHECK(sid==Sid{11} && amount==6 && owner==3 && attacker==Sid{});events.push_back("D"); };
    }
    void Point(std::size_t offset,float x,float y) { Put(fields.bytes,offset,std::bit_cast<std::uint32_t>(x));Put(fields.bytes,offset+4,std::bit_cast<std::uint32_t>(y)); } // 입력의 float 비트를 그대로 저장한다.
    void Double(std::size_t offset,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,offset,static_cast<std::uint32_t>(bits));Put(fields.bytes,offset+4,static_cast<std::uint32_t>(bits>>32)); } // 판본별 시각8BYTE를 준비한다.
    std::size_t Previous() const { return patch ? 120 : 96; } // 후반 진단보다 뒤에 기록되는 이전 좌표 필드다.
    void Diagnostic(std::string_view expression,std::string_view file,std::uint32_t line) { CHECK(file=="\\Ns\\O\\Baseprocess.cpp");CHECK(expression==(line==0x69 ? "formSid == 0 || form.isValid()" : "attachToSid == 0 || s.isValid()"));CHECK(line==0x69 || line==0x6c);events.push_back("A"+std::to_string(line)); } // 미도달 Missile.cpp false 단언을 성공 사건으로 만들지 않는다.
    void Run() { RawMissile logic(pool,types,supplement,state,hooks);logic.Frame(fields); } // 검사마다 같은 입력 주소를 직접 빌린다.
};
void CheckHostDiagnosticDeletion(bool late) { // 실제 ProcessForm/Kernel 삭제를 두 진단 위치에서 각각 실행한다.
    MissileCheckingScene scene;SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(scene.pool,hash,spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);Kernel kernel(32);SquidProcessState clock;
    SquidDestroyHooks deletion;deletion.selected=[] { return Sid{}; };SquidProcessHost host(scene.pool,scene.types,kernel,destroy,clock,{},deletion);Missile* process=nullptr;
    scene.hooks.assertion=[&](auto expression,auto file,auto line) { scene.Diagnostic(expression,file,line);host.Kill(*process,0x10);process=nullptr; };
    auto ordinaryPop=scene.hooks.pop;scene.hooks.pop=[&](auto sid,auto x,auto y,auto flags) { ordinaryPop(sid,x,y,flags);scene.checking=0x80000000;Put(const_cast<MissileFields&>(process->Fields()).bytes,4,0x10005); };
    RawMissile logic(scene.pool,scene.types,scene.supplement,scene.state,scene.hooks);
    process=dynamic_cast<Missile*>(host.Attach(std::make_unique<Missile>(logic,scene.fields),28,Sid{12},0x10));CHECK(process);if (!process) return;const auto form=process->Form();
    if (!late) { scene.checking=0x80000000;Put(const_cast<MissileFields&>(process->Fields()).bytes,4,0x10005); }
    process->RunFrame();CHECK(!process && kernel.Size()==0 && !host.FromForm(form));CHECK((scene.pool.Slot(form)[11]&1)!=0 && (scene.pool.Slot(Sid{12})[11]&3)==0);
    CHECK(scene.events==(late ? std::vector<std::string>{"U12","P1097859072","B","C","A105"} : std::vector<std::string>{"A105"}));
}
}
TEST_CASE(MissileChecking_FirstDiagnosticUsesFullDwordAndRepairsBeforeLookup) { // 첫 진단 뒤의 현재 SID/시각과 flag가 꺼져도 계속되는 두 번째 범위 진단을 확인한다.
    MissileCheckingScene scene;scene.checking=0x80000000;Put(scene.fields.bytes,4,0x10005);Put(scene.fields.bytes,8,0x10006);Put(scene.fields.bytes,20,65535);
    scene.hooks.assertion=[&](auto expression,auto file,auto line) { scene.Diagnostic(expression,file,line);if (line==0x69) { Put(scene.fields.bytes,4,0);scene.checking=0;scene.state.now=.5; }else { Put(scene.fields.bytes,8,12);Put(scene.fields.bytes,20,10); } };
    scene.Run();CHECK(scene.events==std::vector<std::string>({"A105","A108","U12","P1095237632","B","C"}));CHECK(std::bit_cast<float>(Get(scene.fields.bytes,scene.Previous()))==12.5F);
}
TEST_CASE(MissileChecking_CurrentLateGateReloadsFiringAndKeepsSavedDisplay) { // 후반 진단은 이전 좌표 저장보다 먼저이며 최종 발사자만 새 SID로 다시 조회한다.
    MissileCheckingScene scene;auto ordinaryPop=scene.hooks.pop;scene.hooks.pop=[&](auto sid,auto x,auto y,auto flags) { ordinaryPop(sid,x,y,flags);scene.checking=0x80000000;Put(scene.fields.bytes,4,0x10005); };
    scene.hooks.assertion=[&](auto expression,auto file,auto line) { scene.Diagnostic(expression,file,line);CHECK(std::bit_cast<float>(Get(scene.fields.bytes,scene.Previous()))==-10000.0F);Put(scene.fields.bytes,4,0);Put(scene.fields.bytes,8,14);Put(scene.fields.bytes,20,14);scene.pool.AllocatedBytes(Sid{11})[11]=2; };
    scene.Run();CHECK(scene.events==std::vector<std::string>({"U12","P1097859072","B","C","A105","D","X12"}));CHECK(std::bit_cast<float>(Get(scene.fields.bytes,scene.Previous()))==15.0F);
}
TEST_CASE(MissileChecking_LateMissingDeviceRetainsPriorEffects) { // 진단 장치가 선택되기 전의 이동과 miss 요청 소비를 일괄 사전검사로 되돌리지 않는다.
    MissileCheckingScene scene; // 아래 명시 필드는 원문 후반 조기 반환 입력을 준비한다.
    scene.supplement[128].range=40;Put(scene.fields.bytes,40,1);auto ordinaryPop=scene.hooks.pop;
    scene.hooks.pop=[&](auto sid,auto x,auto y,auto flags) { ordinaryPop(sid,x,y,flags);scene.checking=1;Put(scene.fields.bytes,8,kCapacity); };
    CHECK(Throws([&] { scene.Run(); }));CHECK(scene.events==std::vector<std::string>({"U12","P1097859072","B","C"}));CHECK(Get(scene.fields.bytes,40)==0 && Get(scene.fields.bytes,44)==1 && std::bit_cast<float>(Get(scene.fields.bytes,scene.Previous()))==-10000.0F);
    scene.hooks.pop=ordinaryPop;Put(scene.fields.bytes,8,12);scene.events.clear();scene.Run();CHECK(std::bit_cast<float>(Get(scene.fields.bytes,scene.Previous()))==15.0F && scene.events.size()==4); // 유효 SID면 checking1이어도 assertion 장치는 필요 없다.
}
TEST_CASE(MissileChecking_InvalidPositionSkipsLateGateAndPreviousWrite) { // 좌표 실패 조기 종료는 첫 진단만 거쳐 miss 음향/저장 표시 삭제를 요청한다.
    MissileCheckingScene scene;scene.checking=1;Put(scene.fields.bytes,4,kCapacity);Put(scene.fields.bytes,40,1);scene.Double(104,0);scene.Double(112,0);
    scene.hooks.assertion=[&](auto expression,auto file,auto line) { scene.Diagnostic(expression,file,line);Put(scene.fields.bytes,4,0); };
    scene.hooks.impactSound=[&](auto,auto,auto) { scene.events.push_back("S");Put(scene.fields.bytes,4,kCapacity); };scene.Run();
    CHECK(scene.events==std::vector<std::string>({"A105","S","X12"}));CHECK(std::bit_cast<float>(Get(scene.fields.bytes,scene.Previous()))==-10000.0F);
}
TEST_CASE(MissileChecking_CdHandleDoesNotUsePatchBaseDiagnostic) { // CD0044b260에는 이 checking/0048f710 호출이 없으므로 잘못된 Form DWORD도 여기서 읽지 않는다.
    MissileCheckingScene scene(OriginalEdition::Cd1072);scene.checking=0xffffffff;Put(scene.fields.bytes,4,0x10005);scene.hooks.assertion=[](auto,auto,auto) { throw std::logic_error("CD Handle의 미선택 PATCH 진단"); };scene.Run();
    CHECK(scene.events==std::vector<std::string>({"U12","P1097859072","B","C","Q10","X12"}));
}
TEST_CASE(MissileChecking_DisablingCurrentFlagDuringMovementSkipsLateDiagnostic) { // 앞선 관문이 참이어도 Pop 뒤의 현재 DWORD0이면 후반 Form 진단을 실행하지 않는다.
    MissileCheckingScene scene;scene.checking=0x80000000;auto ordinaryPop=scene.hooks.pop;
    scene.hooks.pop=[&](auto sid,auto x,auto y,auto flags) { ordinaryPop(sid,x,y,flags);scene.checking=0;Put(scene.fields.bytes,4,kCapacity); };scene.Run();
    CHECK(scene.events==std::vector<std::string>({"U12","P1097859072","B","C","Q10","X12"}) && Get(scene.fields.bytes,4)==kCapacity);
}
TEST_CASE(MissileChecking_ActualHostDeletionInFirstDiagnosticStopsFields) { CheckHostDiagnosticDeletion(false); } // 첫 진단의 정상 복귀 뒤 삭제된 raw를 읽지 않는다.
TEST_CASE(MissileChecking_ActualHostDeletionInLateDiagnosticStopsFields) { CheckHostDiagnosticDeletion(true); } // 이동/범위 계산 뒤 삭제되어도 이전 좌표/종료를 반복하지 않는다.
