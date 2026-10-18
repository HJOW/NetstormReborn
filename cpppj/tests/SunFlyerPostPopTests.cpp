// 훨리기그 전용 생성/홈/소리/그림자를 실제 보호 원본391행과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFlyerProcess.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 관측에서 따로 보존한 부모/홈/그림자/보조 객체 번호다.
constexpr std::array<std::uint16_t,5> kIds{50,60,61,70,71};
// 원본의 외부 경계 입력만 공급하며 제품 처리기의 결과는 계산하지 않는다.
struct SunScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 같은 현재 객체를 담는 실제 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 flags1 입력이다.
    std::vector<FlyerPostPopType> details=std::vector<FlyerPostPopType>(188); // 같은 번호의 speed/전체 프레임/소리 토큰이다.
    FlyerProcessFields fields; // 원본136바이트를112+24로 나눠 보존한다.
    bool server{},allocated{},soundAllocated{}; // 두 확보의 명시 반환과 현재 서버 입력이다.
    double now{100.25};std::uint32_t processType{31},mutation{},popCount{}; // 경계 뒤에 다시 읽는 현재 전역이다.
    std::string events,attach{"-"}; // 원본 순서와 기본 생성 부착 직전의 전체136바이트다.
    std::unique_ptr<RawFlyerProcess> logic; // 현재 같은 필드의 기본 생성자다.
    // 현재 원본 입력을 다시 설치하고 순서를 기록하는 경계를 준비한다.
    explicit SunScene(const std::vector<std::string>& input) {
        // 관측 대상까지 실제 client 슬롯을 확보한다.
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==71) break;if (!sid.value || sid.value>71) throw std::logic_error("훨리기그 검사 SID 준비 오류"); }
        // 독립 원문과 같은 초기50바이트를 설치한다.
        for (auto sid:kIds) { auto raw=pool.AllocatedBytes({sid});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x50a1e8);Put(raw,4,0,2);Put(raw,6,0,2);Put(raw,8,0,2);raw[10]=73;raw[11]=0;Put(raw,12,60,2);Put(raw,14,std::bit_cast<std::uint32_t>(50.25f));Put(raw,18,std::bit_cast<std::uint32_t>(50.5f));Put(raw,36,2);raw[40]=0; }
        Put(pool.AllocatedBytes({60}),0,0x509930);pool.AllocatedBytes({60})[10]=72;
        server=Number(input[0])!=0;allocated=Number(input[2])!=0;soundAllocated=Number(input[3])!=0;mutation=Number(input[7]);Put(pool.AllocatedBytes({50}),36,Number(input[4]));Put(pool.AllocatedBytes({50}),8,Number(input[6]) ? 60U : 0U,2);
        // 부모TYPE 변경 뒤의 snapshot 차이를 서로 다른 현재 값으로 구별한다.
        for (auto type:{73U,143U}) { types[type].flags1=Number(input[5]) ? 0x400000 : 0;details[type]={std::bit_cast<std::uint32_t>(type==73 ? .75f : 1.25f),type==73 ? 12 : 18,type==73 ? 901U : 911U,type==73 ? 902U : 912U}; }
        fields.bytes.fill(0xa5);fields.sunTail.fill(0xa5);FlyerProcessHooks hooks; // 생성 이전 미정 바이트의 실제 입력이다.
        hooks.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };
        hooks.attach=[this](FlyerProcessFields& live,Sid parent,std::uint32_t type,std::uint32_t flags) { CHECK(&live==&fields);attach=Image();Event("Attach,"+std::to_string(type)+','+std::to_string(parent.value)+','+std::to_string(flags));if (mutation==2) { Put(fields.bytes,84,77);pool.AllocatedBytes({50})[10]=143;Put(pool.AllocatedBytes({50}),12,13,2);now=200; } };
        logic=std::make_unique<RawFlyerProcess>(pool,types,FlyerProcessGlobals{server,now,processType},std::move(hooks));
    }
    // 명시 관측 순서를 중복 제거 없이 쌓는다.
    void Event(std::string value) { if (!events.empty()) events+=';';events+=value; }
    // 추가꼬리를 기본112바이트 뒤에 붙여 전체136바이트를 원문과 같은 순서로 읽는다.
    std::string Image() const { return Hex(fields.bytes)+Hex(fields.sunTail); }
    // 선언한 외부 확보/부착/공간/소리 입력을 원본과 같은 순서로 적용한다.
    FlyerPostPopHooks Hooks() {
        FlyerPostPopHooks hooks;
        hooks.allocateSunProcess=[this] { Event("Allocate,136");if (mutation==7) { pool.AllocatedBytes({50})[10]=143;Put(pool.AllocatedBytes({50}),12,13,2); }if (mutation==8) { server=false;processType=30; }return allocated; };
        hooks.createSunProcess=[this](Sid parent,Sid home,std::uint32_t speed,float parameter) { logic->Construct(fields,parent,home,speed,parameter);RawFlyerProcess::SpecializeSun(fields); };
        hooks.allocateSound=[this] { Event("Allocate,40");return soundAllocated; };
        hooks.createSound=[this](Sid parent,std::uint32_t sound,std::uint32_t alternate,std::uint32_t flags) { Event("Sound,"+std::to_string(parent.value)+','+std::to_string(sound)+','+std::to_string(alternate)+','+std::to_string(flags)); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { Event("Repop,"+std::to_string(sid.value)+','+std::to_string(flags)); };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { Event("Create,"+std::to_string(type)+','+std::to_string(flags));if (mutation==3) { Put(pool.AllocatedBytes({50}),36,7);Put(pool.AllocatedBytes({50}),14,std::bit_cast<std::uint32_t>(90.25f)); }return Sid{61}; };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("Pop,"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags));++popCount;if (mutation==4 && popCount==1) { Put(pool.AllocatedBytes({50}),36,9);Put(pool.AllocatedBytes({50}),18,std::bit_cast<std::uint32_t>(70.25f)); } };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("Unpop,"+std::to_string(sid.value)+','+std::to_string(flags));if (mutation==6) { Put(pool.AllocatedBytes({50}),14,std::bit_cast<std::uint32_t>(100.25f));Put(pool.AllocatedBytes({50}),18,std::bit_cast<std::uint32_t>(110.25f)); } };
        hooks.initializeUnit=[this](Sid sid) { const auto time=std::bit_cast<std::uint64_t>(now);Event("SetTime,"+std::to_string(sid.value)+','+std::to_string(static_cast<std::uint32_t>(time))+','+std::to_string(static_cast<std::uint32_t>(time>>32)));if (mutation==5) { Put(pool.AllocatedBytes({50}),8,60,2);Put(pool.AllocatedBytes({50}),36,11);Put(pool.AllocatedBytes({50}),14,std::bit_cast<std::uint32_t>(80.25f)); } };
        hooks.shadowOffset=[](Sid) { return std::array<float,2>{-.6f,3.5f}; };
        hooks.damageable=[this](Sid,std::uint32_t flags) { Event("Base,"+std::to_string(flags)); };return hooks;
    }
    // 전체 관측 객체250바이트를 현재 풀에서 그대로 읽는다.
    std::string Objects() const { std::string value;for (auto sid:kIds) value+=Hex(pool.Slot({sid}));return value; }
    // 바뀔 수 있는 원본 전역16바이트를 동일 순서로 읽는다.
    std::string Globals() const { std::array<std::uint8_t,16> raw{};Put(raw,0,server ? 1U : 0U);const auto time=std::bit_cast<std::uint64_t>(now);Put(raw,4,static_cast<std::uint32_t>(time));Put(raw,8,static_cast<std::uint32_t>(time>>32));Put(raw,12,processType);return Hex(raw); }
};
}
// 동결한 독립 원문의 전체391행을 빠짐없이 대조한다.
TEST_CASE(SunFlyerPostPop_Native391) {
    std::size_t count{}; // 실제 대조한 고유 입력 수다.
    // 기존 기대 원문을 변경하거나 제품 결과로 다시 만들지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_SUN_FLYER_POSTPOP_FIXTURE).rows) {
        CHECK(row.size()==6);if (row.size()!=6) return;const auto input=Split(row[0],',');SunScene scene(input);RawFlyerPostPop logic(scene.pool,scene.types,scene.details,scene.server,scene.Hooks());logic.PostPop({50},Number(input[1]));
        const std::array<std::string,5> observed{scene.Image(),scene.Objects(),scene.Globals(),scene.attach,scene.events.empty() ? "-" : scene.events}; // 실제 호출 후 결과만 수집한다.
        // 실패한 입력과 필드를 각각 기록해 원문 관측으로 되돌아갈 수 있게 한다.
        for (std::size_t index=0;index<observed.size();++index) { if (observed[index]!=row[index+1]) std::printf("훨리기그 후처리 차이 %s 열%zu\n실제%s\n원문%s\n",row[0].c_str(),index+1,observed[index].c_str(),row[index+1].c_str());CHECK(observed[index]==row[index+1]); }
        ++count;
    }
    CHECK(count==391);
}
