// 실제 원문 비행 제어/수명/프레임/통지의 전체 바이트와 모든 외부 호출을 전수 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFlyerFrame.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,7> kFrameIds{50,51,60,61,62,63,70}; // 두 부모/적/홈/free/dead/Form 입력이다.
constexpr std::array<std::uint32_t,5> kFrameTypes{72,73,80,138,144}; // 실제 조회/소리 stamp 번호다.
// 결과를 계산하지 않고 Native와 같은 raw 입력과 큰 효과의 변경 입력만 공급한다.
struct FlyerFrameScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 원본 고정 free-list 영역을 갖는 실제 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 실제 TYPE의 발자국 입력이다.
    std::vector<FlyerFrameType> details=std::vector<FlyerFrameType>(188); // 사거리/사운드의 독립 입력이다.
    std::vector<std::uint32_t> stamps=std::vector<std::uint32_t>(188); // 단일 타입별 재생 장부다.
    FlyerProcessFields fields;SunAviaryProcessFields sun; // Native와 같은112/48BYTE 원문 이미지다.
    bool server{true},boss{true},checking{};double now{100}; // 효과 뒤 다시 읽을 현재 전역이다.
    std::uint32_t frame{7},sunType{73},windType{138},rainType{144},sunProcessType{32},soundType{47},mutation{}; // 실제 타입/프레임 전역이다.
    std::int32_t may{1},closer{};std::string trace; // 명시 외부 반환과 관찰된 호출 순서다.
    std::unique_ptr<RawFlyerFrame> body; // 모든 현재 입력보다 먼저 해제한다.
    // 전체 입력만 설치한다. 출력 필드나 기대 분기는 계산하지 않는다.
    FlyerFrameScene(char kind,const std::vector<std::string>& input) {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==70) break;if (!sid.value || sid.value>70) throw std::logic_error("비행 Frame 검사 SID 준비 오류"); } // 관찰할 현재 슬롯까지 실제 확보한다.
        for (const auto value:kFrameIds) { // Native와 같은 전체350BYTE다.
            auto raw=pool.AllocatedBytes(Sid{value});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x50a388);Put(raw,4,0,2);Put(raw,6,0,2);Put(raw,8,0,2);raw[10]=static_cast<std::uint8_t>(value<60 ? 144 : value==61 ? 72 : 80);raw[11]=value==62 ? 1 : value==63 ? 2 : 0;Put(raw,12,value==61 ? 50U : 17U,2);
            Put(raw,14,std::bit_cast<std::uint32_t>(value<60 ? 50.25f : 60.25f));Put(raw,18,std::bit_cast<std::uint32_t>(value<60 ? 50.5f : 60.5f));raw[34]=1;raw[40]=0;Put(raw,36,0);Put(raw,42,50);Put(raw,46,50);
        }
        for (const auto value:{50U,51U}) Put(pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(value)}),6,70,2); // 실제 Parent getter가 요구하는 부착 head를 준비한다.
        auto attached=pool.AllocatedBytes(Sid{70});attached[10]=31;attached[11]=8;Put(pool.AllocatedBytes(Sid{51}),36,3);
        for (const auto type:kFrameTypes) { types[type].footX=2;types[type].footY=2;details[type]={100.0f,400+type}; } // 실제 TYPE 입력만 준비한다.
        fields.bytes.fill(0xa5);Put(fields.bytes,0,0x509f60);fields.bytes[8]=0x10;Put(fields.bytes,10,61,2);Put(fields.bytes,12,0,2);Put(fields.bytes,16,std::bit_cast<std::uint32_t>(.75f));Put(fields.bytes,20,std::bit_cast<std::uint32_t>(.5f));Time(32,99.75);Time(40,0);Put(fields.bytes,48,0x509f40);Put(fields.bytes,52,70);Put(fields.bytes,56,50);Time(64,0);Time(72,101);Put(fields.bytes,80,0);Put(fields.bytes,84,1);Put(fields.bytes,88,0);Time(96,101);Time(104,160);sun.bytes.fill(0x5a);Put(sun.bytes,24,8);
        if (kind=='I') { fields.bytes.fill(static_cast<std::uint8_t>(Number(input[2])));now=std::array<double,3>{0,100.25,-5.25}[Number(input[1])]; }
        else if (kind=='A') { Put(pool.AllocatedBytes(Sid{50}),36,Number(input[0]));Time(96,std::array<double,3>{99,100,101}[Number(input[1])]);Put(fields.bytes,12,Number(input[2]),2);Put(fields.bytes,80,Number(input[3]));stamps[144]=Number(input[4]);mutation=Number(input[5]); }
        else if (kind=='C' || kind=='N' || kind=='D') { server=Number(input[0])!=0;boss=Number(input[1])!=0;Put(fields.bytes,12,Number(input[2]),2);Put(fields.bytes,10,Number(input[3]),2);pool.AllocatedBytes(Sid{50})[10]=static_cast<std::uint8_t>(Number(input[4]));pool.AllocatedBytes(Sid{61})[11]=static_cast<std::uint8_t>(Number(input[5]));mutation=Number(input[8]); }
        else {
            server=Number(input[1])!=0;boss=Number(input[2])!=0;pool.AllocatedBytes(Sid{50})[11]=static_cast<std::uint8_t>(Number(input[3]));auto raw=pool.Slot(Sid{50});const auto extra=static_cast<std::uint8_t>(Number(input[4])); // free 입력도 이미 확보한 같은 메모리에 직접 준비한다.
            auto writable=std::span(const_cast<std::uint8_t*>(raw.data()),raw.size());writable[40]=extra;
            Put(fields.bytes,12,Number(input[5]),2);Put(fields.bytes,84,Number(input[8]));Put(fields.bytes,80,Number(input[9]));Put(fields.bytes,88,Number(input[10]));Time(32,100-std::array<double,5>{0,.025,.25,1,-.25}[Number(input[7])]);Time(40,std::array<double,3>{0,99,101}[Number(input[11])]);Time(64,std::array<double,3>{99,100,101}[Number(input[12])]);may=static_cast<std::int32_t>(Number(input[13]));closer=static_cast<std::int32_t>(Number(input[14]));Time(96,std::array<double,2>{99,101}[Number(input[15])]);mutation=Number(input[16]);
            const auto point=std::array<std::array<float,2>,5>{{{60.25f,60.5f},{50.25f,51.5f},{50.5f,51.5f},{50.75f,51.5f},{300,300}}}[Number(input[6])];
            for (const auto value:{60U,61U,62U,63U}) { auto bytes=pool.Slot(Sid{static_cast<std::uint16_t>(value)});auto target=std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size());Put(target,14,std::bit_cast<std::uint32_t>(point[0]));Put(target,18,std::bit_cast<std::uint32_t>(point[1])); } // free/dead도 입력 메모리 자체는 그대로 준비한다.
            if (Number(input[10])==2) Put(sun.bytes,24,9);if (Number(input[11])==1 || Number(input[5])==61) writable[10]=73;
            if (mutation>=13 && mutation<=15) Time(104,99+static_cast<double>(mutation-13));
            if (mutation==16) Put(pool.AllocatedBytes(Sid{61}),12,51,2);
            if (mutation>=17 && mutation<=19) { auto child=pool.AllocatedBytes(Sid{70});child[11]=mutation==17 ? 12 : mutation==18 ? 10 : 9; }
        }
        body=std::make_unique<RawFlyerFrame>(pool,types,details,stamps,Globals(),Hooks());
    }
    void Time(std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,at,static_cast<std::uint32_t>(bits));Put(fields.bytes,at+4,static_cast<std::uint32_t>(bits>>32)); } // 원문 double의 입력 비트만 쓴다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 실제 호출 순서만 기록한다.
    FlyerFrameGlobals Globals() { return {server,boss,checking,now,frame,sunType,windType,rainType,sunProcessType,soundType}; } // 같은 현재 값의 참조다.
    // Native의 명시 경계 인수/변경 입력만 공급하며 제품 제어 결과를 계산하지 않는다.
    FlyerFrameHooks Hooks() {
        FlyerFrameHooks hooks;hooks.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };hooks.robust=[this](FlyerProcessFields&) { Event("Robust");return true; };
        hooks.sidRobust=[this](Sid sid,bool live) { Event("Sid,"+std::to_string(sid.value)+','+(live ? "1" : "0"));return sid.value>0 && sid.value<65535; };hooks.checkForms=[this](Sid sid) { Event("Check,"+std::to_string(sid.value));return body->CheckForms(sid); };
        hooks.runCount=[this](Sid sid) { Event("Count,"+std::to_string(sid.value));return 16; };hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event("Destroy,"+std::to_string(sid.value)+','+std::to_string(flags)); };hooks.finish=[this](FlyerProcessFields&) { Event("Finish"); };
        hooks.launch=[this](Sid sid,Sid target,std::uint32_t flags) { Event("Launch,"+std::to_string(sid.value)+','+std::to_string(target.value)+','+std::to_string(flags));if (mutation==2) { pool.AllocatedBytes(Sid{50})[10]=138;Put(pool.AllocatedBytes(Sid{50}),36,9);Put(fields.bytes,56,51);now=200; } };
        hooks.sound=[this](float x,float y,std::uint32_t token) { Event("Sound,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(token));if (mutation==11) { Put(pool.AllocatedBytes(Sid{50}),36,9);Put(fields.bytes,56,51); } };
        hooks.removeDepend=[this](Sid form,Sid target) { Event("Remove,"+std::to_string(form.value)+','+std::to_string(target.value));if (mutation==3) { Put(fields.bytes,12,60,2);Put(fields.bytes,52,71);fields.bytes[8]=2;now=200; } };
        hooks.addDepend=[this](Sid target,Sid form) { Event("Add,"+std::to_string(target.value)+','+std::to_string(form.value)); };
        hooks.refreshForm=[this](FlyerProcessFields&) { Event("Refresh");MutateRefresh(); };hooks.refreshSun=[this](SunAviaryProcessFields&) { Event("RefreshSun");MutateRefresh(); };
        hooks.refreshTime=[this](Sid sid) { Event("Time,"+std::to_string(sid.value));if (mutation==12) { Put(fields.bytes,20,std::bit_cast<std::uint32_t>(4.0f));Put(fields.bytes,12,61,2);boss=false; } };
        hooks.hit=[this](Sid sid,Sid target) { Event("Hit,"+std::to_string(sid.value)+','+std::to_string(target.value));if (mutation==9) { Put(fields.bytes,80,7);Put(pool.AllocatedBytes(Sid{50}),36,6);now=200; } };
        hooks.offset=[this](Sid,bool) { if (mutation==8) { Put(fields.bytes,10,0,2);Put(fields.bytes,12,61,2); }return std::array<float,2>{1,0}; };hooks.scatter=[this](Sid sid,Sid parent) { Event("Scatter,"+std::to_string(sid.value)+','+std::to_string(parent.value));return std::array<float,2>{0,0}; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("Unpop,"+std::to_string(sid.value)+','+std::to_string(flags));if (mutation==1) { Put(pool.AllocatedBytes(Sid{50}),14,std::bit_cast<std::uint32_t>(71.25f));Put(pool.AllocatedBytes(Sid{50}),18,std::bit_cast<std::uint32_t>(72.5f));Put(fields.bytes,56,51);now=200; } };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("Pop,"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags));Put(pool.AllocatedBytes(sid),14,std::bit_cast<std::uint32_t>(x));Put(pool.AllocatedBytes(sid),18,std::bit_cast<std::uint32_t>(y)); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { Event("Repop,"+std::to_string(sid.value)+','+std::to_string(flags)); };hooks.movedCell=[this](Sid sid) { Event("Cell,"+std::to_string(sid.value)); };hooks.homeNotice=[this](Sid sid,std::uint32_t a,std::uint32_t b) { Event("Home,"+std::to_string(sid.value)+','+std::to_string(a)+','+std::to_string(b)); };
        hooks.mayTarget=[this](Sid sid,Sid target) { Event("May,"+std::to_string(sid.value)+','+std::to_string(target.value));if (mutation==5) { Put(fields.bytes,12,0,2);Put(fields.bytes,10,0,2); }return may!=0; };hooks.closerCount=[this](Sid target,Sid sid) { Event("Closer,"+std::to_string(target.value)+','+std::to_string(sid.value));return closer; };hooks.capacity=[this](Sid sid) { Event("Capacity,"+std::to_string(sid.value));return 3; };
        hooks.findTarget=[this](FlyerProcessFields&) { Event("Find");if (mutation==6) Put(fields.bytes,12,60,2);if (mutation==7) now=200; };hooks.removeSound=[this](Sid sid,std::uint32_t type) { Event("RemoveSound,"+std::to_string(sid.value)+','+std::to_string(type)); };
        hooks.sunProcess=[this](Sid sid,std::uint32_t type)->SunAviaryProcessFields& { Event("Sun,"+std::to_string(sid.value)+','+std::to_string(type));return sun; };hooks.returnDelay=[this](FlyerProcessFields&) { Event("Delay");return 10000000.0; };hooks.command=[this](Sid sid,std::uint32_t command) { Event("Command,"+std::to_string(sid.value)+','+std::to_string(command));if (mutation==10) Put(fields.bytes,12,0,2); };return hooks;
    }
    void MutateRefresh() { if (mutation==4) { Put(fields.bytes,12,60,2);fields.bytes[8]=0;now=200; } } // 두 갱신 경계의 동일 재진입 입력이다.
    std::string Objects() const { std::string result;for (const auto sid:kFrameIds) result+=Hex(pool.Slot(Sid{sid}));return result; } // 일곱 전체 기록을 같은 순서로 읽는다.
    std::string GlobalsImage() const { std::array<std::uint8_t,20> raw{};Put(raw,0,server ? 1U : 0U);Put(raw,4,boss ? 1U : 0U);const auto bits=std::bit_cast<std::uint64_t>(now);Put(raw,8,static_cast<std::uint32_t>(bits));Put(raw,12,static_cast<std::uint32_t>(bits>>32));Put(raw,16,frame);return Hex(raw); } // 현재20BYTE 전역이다.
    std::string Stamps() const { std::array<std::uint8_t,20> raw{};std::size_t at=0;for (const auto type:kFrameTypes) { Put(raw,at,stamps[type]);at+=4; }return Hex(raw); } // 공유 TYPE 다섯 기록이다.
};
// 각 범위를 독립 실행해 한 실패가 다른 원문 비교를 감추지 않게 한다.
void CompareFrame(char kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_FLYER_FRAME_FIXTURE);std::size_t compared{};
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 헤더 이외의 선택한 원문 입력만 처리한다.
        const auto& row=fixture.rows[index];CHECK(row.size()==9);if (row[0]!=std::string(1,kind)) continue;++compared;const auto input=Split(row[1],',');FlyerFrameScene scene(kind,input);std::string result="-";
        if (kind=='I') scene.body->Initialize(scene.fields,Number(input[0])!=0);
        else if (kind=='A') result=std::to_string(scene.body->RainFrame(scene.fields));
        else if (kind=='C') scene.body->ClearTarget(scene.fields);
        else if (kind=='N') scene.body->Notify(scene.fields,Number(input[6]),777,Number(input[7]));
        else if (kind=='D') scene.body->ParentDying(scene.fields);
        else if (Number(input[0])) scene.body->Frame(scene.fields);else scene.body->BaseFrame(scene.fields);
        const auto trace=scene.trace.empty() ? "-" : scene.trace;const auto image=Hex(scene.fields.bytes);
        if (image!=row[2] || scene.Objects()!=row[3] || scene.GlobalsImage()!=row[4] || scene.Stamps()!=row[5] || Hex(scene.sun.bytes)!=row[6] || result!=row[7] || trace!=row[8]) std::fprintf(stderr,"비행 Frame 불일치 %c/%s\n실제%s\n기대%s\n",kind,row[1].c_str(),trace.c_str(),row[8].c_str());
        CHECK(image==row[2]);CHECK(scene.Objects()==row[3]);CHECK(scene.GlobalsImage()==row[4]);CHECK(scene.Stamps()==row[5]);CHECK(Hex(scene.sun.bytes)==row[6]);CHECK(result==row[7]);CHECK(trace==row[8]);
    }
    CHECK(compared==expected);
}
}
TEST_CASE(FlyerFrame_NativeInitialize18) { CompareFrame('I',18); } // 기본/맨 오 워의 시각/padding이다.
TEST_CASE(FlyerFrame_NativeAnimation122) { CompareFrame('A',122); } // 실제frame6 발사/현재 타입 음향 억제다.
TEST_CASE(FlyerFrame_NativeClear26) { CompareFrame('C',26); } // 의존 해제/dirty/WORD/전송 순서다.
TEST_CASE(FlyerFrame_NativeNotify74) { CompareFrame('N',74); } // 미선택 사건의 자연 반환도 포함한다.
TEST_CASE(FlyerFrame_NativeParentDying30) { CompareFrame('D',30); } // 현재 홈의 타입/생존/범위 관문이다.
TEST_CASE(FlyerFrame_NativeControl474) { CompareFrame('F',474); } // 이동/착륙/공격/시간/표적 갱신 전체다.
// 선택된 누락만 거부하고 원문의 앞선 쓰기를 보존하는지 확인한다.
TEST_CASE(FlyerFrame_RequiredBoundaries) {
    FlyerFrameScene scene('A',Split("0,1,0,0,0,0",','));auto hooks=scene.Hooks();hooks.runCount={};RawFlyerFrame missing(scene.pool,scene.types,scene.details,scene.stamps,scene.Globals(),hooks);const auto before=scene.fields.bytes;
    CHECK(Throws([&] { missing.RainFrame(scene.fields); }));CHECK(scene.fields.bytes!=before && scene.trace.empty());
    scene.Time(96,101);CHECK(missing.RainFrame(scene.fields)==0);
    hooks=scene.Hooks();hooks.removeDepend={};RawFlyerFrame missingDepend(scene.pool,scene.types,scene.details,scene.stamps,scene.Globals(),hooks);Put(scene.fields.bytes,12,60,2);const auto current=scene.fields.bytes;CHECK(Throws([&] { missingDepend.ClearTarget(scene.fields); }));CHECK(scene.fields.bytes==current);
}
