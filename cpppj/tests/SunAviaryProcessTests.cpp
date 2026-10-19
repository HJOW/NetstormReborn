// 훨리베이스의 네 원문 범위를 전체 이미지/객체/현재 전역/패킷/호출 순서로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSunAviaryProcess.h"
#include <cstdio>
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,5> kAviaryIds{50,51,60,70,71}; // 두 부모/플라이어/종속 Form 입력이다.
constexpr std::uint32_t kAviaryInputVtable=0x1240f000; // native 관찰의 owner/Pop 외부 가상 표 기록값이다.
// 원문과 같은 현재 입력만 준비한다. 결과는 실제 제품 몸체가 기록한다.
struct AviaryScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 원본 고정 free-list 구간을 포함한 실제 풀이다.
    SunAviaryProcessFields fields; // 현재 실제48BYTE 이미지다.
    bool server{true},checking{};std::uint32_t editor{},processType{32},flyerType{73};double now{100}; // 서로 구별한 현재 전역이다.
    char kind{};std::uint32_t mutation{},frameCalls{};bool allocated{true}; // 명시 외부 입력과 호출 횟수다.
    std::string trace;std::optional<SunAviaryProcessFields> attached; // 선택된 실제 호출과 부착 경계가 받은 이미지다.
    std::unique_ptr<RawSunAviaryProcess> body; // 위 입력보다 먼저 해제할 제품 처리기다.
    // 레코드 위치/타입/미정 바이트는 관찰 입력과 동일하게 설치한다.
    AviaryScene(char scope,const std::vector<std::string>& input):kind(scope) {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==71) break;if (!sid.value || sid.value>71) throw std::logic_error("아비어리 검사 SID 준비 오류"); } // 대상71까지 실제 슬롯을 확보한다.
        for (const auto value:kAviaryIds) { // 원문과 같은 다섯50BYTE 입력을 준비한다.
            const Sid sid{value};auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,kAviaryInputVtable);Put(raw,4,value==70 ? 71U : 0U,2);Put(raw,6,value==50 || value==51 ? 70U : 0U,2);Put(raw,8,127,2);
            raw[10]=static_cast<std::uint8_t>(value==50 || value==51 ? 72 : value==60 ? 73 : value==70 ? 32 : 31);raw[11]=0;Put(raw,12,value==50 ? 150U : value==51 ? 151U : value==60 ? 9U : 0U,2);
            Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(value)+.25F));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(value)+.5F));raw[34]=value==50 ? 1 : value==51 ? 2 : 0;Put(raw,36,value==51 ? 7U : value==60 ? 42U : 0U);raw[40]=0;
        }
        fields.bytes.fill(0xa5);Put(fields.bytes,0,0x509dd8);fields.bytes[8]=0x10;Time(100);Put(fields.bytes,24,1);Put(fields.bytes,32,0x509db8);Put(fields.bytes,36,77);Put(fields.bytes,40,50);Put(fields.bytes,44,0);
        if (kind=='C') { server=Number(input[0])!=0;now=std::array<double,3>{0,100.25,-5.25}[Number(input[1])];fields.bytes.fill(static_cast<std::uint8_t>(Number(input[2]))); }
        else if (kind=='P') {
            server=Number(input[0])!=0;allocated=Number(input[2])!=0;mutation=Number(input[4]);const auto layout=Number(input[3]);
            if (!layout) Put(pool.AllocatedBytes(Sid{50}),6,0,2);
            if (layout==2) { pool.AllocatedBytes(Sid{70})[11]=2;pool.AllocatedBytes(Sid{71})[10]=32; }
            if (layout==3) pool.AllocatedBytes(Sid{70})[11]=1;
        } else if (kind=='F') {
            server=Number(input[0])!=0;editor=Number(input[1]);pool.AllocatedBytes(Sid{50})[40]=static_cast<std::uint8_t>(Number(input[2]));Put(fields.bytes,24,Number(input[3]));Put(pool.AllocatedBytes(Sid{50}),36,std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::stol(input[4]))));Put(fields.bytes,44,Number(input[5]));Time(std::array<double,3>{99,100,101}[Number(input[6])]);mutation=Number(input[7]);
        } else if (kind=='W') { fields.bytes[8]=static_cast<std::uint8_t>(Number(input[0]));Time(123.25);Put(fields.bytes,24,7);Put(fields.bytes,44,5); }
        body=std::make_unique<RawSunAviaryProcess>(pool,Globals(),Hooks());
    }
    SunAviaryProcessGlobals Globals() { return {server,editor,checking,now,processType,flyerType}; } // 같은 현재 전역 주소를 빌린다.
    void Time(double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,16,static_cast<std::uint32_t>(bits));Put(fields.bytes,20,static_cast<std::uint32_t>(bits>>32)); } // raw double을 조립한다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 실제 선택된 호출 순서다.
    // 반환값/변경 입력만 공급하고 제품의 다음 상태나 최종 패킷을 계산하지 않는다.
    SunAviaryProcessHooks Hooks() {
        SunAviaryProcessHooks hooks;
        hooks.findProcess=[this](Sid sid,std::uint32_t type) { Event("Find,"+std::to_string(sid.value)+','+std::to_string(type));if (kind=='P' && mutation==1) server=false;if (kind=='P' && mutation==2) processType=59;return FindAttachedProcess(pool,sid,type); };
        hooks.allocate=[this](std::size_t size)->std::optional<SunAviaryProcessFields> { Event("Allocate,"+std::to_string(size));if (!allocated) return std::nullopt;SunAviaryProcessFields value;value.bytes.fill(0xa5);return value; };
        hooks.attach=[this](std::optional<SunAviaryProcessFields> value,Sid sid,std::uint32_t type,std::uint32_t flags) { Event("Attach,"+std::to_string(type)+','+std::to_string(sid.value)+','+std::to_string(flags)+','+(value ? "1" : "0"));attached=std::move(value); };
        hooks.damageable=[this](Sid,std::uint32_t flags) { Event("Base,"+std::to_string(flags)); };hooks.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };
        hooks.setFrame=[this](Sid sid,std::int32_t frame) { Event("Frame,"+std::to_string(sid.value)+','+std::to_string(frame));Put(pool.AllocatedBytes(sid),36,std::bit_cast<std::uint32_t>(frame));++frameCalls;if (frameCalls==1 && mutation==1) { Put(fields.bytes,40,51);now=200; }if (frameCalls==1 && mutation==5) Put(fields.bytes,24,9); };
        hooks.sound=[this](float x,float y,std::string_view name) { CHECK(name=="apStart.wav");Event("Sound,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y)));if (mutation==2) { Put(fields.bytes,24,9);Put(fields.bytes,44,10); } };
        hooks.checkFrame=[this](Sid,std::int32_t frame,std::uint32_t line) { Event("Check,"+std::to_string(frame)+','+std::to_string(line)); };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { Event("Create,"+std::to_string(type)+','+std::to_string(flags));return Sid{60}; };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) { Event("Owner,"+std::to_string(sid.value)+','+std::to_string(owner));pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(owner);if (mutation==3) { Put(fields.bytes,40,51);now=200; } };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("Pop,"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags));if (mutation==4) { Put(fields.bytes,40,51);now=200; } };
        hooks.bindFlyer=[this](Sid flyer,Sid sid) { Event("Bind,"+std::to_string(flyer.value)+','+std::to_string(sid.value)); };hooks.refreshForm=[this](SunAviaryProcessFields&) { Event("Refresh");if (mutation==6) fields.bytes[8]=0; };return hooks;
    }
    std::string Objects() const { std::string value;for (const auto sid:kAviaryIds) value+=Hex(pool.Slot(Sid{sid}));return value; } // 다섯 전체 기록을 원문 순서대로 읽는다.
    std::string CurrentGlobals() const { std::array<std::uint8_t,20> bytes{};Put(bytes,0,server ? 1U : 0U);Put(bytes,4,editor);const auto bits=std::bit_cast<std::uint64_t>(now);Put(bytes,8,static_cast<std::uint32_t>(bits));Put(bytes,12,static_cast<std::uint32_t>(bits>>32));Put(bytes,16,processType);return Hex(bytes); } // 현재 전역20BYTE다.
};
// 범위를 나누어 한 실패가 다른 원문 검사 행을 감추지 않게 한다.
void CompareAviaryScope(char kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_SUN_AVIARY_FIXTURE);std::size_t compared{};
    for (std::size_t index=1;index<fixture.rows.size();++index) { // TSV 헤더 이외의 해당 원문 입력만 독립적으로 재생한다.
        const auto& row=fixture.rows[index];CHECK(row.size()==8);if (row[0]!=std::string(1,kind)) continue;++compared;const auto input=Split(row[1],',');AviaryScene scene(kind,input);std::string image,objects="-",packet="-",result="-";
        if (kind=='C') scene.body->Initialize(scene.fields);
        else if (kind=='P') scene.body->PostPop(Sid{50},Number(input[1]));
        else if (kind=='F') scene.body->Frame(scene.fields);
        else { std::array<std::uint8_t,64> bytes;bytes.fill(0xa7);result=std::to_string(RawSunAviaryProcess::WriteProcess(scene.fields,bytes,Number(input[1])!=0,Number(input[2])!=0));packet=Hex(bytes); }
        image=kind=='P' ? (scene.attached ? Hex(scene.attached->bytes) : "-") : Hex(scene.fields.bytes);if (kind=='P' || kind=='F') objects=scene.Objects();const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (image!=row[2] || objects!=row[3] || scene.CurrentGlobals()!=row[4] || packet!=row[5] || result!=row[6] || trace!=row[7]) std::fprintf(stderr,"아비어리 불일치 %c/%s\n실제%s\n기대%s\n",kind,row[1].c_str(),trace.c_str(),row[7].c_str());
        CHECK(image==row[2]);CHECK(objects==row[3]);CHECK(scene.CurrentGlobals()==row[4]);CHECK(packet==row[5]);CHECK(result==row[6]);CHECK(trace==row[7]);
    }
    CHECK(compared==expected);
}
}
TEST_CASE(SunAviary_NativeConstructor18) { CompareAviaryScope('C',18); } // 미정 바이트를 포함한 생성자의 모든 쓰기다.
TEST_CASE(SunAviary_NativePostPop98) { CompareAviaryScope('P',98); } // live/dead/free 후보와 개별 할당을 대조한다.
TEST_CASE(SunAviary_NativeFrame594) { CompareAviaryScope('F',594); } // 현재 효과 이후 읽기와 실제 AP 상태 경계다.
TEST_CASE(SunAviary_NativeWriter1024) { CompareAviaryScope('W',1024); } // 전체 dirty/부모/force와 미사용 버퍼를 대조한다.
// 미선택 하위는 요구하지 않고 선택된 누락과 개발자 진단의 부분 쓰기를 보존한다.
TEST_CASE(SunAviary_RequiredBoundaries) {
    AviaryScene scene('P',Split("1,1,1,0,0",','));auto hooks=scene.Hooks();hooks.allocate={};RawSunAviaryProcess missing(scene.pool,scene.Globals(),hooks);CHECK(Throws([&] { missing.PostPop(Sid{50},1); }));CHECK(!scene.attached && scene.trace=="Find,50,32");
    scene.server=false;scene.trace.clear();missing.PostPop(Sid{50},1);CHECK(scene.trace=="Base,1");
    AviaryScene frame('F',Split("1,0,0,2,11,0,1,0",','));frame.checking=true;hooks=frame.Hooks();hooks.checkFrame={};RawSunAviaryProcess diagnostic(frame.pool,frame.Globals(),hooks);const auto before=frame.fields.bytes;CHECK(Throws([&] { diagnostic.Frame(frame.fields); }));CHECK(frame.fields.bytes==before && frame.trace.empty());
    std::array<std::uint8_t,8> shortPacket{};CHECK(Throws([&] { RawSunAviaryProcess::WriteProcess(frame.fields,shortPacket,true); }));CHECK(frame.fields.bytes==before);
}

// 새 독립128입력의 전체 raw/프로세스/효과 순서를 비교하며 기존1734행 원문은 유지한다.
TEST_CASE(SunAviary_NativeHomeNotice128) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/sun-aviary-notice-x86.tsv").string();std::size_t compared{}; // 별도 동결한 실제45b4a0 기대값이다.
    // 서버 관문과 효과 뒤 현재 값 재조회를 각 입력에서 독립 구성한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        const auto input=Split(row[0],',');AviaryScene scene('N',input);scene.server=Number(input[0])!=0;scene.mutation=Number(input[3]);
        auto raw=scene.pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x12200000);raw[10]=72;raw[11]=0;Put(raw,36,7);
        scene.fields.bytes.fill(0xa5);scene.fields.bytes[8]=static_cast<std::uint8_t>(Number(input[1]));scene.Time(100.25);Put(scene.fields.bytes,24,Number(input[2]));Put(scene.fields.bytes,32,0x509db8);Put(scene.fields.bytes,36,70);Put(scene.fields.bytes,40,50);
        SunAviaryProcessHooks hooks; // 큰 효과는 원문 관측과 같은 명시 입력이며 최종 상태를 계산하지 않는다.
        hooks.findProcess=[&](Sid sid,std::uint32_t type) { CHECK(sid.value==50 && type==32);scene.Event("Find:50:32");if (scene.mutation==3) { scene.server=false;scene.processType=99; }return Sid{70}; };
        hooks.processFields=[&](Sid sid) { CHECK(sid.value==70);scene.Event("Resolve:70");return &scene.fields; };
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { CHECK(sid.value==50 && flags==0);scene.Event("Unpop:50:0");raw[11]=4;if (scene.mutation==1) { Put(raw,36,99);scene.fields.bytes[8]=0x10;Put(scene.fields.bytes,24,5);Put(scene.fields.bytes,40,51); } };
        hooks.repop=[&](Sid sid,std::uint32_t flags) { CHECK(sid.value==50 && flags==0);scene.Event("Repop:50:0:"+std::to_string(Get(raw,36)));raw[11]=0;if (scene.mutation==2) { Put(raw,36,77);scene.fields.bytes[8]=0x40;Put(scene.fields.bytes,24,9); } };
        hooks.refreshForm=[&](SunAviaryProcessFields& fields) { CHECK(&fields==&scene.fields);scene.Event("Refresh"); };
        RawSunAviaryProcess body(scene.pool,scene.Globals(),std::move(hooks));body.FlyerDied(Sid{50});
        CHECK(row.size()==9 && row[1]==row[5] && row[2]==row[6] && row[3]==row[7] && row[4]==row[8]);
        CHECK((scene.trace.empty() ? "-" : scene.trace)==row[1]);CHECK(Hex(raw)==row[2]);CHECK(Hex(scene.fields.bytes)==row[3]);CHECK(std::to_string(scene.server ? 1 : 0)+','+std::to_string(scene.processType)==row[4]);++compared;
    }
    CHECK(compared==128);
}

// 원본 NULL 역참조를 성공으로 숨기지 않으며 그 앞의 공간/프레임 효과 순서를 유지한다.
TEST_CASE(SunAviary_HomeNoticeNullAfterSpatialEffects) {
    AviaryScene scene('N',{});std::uint32_t effects{};SunAviaryProcessHooks hooks; // NULL Host 조회만 명시 실패 입력으로 공급한다.
    hooks.findProcess=[](Sid,std::uint32_t) { return Sid{}; };
    hooks.unpop=[&](Sid,std::uint32_t) { ++effects; };
    hooks.repop=[&](Sid sid,std::uint32_t) { CHECK(Get(scene.pool.Slot(sid),36)==27);++effects; };
    RawSunAviaryProcess body(scene.pool,scene.Globals(),hooks);CHECK(Throws([&] { body.FlyerDied(Sid{50}); }));CHECK(effects==2 && Get(scene.pool.Slot(Sid{50}),36)==27);
    scene.server=false;effects=0;RawSunAviaryProcess client(scene.pool,scene.Globals(),{});client.FlyerDied(Sid{50});CHECK(effects==0);
}
