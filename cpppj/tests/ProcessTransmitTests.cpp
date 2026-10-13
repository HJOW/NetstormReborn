// 원본 ProcessForm 서버 부착/갱신의 WriteData·송신·체인/Pop 순서를 독립 PE 관찰과 대조한다.
#include "ConstructionSupport.h"
#include "o/SquidProcess.h"
#include "o/SquidUnpop.h"

namespace {
using namespace netstorm::test::construction;
// 두 판본에서 실제 Kernel/슬롯을 만드는 검사 풀 크기다.
constexpr std::uint32_t kTransmitCapacity=32768;
// 부착 통지만 관찰한다. 실행 몸체는 이 전송 검사에서 호출하지 않는다.
class TransmittedProcess final:public SquidProcess {
public:
    // 원본 순서와 비교할 사건 목록을 참조한다.
    explicit TransmittedProcess(std::vector<std::string>& events):events_(events) {}
    // Kernel 실행은 후속 파생 Gun/Path 검사에서 한다.
    void RunFrame() override {}
    // WriteData/송신/부모 체인 Pop 뒤 한 번 호출되는 실제 가상 통지다.
    void OnAttached() override { events_.push_back("N:"+std::to_string(Form().value)+':'+std::to_string(Parent().value)); }
private:
    std::vector<std::string>& events_;
};
// 같은 SID/Kernel에서 지정 슬롯과 신규 슬롯을 대조하는 장면이다.
struct TransmitScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    SquidHash hash;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    SquidUnpop unpop;
    RawSquidDestroy destroy;
    std::vector<std::string> events;
    Kernel kernel;
    SquidProcessState state;
    SquidProcessHost host;
    std::int32_t size{};
    // 모든 전송 경계는 raw 쓰기 전 현재 필드를 읽는다. 기대 출력은 fixture만 제공한다.
    TransmitScene(OriginalEdition edition,bool server,std::int32_t length):pool(edition,kTransmitCapacity,server),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),unpop(pool,hash,spots),destroy(pool,unpop,types),
        host(pool,types,kernel,destroy,state,{}, {},Transmission()),size(length) {}
    // WriteData와 버퍼 송신 경계를 원본이 호출한 순서 그대로 기록한다.
    ProcessFormTransmission Transmission() {
        return {[this](SquidProcess& process,bool changed) {
            const auto form=process.Form();events.push_back("W:"+std::to_string(form.value)+':'+std::to_string(changed ? 1 : 0)+':'+
                Hex(pool.Slot(form))+':'+Hex(pool.Slot(Sid{20000}))+':'+std::to_string(form.value)+':'+std::to_string(process.Parent().value));return size;
        },[this](std::int32_t length) { events.push_back("S:"+std::to_string(static_cast<std::uint32_t>(length))); }};
    }
};
// 관찰 목록을 동일 구분자로 직렬화한다.
std::string TransmitEvents(const std::vector<std::string>& events) {
    std::string out;
    // 원본 기록 순서를 바꾸지 않는다.
    for (const auto& event:events) { if (!out.empty()) out+=';';out+=event; }
    return out.empty() ? "-" : out;
}
// 입력 표식만 만들고 서버 직렬화/송신/Attach/Pop의 결과는 C++와 원본을 직접 비교한다.
void ReplayTransmit(std::string_view edition) {
    const auto kind=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    static const auto rows=LoadFixture(NETSTORM_PROCESSTRANSMIT_FIXTURE).rows;std::size_t count=0;
    // 전송/억제 flags·부모 이웃·새/지정 Form·양수/음수 WriteData를 모두 재생한다.
    for (const auto& row:rows) {
        if (row[0]!=edition) continue;CHECK(row.size()==10);const auto input=Inputs(row[2]);CHECK(input.size()==7);
        TransmitScene scene(kind,input[0]!=0,std::bit_cast<std::int32_t>(input[6]));
        auto parent=InputRaw(scene.pool,Sid{20000}),head=InputRaw(scene.pool,Sid{50});
        // 원본 합성 입력과 같은 표식이다. 생성 결과는 계산하지 않는다.
        for (auto bytes:{parent,head}) {
            // 각 슬롯의 인접 payload를 보존하는지 비교한다.
            for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(i*29+7);
        }
        parent[10]=100;parent[11]=0;Put(parent,6,input[3] ? 50 : 0,2);
        Put(head,0,input[3]==1 ? (kind==OriginalEdition::Patch1078 ? 0x512098U : 0x505990U) :
            (kind==OriginalEdition::Patch1078 ? 0x501dd0U : 0x5058d8U));head[10]=input[3]==1 ? 46 : 100;head[11]=8;
        const Sid form{static_cast<std::uint16_t>(input[1] ? scene.pool.Layout().serverFirst+50 : 0)};
        if (form.value) {
            auto raw=InputRaw(scene.pool,form);
            // 지정 Take는 기존 주변 바이트를 보존한다.
            for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*29+7);
            raw[11]=static_cast<std::uint8_t>(input[2]);
        }
        auto* process=scene.host.Attach(std::make_unique<TransmittedProcess>(scene.events),input[5],Sid{20000},input[4],form);CHECK(process);
        if (!process) break;
        if (row[1]=="Refresh") { scene.events.clear();scene.host.RefreshForm(*process); }
        const auto actualForm=process->Form();
        const auto meta=std::to_string(scene.pool.FreeCount())+':'+std::to_string(scene.pool.PredictableCursor())+':'+
            std::to_string(scene.kernel.Get(1)==process ? 1 : 0);
        const bool same=TransmitEvents(scene.events)==row[3] && Hex(scene.pool.Slot(actualForm))==row[4] && Hex(parent)==row[5] && Hex(head)==row[6] &&
            std::to_string(actualForm.value)+':'+std::to_string(process->Parent().value)==row[7] && std::to_string(scene.state.typeRotation)==row[8] && meta==row[9];
        CHECK(same);if (!same) { std::printf("ProcessForm 전송 %s %s 행 %zu 불일치\n",std::string(edition).c_str(),row[1].c_str(),count);break; }++count;
    }
    CHECK(count>0 && count*2==rows.size());
}
}
// 패치판 WriteData/버퍼 송신/공통 Pop 억제의 독립 원본 출력이다.
TEST_CASE(ProcessTransmit_ReplaysOriginals) { ReplayTransmit("originals"); }
// CD판 별도 몸체의 같은 전송 순서와 주변 바이트다.
TEST_CASE(ProcessTransmit_ReplaysCd) { ReplayTransmit("originalCD"); }
