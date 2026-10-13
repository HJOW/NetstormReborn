// 지정 ProcessForm 재수신·Kernel 중복 포인터/삭제 순서를 저장된 두 실제 PE 관찰과 대조한다.
#include "ConstructionSupport.h"
#include "o/SquidForms.h"
#include "o/SquidProcess.h"

namespace {
using namespace netstorm::test::construction;
// 실제 PE 실행기의 공유 SID 크기와 판본당 독립 입력 수다.
constexpr std::uint32_t kRetakeCapacity=32768,kRetakeRows=194;
// 프로세스 통지/소멸자만 기록하며 실제 풀·Kernel·삭제 동작은 복원 모듈을 사용한다.
class RetakeProcess final:public SquidProcess {
public:
    // 번호/순서와 소멸자 실행 중 슬롯을 관찰하는 저장소를 참조한다.
    RetakeProcess(Kernel& kernel,std::vector<std::string>& events,std::uint32_t index)
        :kernel_(kernel),events_(events),index_(index) {}
    // 이 검사는 프레임 이벤트를 실행하지 않는다.
    void RunFrame() override {}
    // 기존 Form/부모 필드를 원본 가상 +0x14 시점에 기록한다.
    void OnAttached() override { events_.push_back("N:"+std::to_string(Form().value)+':'+std::to_string(Parent().value)); }
    // 원본 통지가 현재 Form PID를 바꾸는 입력을 받아 다음 Kernel 제거의 재조회 여부를 검사한다.
    void OnFormDestroy(std::uint32_t flags) override {
        events_.push_back("P:"+std::to_string(index_)+':'+std::to_string(flags));
        if (redirect) redirect();
    }
    // 원본 scalar deleting destructor의 인자 1과 소멸자 안의 슬롯 존재를 관찰한다.
    ~RetakeProcess() override {
        if (watched) events_.push_back("G:"+std::to_string(index_)+':'+std::to_string(kernel_.Get(watched) ? 1 : 0));
        events_.push_back("D:"+std::to_string(index_)+':'+std::to_string(kernel_.Get(slot)==this ? 1 : 0)+":1");
        if (duringDestructor) duringDestructor();
    }
    // 현재 Kernel 슬롯 출력의 합성 원본 객체 번호다.
    std::uint32_t Index() const { return index_; }
    ProcessId slot{}; // 검사 입력에 지정한 실제 Kernel 번호다.
    ProcessId watched{}; // 다른 소멸자에서 앞 슬롯이 비었는지 조회하는 입력 번호다.
    std::function<void()> redirect; // 삭제 통지 중 PID를 바꾸는 명시 입력 경계다.
    std::function<void()> duringDestructor; // Clear 중 실제 Kernel 등록/제거를 요청하는 입력 경계다.
private:
    Kernel& kernel_; // 소멸자 완료 전 슬롯을 관찰하는 실제 Kernel이다.
    std::vector<std::string>& events_; // 객체보다 오래 사는 순서 목록이다.
    std::uint32_t index_{}; // 합성 원본 객체 번호와 같다.
};
// 실제 풀/Kernel/Form host를 소유하며 fixture 입력으로 raw 슬롯만 공급하는 장면이다.
struct RetakeScene {
    SidPool pool; // 실제 공유 SID 원시 저장소다.
    std::vector<RiftTypeRecord> types; // 판본별 합성 타입 표다.
    SquidHash hash; // 공통 삭제 모듈의 실제 공간 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 자산 해제에 필요한 공간 입력이다.
    SquidUnpop unpop; // 자산/공통 삭제와 같은 풀을 쓴다.
    RawSquidDestroy destroy; // 실제 Form 수명/깊이/반납을 수행한다.
    SquidProcessState state; // 기존 PID 재등록의 타입 순환 상태다.
    std::vector<std::string> events; // Kernel보다 늦게 파괴되어 소멸자 통지도 저장한다.
    Kernel kernel; // 판본별 실제 크기의 단일 소유권/슬롯 표다.
    SquidProcessHost host; // 실제 지정 재수신과 가상 Form 삭제 분배를 연결한다.
    // 판본과 원본 동일한 크기를 공급하며 게임/창을 열지 않는다.
    explicit RetakeScene(OriginalEdition edition):pool(edition,kRetakeCapacity,false),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),unpop(pool,hash,spots),destroy(pool,unpop,types),
        kernel(edition==OriginalEdition::Patch1078 ? kProcessCapacity : kCdProcessCapacity),host(pool,types,kernel,destroy,state) {}
    // 원하는 뒤 슬롯에 객체를 등록하고 앞 임시 객체를 제거해 원본 합성 포인터 배치를 만든다.
    RetakeProcess* Stage(std::uint32_t slot,std::uint32_t index) {
        std::vector<ProcessId> temporary;
        // 앞의 빈 슬롯은 잠시 채웠다가 대상 등록 뒤 제거한다.
        while (kernel.Size()<slot-1) {
            auto entry=std::make_unique<RetakeProcess>(kernel,events,99);auto* pointer=entry.get();
            pointer->slot=kernel.Add(std::move(entry));temporary.push_back(pointer->slot);
        }
        auto entry=std::make_unique<RetakeProcess>(kernel,events,index);auto* result=entry.get();
        result->slot=kernel.Add(std::move(entry));CHECK(result->slot==slot);
        // 이미 있던 대상 슬롯은 남기고 이번에 넣은 임시 객체만 제거한다.
        for (auto id:temporary) kernel.Remove(id);
        return result;
    }
};
// 원본에서 저장한 순서 목록을 같은 구분자로 읽는 직렬화다.
std::string RetakeEvents(const std::vector<std::string>& events) {
    std::string text;
    // 통지 순서를 바꾸거나 정렬하지 않는다.
    for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
    return text.empty() ? "-" : text;
}
// 원본에서 관찰한 FIFO·개수·예측 커서를 직접 읽는다.
std::string RetakeMeta(const SidPool& pool) {
    return std::to_string(pool.FreeCount())+':'+std::to_string(pool.PredictableCursor())+':'+
        std::to_string(Get(pool.Slot(Sid{0}),4,2))+':'+std::to_string(pool.Tail(true).value)+':'+
        std::to_string(Get(pool.Slot(Sid{0}),18,2))+':'+std::to_string(pool.Tail(false).value);
}
// 기대값과 무관한 합성 raw 입력을 원본 실행기와 같은 바이트로 공급한다.
void RetakeRaw(RetakeScene& scene,std::string_view operation,const std::vector<std::uint32_t>& input) {
    const Sid form{static_cast<std::uint16_t>(input[1])};const bool patch=scene.pool.Edition()==OriginalEdition::Patch1078;
    auto raw=InputRaw(scene.pool,form),parent=InputRaw(scene.pool,Sid{20000}),head=InputRaw(scene.pool,Sid{50});
    // 세 슬롯의 입력 payload 표식이다. 실행 출력은 fixture만 기대값으로 사용한다.
    for (auto bytes:{raw,parent,head}) for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(i*29+7);
    parent[10]=100;parent[11]=0;Put(parent,6,operation=="Free" ? (input[3] ? 50 : 0) : form.value,2);
    Put(raw,0,patch ? 0x512098U : 0x505990U);raw[10]=46;raw[11]=static_cast<std::uint8_t>(input[2]);
    Put(raw,4,50,2);Put(raw,6,0,2);Put(raw,14,0,2);Put(raw,16,20000,2);Put(raw,18,input[0]);
    Put(head,0,input[3]==1 ? (patch ? 0x512098U : 0x505990U) : (patch ? 0x501dd0U : 0x5058d8U));
    head[10]=input[3]==1 ? 46 : 100;head[11]=8;Put(head,4,0,2);Put(head,14,form.value,2);
}
// 저장된 독립 실제 PE 입력을 두 판본의 복원 모듈에 적용한다.
void ReplayRetake(std::string_view edition) {
    const auto kind=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    static const auto rows=LoadFixture(NETSTORM_PROCESSRETAKE_FIXTURE).rows;CHECK(rows.size()==2*kRetakeRows);std::size_t count=0;
    // 모든 결과는 현재 fixture의 실제 명령 관찰과 비교한다.
    for (const auto& row:rows) {
        if (row[0]!=edition) continue;CHECK(row.size()==13);const auto input=Inputs(row[2]);CHECK(input.size()==8);
        RetakeScene scene(kind);const Sid form{static_cast<std::uint16_t>(input[1])};auto* source=scene.Stage(input[0],0);
        auto raw=InputRaw(scene.pool,form),parent=InputRaw(scene.pool,Sid{20000});parent[10]=100;parent[11]=0;Put(parent,6,0,2);raw[11]=1;
        // 소유 객체의 C++ 필드만 정상 부착으로 준비한다. 이후 raw/회전/사건은 입력 상태로 되돌린다.
        if (row[1]=="Register" || row[1]=="Delete" || row[1]=="Clear") CHECK(scene.host.Reattach(*source,46,Sid{20000},0x50,form)==source);
        auto* later=(input[7] || row[1]=="Clear") ? scene.Stage(9,1) : nullptr;
        RetakeRaw(scene,row[1],input);scene.state.typeRotation=kind==OriginalEdition::Patch1078 ? input[6] : 0;scene.events.clear();
        if (row[1]=="Delete" && input[7]) source->redirect=[&scene,form] { Put(InputRaw(scene.pool,form),18,9); };
        if (row[1]=="Clear") {
            later->watched=input[0];
            // 원본 소멸자 경계 안에서도 등록/제거 자체는 실제 Kernel 메서드를 호출한다.
            if (input[7]==1 || input[7]==2) {
                auto registerNext=[&scene] {
                    auto process=std::make_unique<RetakeProcess>(scene.kernel,scene.events,2);auto* pointer=process.get();
                    pointer->slot=scene.kernel.Add(std::move(process));scene.events.push_back("A:2:"+std::to_string(pointer->slot));
                };
                if (input[7]==1) later->duringDestructor=registerNext;else source->duringDestructor=registerNext;
            } else if (input[7]==3) source->duringDestructor=[&scene] { scene.kernel.Remove(9);scene.events.push_back("R:9"); };
        }
        std::uint32_t result=0;auto fields=std::to_string(source->Form().value)+':'+std::to_string(source->Parent().value);
        if (row[1]=="Live" || row[1]=="Free") CHECK(scene.host.Reattach(*source,input[4],Sid{20000},input[5],form)==source);
        else if (row[1]=="Register") result=scene.kernel.Add(*source);
        else if (row[1]=="Clear") scene.kernel.Clear();
        else scene.destroy.Destroy(form,input[5],scene.host.Hooks());
        // Clear 중 새 객체가 해제 주소를 재사용할 수 있으므로 종료 객체의 필드는 살아 있을 때 저장한 값만 쓴다.
        if (row[1]!="Clear" && scene.kernel.Get(input[0])==source) fields=std::to_string(source->Form().value)+':'+std::to_string(source->Parent().value);
        std::string slots;
        // 두 실제 PE의 관찰 범위와 같은 앞 64개 슬롯만 직접 읽는다.
        for (ProcessId id=1;id<65;++id) if (auto* process=dynamic_cast<RetakeProcess*>(scene.kernel.Get(id))) {
            if (!slots.empty()) slots+=';';slots+=std::to_string(id)+':'+std::to_string(process->Index());
        }
        if (slots.empty()) slots="-";
        const bool same=RetakeEvents(scene.events)==row[3] && Hex(scene.pool.Slot(form))==row[4] &&
            Hex(scene.pool.Slot(Sid{20000}))==row[5] && Hex(scene.pool.Slot(Sid{50}))==row[6] && fields==row[7] && slots==row[8] &&
            (row[1]=="Register" || (row[1]=="Clear" && (input[7]==1 || input[7]==2)) || std::to_string(scene.state.typeRotation)==row[9]) && RetakeMeta(scene.pool)==row[10] &&
            std::to_string(scene.destroy.PreDepth())+','+std::to_string(scene.destroy.PostDepth())+",0"==row[11] && std::to_string(result)==row[12];
        CHECK(same);if (!same) { std::printf("ProcessForm 재수신 %s %s 행 %zu 불일치\n",std::string(edition).c_str(),row[1].c_str(),count);break; }++count;
    }
    CHECK(count==kRetakeRows);
}
}

// 패치판의 live 무수정·free Take·중복 포인터·삭제 PID 재조회/소멸자 순서를 대조한다.
TEST_CASE(ProcessRetake_ReplaysOriginals) { ReplayRetake("originals"); }
// CD판의 별도 몸체와 3999개 Kernel 범위를 동일한 독립 관찰과 대조한다.
TEST_CASE(ProcessRetake_ReplaysCd) { ReplayRetake("originalCD"); }

// live 지정 수신의 신규 소유권/다른 부모·다른 PID는 원본 assert의 안전한 입력 경계다.
TEST_CASE(ProcessRetake_RejectsMismatchedLiveOwnership) {
    RetakeScene scene(OriginalEdition::Patch1078);auto* process=scene.Stage(7,0);const Sid form{15000};
    auto parent=InputRaw(scene.pool,Sid{20000});parent[10]=100;parent[11]=0;Put(parent,6,0,2);
    CHECK(scene.host.Reattach(*process,46,Sid{20000},0x50,form)==process);const auto before=Hex(scene.pool.Slot(form));scene.events.clear();
    Put(InputRaw(scene.pool,form),18,1);CHECK(Throws([&] { scene.host.Reattach(*process,30,Sid{20000},0x50,form); }));
    Put(InputRaw(scene.pool,form),18,7);auto second=InputRaw(scene.pool,Sid{20001});second[10]=100;second[11]=0;
    CHECK(Throws([&] { scene.host.Reattach(*process,30,Sid{20001},0x50,form); }));
    CHECK(Hex(scene.pool.Slot(form))==before && scene.kernel.Size()==1 && scene.events.empty());
    RegularProcess unowned(1,0);CHECK(Throws([&] { scene.kernel.Add(unowned); }));
}

// 패치판은 부모 상태를 확인하기 전에 두 SID를 기록하고 free/dead 부모에서는 통지를 생략한다.
TEST_CASE(ProcessRetake_PatchRecordsSidsBeforeInactiveParent) {
    RetakeScene scene(OriginalEdition::Patch1078);auto* process=scene.Stage(7,0);const auto before=RetakeMeta(scene.pool);
    // free와 dead의 원본 조기 반환은 Kernel 등록/회전/Form 쓰기를 수행하지 않는다.
    for (auto state:{std::uint8_t{1},std::uint8_t{2}}) {
        const Sid form{15000},parent{20000};InputRaw(scene.pool,parent)[11]=state;scene.events.clear();
        const auto raw=Hex(scene.pool.Slot(form));CHECK(scene.host.Reattach(*process,46,parent,0x50,form)==nullptr);
        CHECK(process->Form()==form && process->Parent()==parent && scene.state.typeRotation==0 && scene.events.empty());
        CHECK(Hex(scene.pool.Slot(form))==raw && scene.kernel.Size()==1 && RetakeMeta(scene.pool)==before);
    }
}

// 호스트 종료 정리는 두 배열이 살아 있는 동안 수행하고 원본 Clear에서 남는 앞 신규 슬롯도 후속 정리한다.
TEST_CASE(ProcessRetake_DestructorKeepsSlotLifetimeThroughReentry) {
    std::vector<std::string> events;
    {
        Kernel kernel(8);auto first=std::make_unique<RetakeProcess>(kernel,events,0);auto* a=first.get();a->slot=kernel.Add(std::move(first));
        auto second=std::make_unique<RetakeProcess>(kernel,events,1);auto* b=second.get();b->slot=kernel.Add(std::move(second));b->watched=1;
        b->duringDestructor=[&kernel,&events] {
            auto next=std::make_unique<RetakeProcess>(kernel,events,2);auto* c=next.get();c->slot=kernel.Add(std::move(next));
            events.push_back("A:2:"+std::to_string(c->slot));
        };
    }
    CHECK(RetakeEvents(events)=="D:0:1:1;G:1:0;D:1:1:1;A:2:1;D:2:1:1");
}
