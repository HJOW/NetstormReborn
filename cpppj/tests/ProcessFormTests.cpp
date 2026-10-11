// 지정 free ProcessForm의 주변 바이트와 단일 분배의 저장 독립 PE 관찰을 재생한다.
#include "ConstructionSupport.h"
#include "o/RawPacketDispatch.h"

namespace {
using namespace netstorm::test::construction;
// 실행기가 쓰는 풀 크기와 판본별 입력 행 수다.
constexpr std::uint32_t kFormCapacity=32768,kFormRows=612;
// 빈 실행/부착 통지만 제공하며 실제 Kernel 등록과 form 연결은 host가 수행한다.
class TaggedProcess final:public SquidProcess {
public:
    // fixture와 같은 순서의 사건 저장소를 참조한다.
    explicit TaggedProcess(std::vector<std::string>& events):events_(events) {}
    // 프레임 실행은 이 부착 검사에 필요 없다.
    void RunFrame() override {}
    // form/부모가 확정된 뒤의 원본 가상 통지다.
    void OnAttached() override { events_.push_back("N:"+std::to_string(Form().value)+':'+std::to_string(Parent().value)); }
private:
    std::vector<std::string>& events_; // 검사 중 살아 있는 사건 목록이다.
};
// 실제 Kernel/풀/삭제 모듈을 소유한 named form 장면이다.
struct FormScene {
    SidPool pool; // 원본 크기의 공유 SID 저장소다.
    std::vector<RiftTypeRecord> types; // 실제 판본 타입 수의 합성 입력이다.
    SquidHash hash; // 삭제 모듈의 실제 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 실제 공간 표다.
    SquidUnpop unpop; // 삭제에 필요한 실제 공간 정리다.
    RawSquidDestroy destroy; // host의 form 해제 모듈이다.
    Kernel kernel; // 소유권과 DWORD 번호를 실제로 배정한다.
    SquidProcessState state; // Kernel 등록의 타입 순환 기록이다.
    SquidProcessHost host; // 실제 Take/부모 연결을 실행한다.
    std::vector<std::string> events; // 가상 통지 순서다.
    // 부모 상태 외에는 Allocate를 실행하지 않고 fixture 입력만 넣는다.
    explicit FormScene(OriginalEdition edition):pool(edition,kFormCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        unpop(pool,hash,spots),destroy(pool,unpop,types),host(pool,types,kernel,destroy,state) {
        auto parent=InputRaw(pool,Sid{20000});std::fill(parent.begin(),parent.end(),std::uint8_t{});parent[10]=100;
    }
};
// 원본과 같은 구분자로 사건을 직렬화한다.
std::string Join(const std::vector<std::string>& events) {
    std::string out;
    // 관찰한 순서 그대로 연결한다.
    for (const auto& event:events) { if (!out.empty()) out+=';';out+=event; }
    return out.empty() ? "-" : out;
}
// Take가 유지해야 하는 카운터·예측 커서·client/server 머리·꼬리다.
std::string Meta(const SidPool& pool) {
    return std::to_string(pool.FreeCount())+':'+std::to_string(pool.PredictableCursor())+':'+
        std::to_string(Get(pool.Slot(Sid{0}),4,2))+':'+std::to_string(pool.Tail(true).value)+':'+
        std::to_string(Get(pool.Slot(Sid{0}),18,2))+':'+std::to_string(pool.Tail(false).value);
}
// 시각/기록/종료만 공급하는 원본 외부 경계다.
PacketDispatchHooks DispatchHooks(std::vector<std::string>& events) {
    return {[] { return 42U; },[&events](PacketConnection& connection,std::span<const std::uint8_t>,std::uint16_t length) {
        events.push_back("L:"+std::to_string(connection.sender)+':'+std::to_string(length)); },
        [&events](PacketConnection&) { events.push_back("T"); },[&events](PacketConnection&) { events.push_back("C"); },
        [&events](PacketConnection&) { events.push_back("F"); }};
}
// 두 종류의 실제 명령 출력과 C++를 바이트·반환·순서 단위로 비교한다.
void ReplayForm(std::string_view edition) {
    const auto kind=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    static const auto rows=LoadFixture(NETSTORM_PROCESSFORM_FIXTURE).rows;CHECK(rows.size()==3*kFormRows);std::size_t count=0;
    // 모든 fixture 입력에서 출력 기대값은 저장된 원본 관찰만 사용한다.
    for (const auto& row:rows) {
        if (row[0]!=edition) continue;CHECK(row.size()==9);const auto input=Inputs(row[2]);bool same=false;
        if (row[1]=="Attach") {
            CHECK(input.size()==5);FormScene scene(kind);const Sid form{static_cast<std::uint16_t>(input[0])};
            auto raw=InputRaw(scene.pool,form),parent=InputRaw(scene.pool,Sid{20000}),head=InputRaw(scene.pool,Sid{50});
            // 초기 raw 표식만 공급하고 기대 출력은 계산하지 않는다.
            for (auto bytes:{raw,parent,head}) for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(i*29+7);
            raw[11]=static_cast<std::uint8_t>(input[1]);parent[10]=100;parent[11]=0;Put(parent,6,input[2] ? 50 : 0,2);
            Put(head,0,input[2]==1 ? (kind==OriginalEdition::Patch1078 ? 0x512098U : 0x505990U) : (kind==OriginalEdition::Patch1078 ? 0x501dd0U : 0x5058d8U));
            head[10]=input[2]==1 ? 46 : 100;head[11]=8;
            // 경계가 돌려준 번호 7을 실제 Kernel의 첫 여섯 슬롯을 채워 재현한다.
            for (std::uint32_t i=0;i<6;++i) CHECK(scene.kernel.Add(std::make_unique<RegularProcess>(1,0.0F))==i+1);
            const auto before=Meta(scene.pool);auto* process=scene.host.Attach(std::make_unique<TaggedProcess>(scene.events),input[4],Sid{20000},input[3],form);
            CHECK(process && scene.kernel.Find(*process)==7 && Get(raw,18)==7 && scene.kernel.Size()==7 && scene.state.typeRotation==(kind==OriginalEdition::Patch1078 ? 1U : 0U));
            same=Join(scene.events)==row[3] && Hex(raw)==row[4] && Hex(parent)==row[5] && Hex(head)==row[6] &&
                std::to_string(process->Form().value)+':'+std::to_string(process->Parent().value)==row[7] && Meta(scene.pool)==row[8] && Meta(scene.pool)==before;
        } else {
            CHECK(input.size()==11);std::vector<std::string> events;RawPacketDispatch dispatch(kind,DispatchHooks(events));
            const auto type=input[0];const bool registerable=type && type<=(kind==OriginalEdition::Patch1078 ? 137U : 126U);
            if (registerable) {
                if (input[4]) dispatch.Register(type,[&](std::uint32_t sender,PacketConnection& connection,std::span<const std::uint8_t>) {
                    events.push_back("H:"+std::to_string(sender)+':'+std::to_string(connection.depth));if (input[9]) connection.flags|=0x40; });
                dispatch.Observe(type,[](std::uint32_t) { CHECK(false); });dispatch.Version(type,static_cast<std::uint8_t>(input[2]),static_cast<std::uint8_t>(input[3]));
                dispatch.RequireLength(type,static_cast<std::uint16_t>(input[5]));
            }
            std::array<std::uint8_t,64> packet;packet.fill(0xcc);packet[0]=static_cast<std::uint8_t>(input[1]);packet[1]=static_cast<std::uint8_t>(type);Put(packet,2,input[6],2);
            PacketConnection connection{input[7],input[10],11,22,input[8],false};const auto result=dispatch.Dispatch(connection,packet);
            const auto observed=std::to_string(result ? 1 : 0)+':'+std::to_string(connection.flags)+':'+std::to_string(connection.received)+':'+
                std::to_string(connection.heartbeat)+':'+std::to_string(connection.depth);
            same=Join(events)==row[3] && observed==row[4] && Hex(packet)==row[5];
            if (registerable) {
                const auto expected=Split(row[6],':');const auto& registration=dispatch.Registration(type);
                same= same && static_cast<bool>(registration.receive)==(Number(expected[0])!=0) && static_cast<bool>(registration.observer)==(Number(expected[1])!=0) &&
                    registration.version==Number(expected[2]) && registration.mismatch==Number(expected[3]);
            }
        }
        CHECK(same);if (!same) { std::printf("지정 form/패킷 %s %s 행 %zu 불일치\n",std::string(edition).c_str(),row[1].c_str(),count);break; }++count;
    }
    CHECK(count==kFormRows);
}
}

// 패치판 지정 form/분배의 모든 독립 원본 관찰이다.
TEST_CASE(ProcessForm_ReplaysOriginals) { ReplayForm("originals"); }
// CD판의 별도 몸체와 SEH를 거친 모든 관찰이다.
TEST_CASE(ProcessForm_ReplaysCd) { ReplayForm("originalCD"); }
// 별도 10.37 PE의 모든 지정 form/분배 관찰이다.
TEST_CASE(ProcessForm_ReplaysExtra1037) { ReplayForm("original1037"); }

// 잘못된 지정 form과 체인은 Kernel/목록/부모를 바꾸기 전에 거부한다.
TEST_CASE(ProcessForm_RejectsInvalidNamedFormsBeforeRegistration) {
    // 두 판본의 서로 다른 서버 범위와 예약 머리를 모두 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        FormScene scene(edition);const auto first=scene.pool.Layout().serverFirst;const auto before=Meta(scene.pool);const auto parent=Hex(scene.pool.Slot(Sid{20000}));
        // client/예약/범위 밖 번호에는 Take를 허용하지 않는다.
        for (auto value:{std::uint32_t{5},static_cast<std::uint32_t>(scene.pool.Layout().predictableFirst),kFormCapacity}) {
            CHECK(Throws([&] { scene.host.Attach(std::make_unique<TaggedProcess>(scene.events),30,Sid{20000},0x10,Sid{static_cast<std::uint16_t>(value)}); }));
        }
        auto raw=InputRaw(scene.pool,Sid{static_cast<std::uint16_t>(first)});raw[11]=0;
        CHECK(Throws([&] { scene.host.Attach(std::make_unique<TaggedProcess>(scene.events),30,Sid{20000},0x10,Sid{static_cast<std::uint16_t>(first)}); }));raw[11]=1;
        Put(InputRaw(scene.pool,Sid{20000}),6,first,2);
        CHECK(Throws([&] { scene.host.Attach(std::make_unique<TaggedProcess>(scene.events),30,Sid{20000},0x10,Sid{static_cast<std::uint16_t>(first)}); }));
        Put(InputRaw(scene.pool,Sid{20000}),6,0,2);CHECK(scene.kernel.Size()==0 && scene.events.empty() && scene.state.typeRotation==0 && Meta(scene.pool)==before && Hex(scene.pool.Slot(Sid{20000}))==parent);
    }
}

// 기록 중 재등록과 콜백 자체 재등록/중첩은 현재 처리기 선택과 깊이를 보존한다.
TEST_CASE(PacketDispatch_UsesCurrentHandlerAfterRecordAndRestoresNestedDepth) {
    std::vector<std::string> events;auto hooks=DispatchHooks(events);RawPacketDispatch* current=nullptr;bool replace=true;
    hooks.record=[&](PacketConnection&,std::span<const std::uint8_t>,std::uint16_t) {
        if (replace) { replace=false;current->Register(55,[&](std::uint32_t,PacketConnection& c,std::span<const std::uint8_t> packet) {
            events.push_back(std::to_string(c.depth));current->Register(55,[&](std::uint32_t,PacketConnection& nested,std::span<const std::uint8_t>) { events.push_back(std::to_string(nested.depth)); });
            CHECK(current->Dispatch(c,packet));events.push_back(std::to_string(c.depth)); }); }
    };
    RawPacketDispatch dispatch(OriginalEdition::Patch1078,hooks);current=&dispatch;dispatch.Register(55,[](std::uint32_t,PacketConnection&,std::span<const std::uint8_t>) { CHECK(false); });
    std::array<std::uint8_t,4> packet{1,55,4,0};PacketConnection connection;CHECK(dispatch.Dispatch(connection,packet));CHECK(Join(events)=="1;2;1" && connection.depth==0);
    dispatch.Register(55,[](std::uint32_t,PacketConnection&,std::span<const std::uint8_t>) { throw std::runtime_error("검사 예외"); });
    CHECK(Throws([&] { dispatch.Dispatch(connection,packet); }) && connection.depth==0);
}

// 원본 길이 표와 호스트 입력 경계, 미지원 집합/지연 분기를 분리해서 검사한다.
TEST_CASE(PacketDispatch_ValidatesNativeLengthsAndUnsupportedPaths) {
    std::vector<std::string> events;RawPacketDispatch dispatch(OriginalEdition::Patch1078,DispatchHooks(events));
    std::array<std::uint8_t,4> packet{1,16,4,0};dispatch.Register(16,[](std::uint32_t,PacketConnection&,std::span<const std::uint8_t>) {});
    CHECK(dispatch.Registration(55).length==65535 && dispatch.Registration(16).length==8);CHECK(!dispatch.Validate(packet));
    Put(packet,2,dispatch.Registration(16).length,2);CHECK(dispatch.Validate(packet));
    CHECK(Throws([&] { dispatch.Register(0,{}); }) && Throws([&] { dispatch.Version(138,1,0); }) && Throws([&] { dispatch.Validate(std::span<const std::uint8_t>(packet).first(3)); }));
    packet[1]=4;Put(packet,2,dispatch.Registration(4).length,2);PacketConnection connection;CHECK(Throws([&] { dispatch.Dispatch(connection,packet); }));
    packet[1]=6;Put(packet,2,dispatch.Registration(6).length,2);connection.delayed=true;CHECK(Throws([&] { dispatch.Dispatch(connection,packet); }));CHECK(events.empty());
}
