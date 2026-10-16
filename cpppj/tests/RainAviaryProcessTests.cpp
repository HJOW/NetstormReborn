// 맨 오 워 풀의 네 원문 범위를 전체 이미지/객체/현재 전역/패킷/호출 순서로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawRainAviaryProcess.h"
#include "o/RawSunAviaryProcess.h" // 같은 원본004b1a90의 이미 복원한 첫 Process 조회를 공유한다.
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,5> kAviaryIds{50,51,60,70,71}; // 두 부모/플라이어/종속 Form 입력이다.
constexpr std::uint32_t kAviaryInputVtable=0x1240f000; // native 관찰의 owner/Pop 외부 가상 표 기록값이다.
// 원문과 같은 현재 입력만 준비한다. 결과는 실제 제품 몸체가 기록한다.
struct AviaryScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 원본 고정 free-list 구간을 포함한 실제 풀이다.
    RainAviaryProcessFields fields; // 현재 실제48BYTE 이미지다.
    bool fort{},friendlyFire{},server{true},checking{};std::uint32_t editor{},processType{60},flyerType{144};double now{100}; // 서로 구별한 현재 전역이다.
    char kind{};std::uint32_t mutation{},frameCalls{};bool allocated{true}; // 명시 외부 입력과 호출 횟수다.
    std::string trace;std::optional<RainAviaryProcessFields> attached; // 선택된 실제 호출과 부착 경계가 받은 이미지다.
    std::unique_ptr<RawRainAviaryProcess> body; // 위 입력보다 먼저 해제할 제품 처리기다.
    // 레코드 위치/타입/미정 바이트는 관찰 입력과 동일하게 설치한다.
    AviaryScene(char scope,const std::vector<std::string>& input):kind(scope) {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==71) break;if (!sid.value || sid.value>71) throw std::logic_error("아비어리 검사 SID 준비 오류"); } // 대상71까지 실제 슬롯을 확보한다.
        for (const auto value:kAviaryIds) { // 원문과 같은 다섯50BYTE 입력을 준비한다.
            const Sid sid{value};auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,kAviaryInputVtable);Put(raw,4,value==70 ? 71U : 0U,2);Put(raw,6,value==50 || value==51 ? 70U : 0U,2);Put(raw,8,127,2);
            raw[10]=static_cast<std::uint8_t>(value==50 || value==51 ? 143 : value==60 ? 144 : value==70 ? 60 : 31);raw[11]=0;Put(raw,12,value==50 ? 150U : value==51 ? 151U : value==60 ? 9U : 0U,2);
            Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(value)+.25F));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(value)+.5F));raw[34]=value==50 ? 1 : value==51 ? 2 : 0;Put(raw,36,value==51 ? 7U : value==60 ? 42U : 0U);raw[40]=0;
        }
        fields.bytes.fill(0xa5);Put(fields.bytes,0,0x509e20);fields.bytes[8]=0x10;Time(100);Put(fields.bytes,24,1);Put(fields.bytes,32,0x509e00);Put(fields.bytes,36,77);Put(fields.bytes,40,50);Put(fields.bytes,44,0);
        if (kind=='C') { server=Number(input[0])!=0;now=std::array<double,3>{0,100.25,-5.25}[Number(input[1])];fields.bytes.fill(static_cast<std::uint8_t>(Number(input[2]))); }
        else if (kind=='P') {
            server=Number(input[0])!=0;fort=Number(input[1])!=0;friendlyFire=Number(input[2])!=0;allocated=Number(input[4])!=0;mutation=Number(input[6]);const auto layout=Number(input[5]);
            if (!layout) Put(pool.AllocatedBytes(Sid{50}),6,0,2);
            if (layout==2) { pool.AllocatedBytes(Sid{70})[11]=2;pool.AllocatedBytes(Sid{71})[10]=60; }
            if (layout==3) pool.AllocatedBytes(Sid{70})[11]=1;
        } else if (kind=='F') {
            server=Number(input[0])!=0;editor=Number(input[1]);pool.AllocatedBytes(Sid{50})[40]=static_cast<std::uint8_t>(Number(input[2]));Put(fields.bytes,24,Number(input[3]));Put(pool.AllocatedBytes(Sid{50}),36,std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::stol(input[4]))));Put(fields.bytes,44,Number(input[5]));Time(std::array<double,3>{99,100,101}[Number(input[6])]);mutation=Number(input[7]);
        } else if (kind=='W') { fields.bytes[8]=static_cast<std::uint8_t>(Number(input[0]));Time(123.25);Put(fields.bytes,24,7);Put(fields.bytes,44,5); }
        body=std::make_unique<RawRainAviaryProcess>(pool,Globals(),Hooks());
    }
    RainAviaryProcessGlobals Globals() { return {fort,friendlyFire,server,editor,checking,now,processType,flyerType}; } // 같은 현재 전역 주소를 빌린다.
    void Time(double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,16,static_cast<std::uint32_t>(bits));Put(fields.bytes,20,static_cast<std::uint32_t>(bits>>32)); } // raw double을 조립한다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 실제 선택된 호출 순서다.
    // 반환값/변경 입력만 공급하고 제품의 다음 상태나 최종 패킷을 계산하지 않는다.
    RainAviaryProcessHooks Hooks() {
        RainAviaryProcessHooks hooks;
        hooks.findProcess=[this](Sid sid,std::uint32_t type) { Event("Find,"+std::to_string(sid.value)+','+std::to_string(type));if (kind=='P' && mutation==1) server=false;if (kind=='P' && mutation==2) processType=59;return FindAttachedProcess(pool,sid,type); };
        hooks.allocate=[this](std::size_t size)->std::optional<RainAviaryProcessFields> { Event("Allocate,"+std::to_string(size));if (!allocated) return std::nullopt;RainAviaryProcessFields value;value.bytes.fill(0xa5);return value; };
        hooks.attach=[this](std::optional<RainAviaryProcessFields> value,Sid sid,std::uint32_t type,std::uint32_t flags) { Event("Attach,"+std::to_string(type)+','+std::to_string(sid.value)+','+std::to_string(flags)+','+(value ? "1" : "0"));attached=std::move(value); };
        hooks.damageable=[this](Sid,std::uint32_t flags) { Event("Base,"+std::to_string(flags)); };hooks.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };
        hooks.setFrame=[this](Sid sid,std::int32_t frame) { Event("Frame,"+std::to_string(sid.value)+','+std::to_string(frame));Put(pool.AllocatedBytes(sid),36,std::bit_cast<std::uint32_t>(frame));++frameCalls;if (frameCalls==1 && mutation==1) { Put(fields.bytes,40,51);now=200; }if (frameCalls==1 && mutation==5) Put(fields.bytes,24,9); };
        hooks.sound=[this](float x,float y,std::string_view name) { CHECK(name=="manOWarBuild.wav");Event("Sound,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y)));if (mutation==2) { Put(fields.bytes,24,9);Put(fields.bytes,44,10); } };
        hooks.checkFrame=[this](Sid,std::int32_t frame,std::uint32_t line) { Event("Check,"+std::to_string(frame)+','+std::to_string(line)); };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { Event("Create,"+std::to_string(type)+','+std::to_string(flags));return Sid{60}; };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) { Event("Owner,"+std::to_string(sid.value)+','+std::to_string(owner));pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(owner);if (mutation==3) { Put(fields.bytes,40,51);now=200; } };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("Pop,"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags));if (mutation==4) { Put(fields.bytes,40,51);now=200; } };
        hooks.bindFlyer=[this](Sid flyer,Sid sid) { Event("Bind,"+std::to_string(flyer.value)+','+std::to_string(sid.value)); };return hooks;
    }
    std::string Objects() const { std::string value;for (const auto sid:kAviaryIds) value+=Hex(pool.Slot(Sid{sid}));return value; } // 다섯 전체 기록을 원문 순서대로 읽는다.
    std::string CurrentGlobals() const { std::array<std::uint8_t,28> bytes{};Put(bytes,0,server ? 1U : 0U);Put(bytes,4,editor);const auto bits=std::bit_cast<std::uint64_t>(now);Put(bytes,8,static_cast<std::uint32_t>(bits));Put(bytes,12,static_cast<std::uint32_t>(bits>>32));Put(bytes,16,processType);Put(bytes,20,fort ? 1U : 0U);Put(bytes,24,friendlyFire ? 1U : 0U);return Hex(bytes); } // 현재 전역28BYTE다.
};
// 범위를 나누어 한 실패가 다른 원문 검사 행을 감추지 않게 한다.
void CompareAviaryScope(char kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_RAIN_AVIARY_FIXTURE);std::size_t compared{};
    for (std::size_t index=1;index<fixture.rows.size();++index) { // TSV 헤더 이외의 해당 원문 입력만 독립적으로 재생한다.
        const auto& row=fixture.rows[index];CHECK(row.size()==8);if (row[0]!=std::string(1,kind)) continue;++compared;const auto input=Split(row[1],',');AviaryScene scene(kind,input);std::string image,objects="-",packet="-",result="-";
        if (kind=='C') scene.body->Initialize(scene.fields);
        else if (kind=='P') scene.body->PostPop(Sid{50},Number(input[3]));
        else if (kind=='F') scene.body->Frame(scene.fields);
        else { std::array<std::uint8_t,64> bytes;bytes.fill(0xa7);result=std::to_string(RawRainAviaryProcess::WriteProcess(scene.fields,bytes,Number(input[1])!=0,Number(input[2])!=0));packet=Hex(bytes); }
        image=kind=='P' ? (scene.attached ? Hex(scene.attached->bytes) : "-") : Hex(scene.fields.bytes);if (kind=='P' || kind=='F') objects=scene.Objects();const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (image!=row[2] || objects!=row[3] || scene.CurrentGlobals()!=row[4] || packet!=row[5] || result!=row[6] || trace!=row[7]) std::fprintf(stderr,"아비어리 불일치 %c/%s\n실제%s\n기대%s\n",kind,row[1].c_str(),trace.c_str(),row[7].c_str());
        CHECK(image==row[2]);CHECK(objects==row[3]);CHECK(scene.CurrentGlobals()==row[4]);CHECK(packet==row[5]);CHECK(result==row[6]);CHECK(trace==row[7]);
    }
    CHECK(compared==expected);
}
}
TEST_CASE(RainAviary_NativeConstructor18) { CompareAviaryScope('C',18); } // 미정 바이트를 포함한 생성자의 모든 쓰기다.
TEST_CASE(RainAviary_NativePostPop386) { CompareAviaryScope('P',386); } // live/dead/free 후보와 개별 할당을 대조한다.
TEST_CASE(RainAviary_NativeFrame174) { CompareAviaryScope('F',174); } // 현재 효과 이후 읽기와 실제 AP 상태 경계다.
TEST_CASE(RainAviary_NativeWriter1024) { CompareAviaryScope('W',1024); } // 전체 dirty/부모/force와 미사용 버퍼를 대조한다.
// 미선택 하위는 요구하지 않고 선택된 누락과 개발자 진단의 부분 쓰기를 보존한다.
TEST_CASE(RainAviary_RequiredBoundaries) {
    AviaryScene scene('P',Split("1,0,0,1,1,0,0",','));auto hooks=scene.Hooks();hooks.allocate={};RawRainAviaryProcess missing(scene.pool,scene.Globals(),hooks);CHECK(Throws([&] { missing.PostPop(Sid{50},1); }));CHECK(!scene.attached && scene.trace=="Find,50,60");
    scene.server=false;scene.trace.clear();missing.PostPop(Sid{50},1);CHECK(scene.trace=="Base,1");
    AviaryScene frame('F',Split("1,0,0,2,11,0,1,0",','));frame.checking=true;hooks=frame.Hooks();hooks.checkFrame={};RawRainAviaryProcess diagnostic(frame.pool,frame.Globals(),hooks);const auto before=frame.fields.bytes;CHECK(Throws([&] { diagnostic.Frame(frame.fields); }));CHECK(frame.fields.bytes==before && frame.trace.empty());
    std::array<std::uint8_t,8> shortPacket{};CHECK(Throws([&] { RawRainAviaryProcess::WriteProcess(frame.fields,shortPacket,true); }));CHECK(frame.fields.bytes==before);
}
