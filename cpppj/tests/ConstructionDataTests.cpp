// 세 PE의 독립 패킷 관찰과 실제 Start→전송→시각 재설정→수신→Kernel 완료를 검사한다.
#include "ConstructionNoticeSupport.h"
#include "o/RawConstructionData.h"

namespace {
using namespace netstorm::test::construction;
// 원본 관찰의 입력 DWORD 순서와 판본별 행 수, 저장소의 소리 표식이다.
enum Input : std::size_t { Dirty,Form,Parent,Force,Include,StartLow,StartHigh,EndLow,EndHigh,Additional,Protocol,Message,Free,Pid,Present,Mutate,Type,Pattern };
constexpr std::size_t kRows=1780;
constexpr std::uint32_t kSound=0x15000040;
// 두 DWORD를 산술 변환 없이 double로 묶는다.
double Time(std::uint32_t low,std::uint32_t high) { return std::bit_cast<double>((static_cast<std::uint64_t>(high)<<32)|low); }
// 필드 전체를 실제 기계어 관찰의 일곱 항목으로 만든다.
std::string Fields(const ConstructionProcessFields& fields) {
    const auto start=std::bit_cast<std::uint64_t>(fields.start),end=std::bit_cast<std::uint64_t>(fields.end);std::string out;
    // NaN을 포함한 시각 비트와 dirty/소리/추가 바이트를 직접 비교한다.
    for (auto value:{static_cast<std::uint32_t>(fields.dirty),static_cast<std::uint32_t>(start),static_cast<std::uint32_t>(start>>32),
        static_cast<std::uint32_t>(end),static_cast<std::uint32_t>(end>>32),fields.sound,static_cast<std::uint32_t>(fields.additional)}) {
        if (!out.empty()) out+=':';out+=std::to_string(value);
    }
    return out;
}
// 기계어가 받은 패킷 입력을 읽는다. 기대 출력의 패킷 생성 규칙을 재구현하지 않는다.
std::vector<std::uint8_t> Bytes(std::string_view hex) {
    std::vector<std::uint8_t> result;
    // 두 16진수 글자씩 입력 바이트로 옮긴다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(i,2)),nullptr,16)));
    return result;
}
// 사건 순서를 원본 관찰과 같은 구분자로 잇는다.
std::string Events(const std::vector<std::string>& events) {
    std::string out;
    // 호출 순서를 바꾸지 않고 빈 목록은 '-'로 표시한다.
    for (const auto& event:events) { if (!out.empty()) out+=';';out+=event; }
    return out.empty() ? "-" : out;
}
// 실제 form/Kernel을 소유한 검사 월드다. 새 수신 form 부착은 이 단계에서 별도 경계다.
struct DataScene {
    SidPool pool; // 프로세스와 부모가 공유하는 실제 풀이다.
    std::vector<RiftTypeRecord> types; // 판본별 타입 표다.
    SquidHash hash; // form 삭제가 사용하는 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 공간 삭제 표다.
    SquidUnpop unpop; // 실제 form 삭제의 공간 정리다.
    RawSquidDestroy destroy; // Kernel 완료 때 form을 반납한다.
    Kernel kernel; // 실제 프로세스 슬롯을 소유한다.
    SquidProcessState clock; // 실제 host의 권한/시각이다.
    SquidProcessHost host; // 부모 연결과 form→DWORD Kernel 조회다.
    ConstructionProcessState state; // 건설 프레임이 읽는 현재 전역이다.
    RawConstructionProcess logic; // Start/ResetEnd/완료의 복원 몸체다.
    // 실제 부모를 준비하되 관찰하지 않는 효과는 빈 경계로 연결한다.
    explicit DataScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        unpop(pool,hash,spots),destroy(pool,unpop,types),host(pool,types,kernel,destroy,clock),logic(pool,types,state,Hooks(),&host) {
        auto raw=InputRaw(pool,Sid{20000});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=100;raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=1;
    }
    // HP와 abstract 완료만 실제 raw 상태에 연결하고 음향/표시 호출은 비워 둔다.
    ConstructionProcessHooks Hooks() {
        ConstructionProcessHooks hooks;
        hooks.reserve=[] { return true; };hooks.effect=[](Sid,bool) {};hooks.loadSound=[](std::string_view) { return kSound; };
        hooks.reserveSound=[] { return false; };hooks.attachSound=[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) {};
        hooks.refresh=[] {};hooks.updateDisplay=[](Sid) {};hooks.removeContained=[](Sid,std::uint32_t) {};
        hooks.playAt=[](float,float,std::string_view,std::uint32_t) {};hooks.readySound=[](std::uint32_t) { return 0U; };hooks.play=[](std::uint32_t) {};
        hooks.unpop=[this](Sid sid,std::uint32_t) { pool.AllocatedBytes(sid)[11]|=4; };
        hooks.repop=[this](Sid sid,std::uint32_t) { pool.AllocatedBytes(sid)[11]&=0xfb; };
        hooks.push=[](float,float,std::uint32_t,std::uint32_t) {};hooks.maxHitPoints=[](Sid) { return 100; };return hooks;
    }
    // 사용하지 않는 새 객체 경계를 갖춘 기존 form 수신기다. lookup은 실제 host가 담당한다.
    ConstructionDataHooks DataHooks() {
        ConstructionDataHooks hooks;hooks.allocate=[] { return static_cast<ConstructionProcessFields*>(nullptr); };
        hooks.attach=[](ConstructionProcessFields&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) {};return hooks;
    }
};
// 모든 입력에서 필드/부모 출력/사건/패킷 전체/주변 form을 독립 원본 출력과 비교한다.
void Replay(std::string_view edition) {
    const auto kind=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    static const auto fixture=LoadFixture(NETSTORM_CONSTRUCTIONDATA_FIXTURE).rows;CHECK(fixture.size()==3*kRows);
    SidPool pool(kind,kCapacity,false);ConstructionProcessState state;std::size_t count=0;
    // 행별 입력 필드와 경계 응답만 준비하며 출력은 fixture에서 읽는다.
    for (const auto& row:fixture) {
        if (row[0]!=edition) continue;CHECK(row.size()==9);const auto input=Inputs(row[2]);CHECK(input.size()==18);
        ConstructionProcessFields fields{static_cast<std::uint8_t>(input[Dirty]),Time(input[StartLow],input[StartHigh]),Time(input[EndLow],input[EndHigh]),kSound,static_cast<std::uint8_t>(input[Additional])};
        auto packet=Bytes(row[6]);std::vector<std::string> events;std::string output="-";state.processType=input[Type];
        const Sid form{static_cast<std::uint16_t>(row[1]=="Receive" ? input[Form] : 90)};auto raw=InputRaw(pool,form);
        std::fill(raw.begin(),raw.end(),std::uint8_t{0xa7});raw[11]=static_cast<std::uint8_t>(input[Free]);Put(raw,18,input[Pid]);
        if (row[1]=="Write") RawConstructionData::Write(fields,packet,input[Form],input[Parent],input[Force]!=0,{static_cast<std::uint8_t>(input[Protocol]),static_cast<std::uint8_t>(input[Message])});
        else if (row[1]=="Wrapper") {
            DataScene scene(kind);if (input[Form]==6) CHECK(scene.host.AddRegular(Sid{20000},1,0));
            auto* process=static_cast<ConstructionProcess*>(scene.host.Attach(std::make_unique<ConstructionProcess>(scene.logic,fields),30,Sid{20000},0x10));
            CHECK(process && process->Form().value==input[Form]);
            if (process) { RawConstructionData::WriteProcess(*process,packet,input[Include]!=0,input[Force]!=0);fields=process->Fields(); }
        } else if (row[1]=="Read" || row[1]=="ReadNull") output=std::to_string(RawConstructionData::Read(row[1]=="ReadNull" ? nullptr : &fields,packet));
        else {
            ConstructionDataHooks hooks;
            // 할당은 현재 타입/패킷을 바꿀 수 있다. 수신 몸체가 캡처한 form과 재조회한 값을 대조한다.
            hooks.allocate=[&] {
                events.push_back("A:56");if (input[Mutate]==1) state.processType=44;
                if (input[Mutate]==2) { Put(packet,4,91);Put(packet,9,0xffffffff); }return &fields;
            };
            // 명시 form 부착의 원본 인자만 기록하고 raw form은 바꾸지 않는다.
            hooks.attach=[&](ConstructionProcessFields&,std::uint32_t type,std::uint32_t parent,std::uint32_t captured,std::uint32_t flags) {
                events.push_back("J:"+std::to_string(type)+':'+std::to_string(parent)+':'+std::to_string(captured)+':'+std::to_string(flags));
            };
            // Kernel 번호의 DWORD 전체를 기록하며 지정된 슬롯 유무만 공급한다.
            hooks.lookup=[&](ProcessId pid) { events.push_back("K:"+std::to_string(pid));return input[Present] ? &fields : nullptr; };
            RawConstructionData receiver(pool,state,hooks);receiver.Receive(packet);
        }
        const bool same=Events(events)==row[3] && Fields(fields)==row[4] && output==row[5] && Hex(packet)==row[7] && Hex(raw)==row[8];
        CHECK(same);if (!same) { std::printf("건설 상태 %s %s 행 %zu 불일치\n",std::string(edition).c_str(),row[1].c_str(),count);break; }++count;
    }
    CHECK(count==kRows);
}
}

// 패치판의 모든 비트/분기/래퍼 관찰을 재생한다.
TEST_CASE(ConstructionData_ReplaysOriginals) { Replay("originals"); }
// CD판의 별도 몸체와 SEH를 거친 관찰을 재생한다.
TEST_CASE(ConstructionData_ReplaysCd) { Replay("originalCD"); }
// 별도 10.37 PE의 모든 관찰을 재생한다.
TEST_CASE(ConstructionData_ReplaysExtra1037) { Replay("original1037"); }

// 시작의 전체 전송과 끝 시각의 부분 전송을 실제 수신/Kernel 진행/완료에 연결한다.
TEST_CASE(ConstructionData_ActualHostRoundTripAndDelayedCompletion) {
    // 두 판본의 실제 부모 HP 폭과 form 삭제까지 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        DataScene scene(edition);scene.logic.Start(Sid{20000},8.0F);auto* process=dynamic_cast<ConstructionProcess*>(scene.host.FromForm(Sid{5}));CHECK(process);
        if (!process) continue;std::array<std::uint8_t,30> packet{};
        CHECK(RawConstructionData::WriteProcess(*process,packet,true,false)==30 && process->Fields().dirty==0 && packet[8]==255);
        ConstructionProcessFields copy;CHECK(RawConstructionData::Read(&copy,packet)==20000 && copy.start==0 && copy.end==8 && copy.additional==0);
        scene.state.now=5;scene.logic.ResetEndFromForm(Sid{5},1);CHECK(process->Fields().end==15 && process->Fields().dirty==4);
        CHECK(RawConstructionData::WriteProcess(*process,packet,false,false)==17 && packet[8]==4 && process->Fields().dirty==0);
        process->Fields().end=8;process->Fields().additional=0x7b;RawConstructionData receiver(scene.pool,scene.state,scene.DataHooks(),&scene.host);
        receiver.Receive(std::span<const std::uint8_t>(packet).first(17));CHECK(process->Fields().end==15 && process->Fields().additional==0x7b && process->Fields().dirty==0);
        scene.state.now=8;scene.kernel.RunFrame();CHECK(scene.kernel.Size()==1 && Get(scene.pool.Slot(Sid{20000}),26,edition==OriginalEdition::Patch1078 ? 4 : 2)==53);
        scene.state.now=15;scene.kernel.RunFrame();CHECK(scene.kernel.Size()==0 && Get(scene.pool.Slot(Sid{20000}),26,edition==OriginalEdition::Patch1078 ? 4 : 2)==100);
        CHECK(!(scene.pool.Slot(Sid{20000})[edition==OriginalEdition::Patch1078 ? 40 : 35]&1));
    }
}

// 모든 부족 버퍼 길이는 필드/패킷 변경 전에 거부하고 선언 길이는 읽기 판단에 사용하지 않는다.
TEST_CASE(ConstructionData_RejectsTruncatedBuffersBeforeMutation) {
    ConstructionProcessFields fields{255,1.0,2.0,kSound,0x5a};const auto before=Fields(fields);std::array<std::uint8_t,30> packet{};
    // 최대 쓰기의 모든 부족 길이와 최대 읽기의 모든 부족 길이를 검사한다.
    for (std::size_t size=0;size<packet.size();++size) {
        packet.fill(0xcc);CHECK(Throws([&] { RawConstructionData::Write(fields,std::span<std::uint8_t>(packet).first(size),90,20000,true); }));
        CHECK(Fields(fields)==before && Hex(packet)==std::string(60,'c'));packet.fill(0);packet[8]=255;
        CHECK(Throws([&] { RawConstructionData::Read(&fields,std::span<const std::uint8_t>(packet).first(size)); }));CHECK(Fields(fields)==before);
    }
    packet.fill(0);packet[8]=255;Put(packet,2,0xffff,2);CHECK(RawConstructionData::Read(&fields,packet)==0 && fields.dirty==255 && fields.sound==kSound);
    CHECK(Throws([&] { RawConstructionData::Read(nullptr,packet); }));
}

// 부정한 새/기존 상태와 할당 실패는 명시 form 부착 경계를 실행하지 않는다.
TEST_CASE(ConstructionData_RejectsInvalidReceiveAndMissingHooks) {
    DataScene scene(OriginalEdition::Patch1078);ConstructionProcessFields fields;std::vector<std::string> events;
    auto hooks=scene.DataHooks();hooks.lookup=[&](ProcessId) { return &fields; };
    hooks.attach=[&](ConstructionProcessFields&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { events.push_back("attach"); };
    RawConstructionData receiver(scene.pool,scene.state,hooks);std::array<std::uint8_t,30> packet{};Put(packet,4,90);auto raw=InputRaw(scene.pool,Sid{90});raw[11]=1;
    packet[8]=15;CHECK(Throws([&] { receiver.Receive(packet); }));packet[8]=255;CHECK(Throws([&] { receiver.Receive(packet); }));
    raw[11]=0;CHECK(Throws([&] { receiver.Receive(packet); }));Put(packet,4,0x1005a);CHECK(Throws([&] { receiver.Receive(packet); }));CHECK(events.empty());
    hooks.allocate={};CHECK(Throws([&] { RawConstructionData invalid(scene.pool,scene.state,hooks); }));
    hooks=scene.DataHooks();CHECK(Throws([&] { RawConstructionData invalid(scene.pool,scene.state,hooks); }));
    DataScene other(OriginalEdition::Patch1078);CHECK(Throws([&] { RawConstructionData invalid(scene.pool,scene.state,hooks,&other.host); }));
}

// 다른 프로세스/오래된 form/상위 비트 Kernel 번호가 건설 필드를 덮어쓰지 못한다.
TEST_CASE(ConstructionData_ActualHostRejectsWrongAndStaleForms) {
    DataScene scene(OriginalEdition::Patch1078);scene.logic.Start(Sid{20000},8.0F);auto* process=dynamic_cast<ConstructionProcess*>(scene.host.FromForm(Sid{5}));CHECK(process);
    if (!process) return;const auto before=Fields(process->Fields());std::array<std::uint8_t,30> packet{};packet[8]=4;Put(packet,4,5);
    RawConstructionData receiver(scene.pool,scene.state,scene.DataHooks(),&scene.host);Put(scene.pool.AllocatedBytes(Sid{5}),18,0x10001);
    CHECK(Throws([&] { receiver.Receive(packet); }) && Fields(process->Fields())==before);packet[8]=0x80;CHECK(!Throws([&] { receiver.Receive(packet); }));
    Put(scene.pool.AllocatedBytes(Sid{5}),18,1);auto stale=InputRaw(scene.pool,Sid{90});std::copy(scene.pool.Slot(Sid{5}).begin(),scene.pool.Slot(Sid{5}).end(),stale.begin());
    Put(packet,4,90);packet[8]=4;CHECK(Throws([&] { receiver.Receive(packet); }) && Fields(process->Fields())==before);
    auto* regular=scene.host.AddRegular(Sid{20000},1,0);CHECK(regular);if (regular) { Put(packet,4,regular->Form().value);CHECK(Throws([&] { receiver.Receive(packet); })); }
}
