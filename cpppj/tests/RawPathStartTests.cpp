// 두 실제 PE의 시작/Init/BFS/Prepare/사제 통지를 같은 raw 풀·Host·공유 서버 패킷으로 재생한다.
#include "RawSceneSupport.h"
#include "client/RawPriestWorldBindings.h"
#include "o/RawPathStart.h"
#include "o/RawPathSearch.h"
#include "o/RawPathData.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 상위 호출의 명령 DWORD 세 개다. 마지막 인자도 BYTE로 줄이지 않는다.
constexpr std::uint32_t kHelper=0x12345678,kTailWord=0x87654321,kTailByte=0xa5fedcba;
// 원본 관찰 배열의 꼭짓점/방향/시각 비트를 직접 문자열로 읽는다.
std::string StartPoints(const RawPathRuntime& runtime) {
    std::string text;
    // 원본 배열 순서로 읽으며 기대 경로를 생성하지 않는다.
    for (std::int32_t i=0;i<runtime.Count();++i) {
        if (!text.empty()) text+=';';const auto point=runtime.Point(i);
        text+=std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+std::to_string(std::bit_cast<std::uint32_t>(point.y))+','+
            std::to_string(runtime.Direction(i))+','+std::to_string(std::bit_cast<std::uint64_t>(runtime.Time(i)));
    }
    return text.empty() ? "-" : text;
}
// 외부 경계로 관찰한 호출을 이어 붙인다. 관찰하지 않은 생성자/삭제 구현을 덧붙이지 않는다.
std::string JoinStart(const std::vector<std::string>& events) {
    std::string result;
    // 사건 사이에 원본 fixture와 같은 구분자를 넣는다.
    for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }
    return result.empty() ? "-" : result;
}
// 실제 PE의 Kernel/Frame 관찰 경계는 반환/체인/프레임/예약 검사로 대조하고 여기서는 공유 효과만 고른다.
std::string StartSharedEvents(const std::string& text) {
    std::vector<std::string> selected;
    // 생성·이전 정리·겹침 요청·Init Pop만 정확한 순서/인자로 대조한다.
    for (const auto& event:Split(text,';')) if (event=="C" || event=="K" || event.starts_with("P:") || event.starts_with("B:")) selected.push_back(event);
    return JoinStart(selected);
}
// PE에 공급한 고유 장면과 같은 SID/hash/spot/프레임/전역을 만든다.
struct StartScene {
    OriginalEdition edition; // 두 원본 판본의 실제 필드 폭을 고른다.
    SidPool pool; // 서버와 클라이언트 번호를 실제 원본 free list로 배분하는 유일한 풀이다.
    SquidHash hash; // 초기 표면과 사제 간헐천 조회가 공유하는 네 단계 해시다.
    std::vector<RiftTypeRecord> types; // 현재 genus/발자국을 매 조회에 공급하는 실제 타입 레코드다.
    std::vector<RiftTypeFrames> frames; // AA 표면 프레임을 실제 접합 계산에 공급한다.
    std::vector<SquidDisplayShape> shapes; // 실행하지 않는 걷기 애니메이션의 명시 입력 저장소다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 표면 spot6을 실제 BFS가 읽는다.
    RawPathSearchState searchState; // 실제 방향 우선순위/권한 입력이다.
    SquidProcessState clock; // 같은 Prepare/Host가 읽는 시계와 boss 입력이다.
    RawPathProcessState pathState; // 실제 Path 프로세스의 공유 상태다.
    std::vector<std::string> effects; // PE와 공유하는 외부 효과 요청의 관찰 기록이다.
    std::uint32_t dude{70},teleport{134},mana{105},priest{158},geyser{122}; // 원본 mutable 부모 및 간헐천 타입 번호다.
    bool snap{},retry{}; // 목적지 스냅과 실패 재귀 guard를 참조로 공유한다.
    // 원본과 같은 client5..53 번호를 장면 입력으로 확보하며 결과를 계산하지 않는다.
    explicit StartScene(OriginalEdition chosen):edition(chosen),pool(chosen,32768,true),types(chosen==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({})),shapes(types.size()),pathState{clock} {
        // 사용 중인 raw 입력 슬롯을 실제 client free list에서 순서대로 확보한다.
        for (std::uint16_t id=5;id<=53;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 현재 raw 객체를 해당 단계의 실제 머리/next로 등록한다.
    void Insert(Sid sid,std::uint32_t type,float x,float y,int level,bool buried=false) {
        auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);
        raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=buried ? 8 : 0;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));
        types[type].footX=types[type].footY=1;auto& head=hash.Bucket(level,x,y);Put(raw,4,head,2);head=sid.value;
    }
    // profile별 실제 장면 입력만 공급한다. 경로 생성은 RawPathSearch가 한다.
    void Setup(std::uint32_t parentType,std::uint32_t profile,bool boss) {
        clock.now=123.125;clock.boss=boss;snap=profile==1;retry=profile==7;std::uint16_t next=6;
        types[83].flags1=TypeFlag1::kSurface;types[83].flags2=2;frames[83]=RiftTypeFrames({{'A','A',1,0}});
        // 네모 표면의 모든 셀을 실제 Graph3/spot6 상태로 넣는다.
        for (int y=9;y<15;++y) {
            // 같은 줄의 여섯 셀도 각각 실제 독립 SID를 갖는다.
            for (int x=9;x<15;++x) {
                const Sid sid{next++};Insert(sid,83,static_cast<float>(x),static_cast<float>(y),0);
                pool.AllocatedBytes(sid)[edition==OriginalEdition::Patch1078 ? 30 : 28]=3;spots[y*256+x]=6;
            }
        }
        Insert(Sid{50},122,13.75F,13.25F,1);Insert(Sid{51},122,13.75F,13.25F,1,true);Insert(Sid{52},122,13.5F,13.5F,1);
        if (profile==6) { Insert(Sid{53},150,10.75F,10.25F,1);types[150].flags2=0x40000; }
        auto raw=pool.AllocatedBytes(Sid{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(parentType);raw[11]=profile==4 ? 4 : 0;
        Put(raw,0,edition==OriginalEdition::Patch1078 ? 0x501dd0U : 0x5058d8U);
        Put(raw,14,std::bit_cast<std::uint32_t>(9.2F));Put(raw,18,std::bit_cast<std::uint32_t>(9.3F));raw[edition==OriginalEdition::Patch1078 ? 34 : 32]=2;raw[edition==OriginalEdition::Patch1078 ? 33 : 31]=1;
        if (profile!=4) { auto& head=hash.Bucket(1,9.2F,9.3F);Put(raw,4,head,2);head=5; }
        types[parentType].footX=types[parentType].footY=1;
        if (profile==2 || profile==6 || profile==7) { hash.Cell(0,10,10)=0;hash.Cell(0,9,9)=0; }
    }
    // 목적지 입력을 PE 도구와 동일한 float32로 반환한다.
    RawPathPoint Destination(std::uint32_t profile) const { return profile==3 ? RawPathPoint{-1.25F,400.5F} : profile==5 ? RawPathPoint{9.2F,9.3F} : RawPathPoint{13.2F,13.3F}; }
};
}

// 두 판본의 모든 타입/genus 입력과 CD 미등록 타입 assert를 실제 선택 몸체로 재생한다.
TEST_CASE(rawPathStart_native_process_type_priority) {
    const auto fixture=LoadFixture(NETSTORM_PATHPROCESSTYPE_FIXTURE);std::size_t checked=0;
    // 각 실제 PE의 전체 타입 번호와 balloon genus 교차를 기존 단일 선택 몸체로 대조한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==5);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);const auto type=Number(row[1]);types[type].flags2=Number(row[2]);netstorm::client::RawMovementProcessTypes state;
        if (Number(row[4])) CHECK(Throws([&] { netstorm::client::SelectMovementProcessType(edition,types,state,type); }));
        else CHECK(netstorm::client::SelectMovementProcessType(edition,types,state,type)==Number(row[3]));++checked;
    }
    CHECK(checked==718);
}

// 원본 Start512개를 실제 BFS/Host/RawPathData/SquidFrame에 연결해 결과와 공유 효과를 대조한다.
TEST_CASE(rawPathStart_native_creation_search_server_and_priest) {
    const auto fixture=LoadFixture(NETSTORM_PATHSTART_FIXTURE);std::size_t checked=0;
    // 모든 고유 장면은 서로의 free list/예약/경로를 공유하지 않는 새 월드로 만든다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==8);const auto input=Split(row[1],',');CHECK(input.size()==6);
        const auto type=Number(input[0]),profile=Number(input[1]),pending=Number(input[5]);const bool boss=Number(input[2])!=0,legacy=Number(input[3])!=0,replace=Number(input[4])!=0;
        StartScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);scene.Setup(type,profile,boss);
        SquidPop pop(scene.pool,scene.hash,scene.spots);SquidUnpop unpop(scene.pool,scene.hash,scene.spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);Kernel kernel(32);
        // 화면/네트워크 삭제 송신은 호출 관찰 경계다. 실제 Form 제거/부모체인은 Host가 처리한다.
        SquidDestroyHooks deletion;deletion.selected=[] { return Sid{}; };deletion.emit=[](const SquidDestroyEvent& event) { CHECK(event.effect==SquidDestroyEffect::Transmit); };
        SquidProcessHost host(scene.pool,scene.types,kernel,destroy,scene.clock,{},deletion);RawPathSearch search(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.searchState);
        RawPathDataHeader header;std::vector<std::uint8_t> buffer(2048,0xa5);std::uint32_t sent=0,updates=0,metadataReads=0;Sid sentForm;
        host.SetFormTransmit(MakePathFormTransmission(scene.pool,scene.types,header,buffer,[&](std::int32_t length) {
            CHECK(length>0);CHECK(Get(buffer,6,2)==5);sentForm=Sid{static_cast<std::uint16_t>(Get(buffer,4,2))};CHECK(sentForm.value>=scene.pool.Layout().serverFirst);++sent;
        }));
        // 실제 pop/unpop을 공급하고 실행하지 않은 걷기/도착 효과는 예외로 경계를 지킨다.
        const auto processHooks=[&](bool old) {
            RawPathProcessHooks hooks;
            hooks.unpop=[&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,scene.types[scene.pool.Slot(sid)[10]],flags); };
            hooks.pop=[&,old](Sid sid,RawPathPoint point,std::uint32_t flags) {
                if (!old) scene.effects.push_back("P:"+std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+std::to_string(std::bit_cast<std::uint32_t>(point.y))+','+std::to_string(flags));
                CHECK(pop.Pop(sid,scene.types[scene.pool.Slot(sid)[10]],1,1,point.x,point.y,flags)==RawPopResult::Registered);
            };
            hooks.repop=[&](Sid sid,std::uint32_t flags) { const auto raw=scene.pool.Slot(sid);CHECK(pop.Pop(sid,scene.types[raw[10]],1,1,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),flags)==RawPopResult::Registered); };
            hooks.halt=[](Sid)->std::uint32_t { throw std::logic_error("시작 대조의 Halt 미실행 경계"); };hooks.arrival=[](Sid,std::uint32_t,std::uint32_t)->std::uint32_t { throw std::logic_error("시작 대조의 Arrival 미실행 경계"); };
            hooks.soundId=[](Sid)->std::uint32_t { throw std::logic_error("시작 대조의 발소리 미실행 경계"); };hooks.stepSound=[](Sid,float,float,std::uint32_t) { throw std::logic_error("시작 대조의 발소리 미실행 경계"); };
            hooks.clearBlocked=[](Sid,RawPathPoint,std::uint32_t,std::uint32_t) { throw std::logic_error("시작 대조의 도착 겹침 미실행 경계"); };
            // native Kill 인자는0이지만 실제 서버의 client Form 삭제는 공통 destroy가0x10을 더해 cleanup에 전달한다.
            hooks.cleanup=[&,old](RawPathRuntime&,Sid parent,std::uint32_t flags) { CHECK(parent==Sid{5} && flags==(old ? 0x10U : 0U));if (old) scene.effects.push_back("K"); };
            return MakePathSearchHooks(scene.pool,search,std::move(hooks));
        };
        if (legacy) {
            auto old=std::make_unique<RawPathProcess>(scene.pool,scene.hash,scene.types,scene.frames,scene.shapes,scene.spots,scene.pathState,processHooks(true),type==158);
            old->Runtime().SetTargets(0x87654321,0xfedcba98);CHECK(RawPathProcess::Attach(host,std::move(old),Sid{5},0x10)!=nullptr);
        }
        // 서로 다른 사건을 반복 등록해 사제 시작의 전체 삭제/다시 검색을 검사한다.
        for (const auto event:{0x25cU,0x25dU}) for (std::uint32_t i=0;i<pending;++i) CHECK(host.AddRegular(Sid{5},event,1)!=nullptr);
        SquidFrame frame(scene.pool,scene.types,{
            [](Sid,std::int32_t) { return std::array<float,2>{1,1}; },
            [&](Sid sid,std::uint32_t flags) { CHECK(sid==Sid{5} && flags==0);++updates; },
            [&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,scene.types[scene.pool.Slot(sid)[10]],flags); },
            [&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(pop.Pop(sid,scene.types[scene.pool.Slot(sid)[10]],1,1,x,y,flags)==RawPopResult::Registered); }
        });
        RawContainedLookup lookup(scene.pool,[&](Sid form,std::uint32_t flags) { auto* process=host.FromForm(form);CHECK(process!=nullptr);host.Kill(*process,flags); });netstorm::client::RawMovementProcessTypes typeState;
        RawPathStartHooks startHooks;
        startHooks.processType=[&](std::uint32_t parentType) { return netstorm::client::SelectMovementProcessType(scene.edition,scene.types,typeState,parentType); };
        startHooks.create=[&](bool priest) { scene.effects.push_back("C");return std::make_unique<RawPathProcess>(scene.pool,scene.hash,scene.types,scene.frames,scene.shapes,scene.spots,scene.pathState,processHooks(false),priest); };
        startHooks.metadata=[&](Sid parent) {
            CHECK(parent==Sid{5});++metadataReads;
            if (metadataReads==2) { CHECK(!host.FindEvent(parent,0x25c) && !host.FindEvent(parent,0x25d)); }
            return RawPathStartType{0.7F,90,7};
        };
        startHooks.clearBlocked=[&](RawPathPoint point,std::uint32_t parentType,std::uint32_t owner) { scene.effects.push_back("B:"+std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+std::to_string(std::bit_cast<std::uint32_t>(point.y))+','+std::to_string(parentType)+','+std::to_string(owner)); };
        RawPathStart starter(scene.pool,scene.hash,scene.types,host,lookup,frame,{scene.clock,scene.dude,scene.teleport,scene.mana,scene.priest,scene.geyser,scene.snap,scene.retry},std::move(startHooks));
        auto* result=starter.Start({Sid{5},scene.Destination(profile),replace ? 1U : 0U,kHelper,kTailWord,kTailByte,0x13579bdf});
        CHECK((result!=nullptr)==(Number(row[2])!=0));CHECK(JoinStart(scene.effects)==StartSharedEvents(row[3]));
        if (result) {
            CHECK(StartPoints(result->Runtime())==row[5]);const auto layout=RawPathRuntime::Layout(scene.edition);const auto raw=result->Runtime().Bytes();std::vector<std::string> fields;
            // 저장된 명령 필드 폭을 그대로 읽어 native DWORD 관찰과 대조한다.
            for (const auto offset:std::array<std::size_t,5>{layout.targetA,layout.targetB,layout.helper,scene.edition==OriginalEdition::Patch1078 ? 0x186e4U : 0x2f1cU,scene.edition==OriginalEdition::Patch1078 ? 0x186e8U : 0x2f20U}) fields.push_back(std::to_string(Get(raw,offset)));
            if (scene.edition==OriginalEdition::Patch1078) { fields.push_back(std::to_string(Get(raw,0x186dc)));fields.push_back(std::to_string(Get(raw,0x186e0))); }
            std::string command;
            // 관찰한 필드 사이에 동일한 구분자를 넣고 원본 숫자 폭을 유지한다.
            for (const auto& field:fields) { if (!command.empty()) command+=',';command+=field; }
            CHECK(command==row[4]);CHECK(Get(scene.pool.Slot(Sid{5}),6,2)==result->Form().value);
            CHECK(Get(scene.pool.Slot(result->Form()),16,2)==5 && Get(raw,4)==result->Form().value && Get(raw,8)==5);
            if (type==158) { CHECK(sent==1 && result->Form()==sentForm && updates==2);CHECK(!host.FindEvent(Sid{5},0x25c) && !host.FindEvent(Sid{5},0x25d)); }
            else { CHECK(sent==0 && result->Form().value<scene.pool.Layout().serverFirst && updates==0); }
        } else { CHECK(sent==0 && updates==0 && metadataReads==0); }
        CHECK(Get(scene.pool.Slot(Sid{5}),scene.edition==OriginalEdition::Patch1078 ? 36 : 34,scene.edition==OriginalEdition::Patch1078 ? 4 : 1)==Number(row[6]));
        std::array<std::uint32_t,2> remaining{};Sid current{static_cast<std::uint16_t>(Get(scene.pool.Slot(Sid{5}),6,2))};
        // 같은 Host의 현재 종속 체인에서 실제 남은 각 사건의 개수를 읽는다.
        while (current.value) {
            if (auto* regular=dynamic_cast<RegularProcess*>(host.FromForm(current))) {
                if (regular->Event()==0x25c) ++remaining[0];if (regular->Event()==0x25d) ++remaining[1];
            }
            current=Sid{static_cast<std::uint16_t>(Get(scene.pool.Slot(current),4,2))};
        }
        CHECK(std::to_string(remaining[0])+','+std::to_string(remaining[1])+','+(scene.retry ? "1" : "0")==row[7]);++checked;
    }
    CHECK(checked==512);
}
