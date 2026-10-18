// 보호 원본의 실제 전용 비행410입력과 전체 상태/반환/효과 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSunFlyerFrame.h"
#include "o/RawFlyerFrame.h"
#include "o/RawGunTarget.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 원문 관측에서 따로 보존한9개 현재 객체다.
constexpr std::array<std::uint16_t,9> kIds{50,51,60,61,62,63,70,80,81};
// 선언한 입력과 기존 공통 몸체를 같은 원문136바이트/풀에 연결한다.
struct SunFrameScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // synthetic 현재 입력의 실제 풀이다.
    SquidHash hash; // 빈 표적 검사의 같은 실제 해시다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 공통 통지/부모 타입이다.
    std::vector<SquidDisplayShape> shapes=std::vector<SquidDisplayShape>(188); // 현재 SHP 월드 폭 입력이다.
    std::vector<SunFlyerFrameType> data=std::vector<SunFlyerFrameType>(188); // 현재 폭탄의 원본 세 필드와 소리다.
    std::vector<FlyerFrameType> commonData=std::vector<FlyerFrameType>(188); // 기존 공통 몸체의 같은 번호 표다.
    std::array<std::uint32_t,188> stamps{}; // 공통 비행의 현재 사운드 프레임 표다.
    FlyerProcessFields fields; // 기존112+추가24의 실제 원문 이미지다.
    bool server{true},boss{true},paused{};double now{100}; // 명시 현재 전역 입력이다.
    std::uint32_t bombType{163},mutation{},count{9},may{},frameNumber{7},sunType{73},windType{138},rainType{144},sunProcess{32},soundProcess{47},processType{31}; // 실제 전역/경계 반환의 입력이다.
    std::string events; // 실제 외부 호출의 원문 순서다.
    std::unique_ptr<RawFlyerFrame> common; // 실제 기존 통지/홈 통지를 재사용한다.
    std::unique_ptr<RawSunFlyerFrame> sun; // 새 전용 몸체의 현재 참조다.
    // 원문과 같은 현재 풀/필드/전역을 준비한다. 결과를 계산하지 않는다.
    SunFrameScene(char kind,const std::vector<std::uint64_t>& input) {
        // 관측 마지막 번호까지 실제 client 슬롯을 확보한다.
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==81) break;if (!sid.value || sid.value>81) throw std::logic_error("훨리기그 Frame 검사 SID 준비 오류"); }
        // 모든 관측 객체의 초기50바이트를 구별한다.
        for (auto sid:kIds) { auto raw=pool.AllocatedBytes({sid});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,sid<60 ? 0x50a1e8 : sid>=80 ? 0x509b98 : 0x501dd0);Put(raw,4,0,2);Put(raw,6,sid==50 || sid==51 ? 70U : 0U,2);Put(raw,8,0,2);raw[10]=sid<60 ? 73 : sid>=80 ? 163 : 72;raw[11]=0;Put(raw,12,sid==61 ? 50U : 0U,2);Put(raw,14,std::bit_cast<std::uint32_t>(50.25f));Put(raw,18,std::bit_cast<std::uint32_t>(50.5f));Put(raw,36,0);raw[34]=1;raw[40]=0; }
        // getter의 물리 헤더 폭은 TYPE 발자국2와 다른2.25부터 시작한다.
        shapes[73].loaded=true;shapes[73].frameCount=20;
        for (std::size_t index=0;index<20;++index) { SquidDisplayFrame frame;frame.cellWidth=2.25f+static_cast<float>(index)*.125f;shapes[73].frames.push_back(frame); }
        // 현재 원본 타입의 불변 폭탄 필드다.
        for (std::size_t type=0;type<188;++type) { types[type].footX=types[type].footY=2;data[type]={3,4,6,type==163 ? 881U : 882U}; }
        fields.bytes.fill(0xa5);fields.sunTail.fill(0xa5);Put(fields.bytes,0,0x509f24);fields.bytes[8]=0x10;Put(fields.bytes,10,61,2);Put(fields.bytes,12,60,2);Put(fields.bytes,48,0x509f04);Put(fields.bytes,52,70);Put(fields.bytes,56,50);Put(fields.bytes,80,1);Put(fields.bytes,84,1);Put(fields.bytes,88,0);Put(fields.bytes,92,0);Put(fields.bytes,96,0);Time(104,100);Time(112,0);Time(120,100);Write(128,std::bit_cast<std::uint32_t>(-10000.0f));Write(132,std::bit_cast<std::uint32_t>(-10000.0f));
        if (kind=='I') { now=std::bit_cast<double>(input[0]);fields.bytes.fill(static_cast<std::uint8_t>(input[1]));fields.sunTail.fill(static_cast<std::uint8_t>(input[1])); }
        else if (kind=='A') { now=std::bit_cast<double>(input[0]);Put(fields.bytes,52,static_cast<std::uint32_t>(input[1]));count=static_cast<std::uint32_t>(input[2]); }
        else if (kind=='N') { server=input[0]!=0;Put(fields.bytes,12,static_cast<std::uint32_t>(input[3]),2);mutation=static_cast<std::uint32_t>(input[5]); }
        else if (kind=='P') { server=input[0]!=0;Put(fields.bytes,10,static_cast<std::uint32_t>(input[1]),2);pool.AllocatedBytes({61})[11]=static_cast<std::uint8_t>(input[2]);Write(96,static_cast<std::uint32_t>(input[3])); }
        else if (kind=='C') { Write(96,static_cast<std::uint32_t>(input[0]));mutation=static_cast<std::uint32_t>(input[1]); }
        else if (kind=='E') { server=input[0]!=0;mutation=static_cast<std::uint32_t>(input[1]); }
        else if (kind=='F') { Write(84,static_cast<std::uint32_t>(input[0]));pool.AllocatedBytes({50})[40]=static_cast<std::uint8_t>(input[1]);boss=input[2]!=0;paused=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(input[3]))>0;Write(96,static_cast<std::uint32_t>(input[4]));Put(pool.AllocatedBytes({80}),36,static_cast<std::uint32_t>(input[5]));pool.AllocatedBytes({80})[11]=static_cast<std::uint8_t>(input[6]);Time(104,100-std::array<double,3>{0,.25,1}[input[7]]);Time(120,input[8] ? 99 : 101);may=static_cast<std::uint32_t>(input[9]);Time(112,std::array<double,4>{0,99,100,101}[input[10]]);Write(132,std::bit_cast<std::uint32_t>(input[11] ? 49.0f : 51.0f));mutation=static_cast<std::uint32_t>(input[12]); }
        FlyerFrameHooks base; // 실제 공통45b760/45b7a0과 전용 통지 재진입의 경계다.
        base.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };
        base.removeDepend=[this](Sid form,Sid target) { Event("Remove,"+std::to_string(form.value)+','+std::to_string(target.value)+",0"); };
        base.refreshForm=[this](FlyerProcessFields&) { Event("Refresh"); };
        base.homeNotice=[this](Sid home,std::uint32_t first,std::uint32_t second) { Event("Home,"+std::to_string(home.value)+','+std::to_string(first)+','+std::to_string(second)); };
        base.sunNotify=[this](FlyerProcessFields& live,std::uint32_t message,std::uint32_t first,std::uint32_t second) { sun->Notify(live,message,first,second); };
        static const bool checking=false; // 원본 관측은 개발자 추가 진단을 끈다.
        common=std::make_unique<RawFlyerFrame>(pool,types,commonData,stamps,FlyerFrameGlobals{server,boss,checking,now,frameNumber,sunType,windType,rainType,sunProcess,soundProcess},std::move(base));
        sun=std::make_unique<RawSunFlyerFrame>(pool,shapes,data,SunFlyerFrameGlobals{boss,paused,now,bombType},Hooks());
    }
    // 실제136바이트 원문에 DWORD를 쓴다.
    void Write(std::size_t at,std::uint32_t value) { if (at<112) Put(fields.bytes,at,value);else Put(fields.sunTail,at-112,value); }
    // 현재double 저장 두DWORD의 명시 입력이다.
    void Time(std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Write(at,static_cast<std::uint32_t>(bits));Write(at+4,static_cast<std::uint32_t>(bits>>32)); }
    // 원문 사건을 생략/중복 제거 없이 보존한다.
    void Event(std::string value) { if (!events.empty()) events+=';';events+=value; }
    // 원문 observer와 같은 외부 효과/변경 입력을 공급한다.
    SunFlyerFrameHooks Hooks() {
        SunFlyerFrameHooks hooks;
        hooks.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };
        hooks.robust=[this](Sid sid,bool checking) { Event("Sid,"+std::to_string(sid.value)+','+std::to_string(checking));return sid.value>0 && sid.value<65535; };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event("Destroy,"+std::to_string(sid.value)+','+std::to_string(flags));if (mutation==6) Write(96,81); };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("Unpop,"+std::to_string(sid.value)+','+std::to_string(flags));if (mutation==1 && sid.value==50) { Put(pool.AllocatedBytes({50}),14,std::bit_cast<std::uint32_t>(71.25f));Put(pool.AllocatedBytes({50}),18,std::bit_cast<std::uint32_t>(72.5f));Write(56,51);now=200; } };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("Pop,"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags));Put(pool.AllocatedBytes(sid),14,std::bit_cast<std::uint32_t>(x));Put(pool.AllocatedBytes(sid),18,std::bit_cast<std::uint32_t>(y));if (mutation==4 && sid.value==80) Put(pool.AllocatedBytes({50}),18,std::bit_cast<std::uint32_t>(52.5f)); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { Event("Repop,"+std::to_string(sid.value)+','+std::to_string(flags));if (mutation==5) Write(96,0); };
        hooks.mayTarget=[this](Sid parent,Sid target) { Event("May,"+std::to_string(parent.value)+','+std::to_string(target.value));if (mutation==3) { Write(56,51);Put(fields.bytes,12,61,2); }return may!=0; };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { Event("Create,"+std::to_string(type)+','+std::to_string(flags));pool.AllocatedBytes({80})[10]=static_cast<std::uint8_t>(type);if (mutation==7) Write(96,81);if (mutation==8) pool.AllocatedBytes({80})[10]=164;return Sid{80}; };
        hooks.sound=[this](float x,float y,std::uint32_t sound) { Event("Sound,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(sound));if (mutation==2) now=200; };
        hooks.runCount=[this](Sid parent) { Event("Count,"+std::to_string(parent.value));return static_cast<std::int32_t>(count); };
        hooks.baseFrame=[this](FlyerProcessFields&) { Event("Base"); };
        hooks.baseNotify=[this](FlyerProcessFields& live,std::uint32_t message,std::uint32_t first,std::uint32_t second) { common->BaseNotify(live,message,first,second); };
        hooks.baseParentDying=[this](FlyerProcessFields& live) { common->ParentDying(live); };return hooks;
    }
    // 실제 관측 전체136바이트를 읽는다.
    std::string Image() const { return Hex(fields.bytes)+Hex(fields.sunTail); }
    // 현재9개 객체450바이트를 같은 순서로 읽는다.
    std::string Objects() const { std::string raw;for (auto sid:kIds) raw+=Hex(pool.Slot({sid}));return raw; }
    // 실제 원문20바이트 전역을 현재 값으로 읽는다.
    std::string Globals(std::uint32_t pause) const { std::array<std::uint8_t,20> raw{};Put(raw,0,server ? 1U : 0U);Put(raw,4,boss ? 1U : 0U);Put(raw,8,pause);const auto time=std::bit_cast<std::uint64_t>(now);Put(raw,12,static_cast<std::uint32_t>(time));Put(raw,16,static_cast<std::uint32_t>(time>>32));return Hex(raw); }
};
}
// 새 C++ 전에 동결한 모든 전용 비행/기하/현재 반환/효과를 대조한다.
TEST_CASE(SunFlyerFrame_Native410) {
    std::size_t count{}; // 전체 독립 원문 대조 수다.
    // 원본 기대를 제품 계산으로 만들지 않고 전수 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_SUN_FLYER_FRAME_FIXTURE).rows) {
        CHECK(row.size()==7);if (row.size()!=7) return;const auto kind=row[0][0];std::vector<std::uint64_t> input; // 현재 명시 입력이다.
        if (!row[1].empty()) for (const auto& part:Split(row[1],',')) input.push_back(std::stoull(part)); // double 비트도64비트 그대로 보존한다.
        SunFrameScene scene(kind,input);std::string result="-";
        if (kind=='I') scene.sun->Initialize(scene.fields);
        else if (kind=='A') result=std::to_string(static_cast<std::uint32_t>(scene.sun->Animation(scene.fields)));
        else if (kind=='N') scene.sun->Notify(scene.fields,static_cast<std::uint32_t>(input[1]),static_cast<std::uint32_t>(input[2]),static_cast<std::uint32_t>(input[4]));
        else if (kind=='P') scene.sun->ParentDying(scene.fields);
        else if (kind=='C') scene.sun->ClearBomb(scene.fields);
        else if (kind=='R') result=std::to_string(std::bit_cast<std::uint64_t>(RawSunFlyerFrame::ReturnDelay()));
        else if (kind=='E') { FlyerProcessHooks hooks;hooks.assertion=[&](std::uint32_t line) { scene.Event("Assert,"+std::to_string(line)); };hooks.pieces=[&](Sid parent) { scene.Event("Pieces,"+std::to_string(parent.value));if (scene.mutation==9) scene.Write(56,51); };hooks.destroy=[&](Sid parent,std::uint32_t flags) { scene.Event("Destroy,"+std::to_string(parent.value)+','+std::to_string(flags)); };RawFlyerProcess logic(scene.pool,scene.types,FlyerProcessGlobals{scene.server,scene.now,scene.processType},std::move(hooks));logic.Finish(scene.fields); }
        else if (kind=='Z') { std::vector<GunTargetType> targetTypes(188);GunTargetState state;RawGunTarget logic(scene.pool,scene.hash,scene.types,targetTypes,scene.shapes,state);result=std::to_string(logic.MayTarget({50.25f,50.5f,49.25f,49.5f,73,0,1,0},{})); }
        else scene.sun->Frame(scene.fields);
        const std::array<std::string,5> actual{scene.Image(),scene.Objects(),scene.Globals(kind=='F' ? static_cast<std::uint32_t>(input[3]) : 0),result,scene.events.empty() ? "-" : scene.events}; // 실제 실행 뒤의 결과만 읽는다.
        // 차이가 있는 입력/열을 기록하고 정상 사례로 대체하지 않는다.
        for (std::size_t index=0;index<actual.size();++index) { if (actual[index]!=row[index+2]) std::printf("훨리기그 비행 차이 %c %s 열%zu\n실제%s\n원문%s\n",kind,row[1].c_str(),index+2,actual[index].c_str(),row[index+2].c_str());CHECK(actual[index]==row[index+2]); }
        ++count;
    }
    CHECK(count==410);
}
