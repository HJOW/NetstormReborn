// 원본 Path의 전체 생성자 이미지·시간표·한 단계씩 움직이는 보간을 독립 native 결과와 비교한다.
#include "RawSceneSupport.h"
#include "o/RawPathProcess.h"
#include "o/SquidFactory.h"
#include "o/SquidPop.h"
#include "o/SquidPostPop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// fixture의 소수 좌표는 float 입력으로 먼저 좁힌다.
const std::array<RawPathPoint,4> kPoints{{{10,20},{13.1F,24.7F},{15.2F,19.8F},{23.9F,27.6F}}};
// 원본 시간표 비교 입력의 방향/초기 시각이다.
const std::array<std::int32_t,4> kDirections{7,0,4,6};
const std::array<double,4> kTimes{0,1,2,3};
// 독립 생성자 byte 관찰과 같은 FNV32를 계산한다.
std::uint32_t Fnv(std::span<const std::uint8_t> bytes) {
    std::uint32_t value=2166136261U;
    // 미초기화 영역까지 원본 byte 전체와 비교한다.
    for (const auto byte:bytes) value=(value^byte)*16777619U;
    return value;
}
// DWORD/QWORD 입력을 IEEE float/double로 해석한다.
float Float(const std::string& value) { return std::bit_cast<float>(Number(value)); }
// double의 64비트 패턴을 잘라내지 않는다.
double Double(const std::string& value) { return std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(value))); }
// 두 native 판본 이름을 원본 판본 enum으로 바꾼다.
OriginalEdition Edition(const std::string& value) { return value=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072; }
// 실제 타입 번호와 글자 구간/부모 입력이다. 네이티브의 합성 부모와 같은 값을 준비한다.
std::vector<RiftTypeRecord> ProcessTypes(OriginalEdition edition) {
    std::vector<RiftTypeRecord> result(edition==OriginalEdition::Patch1078 ? 188 : 171);
    result[134].footX=result[134].footY=result[158].footX=result[158].footY=1;return result;
}
// 부모의 외부 가상 이벤트는 native 입력 스크립트다. 실제 공간/Kernel/Form 수명은 복원된 모듈들을 사용한다.
struct PathScene {
    // edition: 판본별 필드 배치와 동작을 선택하는 원본 버전. 입력 구성 또는 실행 결과 대조에 사용한다.
    OriginalEdition edition;
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
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536,6);
    // book: 등록 목록과 개수의 장부. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidPostPopState book;
    // postPop: 객체 Pop 이후 목록·공간 후처리를 수행하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidPostPop postPop;
    // unpop: 객체의 공간 등록을 해제하는 Unpop 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidUnpop unpop;
    // pop: 객체를 공간 지도에 등록하는 Pop 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidPop pop;
    // destroy: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    RawSquidDestroy destroy;
    // kernel: 프로세스 등록과 프레임 실행을 담당하는 커널. 입력 구성 또는 실행 결과 대조에 사용한다.
    Kernel kernel;
    // clock: 프레임 판정에 현재 게임 시각을 공급하는 시계. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidProcessState clock;
    // host: Form이나 프로세스의 등록·통지·수명을 연결하는 호스트. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidProcessHost host;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    RawPathProcessState state;
    // factory: 타입에 맞는 객체를 생성하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidFactory factory;
    // parent: 부착·통지 대상인 부모 객체 또는 부모 SID. 입력 구성 또는 실행 결과 대조에 사용한다.
    Sid parent;
    // process: same pool/hash/spot/Kernel 위에서 원본 이동 프로세스를 실행한다. 별도 좌표/공간 복사본을 만들지 않는다 (RawPathProcess).
    RawPathProcess* process{};
    // pid: 커널의 프로세스 등록 번호. 입력 구성 또는 실행 결과 대조에 사용한다.
    ProcessId pid{};
    // returnCode: 반환 코드. 입력 구성 또는 실행 결과 대조에 사용한다.
    // haltCode: 중단 코드. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint32_t returnCode{1002},haltCode{10};
    // willDelete: 예정 삭제 여부. 입력 구성 또는 실행 결과 대조에 사용한다.
    // deleted: 삭제된 여부. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool willDelete{},deleted{};
    // finalImage: 최종 이미지. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint8_t> finalImage;
    // events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::string> events;
    // externalDeletion: 외부 삭제. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<SquidDestroyEvent> externalDeletion;
    // native 입력의 parent5/Form7은 client 번호다. 합성 local flags0x10 부착이며 actual 서버 Path flags0와 구별한다.
    PathScene(OriginalEdition chosen,bool priest):edition(chosen),pool(chosen,32768,false),types(ProcessTypes(chosen)),
        frames(types.size(),RiftTypeFrames({})),shapes(types.size()),postPop(pool,types,book),unpop(pool,hash,spots),pop(pool,hash,spots,nullptr,&postPop),destroy(pool,unpop,types),kernel(32),
        host(pool,types,kernel,destroy,clock,{},SquidDestroyHooks{[this](const SquidDestroyEvent& event) { externalDeletion.push_back(event); },[] { return Sid{}; },{}}),state{clock},factory(pool,types) {
        book.suppressed=true;
        std::vector<FrameCode> codes;
        // 실제 letterRun 입력은 방향마다 두 프레임이다.
        for (int direction=0;direction<8;++direction) {
            codes.push_back({static_cast<std::uint8_t>('A'+direction),'P',1,0});codes.push_back({static_cast<std::uint8_t>('A'+direction),'P',2,0});
        }
        const std::uint32_t type=priest ? 158 : 134;frames[type]=RiftTypeFrames(std::move(codes));shapes[type].loaded=true;shapes[type].frameCount=16;shapes[type].frames.resize(16);
        // 원본 shape helper가 읽는 칸 hotspot을 물리 프레임마다 제공한다.
        for (auto& shape:shapes[type].frames) { shape.cellWidth=shape.cellHeight=1;shape.cellHotspotX=0.3F;shape.cellHotspotY=0.2F; }
        parent=factory.Create(type,2);CHECK(pop.Pop(parent,types[type],1,1,10,20,0)==RawPopResult::Registered);
        // 원본 입력의 Form7을 맞춘다. 중간 번호6은 할당된 void 입력이며 경로/부모에 부착하지 않는다.
        CHECK(parent==Sid{5});CHECK(pool.Allocate(2)==Sid{6});
        auto next=std::make_unique<RawPathProcess>(pool,hash,types,frames,shapes,spots,state,Hooks(),priest);
        const std::array<RawPathPoint,3> points{{{10,20},{15,20},{17,22}}};const std::array<std::int32_t,3> directions{0,0,2};const std::array<double,3> times{0,1,3};next->Runtime().SetPath(points,directions,times);
        process=RawPathProcess::Attach(host,std::move(next),parent,0x10);CHECK(process!=nullptr);CHECK(process->Form()==Sid{7});pid=kernel.Find(*process);events.clear();
    }
    // 살아 있는 Kernel 포인터만 제거하며 부모/효과의 수명보다 앞서 정리한다.
    ~PathScene() { kernel.Clear(); }
    // 요청 순서를 fixture와 같은 문자열로 만든다.
    void Event(std::string value) { events.push_back(std::move(value)); }
    // 원본 POP/UNPOP은 actual 등록을 바꾸고 부모 virtual/소리 경계는 입력 script로 응답한다.
    RawPathProcessHooks Hooks() {
        RawPathProcessHooks hooks;
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { if (!willDelete) Event("unpop,"+std::to_string(flags));unpop.Unpop(sid,types[pool.Slot(sid)[10]],flags); };
        hooks.pop=[this](Sid sid,RawPathPoint point,std::uint32_t flags) {
            if (!willDelete) Event("pop,"+std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+std::to_string(std::bit_cast<std::uint32_t>(point.y))+','+std::to_string(flags));
            CHECK(pop.Pop(sid,types[pool.Slot(sid)[10]],1,1,point.x,point.y,flags)==RawPopResult::Registered);
        };
        hooks.repop=[this](Sid sid,std::uint32_t flags) {
            if (!willDelete) Event("repop,"+std::to_string(flags));const auto raw=pool.Slot(sid);
            CHECK(pop.Pop(sid,types[raw[10]],1,1,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),flags)==RawPopResult::Registered);
        };
        hooks.halt=[this](Sid) { Event("halt,"+std::to_string(haltCode));if (haltCode==11) willDelete=true;return haltCode; };
        hooks.arrival=[this](Sid,std::uint32_t first,std::uint32_t second) {
            Event("arrival,"+std::to_string(first)+','+std::to_string(second)+','+std::to_string(returnCode));
            if (returnCode!=1002) willDelete=true;
            if (returnCode==1001) host.Kill(*process,0);return returnCode;
        };
        hooks.soundId=[](Sid) { return 321U; };
        hooks.stepSound=[this](Sid,float x,float y,std::uint32_t sound) { Event("sound,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(sound)); };
        hooks.observationStamp=[] { return std::uint64_t{0x12345678}; };
        hooks.clearBlocked=[this](Sid,RawPathPoint,std::uint32_t,std::uint32_t) { Event("clearBlocked"); };
        hooks.cleanup=[this](RawPathRuntime& runtime,Sid,std::uint32_t flags) {
            deleted=true;finalImage.assign(runtime.Bytes().begin(),runtime.Bytes().end());
            if (returnCode!=1001 || haltCode==11) Event("kill,"+std::to_string(flags));
        };
        return hooks;
    }
    // 원본 current/angle를 입력에서 공급하는 public API를 사용한다.
    void AdvanceInput(std::int32_t current,float angle) {
        process->Runtime().RestoreStep(current,angle);
    }
    // 살아 있는 객체 또는 삭제 직전의 raw 이미지를 같은 필드 폭으로 비교한다.
    std::string Fields() const {
        const auto layout=RawPathRuntime::Layout(edition);const auto bytes=deleted ? std::span<const std::uint8_t>(finalImage) : process->Runtime().Bytes();const auto raw=pool.Slot(parent);
        return std::to_string(Get(raw,14))+','+std::to_string(Get(raw,18))+','+std::to_string(Get(raw,edition==OriginalEdition::Patch1078 ? 36 : 34,edition==OriginalEdition::Patch1078 ? 4 : 1))+','+
            std::to_string(Get(bytes,layout.current))+','+std::to_string(Get(bytes,layout.angle))+','+std::to_string(Get(bytes,layout.finished))+','+std::to_string(deleted ? 1 : 0);
    }
    // 외부 가상 요청과 실제 공간 요청을 한 순서로 비교한다.
    std::string Events() const {
        std::string result;
        // 요청은 원본 발생 순서다.
        for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }
        return result.empty() ? "-" : result;
    }
};
}
// 생성자에서 쓰지 않은 꼭짓점/방향/빈 필드도 같은 byte로 남아야 한다.
TEST_CASE(rawPathRuntime_native_constructor) {
    const auto fixture=LoadFixture(NETSTORM_PATHRUNTIME_FIXTURE);std::size_t count=0;
    // 두 derived 생성자와 두 heap 입력을 모두 확인한다.
    for (const auto& row:fixture.rows) if (row[0]=="ctor") {
        const RawPathRuntime path(Edition(row[1]),Number(row[2])!=0,static_cast<std::uint8_t>(Number(row[3])));
        CHECK(Fnv(path.Bytes())==Number(row[4]));++count;
    }
    CHECK(count==8);
}
// 각 꼭짓점의 double 시각을 오차 허용 없이 비교한다.
TEST_CASE(rawPathRuntime_native_prepare) {
    const auto fixture=LoadFixture(NETSTORM_PATHRUNTIME_FIXTURE);std::size_t count=0;
    // 속도·회전 속도·2초 경계의 독립 입력이다.
    for (const auto& row:fixture.rows) if (row[0]=="prepare") {
        RawPathRuntime path(Edition(row[1]));path.SetPath(kPoints,kDirections);
        path.Prepare(Double(row[2]),Double(row[3]),Float(row[4]),Float(row[5]),7);const auto expected=Split(row[6],',');
        // 예상 시각을 코드에서 다시 계산하지 않고 원본 QWORD와 대조한다.
        for (std::int32_t i=0;i<4;++i) CHECK(std::bit_cast<std::uint64_t>(path.Time(i))==std::stoull(expected[i]));
        ++count;
    }
    CHECK(count==48);
}
// Step 호출 연쇄에서 같은 현재 단계와 각도 필드가 계속 이어져야 한다.
TEST_CASE(rawPathRuntime_native_step) {
    const auto fixture=LoadFixture(NETSTORM_PATHRUNTIME_FIXTURE);std::size_t count=0;
    std::unique_ptr<RawPathRuntime> path;RawPathObservation observation;std::string lastEdition;int lastProfile=-1;
    // 각 profile은 처음만 초기화하고 원본처럼 한 꼭짓점씩 다음 호출에 이어진다.
    for (const auto& row:fixture.rows) if (row[0]=="step") {
        const int profile=std::stoi(row[2]);
        if (lastEdition!=row[1] || lastProfile!=profile) {
            path=std::make_unique<RawPathRuntime>(Edition(row[1]));const std::array<std::int32_t,4> directions{profile,(profile+1)%8,(profile+5)%8,profile};
            path->SetPath(kPoints,directions,kTimes);observation={};lastEdition=row[1];lastProfile=profile;
        }
        CHECK(path->Current()==static_cast<std::int32_t>(Number(row[4])));CHECK(std::bit_cast<std::uint32_t>(path->Angle())==Number(row[5]));
        std::size_t sounds=0;const auto sample=path->Step(Double(row[3]),{10,20},observation,[&sounds] { ++sounds; },0x12345678);
        const auto expected=Split(row[6],',');
        CHECK(std::bit_cast<std::uint32_t>(sample.position.x)==Number(expected[0]));CHECK(std::bit_cast<std::uint32_t>(sample.position.y)==Number(expected[1]));
        CHECK(std::bit_cast<std::uint32_t>(sample.angle)==Number(expected[2]));CHECK(path->Current()==static_cast<std::int32_t>(Number(expected[3])));
        CHECK(std::bit_cast<std::uint32_t>(path->Angle())==Number(expected[4]));CHECK(sample.finished==(Number(expected[5])!=0));CHECK(sounds==Number(expected[6]));++count;
    }
    CHECK(count==160);
}
// 원본의 실패 상태를 직선 경로나 빈 효과로 성공시켜서는 안 된다.
TEST_CASE(rawPathRuntime_reject_invalid) {
    RawPathRuntime path(OriginalEdition::Patch1078);RawPathObservation observation;
    CHECK(Throws([&] { path.Prepare(0,0,1,1,0); }));CHECK(Throws([&] { path.Step(0,{1,1},observation,{}); }));
    const std::array<RawPathPoint,1> point{{{1,1}}};const std::array<std::int32_t,1> direction{0};path.SetPath(point,direction);
    CHECK(Throws([&] { path.Prepare(0,0,0,1,0); }));CHECK(Throws([&] { path.Prepare(0,0,1,0,0); }));
    CHECK(RawPathRuntime::DirectionDelta(0,4)==180);CHECK(RawPathRuntime::DirectionDelta(7,0)==45);CHECK(RawPathRuntime::DirectionDelta(0,7)==-45);
}
// 실제 Kernel에서 type23/type54 RunFrame을 돌리고 raw 공간·프레임·필드·가상 요청 순서를 대조한다.
TEST_CASE(rawPathProcess_native_frame) {
    const auto fixture=LoadFixture(NETSTORM_PATHRUNTIME_FIXTURE);std::size_t count=0;
    // 각 입력마다 동일한 실제 공간/Form/Kernel을 새로 준비한다.
    for (const auto& row:fixture.rows) if (row[0]=="frame") {
        PathScene scene(Edition(row[1]),Number(row[2])!=0);scene.AdvanceInput(static_cast<std::int32_t>(Number(row[3])),Float(row[5]));scene.clock.now=Double(row[4]);
        scene.kernel.RunFrame();CHECK(scene.Fields()==row[6]);CHECK(scene.Events()==row[7]);++count;
    }
    CHECK(count==120);
}
// 1001 자기 삭제 뒤 this를 읽지 않고 1002는 실제 Kernel/Form에 그대로 남는다.
TEST_CASE(rawPathProcess_native_arrival) {
    const auto fixture=LoadFixture(NETSTORM_PATHRUNTIME_FIXTURE);std::size_t count=0;
    // 두 판본/두 process/세 반환값/두 Halt를 전부 대조한다.
    for (const auto& row:fixture.rows) if (row[0]=="arrival") {
        PathScene scene(Edition(row[1]),Number(row[2])!=0);scene.returnCode=Number(row[3]);scene.haltCode=Number(row[4]);scene.AdvanceInput(3,0);scene.clock.now=3;
        scene.process->Arrive();CHECK(scene.Fields()==row[5]);CHECK(scene.Events()==row[6]);CHECK((scene.kernel.Size()!=0)==(scene.returnCode==1002 && scene.haltCode==10));++count;
    }
    CHECK(count==24);
}
// 실제 경로 탐색을 연결하지 않으면 Init은 명시적으로 멈춘다. 타입/풀 혼용도 부착 전에 거부한다.
TEST_CASE(rawPathProcess_missing_search_and_pool) {
    PathScene scene(OriginalEdition::Patch1078,true);
    // Init은 실제 표면 번호가 있을 때만 검색 경계를 요구한다. 이 검사는 해당 WORD 입력을 명시 공급한다.
    const auto surface=scene.factory.Create(83,2);scene.hash.Cell(0,10,20)=surface.value;
    CHECK(Throws([&] { scene.process->InitializePath(scene.parent,{15,20},{10,20}); }));
    CHECK(scene.kernel.Size()==1);CHECK(scene.process->Runtime().Current()==0);
    scene.types[158].flags2|=0x10000;auto* freeze=scene.host.AddRegular(scene.parent,0x5105,100);
    CHECK(freeze!=nullptr);scene.clock.now=0.5;scene.kernel.RunFrame();CHECK(scene.process->Runtime().Current()==0);CHECK(scene.Events()=="-");
}
