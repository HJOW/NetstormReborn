// 비행 생성/부착 전후/실제 그림자/홈 연결/상태 writer를 독립 원문 자료와 전수 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFlyerProcess.h"
#include "o/RawSunAviaryProcess.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,5> kFlyerIds{50,60,61,70,71}; // 부모/기존그림자/새그림자/종속2개의 실제 입력이다.
// 원문과 같은 현재 입력을 설치하고 효과의 인수/명시 변경 입력만 기록한다.
struct FlyerScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 실제 free-list 구간을 포함하는 같은 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 실제 genus/flags1의 입력 표다.
    std::vector<FlyerPostPopType> details=std::vector<FlyerPostPopType>(188); // 실제 speed/전체 프레임 입력이다.
    FlyerProcessFields fields; // 같은112BYTE 이미지다.
    bool server{true},allocated{true};double now{100.25};std::uint32_t processType{31},mutation{},popCount{}; // 명시 현재 전역과 외부 경계 입력이다.
    char kind{};std::string trace,attachImage{"-"}; // 원문 호출 순서와 부착 직전 이미지다.
    std::unique_ptr<RawFlyerProcess> logic;std::unique_ptr<RawFlyerPostPop> post; // 입력보다 먼저 해제할 실제 제품 몸체다.
    // 입력 객체/필드/TYPE만 준비하며 기대 출력은 계산하지 않는다.
    FlyerScene(char scope,const std::vector<std::string>& input):kind(scope) {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==71) break;if (!sid.value || sid.value>71) throw std::logic_error("비행 검사 SID 준비 오류"); } // 관찰 입력의71번까지 실제 확보한다.
        for (const auto value:kFlyerIds) { auto raw=pool.AllocatedBytes(Sid{value});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x50a388);Put(raw,4,value==70 ? 71U : 0U,2);Put(raw,6,value==50 ? 70U : 0U,2);Put(raw,8,0,2);raw[10]=value<70 ? 144 : 31;raw[11]=0;Put(raw,12,17,2);Put(raw,14,std::bit_cast<std::uint32_t>(50.25f));Put(raw,18,std::bit_cast<std::uint32_t>(50.5f));Put(raw,36,2);raw[40]=0; } // 전체250BYTE를 독립 관찰 입력과 맞춘다.
        for (const auto type:{31U,143U,144U}) { types[type].flags1=0x400000;types[type].flags2=0x100000;details[type]={std::bit_cast<std::uint32_t>(type==143 ? 1.25f : .75f),12}; } // 실제 해당 TYPE의 현재 입력 필드다.
        fields.bytes.fill(0xa5);fields.bytes[8]=0x10;Put(fields.bytes,52,77);Put(fields.bytes,56,50);
        if (kind=='C') { server=Number(input[0])!=0;fields.bytes.fill(static_cast<std::uint8_t>(Number(input[4])));mutation=Number(input[5]); }
        else if (kind=='P') { server=Number(input[0])!=0;allocated=Number(input[2])!=0;Put(pool.AllocatedBytes(Sid{50}),36,Number(input[3]));details[144].frames=std::bit_cast<std::int32_t>(Number(input[4]));types[144].flags1=Number(input[5]) ? 0x400000 : 0;Put(pool.AllocatedBytes(Sid{50}),8,Number(input[6]) ? 60U : 0U,2);mutation=Number(input[7]);fields.bytes.fill(0xa5); }
        else if (kind=='B') {
            types[144].flags2=Number(input[0]);const auto layout=Number(input[1]);if (!layout) Put(pool.AllocatedBytes(Sid{50}),6,0,2);if (layout==2) pool.AllocatedBytes(Sid{70})[11]=2;if (layout==3) pool.AllocatedBytes(Sid{70})[11]=1;if (layout==4) { pool.AllocatedBytes(Sid{70})[10]=32;pool.AllocatedBytes(Sid{71})[11]=2; }
        } else if (kind=='W') { fields.bytes[8]=static_cast<std::uint8_t>(Number(input[0]));Put(fields.bytes,10,65534,2);Put(fields.bytes,12,65533,2);Put(fields.bytes,16,0x3f400000);Put(fields.bytes,20,0x3f000000);Put(fields.bytes,24,0xffffffff);Time(32,123.25);Time(40,-5.75); }
        logic=std::make_unique<RawFlyerProcess>(pool,types,FlyerProcessGlobals{server,now,processType},ProcessHooks());post=std::make_unique<RawFlyerPostPop>(pool,types,details,server,PostHooks());
    }
    void Time(std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,at,static_cast<std::uint32_t>(bits));Put(fields.bytes,at+4,static_cast<std::uint32_t>(bits>>32)); } // 실제 double 입력만 쓴다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 현재 호출을 순서대로 연결한다.
    // 결과를 만들지 않고 원문과 같은 외부 부착/조회 입력을 공급한다.
    FlyerProcessHooks ProcessHooks() {
        FlyerProcessHooks hooks;hooks.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line));if (mutation==1) { now=200;processType=30;fields.bytes[8]=0x10; } };
        hooks.attach=[this](FlyerProcessFields& current,Sid parent,std::uint32_t type,std::uint32_t flags) { CHECK(&current==&fields);attachImage=Hex(current.bytes);Event("Attach,"+std::to_string(type)+','+std::to_string(parent.value)+','+std::to_string(flags));if (mutation==2) { Put(fields.bytes,84,77);pool.AllocatedBytes(Sid{50})[10]=143;Put(pool.AllocatedBytes(Sid{50}),12,13,2);now=200; } };
        hooks.findProcess=[this](Sid parent,std::uint32_t type) { return FindAttachedProcess(pool,parent,type); };
        hooks.fields=[this](Sid form)->FlyerProcessFields& { Event("Fields,"+std::to_string(form.value));return fields; };return hooks;
    }
    // 시간/공간/할당 경계만 명시 관찰과 같은 인수/변경 입력으로 공급한다.
    FlyerPostPopHooks PostHooks() {
        FlyerPostPopHooks hooks;hooks.allocateRainProcess=[this] { Event("Allocate,112");if (mutation==7) { pool.AllocatedBytes(Sid{50})[10]=143;Put(pool.AllocatedBytes(Sid{50}),12,13,2); }if (mutation==8) { server=false;processType=30; }return allocated; };
        hooks.createRainProcess=[this](Sid parent,Sid home,std::uint32_t speed,float parameter) { logic->Construct(fields,parent,home,speed,parameter);RawFlyerProcess::SpecializeRain(fields); };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { Event("Create,"+std::to_string(type)+','+std::to_string(flags));if (mutation==3) { Put(pool.AllocatedBytes(Sid{50}),36,7);Put(pool.AllocatedBytes(Sid{50}),14,std::bit_cast<std::uint32_t>(90.25f)); }return Sid{61}; };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("Pop,"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags));++popCount;if (mutation==4 && popCount==1) { Put(pool.AllocatedBytes(Sid{50}),36,9);Put(pool.AllocatedBytes(Sid{50}),18,std::bit_cast<std::uint32_t>(70.25f)); } };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("Unpop,"+std::to_string(sid.value)+','+std::to_string(flags));if (mutation==6) { Put(pool.AllocatedBytes(Sid{50}),14,std::bit_cast<std::uint32_t>(100.25f));Put(pool.AllocatedBytes(Sid{50}),18,std::bit_cast<std::uint32_t>(110.25f)); } };
        hooks.initializeUnit=[this](Sid sid) { const auto bits=std::bit_cast<std::uint64_t>(now);Event("SetTime,"+std::to_string(sid.value)+','+std::to_string(static_cast<std::uint32_t>(bits))+','+std::to_string(static_cast<std::uint32_t>(bits>>32)));if (mutation==5) { Put(pool.AllocatedBytes(Sid{50}),8,60,2);Put(pool.AllocatedBytes(Sid{50}),36,11);Put(pool.AllocatedBytes(Sid{50}),14,std::bit_cast<std::uint32_t>(80.25f)); } };
        hooks.shadowOffset=[](Sid) { return std::array<float,2>{.5f,.5f}; };hooks.damageable=[this](Sid,std::uint32_t flags) { Event("Base,"+std::to_string(flags)); };return hooks;
    }
    std::string Objects() const { std::string value;for (const auto sid:kFlyerIds) value+=Hex(pool.Slot(Sid{sid}));return value; } // 전체 객체250BYTE다.
    std::string Globals() const { std::array<std::uint8_t,16> raw{};Put(raw,0,server ? 1U : 0U);const auto bits=std::bit_cast<std::uint64_t>(now);Put(raw,4,static_cast<std::uint32_t>(bits));Put(raw,8,static_cast<std::uint32_t>(bits>>32));Put(raw,12,processType);return Hex(raw); } // 현재 전역16BYTE다.
};
// 실제 각 입력의 전체 필드/객체/현재 전역/패킷/부착 직전/호출 순서를 비교한다.
void CompareFlyer(char kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_FLYER_POSTPOP_FIXTURE);std::size_t compared{};
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 헤더를 제외한 해당 원문 입력을 독립 재생한다.
        const auto& row=fixture.rows[index];CHECK(row.size()==9);if (row[0]!=std::string(1,kind)) continue;++compared;const auto input=Split(row[1],',');FlyerScene scene(kind,input);std::string packet="-",result="-";
        if (kind=='C') scene.logic->Construct(scene.fields,Sid{50},Sid{static_cast<std::uint16_t>(Number(input[1]))},Number(input[2]),std::bit_cast<float>(Number(input[3])));
        else if (kind=='P') scene.post->PostPop(Sid{50},Number(input[1]));
        else if (kind=='B') scene.logic->Bind(Sid{50},Sid{static_cast<std::uint16_t>(Number(input[2]))});
        else { std::array<std::uint8_t,64> raw;raw.fill(0xa7);result=std::to_string(RawFlyerProcess::WriteProcess(scene.fields,raw,Number(input[1])!=0,Number(input[2])!=0));packet=Hex(raw); }
        const auto image=kind=='P' && scene.attachImage=="-" ? "-" : Hex(scene.fields.bytes);const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (image!=row[2] || scene.Objects()!=row[3] || scene.Globals()!=row[4] || packet!=row[5] || result!=row[6] || scene.attachImage!=row[7] || trace!=row[8]) std::fprintf(stderr,"비행 불일치 %c/%s\n실제%s\n기대%s\n",kind,row[1].c_str(),trace.c_str(),row[8].c_str());
        CHECK(image==row[2]);CHECK(scene.Objects()==row[3]);CHECK(scene.Globals()==row[4]);CHECK(packet==row[5]);CHECK(result==row[6]);CHECK(scene.attachImage==row[7]);CHECK(trace==row[8]);
    }
    CHECK(compared==expected);
}
}
TEST_CASE(Flyer_NativeConstructor38) { CompareFlyer('C',38); } // 기본 표 부착 이전의 모든 쓰기와 padding/현재 전역이다.
TEST_CASE(Flyer_NativePostPop329) { CompareFlyer('P',329); } // 맨 오 워 확장과 그림자의 생성/재배치 순서다.
TEST_CASE(Flyer_NativeBind30) { CompareFlyer('B',30); } // 실제 첫nondead Form 조회와 WORD 홈/dirty 쓰기다.
TEST_CASE(Flyer_NativeWriter1024) { CompareFlyer('W',1024); } // 모든 dirty/부모/force의 전체64BYTE 출력이다.
// 선택 누락과 원문 앞선 쓰기, 짧은 패킷의 사전 거부를 검사한다.
TEST_CASE(Flyer_RequiredBoundaries) {
    FlyerScene scene('P',Split("1,1,1,2,12,1,1,0",','));auto hooks=scene.PostHooks();hooks.create={};RawFlyerPostPop missing(scene.pool,scene.types,scene.details,scene.server,hooks);
    CHECK(Throws([&] { missing.PostPop(Sid{50},1); }));CHECK(scene.attachImage!="-");CHECK(Get(scene.fields.bytes,0)==0x509f60);CHECK(scene.trace=="Allocate,112;Attach,31,50,8");
    CHECK(Throws([&] { scene.logic->Frame(scene.fields); }));CHECK(Throws([&] { scene.logic->Notify(scene.fields,2,0,50); }));CHECK(Throws([&] { scene.logic->ParentDying(scene.fields); }));
    const auto before=scene.fields.bytes;std::array<std::uint8_t,8> packet{};CHECK(Throws([&] { RawFlyerProcess::WriteProcess(scene.fields,packet,true,true); }));CHECK(scene.fields.bytes==before);
}
