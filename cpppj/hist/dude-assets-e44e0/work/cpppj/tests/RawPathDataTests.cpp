// 서버 Path의 실제 PE 패킷과 flags0 부착을 동일 raw 풀·공유 버퍼 위에서 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPathData.h"
#include "o/RawPathProcess.h"
#include "o/SquidFactory.h"
#include "o/SquidPop.h"
#include "o/SquidPostPop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// native와 같은 독립 꼭짓점/방향 입력을 제공하며 예상 압축 패킷은 계산하지 않는다.
std::pair<std::vector<RawPathPoint>,std::vector<std::int32_t>> Inputs(std::uint32_t profile) {
    switch (profile) {
    case 0:return {{{10,20}},{0}};
    case 1:return {{{10,20},{10,20},{10,20}},{0,7,2}};
    case 2:return {{{10,20},{11,20}},{0,0}};
    case 3:return {{{10,20},{41,20}},{0,0}};
    case 4:return {{{10,20},{42,20}},{0,0}};
    case 5:return {{{10,20},{73,20}},{0,0}};
    case 6:return {{{10,20},{100,110}},{0,1}};
    case 7:return {{{10,20},{10,40},{80,40},{20,100}},{0,2,4,6}};
    case 8:return {{{10.9F,20.1F},{10.01F,20.9F},{11.9F,20.1F}},{0,1,8}};
    case 9:return {{{-1.9F,-2.1F},{-32.9F,20.9F},{255.9F,260.1F}},{0,7,15}};
    case 10:return {{{255.9F,256.9F},{1.9F,-1.9F}},{0,-1}};
    case 11:return {{{10,20},{10,80},{30,80}},{0,3,0}};
    default:throw std::invalid_argument("Path fixture 프로필");
    }
}
// fixture의 QWORD와 같은 raw 필드를 주변 여백 변경 없이 쓴다.
void Put64(std::span<std::uint8_t> bytes,std::size_t offset,std::uint64_t value) {
    // 낮은 바이트부터 전체 QWORD를 보존한다.
    for (std::size_t i=0;i<8;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
}

// 두 PE의 독립768개 패킷 결과와 전체 공유 버퍼 보존을 C++ 실제 작성기로 재생한다.
TEST_CASE(rawPathData_native_shared_buffer_packets) {
    const auto fixture=LoadFixture(NETSTORM_PATHDATA_FIXTURE);std::size_t checked=0;
    // 두 PE의 모든 고유 입력을 재생하고 반환 길이 밖의 stale buffer까지 비교한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==8);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto profile=Number(row[1]),genus=Number(row[2]),variant=Number(row[3]);const auto layout=RawPathRuntime::Layout(edition);
        RawPathRuntime runtime(edition,(profile&1)!=0);const auto [points,directions]=Inputs(profile);runtime.SetPath(points,directions);
        std::vector<std::uint8_t> image(runtime.Bytes().begin(),runtime.Bytes().end());
        Put(image,4,32700+profile);Put(image,8,5);Put64(image,layout.times,std::stoull(row[5]));
        Put(image,layout.helper,0x12340000+profile*65537);
        const auto tail=edition==OriginalEdition::Patch1078 ? 0x186e4U : 0x2f1cU;
        Put(image,tail,0x87654321+profile);Put(image,tail+4,0xa5+profile);
        const auto before=image;
        RawPathDataHeader header{static_cast<std::uint8_t>(0x30+variant),static_cast<std::uint8_t>(variant ? 0x81 : 0xfe),static_cast<std::uint16_t>(variant ? 0 : 0x8123)};
        std::vector<std::uint8_t> buffer(2048,variant ? 0xa5 : 0x5a);
        CHECK(RawPathData(edition).Write(image,Sid{static_cast<std::uint16_t>(32700+profile)},Sid{5},genus,header,buffer)==static_cast<std::int32_t>(Number(row[6])));
        CHECK(Hex(buffer)==row[7]);CHECK(image==before);++checked;
    }
    CHECK(checked==768);
}

// 동일 raw 풀의 서버 flags0 부착·송신 전 체인·부착 후 공간 상태·삭제 전파/해제를 검사한다.
TEST_CASE(rawPathData_server_flags0_host_and_required_boundaries) {
    // 각 판본의 서버 Host를 실제 factory/pop/unpop/destroy/kernel 위에서 조립한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,32768,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);types[134].footX=types[134].footY=1;
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());std::vector<std::uint8_t> spots(65536);
        SquidPostPopState book;book.suppressed=true;SquidPostPop postPop(pool,types,book);SquidUnpop unpop(pool,hash,spots);SquidPop pop(pool,hash,spots,nullptr,&postPop);
        RawSquidDestroy destroy(pool,unpop,types);Kernel kernel(32);SquidProcessState clock;clock.boss=true;
        std::vector<SquidDestroyEvent> deletion; // 실제 서버 Form 삭제의 송신 요청을 기록하는 명시 관찰 경계다.
        SquidProcessHost host(pool,types,kernel,destroy,clock,{},SquidDestroyHooks{[&](const SquidDestroyEvent& event) {
            CHECK(event.effect==SquidDestroyEffect::Transmit);deletion.push_back(event);
        },[] { return Sid{}; },{}});
        SquidFactory factory(pool,types);const auto parent=factory.Create(134,2);CHECK(parent==Sid{5});
        CHECK(pop.Pop(parent,types[134],1,1,10,20,0)==RawPopResult::Registered);
        RawPathDataHeader header;std::vector<std::uint8_t> buffer(2048,0xa5);std::uint32_t sent=0,cleanup=0;Sid sentForm;
        host.SetFormTransmit(MakePathFormTransmission(pool,types,header,buffer,[&](std::int32_t size) {
            CHECK(size==25);CHECK(Get(buffer,6,2)==parent.value);sentForm=Sid{static_cast<std::uint16_t>(Get(buffer,4,2))};
            CHECK(sentForm.value>=pool.Layout().serverFirst);CHECK(Get(pool.Slot(parent),6,2)==0);CHECK(Get(pool.Slot(sentForm),16,2)==0);
            CHECK(buffer[8]==0xa5 && buffer[9]==0xa5 && buffer[10]==0xa5);++sent;
        }));
        RawPathProcessHooks hooks;
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,types[pool.Slot(sid)[10]],flags); };
        hooks.pop=[&](Sid sid,RawPathPoint point,std::uint32_t flags) { CHECK(pop.Pop(sid,types[pool.Slot(sid)[10]],1,1,point.x,point.y,flags)==RawPopResult::Registered); };
        hooks.repop=[&](Sid sid,std::uint32_t flags) { const auto raw=pool.Slot(sid);CHECK(pop.Pop(sid,types[raw[10]],1,1,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),flags)==RawPopResult::Registered); };
        hooks.halt=[](Sid) { return 10U; };hooks.arrival=[](Sid,std::uint32_t,std::uint32_t) { return 1000U; };
        hooks.soundId=[](Sid) { return 0U; };hooks.stepSound=[](Sid,float,float,std::uint32_t) { throw std::logic_error("부착 검사에서 발소리는 미실행"); };
        hooks.clearBlocked=[](Sid,RawPathPoint,std::uint32_t,std::uint32_t) { throw std::logic_error("부착 검사에서 겹침 정리는 미실행"); };
        hooks.cleanup=[&](RawPathRuntime&,Sid sid,std::uint32_t flags) { CHECK(sid==parent && flags==0);++cleanup; };
        RawPathProcessState state{clock};auto next=std::make_unique<RawPathProcess>(pool,hash,types,frames,shapes,spots,state,std::move(hooks),false);
        const std::array<RawPathPoint,2> points{{{10,20},{11,20}}};const std::array<std::int32_t,2> directions{0,0};next->Runtime().SetPath(points,directions);next->Prepare(0,1,90);
        auto* process=RawPathProcess::Attach(host,std::move(next),parent,0);CHECK(process!=nullptr);CHECK(sent==1);CHECK(process->Form()==sentForm);
        CHECK(Get(pool.Slot(parent),6,2)==sentForm.value);CHECK(Get(pool.Slot(sentForm),16,2)==parent.value);CHECK((pool.Slot(sentForm)[11]&12)==8);
        CHECK(Get(process->Runtime().Bytes(),4)==sentForm.value);CHECK(Get(process->Runtime().Bytes(),8)==parent.value);
        SidPool foreign(edition,32768,true);const auto rejected=MakePathFormTransmission(foreign,types,header,buffer,[](std::int32_t) { throw std::logic_error("다른 풀은 송신 전 거부"); });
        CHECK(Throws([&] { rejected.writeData(*process,false); }));
        host.Kill(*process,0);CHECK(cleanup==1);CHECK(kernel.Size()==0);CHECK(deletion.size()==1);
        if (!deletion.empty()) CHECK(deletion.front().sid==sentForm);
        CHECK(Get(pool.Slot(parent),6,2)==0);CHECK((pool.Slot(sentForm)[11]&1)!=0);
        CHECK(Throws([&] { MakePathFormTransmission(pool,types,header,buffer,{}); }));
        const auto adapter=MakePathFormTransmission(pool,types,header,buffer,[](std::int32_t) { throw std::logic_error("송신 검사 경계"); });
        RegularProcess regular(1,0);CHECK(Throws([&] { adapter.writeData(regular,false); }));
        RawPathRuntime invalid(edition);CHECK(Throws([&] { RawPathData(edition).Write(invalid,Sid{6},parent,0,header,buffer); }));
    }
}

// 실제 가상 Prepare48개의 시각 표를 재생해 패치의 override와 CD의 타입 속도 선택 차이를 확인한다.
TEST_CASE(rawPathData_native_process_prepare_override_editions) {
    const auto fixture=LoadFixture(NETSTORM_PATHPREPARE_FIXTURE);std::size_t checked=0;
    // 실제 가상+0x30의 두 판본 결과를 원본53비트 시각 배열과 대조한다. 호출되지 않는 효과는 명시 예외 경계다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==6);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        SidPool pool(edition,32768,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());std::vector<std::uint8_t> spots(65536);
        SquidProcessState clock;clock.now=std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(row[4])));RawPathProcessState state{clock};RawPathProcessHooks hooks;
        hooks.unpop=hooks.repop=[](Sid,std::uint32_t) { throw std::logic_error("Prepare 검사에서 공간 변경 미실행"); };
        hooks.pop=[](Sid,RawPathPoint,std::uint32_t) { throw std::logic_error("Prepare 검사에서 Pop 미실행"); };
        hooks.halt=[](Sid)->std::uint32_t { throw std::logic_error("Prepare 검사에서 Halt 미실행"); };
        hooks.arrival=[](Sid,std::uint32_t,std::uint32_t)->std::uint32_t { throw std::logic_error("Prepare 검사에서 도착 미실행"); };
        hooks.soundId=[](Sid)->std::uint32_t { throw std::logic_error("Prepare 검사에서 효과음 조회 미실행"); };
        hooks.stepSound=[](Sid,float,float,std::uint32_t) { throw std::logic_error("Prepare 검사에서 효과음 미실행"); };
        hooks.clearBlocked=[](Sid,RawPathPoint,std::uint32_t,std::uint32_t) { throw std::logic_error("Prepare 검사에서 겹침 정리 미실행"); };
        hooks.cleanup=[](RawPathRuntime&,Sid,std::uint32_t) { throw std::logic_error("Prepare 검사에서 수명 정리 미실행"); };
        RawPathProcess process(pool,hash,types,frames,shapes,spots,state,std::move(hooks),false);
        const std::array<RawPathPoint,4> points{{{10,20},{13.1F,24.7F},{15.2F,19.8F},{23.9F,27.6F}}};const std::array<std::int32_t,4> directions{7,0,4,6};process.Runtime().SetPath(points,directions);
        process.Prepare(std::bit_cast<float>(Number(row[1])),std::bit_cast<float>(Number(row[2])),std::bit_cast<float>(Number(row[3])));
        const auto expected=Split(row[5],',');CHECK(expected.size()==4);
        // 실제 하위 Path 시각 함수가 저장한 각 QWORD를 비트 단위로 비교한다.
        for (std::int32_t i=0;i<4;++i) CHECK(std::bit_cast<std::uint64_t>(process.Runtime().Time(i))==std::stoull(expected[i]));
        ++checked;
    }
    CHECK(checked==48);
}
