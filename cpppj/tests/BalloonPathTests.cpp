// 벌룬42의 전체 경로 바이트와 그림자 효과를 독립 원문으로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBalloonFrame.h"
#include "o/RawBalloonBody.h"

using namespace netstorm::test::rawscene;
namespace {
// 원문 외부 효과의 모든 인자를 같은 순서로 기록한다.
void Event(std::string& events,std::string name,std::initializer_list<std::uint32_t> values={}) {
    if (!events.empty()) events+=';';events+=name;
    // 모든 DWORD를 부호 축약 없이 덧붙인다.
    for (const auto value:values) events+=':'+std::to_string(value);
}
// 테스트 입력의 실제 슬롯50BYTE만 초기화한다.
std::span<std::uint8_t> Raw(SidPool& pool,std::uint16_t sid) { return {const_cast<std::uint8_t*>(pool.Slot(Sid{sid}).data()),50}; }
// 원문 안의 실제 finder 호출 표식은 C++의 기존 실제 finder 외부 효과와 구별한다.
std::string External(std::string_view text) {
    std::string result;
    // 원문 기대값은 읽기만 하며 내부 경계 표식만 제외한다.
    for (const auto& part:Split(text,';')) if (part!="-" && !part.starts_with("F:")) { if (!result.empty()) result+=';';result+=part; }
    return result.empty() ? "-" : result;
}
}

// 새 생성자/이륙 Init/실제 Step·프레임의 전체100080BYTE와 부모/관찰 전역을 검증한다.
TEST_CASE(BalloonPath_IndependentNativeBodies) {
    std::size_t count{}; //독립 원문의 고유 입력 수다.
    // 각 입력은 새 실제 runtime과 같은 물리 바이트로 시작한다.
    for (const auto& row:LoadFixture(NETSTORM_BALLOON_PATH_FIXTURE).rows) {
        CHECK(row.size()==11);if (row.size()!=11) continue;const auto values=Split(row[0],',');CHECK(values.size()==9);if (values.size()!=9) continue;std::array<std::uint32_t,9> a{};
        // 원문 입력 DWORD만 변환한다.
        for (std::size_t index=0;index<a.size();++index) a[index]=Number(values[index]);
        // 두 정밀도의 저장 원문을 모두 비교한다.
        for (std::size_t index=1;index<=5;++index) CHECK(row[index]==row[index+5]);
        if (!a[0]) {
            // 실제 할당 실패 때에는 생성자가 실행되지 않는다.
            if (a[2]) { RawPathRuntime runtime(OriginalEdition::Patch1078,false,static_cast<std::uint8_t>(a[1]),false,false,true);CHECK(Adler(runtime.Bytes())==Number(row[2])); }
            else CHECK(Adler(std::vector<std::uint8_t>(0x186f0,static_cast<std::uint8_t>(a[1])))==Number(row[2]));
            CHECK(row[1]=="N:100080" && Number(row[3])==a[2]);++count;continue;
        }
        SidPool pool(OriginalEdition::Patch1078,32768);auto raw=Raw(pool,50);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x5040c0);Put(raw,6,7,2);raw[10]=static_cast<std::uint8_t>(a[3]);raw[30]=static_cast<std::uint8_t>(a[2]);Put(raw,14,std::bit_cast<std::uint32_t>(10.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));RawPathRuntime runtime(OriginalEdition::Patch1078,false,0,false,false,true);runtime.SetAttachment(Sid{7},Sid{50});RawPathObservation observation;std::string events;
        if (a[0]==1) {
            std::vector<RawPathPoint> prefix(a[1]);std::vector<std::int32_t> directions(a[1]);runtime.SetPath(prefix,directions);
            runtime.InitializeFlight({10,20},{std::bit_cast<float>(a[4]),std::bit_cast<float>(a[5])},44,static_cast<std::uint8_t>(a[2]));
        } else {
            std::array<RawPathPoint,4> points{{{10,20},{10,16},{14,16},{14,20}}};if (a[8]==2) points.fill({10,20});const std::array<std::int32_t,4> directions{0,0,2,2};const auto times=a[8]==1 ? std::array<double,4>{0,1,1.00001,1.00002} : std::array<double,4>{0,1,3,4}; //원문 경로/시각 입력이다.
            runtime.SetPath(points,directions,times);runtime.RestoreStep(static_cast<std::int32_t>(a[1]),0,a[6]==1);const double now=std::bit_cast<float>(a[4]);auto watch=std::make_shared<bool>(true); //원본 현재 게임 시각과 자기삭제 관찰자다.
            const auto position=[&] { return RawPathPoint{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))}; }; //현재 같은 부모좌표다.
            BalloonFrameHooks hooks;hooks.halt=[&] { Event(events,"H");return a[6]==2; };
            hooks.step=[&] { return runtime.Step(now,position(),observation,[&] { const auto point=position();Event(events,"S",{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y),123}); }); };
            hooks.arrive=[&] { Event(events,"A");runtime.SetFinished(true); };hooks.unpop=[&](Sid sid,std::uint32_t flags) { Event(events,"U",{sid.value,flags});if (a[7]==2) raw[30]=99; };
            hooks.pop=[&](Sid,RawPathPoint point,std::uint32_t flags) { Event(events,"P",{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y),flags});Put(raw,14,std::bit_cast<std::uint32_t>(point.x));Put(raw,18,std::bit_cast<std::uint32_t>(point.y)); };
            hooks.flame=[&](float x,float y) { Event(events,"F",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),0});if (a[7]==1) { Put(raw,14,std::bit_cast<std::uint32_t>(30.0F));Put(raw,18,std::bit_cast<std::uint32_t>(40.0F)); } };
            RunBalloonFrame(pool,Sid{50},runtime,now,111,hooks,watch);
        }
        if ((events.empty() ? "-" : events)!=row[1] || Adler(runtime.Bytes())!=Number(row[2]) || Hex(raw)!=row[3] || observation.flags!=Number(row[4]) || observation.stamp!=Number(row[5])) {
            const auto text="벌룬 경로 원문 행"+std::to_string(count)+" 입력"+row[0]+" 효과 실제"+events+" 원본"+row[1]+" Adler 실제"+std::to_string(Adler(runtime.Bytes()))+" 원본"+row[2];netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());
        }
        ++count;
    }
    CHECK(count==688);
}

// 실제 두 그림자 몸체와 같은 현재 TYPE/효과/전체150BYTE를 비교한다.
TEST_CASE(BalloonPath_IndependentShadowBodies) {
    std::size_t count{}; //최초 원문의 고유 입력 수다.
    auto rows=LoadFixture(NETSTORM_BALLOON_BODY_FIXTURE).rows;const auto heights=LoadFixture(NETSTORM_BALLOON_BODY_HEIGHT_FIXTURE).rows;rows.insert(rows.end(),heights.begin(),heights.end()); //기존233원문과 별도256고도 원문을 모두 읽는다.
    // 실제 finder의 내부 조회는 C++에서도 기존 몸체를 그대로 사용한다.
    for (const auto& row:rows) {
        CHECK(row.size()==7);if (row.size()!=7) continue;CHECK(row[1]==row[4] && row[2]==row[5] && row[3]==row[6]);const auto values=Split(row[0],',');CHECK(values.size()==11);if (values.size()!=11) continue;std::array<std::uint32_t,11> a{};
        // 입력 이외의 원문 출력은 계산하지 않는다.
        for (std::size_t index=0;index<a.size();++index) a[index]=Number(values[index]);
        SidPool pool(OriginalEdition::Patch1078,32768);std::vector<RiftTypeRecord> types(188);types[111].flags1=a[4];std::vector<std::uint8_t> spots(65536,static_cast<std::uint8_t>(a[7]));CarrierPreDestroyState contained;contained.boss=a[6]!=0;CarrierPostPopState postState;postState.loadingDepth=0;std::int32_t frames=4;std::string events;std::size_t pops{}; //현재 공통 상태와 명시 효과 입력이다.
        // 서로 다른 부모/두 그림자/실제내용물Form의 원문 입력이다.
        for (const std::uint16_t sid:std::array<std::uint16_t,4>{50,51,52,53}) {
            auto raw=Raw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x5040c0);raw[10]=111;raw[35]=17;raw[40]=sid==50 ? 0x10 : 2;Put(raw,14,std::bit_cast<std::uint32_t>(sid==50 && a[10] ? 0.0F : 98.0F));Put(raw,18,std::bit_cast<std::uint32_t>(122.0F));Put(raw,36,sid==50 ? a[2] : 7);
            if (sid==50) { Put(raw,6,a[9],2);Put(raw,8,a[3],2);raw[30]=static_cast<std::uint8_t>(a[5]); }
            if (sid==53) { raw[10]=6;Put(raw,18,111,2);Put(raw,20,1,2);Put(raw,22,0,2);Put(raw,24,50,2); }
        }
        const auto parent=Raw(pool,50);CarrierPostPopHooks postHooks;postHooks.validateBase=[](Sid,std::uint32_t) {};postHooks.base=[&](Sid sid,std::uint32_t flags) { Event(events,"B",{sid.value,flags}); };postHooks.carrierCheck=[](Sid) { throw std::logic_error("별도 Carrier check를 호출하면 오류"); };postHooks.claimGround=[](float,float,std::uint8_t) { throw std::logic_error("loading0의 지면 주장 오류"); };RawCarrierPostPop post(pool,types,postState,std::move(postHooks));
        CarrierPreDestroyHooks preHooks;preHooks.validateDamageable=[](Sid,std::uint32_t) {};preHooks.damageablePre=[&](Sid sid,std::uint32_t flags) { Event(events,"D",{sid.value,flags});if (a[8]==3) contained.boss=false; };preHooks.dependentFound=[&] { Event(events,"R"); };RawCarrierPreDestroy pre(pool,contained,std::move(preHooks));
        BalloonBodyHooks hooks;hooks.frames=[&](std::uint32_t type) { CHECK(type==111);return frames; };hooks.create=[&](std::uint32_t type,std::uint32_t flags) { Event(events,"C",{type,flags});if (a[8]==1) { Put(parent,36,1);frames=5; }return Sid{static_cast<std::uint16_t>(a[3] ? 52 : 51)}; };
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { Event(events,"P",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags});++pops;if (a[8]==2 && pops==1) { Put(parent,14,std::bit_cast<std::uint32_t>(98.25F));Put(parent,18,std::bit_cast<std::uint32_t>(121.75F));parent[30]=30; }if (a[8]==4 && pops==2) Put(parent,8,52,2); };
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { Event(events,"U",{sid.value,flags});if (a[8]==3) { Put(parent,14,std::bit_cast<std::uint32_t>(13.5F));Put(parent,18,std::bit_cast<std::uint32_t>(23.25F));parent[40]=16;frames=6; } };
        hooks.check=[&](Sid sid) { Event(events,"K",{sid.value});if (a[8]==5) contained.boss=true; };hooks.destroy=[&](Sid sid,std::uint32_t flags) { Event(events,"X",{sid.value,flags});Raw(pool,sid.value)[11]=1;if (a[8]==1) Put(parent,8,0,2);if (a[8]==2) contained.boss=false; };
        RawBalloonBody body(pool,types,spots,contained.boss,post,pre,std::move(hooks));if (a[0]) body.PreDestroy(Sid{50},a[1]);else body.PostPop(Sid{50},a[1]);std::string raw;
        // 부모와 두 그림자의 물리 바이트 전체를 원문 순서로 기록한다.
        for (const std::uint16_t sid:std::array<std::uint16_t,3>{50,51,52}) raw+=Hex(pool.Slot(Sid{sid}));
        const auto current=std::to_string(contained.boss ? 1 : 0)+','+std::to_string(frames);
        if ((events.empty() ? "-" : events)!=External(row[1]) || raw!=row[2] || current!=row[3]) { const auto text="벌룬 그림자 원문 행"+std::to_string(count)+" 입력"+row[0]+" 효과 실제"+events+" 원본"+row[1]+" raw일치"+std::to_string(raw==row[2]);netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str()); }++count;
    }
    CHECK(count==489);
}

// 저장 가상60은 signed frame을 먼저 읽고 현재 TYPE의 그림 개수와 비교한다.
TEST_CASE(FortObjectSaveBalloon_RobustCurrentFrameAndType) {
    SidPool pool(OriginalEdition::Patch1078,32768);std::vector<RiftTypeRecord> types(188);std::vector<std::uint8_t> spots(65536);CarrierPostPopState state;CarrierPreDestroyState destruction;CarrierPostPopHooks postHooks;postHooks.carrierCheck=[](Sid) { throw std::logic_error("Robust에서 Carrier check 호출 오류"); };postHooks.claimGround=[](float,float,std::uint8_t) { throw std::logic_error("Robust에서 지면 주장 호출 오류"); };postHooks.validateBase=postHooks.base=[](Sid,std::uint32_t) { throw std::logic_error("Robust에서 Pop 호출 오류"); };RawCarrierPostPop post(pool,types,state,std::move(postHooks));CarrierPreDestroyHooks preHooks;preHooks.validateDamageable=preHooks.damageablePre=[](Sid,std::uint32_t) { throw std::logic_error("Robust에서 삭제 호출 오류"); };preHooks.dependentFound=[] { throw std::logic_error("Robust에서 종속 조회 오류"); };RawCarrierPreDestroy pre(pool,destruction,std::move(preHooks));auto raw=Raw(pool,50);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x5040c0);raw[10]=111;bool mutate{};std::uint32_t expected=111;std::int32_t count=4;
    BalloonBodyHooks hooks;hooks.frames=[&](auto type) { CHECK(type==expected);if (mutate) Put(raw,36,999);return count; };RawBalloonBody body(pool,types,spots,destruction.boss,post,pre,std::move(hooks));
    Put(raw,36,3);CHECK(body.Robust(Sid{50}));Put(raw,36,4);CHECK(!body.Robust(Sid{50}));Put(raw,36,0xffffffffU);CHECK(body.Robust(Sid{50}));Put(raw,36,3);mutate=true;CHECK(body.Robust(Sid{50}) && Get(raw,36)==999);
    raw[10]=146;expected=146;Put(raw,36,3);CHECK(body.Robust(Sid{50}));Put(raw,0,0x504198);Put(raw,36,3);CHECK(body.Robust(Sid{50}));count=-2;Put(raw,36,0xffffffffU);CHECK(!body.Robust(Sid{50}));
}
