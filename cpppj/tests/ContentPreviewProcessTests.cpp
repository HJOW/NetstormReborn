// 내용물 표시 타이머의 독립 원문과 같은 Host의 실제 삭제 수명을 확인한다.
#include "RawSceneSupport.h"
#include "o/RawContentPreviewProcess.h"
#include <bit>
#include <cmath>
#include <stdexcept>
using namespace netstorm::test::rawscene;

namespace {
// 원문64비트 시각과32비트 인자를 손실 없이 읽는다.
std::vector<std::uint64_t> PreviewNumbers(std::string_view text) {
    std::vector<std::uint64_t> values; // 입력 비트이며 기대 분기를 계산하지 않는다.
    // 각 십진수의 전체 원문 폭을 보관한다.
    for (const auto& value:Split(text,',')) values.push_back(std::stoull(value));
    return values;
}
// 원본24바이트의 double 비트를 기록한다.
void PreviewTime(ContentPreviewFields& fields,std::uint64_t bits) { Put(fields.bytes,16,static_cast<std::uint32_t>(bits));Put(fields.bytes,20,static_cast<std::uint32_t>(bits>>32)); }
// 원문 관찰의 순서 있는 효과를 기록한다.
void PreviewEvent(std::string& trace,std::string_view name,std::initializer_list<std::uint64_t> values={}) {
    if (!trace.empty()) trace+=';';trace+=name;
    // 전체 인자를 콜론으로 이어 원문과 같은 형식을 만든다.
    for (const auto value:values) trace+=':'+std::to_string(value);
}
// Kernel/풀/삭제를 공유하는 단일 명시 장면이다. 자산 그래픽은 void 입력으로 제한한다.
struct PreviewHostScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 실제 부모와 ProcessForm의 같은 풀이다.
    SquidHash hash; // 실제 Unpop이 공유하는 해시다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 명시 타입 표이며 자산의 void 삭제만 관찰한다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 같은 공간 지도다.
    SquidUnpop unpop{pool,hash,spots}; // 공통 공간 해제다.
    RawSquidDestroy destroy{pool,unpop,types}; // 실제 종속/깊이/반납 몸체다.
    Kernel kernel; // 프로세스의 유일한 소유권이다.
    SquidProcessState state; // 실제 Host와 타이머가 같은 now를 본다.
    std::uint32_t sharedType=47; // SoundProcess와 공유하는 현재 전역의 명시 입력이다.
    std::uint32_t deletes{},lastFlags{}; // 부모 가상 삭제에 실제 전달된 횟수와 flags다.
    SquidProcessHost host; // 부모 종속/ProcessForm/Kernel을 함께 관리한다.
    RawContentPreviewProcess logic; // Kernel보다 먼저 파괴되므로 소멸자에서 Kernel을 먼저 비운다.
    ContentPreviewProcess* latest{}; // 살아 있을 때만 읽는 최근 부착 결과다.
    // 같은 현재 부모와 원문 시각/프로세스 타입을 실제 부착·삭제에 연결한다.
    PreviewHostScene():host(pool,types,kernel,destroy,state,{},SquidDestroyHooks{
        [this](const SquidDestroyEvent& event) {
            if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();
            else if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();
            else throw std::logic_error("void 미리보기 장면에 없는 자산 효과");
        },[] { return Sid{}; },{}}),
        logic(pool,state.now,sharedType,ContentPreviewHooks{
            [this](ContentPreviewFields& fields,Sid parent,std::uint32_t kind,std::uint32_t flags) {
                latest=static_cast<ContentPreviewProcess*>(host.Attach(std::make_unique<ContentPreviewProcess>(logic,fields),kind,parent,flags));
            },
            [this](Sid parent,std::uint32_t flags) { ++deletes;lastFlags=flags;destroy.Destroy(parent,flags,host.Hooks()); },
            [] { throw std::logic_error("부착된 부모에 실제 종속 머리가 없습니다"); },{}}) {}
    ~PreviewHostScene() { kernel.Clear(); } // 실패 경로에서도 로직이 살아 있는 동안 남은 프로세스를 정리한다.
    // 원래 내용물 미리보기처럼 클라이언트 자산을 만들고 공간 미등록 void 상태를 명시한다.
    Sid Parent() { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);Put(raw,0,0x501dd0U);raw[10]=82;raw[11]=4;raw[40]=8;return sid; }
    // 실제 생성 몸체에서 같은 Host 부착 경계까지 실행한다.
    ContentPreviewProcess* Add(Sid parent,float duration) { ContentPreviewFields fields;logic.Construct(fields,parent,duration);return latest; }
};
}
// 53비트 기본 CRT 정밀도의 결과를 재생한다. 64비트 결과는 독립 보존된 별도 관측이다.
TEST_CASE(ContentPreviewProcess_NativeConstructorFrameNotifyDestructor) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);std::size_t count{}; // 독립 입력마다 물리 슬롯을 다시 채운다.
    // 기대 raw와 효과는 실제 원본 실행에서만 읽는다.
    for (const auto& columns:LoadFixture(NETSTORM_CONTENT_PREVIEW_PROCESS_FIXTURE).rows) {
        CHECK(columns.size()==5);if (columns.size()!=5) continue;
        const auto row=PreviewNumbers(columns[0]);CHECK(row.size()==9);ContentPreviewFields fields;fields.bytes.fill(0xa5);
        Put(fields.bytes,0,0x503c1cU);Put(fields.bytes,4,90);Put(fields.bytes,8,static_cast<std::uint32_t>(row[4]));PreviewTime(fields,row[3]);
        const Sid parent{static_cast<std::uint16_t>(row[4])};const auto slot=pool.Slot(parent);auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(slot.data()),slot.size());Put(raw,6,static_cast<std::uint32_t>(row[6]),2);
        double now=std::bit_cast<double>(row[1]);const auto kind=static_cast<std::uint32_t>(row[5]);std::string trace; // 현재 명시 clock/type/효과다.
        RawContentPreviewProcess logic(pool,now,kind,ContentPreviewHooks{
            [&](ContentPreviewFields& attached,Sid sid,std::uint32_t type,std::uint32_t flags) {
                PreviewEvent(trace,"A",{type,sid.value,0,flags});trace+=':'+Hex(attached.bytes);Put(attached.bytes,4,90);Put(attached.bytes,8,sid.value);
                if (row[7]==1) now=17.25;
                if (row[7]==2) PreviewTime(attached,std::bit_cast<std::uint64_t>(7.25));
            },
            [&](Sid sid,std::uint32_t flags) { PreviewEvent(trace,"D",{sid.value,flags});if (row[7]==4) fields.bytes.fill(0xcc); },
            [&] { PreviewEvent(trace,"!");trace+=":parent->getHead() != 0:164";if (row[7]==3) Put(fields.bytes,8,51); },
            [&](ContentPreviewFields& released) { PreviewEvent(trace,"F");trace+=':'+Hex(released.bytes); }});
        switch (row[0]) {
        case 0:logic.Construct(fields,parent,std::bit_cast<float>(static_cast<std::uint32_t>(row[2])));break;
        case 1:logic.Run(fields);break;
        case 2:RawContentPreviewProcess::Notify(fields,static_cast<std::uint32_t>(row[8]),0x80000000U,0xffffffffU);break;
        case 3:logic.Destruct(fields,static_cast<std::uint32_t>(row[8]));break;
        default:CHECK(false);break;
        }
        CHECK(Hex(fields.bytes)==columns[1]);CHECK((trace.empty() ? "-" : trace)==columns[2]);++count;
    }
    CHECK(count==957);
}
// 부모 삭제가 실행 중인 자신과 다른 만료 타이머의 Form/Kernel을 한 번씩 제거해야 한다.
TEST_CASE(ContentPreviewProcess_ActualHostExpiryAndExternalParentDelete) {
    // 자연 만료와 외부 부모 삭제는 같은 종속 수명 경로를 거친다.
    for (const bool external:{false,true}) {
        PreviewHostScene scene;scene.state.now=16777217.25;const auto parent=scene.Parent();auto* first=scene.Add(parent,0.2f);CHECK(first);
        const auto firstForm=first->Form();const auto deadline=first->Fields().Deadline();const auto firstPid=scene.kernel.Find(*first);
        auto* second=scene.Add(parent,0.8f);CHECK(second);const auto secondForm=second->Form();const auto secondPid=scene.kernel.Find(*second);
        CHECK(scene.kernel.Size()==2 && scene.pool.Slot(firstForm)[10]==47 && scene.pool.Slot(secondForm)[10]==47);
        CHECK(Get(first->Fields().bytes,4)==firstForm.value && Get(first->Fields().bytes,8)==parent.value);
        const auto before=first->Fields().bytes;first->Notify(3,0,0);CHECK(first->Fields().bytes==before);
        scene.state.now=std::nextafter(deadline,-INFINITY);scene.kernel.RunFrame();CHECK(scene.kernel.Size()==2 && scene.deletes==0);
        if (external) scene.destroy.Destroy(parent,0,scene.host.Hooks());
        else { scene.state.now=deadline;scene.kernel.RunFrame(); }
        CHECK(scene.kernel.Size()==0 && !scene.kernel.Get(firstPid) && !scene.kernel.Get(secondPid));
        CHECK((scene.pool.Slot(parent)[11]&1) && (scene.pool.Slot(firstForm)[11]&1) && (scene.pool.Slot(secondForm)[11]&1));
        CHECK(scene.destroy.PreDepth()==0 && scene.destroy.PostDepth()==0);
        CHECK(scene.deletes==(external ? 0U : 1U));if (!external) CHECK(scene.lastFlags==0x400000U);
    }
}
// 없는 효과는 선택될 때 거부하며 실제 빈 통지는 연결 없이 허용한다.
TEST_CASE(ContentPreviewProcess_LazyEffectsAndEdition) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);double now=1.0;std::uint32_t kind=47;RawContentPreviewProcess logic(pool,now,kind,{});ContentPreviewFields fields;
    PreviewTime(fields,std::bit_cast<std::uint64_t>(2.0));logic.Run(fields);RawContentPreviewProcess::Notify(fields,1,2,3);
    PreviewTime(fields,0x7ff8123456789abcULL);logic.Run(fields); // 비교 불가도 부모와 삭제 효과를 요구하지 않는다.
    bool rejected{};try { logic.Construct(fields,Sid{50},0.2f); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    logic.Destruct(fields,0);CHECK(Get(fields.bytes,4)==0xffffcfc7U && Get(fields.bytes,8)==0xffffcfc7U);
    rejected=false;try { logic.Destruct(fields,1); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    SidPool cd(OriginalEdition::Cd1072,15000,true);rejected=false;try { RawContentPreviewProcess invalid(cd,now,kind,{}); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
