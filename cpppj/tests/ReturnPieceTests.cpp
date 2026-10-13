// 생산 칸 반환/contained 생성의 독립 PE 관찰과 실제 factory·Regular·상위 호출 연결을 검사한다.
#include "ConstructionSupport.h"
#include "client/RawCursorReturnPiece.h"

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 원본 실행기의 합성 부모와 생성 번호다. 주변 raw 바이트도 fixture와 함께 비교한다.
constexpr Sid kParent{77},kBorn{100};
// 입력 열은 원본 실행기와 같으며 기대 출력은 저장 fixture만 사용한다.
enum Column : std::size_t { Type,Flags,Quality,Keep,Fort,Index,Seed,Argument,AttachFlags,RawType,
    Allocation,FactoryChange,NotifyChange,AttachChange,AllocateChange };
// 저장 관찰을 한 번 읽으며 원본 PE/Python을 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_RETURNPIECE_FIXTURE).rows;return rows; }
// 원본 명시 효과와 현재 전역 변경을 그대로 공급하는 장면이다.
struct ReturnScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;
    // containedState: 원본 contained/bridge 타입 전역이다. 외부 효과가 바꾼 값은 사용 시점에 다시 읽는다 (ContainedCreateState).
    ContainedCreateState containedState;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    CursorReturnPieceState state;
    // random: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom).
    GameRandom random;
    // input: 처리할 입력 또는 입력 분배기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint32_t> input;
    // events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::string events;
    // 실제 모듈 참조는 행 사이에 유지한다.
    explicit ReturnScene(OriginalEdition edition):pool(edition,kCapacity,true) {}
    // 사건을 DWORD 십진수로 기록한다.
    void Append(char letter,std::initializer_list<std::uint32_t> values) {
        if (!events.empty()) events+=';';events+=letter;
        // 원본 사건 인자 순서와 폭을 보존한다.
        for (const auto value:values) events+=':'+std::to_string(value);
    }
    // raw 슬롯의 주변 바이트와 전역 입력만 준비한다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        input=values;events.clear();containedState={};state={};state.type=values[Type];state.keepPieces=values[Keep];
        state.fort=values[Fort];state.lifetimeIndex=static_cast<std::uint8_t>(values[Index]);random.SetState(values[Seed]);
        auto raw=InputRaw(pool,kBorn);
        // 모든 주변 바이트에 서로 다른 값을 두어 좁은 쓰기 폭을 검사한다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*37+11);
        Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(values[RawType]);raw[11]=4;
    }
    // 생성/알림/부착의 실제 관찰과 경계 변경만 공급한다.
    ContainedCreateHooks ContainedHooks() {
        ContainedCreateHooks hooks;
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) {
            Append('C',{type,flags});if (input[FactoryChange]) { containedState.bridgeType=6;containedState.containedType=7;state.bridgeType=6;state.type=101; }return kBorn;
        };
        hooks.notifySurface=[this](Sid sid) {
            Append('N',{sid.value});events+=':'+Hex(pool.Slot(sid));
            if (input[NotifyChange]) { auto raw=InputRaw(pool,sid);Put(raw,12,0x9876,2);Put(raw,20,0xffff,2);raw[pool.Edition()==OriginalEdition::Patch1078 ? 30 : 26]=0xfe; }
        };
        hooks.attach=[this](Sid sid,Sid parent,std::uint32_t flags) {
            Append('J',{sid.value,parent.value,flags});events+=':'+Hex(pool.Slot(sid));
            if (input[AttachChange]) { state.keepPieces^=1;state.type=102; }
        };
        return hooks;
    }
    // 할당 성공/실패와 할당 뒤 현재 시간 옵션 변경을 공급한다.
    CursorReturnPieceHooks ReturnHooks() {
        CursorReturnPieceHooks hooks;
        hooks.allocateTimer=[this](std::uint32_t size) {
            Append('A',{size});if (input[AllocateChange]) { state.fort^=1;state.lifetimeIndex=2; }return input[Allocation]!=0;
        };
        hooks.createTimer=[this](Sid parent,std::uint32_t event,float duration) { Append('T',{event,parent.value,Bits(duration)}); };
        return hooks;
    }
    // 원본 실행기의 최종 전역/난수/반환 SID 8개를 같은 순서로 관찰한다.
    std::string Snapshot(std::uint32_t result) const {
        const std::array values{state.type,containedState.bridgeType,containedState.containedType,state.keepPieces,state.fort,
            random.State(),static_cast<std::uint32_t>(state.lifetimeIndex),result};std::string text;
        // 관찰 칸은 원본의 저장 순서대로 적는다.
        for (const auto value:values) text+=(text.empty() ? "" : ",")+std::to_string(value);
        return text;
    }
};
// 각 판본의 Return/Create 관찰을 원본을 실행하지 않고 재생한다.
void Replay(std::string_view edition) {
    ReturnScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 기대 결과는 독립 저장값과만 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==6);const auto v=Inputs(row[2]);CHECK(v.size()==15);scene.Prepare(v);
        RawContainedCreate contained(scene.pool,scene.containedState,scene.ContainedHooks());
        RawCursorReturnPiece body(contained,scene.state,scene.random,scene.ReturnHooks());std::uint32_t result=0;
        if (row[1]=="Return") body.Return(v[Flags],kParent.value,v[Quality]);
        else if (row[1]=="Create") result=contained.Create(kParent,v[Type],v[Argument],v[Flags],v[AttachFlags],v[Quality]).value;
        else {
            scene.pool.Reset();std::vector<RiftTypeRecord> types(scene.pool.Edition()==OriginalEdition::Patch1078 ? 188 : 171);
            SquidFactory factory(scene.pool,types);scene.Append('S',{v[Flags]});result=factory.CreateContained(v[Flags]).value;
        }
        const auto raw=scene.pool.Slot(row[1]=="Factory" ? Sid{static_cast<std::uint16_t>(result)} : kBorn);
        const bool same=scene.events==row[3] && Hex(raw)==row[4] && scene.Snapshot(result)==row[5];CHECK(same);
        if (!same) { std::printf("조각 반환 %s 행 %zu 불일치\n입력 %s\n기대 %s / %s / %s\n실제 %s / %s / %s\n",std::string(edition).c_str(),count,row[2].c_str(),row[3].c_str(),row[4].c_str(),row[5].c_str(),scene.events.c_str(),Hex(scene.pool.Slot(kBorn)).c_str(),scene.Snapshot(result).c_str());break; }++count;
    }
    CHECK(count==633 && count*3==Fixture().size());
}
}
// 패치판 품질/플래그 쓰기와 SEH 정상 반환 관찰이다.
TEST_CASE(ReturnPiece_ReplaysOriginals) { Replay("originals"); }
// CD 단어 helper와 품질 위치/쓰기 순서도 비교한다.
TEST_CASE(ReturnPiece_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE의 독립 실행 결과를 비교한다.
TEST_CASE(ReturnPiece_ReplaysExtra1037) { Replay("original1037"); }

// 호스트 계약 오류는 거부하고 할당 실패/다리 경로는 시간 번호를 사용하지 않는다.
TEST_CASE(ReturnPiece_RejectsInvalidContracts) {
    ReturnScene scene(OriginalEdition::Patch1078);scene.Prepare(Inputs(Fixture().front()[2]));
    CHECK(Throws([&] { RawContainedCreate bad(scene.pool,scene.containedState,{}); }));
    RawContainedCreate contained(scene.pool,scene.containedState,scene.ContainedHooks());
    CHECK(Throws([&] { RawCursorReturnPiece bad(contained,scene.state,scene.random,{}); }));
    RawCursorReturnPiece body(contained,scene.state,scene.random,scene.ReturnHooks());
    CHECK(Throws([&] { body.Return(8,0,3); }));CHECK(Throws([&] { body.Return(8,0xffffffff,3); }));CHECK(scene.events.empty());
    scene.state.lifetimeIndex=3;CHECK(Throws([&] { body.Return(8,kParent.value,3); }));
    scene.input[Allocation]=0;body.Return(8,kParent.value,3);scene.state.type=82;body.Return(8,kParent.value,3);
    CHECK(&body.Pool()==&scene.pool);auto hooks=scene.ContainedHooks();hooks.create=[](std::uint32_t,std::uint32_t) { return Sid{}; };
    RawContainedCreate invalid(scene.pool,scene.containedState,hooks);CHECK(Throws([&] { invalid.Create(kParent,100,0,0,0,0); }));
    SidPool other(OriginalEdition::Patch1078,kCapacity);std::vector<RiftTypeRecord> types(188);SquidFactory factory(scene.pool,types);
    CHECK(Throws([&] { MakeContainedCreateHooks(other,factory,scene.ContainedHooks()); }));
    CHECK(Throws([&] { MakeReturnPieceHooks(other,body,ConstructionLocalHooks{}); }));CHECK(Throws([&] { MakeReturnPieceHooks(other,body,CursorDropHooks{}); }));
}

// 실제 factory로 생성한 반환 조각에 Regular/ProcessForm/Kernel을 붙여 세 소멸 시간의 예약을 확인한다.
TEST_CASE(ReturnPiece_ConnectsFactoryAndRegular) {
    // 두 판본의 실제 동일 풀 연결을 검사한다. 가상 Form 부착과 부모 이벤트는 명시 경계다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);
        SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState processState;processState.now=12.5;
        std::vector<Sid> born;std::size_t handled=0;SquidProcessHost host(pool,types,kernel,destroy,processState,
            [&](Sid parent,std::uint32_t event,std::uint32_t count,float duration) { CHECK(parent==born.back() && event==0 && count==0 && duration==1.0F);++handled;return duration; });
        ContainedCreateState containedState;ContainedCreateHooks hooks;hooks.notifySurface=[](Sid) { CHECK(false); };
        // 부착 경계에서 raw 생성 결과만 관찰한다. Form 파생 부착 몸체를 구현했다고 주장하지 않는다.
        hooks.attach=[&](Sid sid,Sid parent,std::uint32_t flags) {
            CHECK(parent==kParent && flags==0x10);born.push_back(sid);
            auto raw=pool.AllocatedBytes(sid);raw[11]=static_cast<std::uint8_t>((raw[11]&~4)|8);
        };
        RawContainedCreate contained(pool,containedState,MakeContainedCreateHooks(pool,factory,hooks));
        CursorReturnPieceState state;state.type=100;GameRandom rng(17);CursorReturnPieceHooks timerHooks;
        timerHooks.allocateTimer=[](std::uint32_t size) { CHECK(size==40);return true; };
        RawCursorReturnPiece body(contained,state,rng,MakeCursorReturnPieceHooks(pool,host,timerHooks));
        // 원본의 10/5/1초 및 요새 모드 비트가 실제 Regular payload에 저장되는지 확인한다.
        for (std::uint8_t index=0;index<4;++index) {
            state.lifetimeIndex=index;state.fort=index==3;body.Return(0x108,kParent.value,3);const auto sid=born.back();const auto raw=pool.Slot(sid);
            CHECK(raw[10]==6 && Get(raw,18,2)==100 && Get(raw,12,2)==0 && Get(raw,20,2)==0x108 && raw[edition==OriginalEdition::Patch1078 ? 30 : 26]==2);
            const auto* process=host.FindEvent(sid,0);CHECK(process && process->Parent()==sid && process->Time()==12.5 && process->Count()==0);
            CHECK(process && Bits(process->Payload())==(index==0 ? Bits(10.0F) : index==1 ? Bits(5.0F) : index==2 ? Bits(1.0F) : 0x38d1b717U));
        }
        CHECK(kernel.Size()==4 && rng.State()==17);
        SidPool other(edition,kCapacity);CHECK(Throws([&] { MakeCursorReturnPieceHooks(other,host,timerHooks); }));
        // 마지막에 새 1초 타이머만 진행시켜 실제 Regular 양수 반환 예약까지 확인한다.
        state.fort=0;state.lifetimeIndex=2;body.Return(8,kParent.value,1);auto* regular=host.FindEvent(born.back(),0);
        // 오류 주입으로 이벤트가 바뀌어도 실패를 보고하고 null을 역참조하지 않는다.
        CHECK(regular!=nullptr);if (!regular) continue;
        regular->RunFrame();CHECK(handled==1 && regular->Time()==13.5 && regular->Count()==1);
        processState.now=13.0;regular->RunFrame();CHECK(handled==1);
    }
}
