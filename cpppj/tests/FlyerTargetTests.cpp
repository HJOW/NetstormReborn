// 원문 비행 검색/의존 경쟁/포위의 모든 raw 바이트·반환·효과를 독립 fixture와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFlyerTarget.h"
#include "o/RawFlyerFrame.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,22> kTargetIds{50,51,52,53,60,61,62,63,70,71,72,73,80,81,82,83,84,90,91,92,93,94}; // 원문과 같은 전체22객체 순서다.
constexpr std::array<std::uint32_t,11> kTargetTypes{1,6,9,31,63,73,80,81,82,83,144}; // 실제 입력 TYPE 번호다.
// 원문과 같은 입력만 설치하며 선택 결과와 기대 좌표를 미리 계산하지 않는다.
struct FlyerTargetScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 모든 Form/부모와 같은 실제 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 TYPE의 발자국/genus 입력이다.
    std::vector<float> ranges=std::vector<float>(188); // 실제 TYPE+44 입력이다.
    std::vector<FlyerFrameType> frameDetails=std::vector<FlyerFrameType>(188);std::vector<std::uint32_t> stamps=std::vector<std::uint32_t>(188); // checkForms만 실행하는 기존 실제 본체의 자료다.
    FlyerProcessFields fields; // 원문112BYTE의 같은 이미지다.
    bool server{true},checking{},boss{true};double now{100}; // 효과 뒤 현재 값을 다시 읽는 전역이다.
    std::uint32_t sun{73},end{63},depend{1},wind{138},rain{144},sunProcess{32},soundProcess{47},frame{}; // 실제 타입/장기 전역 입력이다.
    std::uint32_t mutation{},may{15};std::size_t index{};std::vector<Sid> candidates{{60},{61},{62},{63}}; // 명시 외부 검색/필터/변경 입력이다.
    std::string trace;std::unique_ptr<RawFlyerFrame> checks;std::unique_ptr<RawFlyerTarget> body; // 실제 두 본체와 선택된 외부 호출 순서다.
    // 결과를 대신 계산하지 않고 원문에 공급한 모든 바이트를 독립 구성한다.
    FlyerTargetScene(char kind,const std::vector<std::string>& v) {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==94) break;if (!sid.value || sid.value>94) throw std::logic_error("비행 표적 검사 SID 확보 오류"); } // 관찰 번호를 실제 풀에서 확보한다.
        for (const auto type:kTargetTypes) { types[type].footX=2;types[type].footY=2;types[type].flags2=(type==73 || type==144) ? 0x100000U : 0U;ranges[type]=30; } // 결과와 독립된 실제 TYPE 입력이다.
        for (const auto sid:kTargetIds) { // 전체50BYTE를 원문과 같은 패턴으로 준비한다.
            auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x50a388);Put(raw,4,0,2);Put(raw,6,0,2);Put(raw,8,0,2);raw[10]=144;raw[11]=0;Put(raw,14,std::bit_cast<std::uint32_t>(50.25f));Put(raw,18,std::bit_cast<std::uint32_t>(50.5f));raw[34]=1;raw[40]=0;Put(raw,36,0);Put(raw,42,0);Put(raw,46,0);
        }
        for (std::uint16_t i=0;i<4;++i) { // 각 부모/후보/프로세스 Form의 독립 명시 연결이다.
            auto raw=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(50+i)});Put(raw,6,70+i,2);Put(raw,14,std::bit_cast<std::uint32_t>(std::array<float,4>{50.25f,55.25f,70.25f,58.25f}[i]));raw[10]=i==3 ? 80 : 144;
            raw=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(60+i)});raw[10]=static_cast<std::uint8_t>(80+i);Put(raw,14,std::bit_cast<std::uint32_t>(std::array<float,4>{60.25f,55.25f,52.25f,80.25f}[i]));Put(raw,18,std::bit_cast<std::uint32_t>(50.5f));
            raw=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(70+i)});Put(raw,0,0x512098);raw[10]=31;raw[11]=8;Put(raw,16,50+i,2);
        }
        for (std::uint16_t i=0;i<5;++i) { // 감시 쌍의 반대 Form 부모는 실제 process Form이다.
            auto raw=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(80+i)});Put(raw,0,0x512110);raw[10]=i==4 ? 6 : 1;raw[11]=8;Put(raw,18,90+i,2);
            raw=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(90+i)});Put(raw,0,0x512110);raw[10]=1;raw[11]=8;Put(raw,16,i<4 ? 70+i : 0,2);
        }
        fields.bytes.fill(0xa5);Put(fields.bytes,0,0x509f60);fields.bytes[8]=0x10;Put(fields.bytes,10,61,2);Put(fields.bytes,12,0,2);Time(40,0);Put(fields.bytes,48,0x509f40);Put(fields.bytes,52,70);Put(fields.bytes,56,50);
        std::vector<std::uint16_t> chain{84,82,80,81,83}; // 순회 순서와 숫자 순서가 다른 의존 입력이다.
        if (kind=='F') {
            const auto scene=Number(v[0]),origin=Number(v[1]),parentType=Number(v[3]);server=Number(v[8])!=0;checking=Number(v[9])!=0;may=Number(v[5]);mutation=Number(v[11]);pool.AllocatedBytes(Sid{50})[10]=static_cast<std::uint8_t>(parentType);ranges[parentType]=std::bit_cast<float>(Number(v[2]));Put(fields.bytes,10,Number(v[4]),2);Time(40,std::array<double,3>{0,99,101}[Number(v[10])]);types[80].flags2=Number(v[6]);types[80].footX=types[80].footY=static_cast<std::int32_t>(Number(v[7]));
            const auto point=std::array<std::array<std::int32_t,2>,5>{{{0,0},{48,49},{0,49},{-2,-3},{17000001,17000003}}}[origin];Put(pool.AllocatedBytes(Sid{50}),42,std::bit_cast<std::uint32_t>(point[0]));Put(pool.AllocatedBytes(Sid{50}),46,std::bit_cast<std::uint32_t>(point[1]));
            if (scene==1) std::reverse(candidates.begin(),candidates.end());if (scene==2) candidates.clear();if (scene==3) candidates={{60},{60},{62}};
            if (scene==4) Put(pool.AllocatedBytes(Sid{61}),14,std::bit_cast<std::uint32_t>(60.25f));if (scene==5) { candidates={{60}};Put(pool.AllocatedBytes(Sid{60}),14,std::bit_cast<std::uint32_t>(79.25f)); }if (scene==6) candidates={{61}};if (scene==7) candidates={{63}};if (scene==8) candidates={{62},{60},{61}};
        } else {
            types[80].footX=static_cast<std::int32_t>(Number(v[2]));types[80].footY=static_cast<std::int32_t>(Number(v[3]));checking=Number(v[7])!=0;mutation=Number(v[8]);const auto scene=Number(v[6]);
            if (scene==1) std::reverse(chain.begin(),chain.end());if (scene==2) chain.clear();if (scene==3) std::erase(chain,std::uint16_t{80});if (scene==4) chain={80};
            if (scene==5) Put(pool.AllocatedBytes(Sid{80}),18,0,2);if (scene==6) pool.AllocatedBytes(Sid{71})[10]=9;if (scene==7) pool.AllocatedBytes(Sid{71})[10]=63;if (scene==8) pool.AllocatedBytes(Sid{51})[10]=80;if (scene==9) pool.AllocatedBytes(Sid{51})[11]=2;
            if (scene==10) Put(pool.AllocatedBytes(Sid{51}),14,std::bit_cast<std::uint32_t>(50.25f));if (scene==11) Put(pool.AllocatedBytes(Sid{51}),14,std::bit_cast<std::uint32_t>(50.250003814697266f));if (scene==12) end=31;if (scene==13) Put(pool.AllocatedBytes(Sid{51}),6,0,2);if (scene==14) { pool.AllocatedBytes(Sid{50})[10]=73;pool.AllocatedBytes(Sid{52})[10]=73; }
        }
        Put(pool.AllocatedBytes(Sid{60}),6,chain.empty() ? 0 : chain.front(),2);
        for (std::size_t i=0;i<chain.size();++i) Put(pool.AllocatedBytes(Sid{chain[i]}),4,i+1<chain.size() ? chain[i+1] : 0,2); // 원문과 같은 현재 next 입력이다.
        if (kind!='F') { pool.AllocatedBytes(Sid{60})[11]=static_cast<std::uint8_t>(Number(v[4]));pool.AllocatedBytes(Sid{50})[11]=static_cast<std::uint8_t>(Number(v[5])); } // free/dead 입력은 전체 링크 준비 뒤 마지막에 설치한다.
        checks=std::make_unique<RawFlyerFrame>(pool,types,frameDetails,stamps,FlyerFrameGlobals{server,boss,checking,now,frame,sun,wind,rain,sunProcess,soundProcess},FlyerFrameHooks{});
        body=std::make_unique<RawFlyerTarget>(pool,types,ranges,Globals(),Hooks());
    }
    void Time(std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,at,static_cast<std::uint32_t>(bits));Put(fields.bytes,at+4,static_cast<std::uint32_t>(bits>>32)); } // 원문 double 입력의 비트만 쓴다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 선택된 실제 효과 순서만 기록한다.
    FlyerTargetGlobals Globals() { return {server,checking,now,sun,end,depend}; } // 같은 현재 전역 참조다.
    // 원문 경계에 공급한 현재 입력 변경만 수행한다.
    void Mutate(std::string_view where) {
        if (where=="Offset" && mutation==1) { Put(fields.bytes,56,51);Put(pool.AllocatedBytes(Sid{50}),14,std::bit_cast<std::uint32_t>(71.25f));Put(pool.AllocatedBytes(Sid{50}),18,std::bit_cast<std::uint32_t>(72.5f)); }
        if (where=="May" && mutation==2) { Put(fields.bytes,10,0,2);Put(fields.bytes,56,51);pool.AllocatedBytes(Sid{50})[10]=73;types[80].flags2=0x120000; }
        if (where=="Begin" && mutation==3) sun=144;
        if (where=="Next" && mutation==4) { now=200;Put(pool.AllocatedBytes(Sid{62}),14,std::bit_cast<std::uint32_t>(49.25f)); }
        if (where=="Process" && mutation==5) Put(pool.AllocatedBytes(Sid{51}),14,std::bit_cast<std::uint32_t>(49.75f));
        if (where=="Watch" && mutation==6) { Put(pool.AllocatedBytes(Sid{82}),4,0,2);Put(pool.AllocatedBytes(Sid{71}),16,52,2); }
        if (where=="Command" && mutation==7) { Put(fields.bytes,12,0,2);Put(fields.bytes,56,51);now=200; }
        if (where=="Delay" && mutation==8) { now=200;Time(40,777); }
        if (where=="Check" && mutation==9) { Put(pool.AllocatedBytes(Sid{50}),42,17);Put(pool.AllocatedBytes(Sid{50}),46,18); }
    }
    FlyerTargetHooks Hooks() {
        FlyerTargetHooks h;h.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };
        h.checkForms=[this](Sid sid) { Event("Check,"+std::to_string(sid.value));Mutate("Check");return checks->CheckForms(sid); };
        h.offset=[this](Sid sid) { Event("Offset,"+std::to_string(sid.value));Mutate("Offset");return std::array<float,2>{1,0}; };
        h.begin=[this](SquidSearchArea area,std::uint32_t flags) { Event("Begin,"+std::to_string(area.left)+','+std::to_string(area.top)+','+std::to_string(area.right)+','+std::to_string(area.bottom)+','+std::to_string(flags));Mutate("Begin");index=0;return candidates.empty() ? Sid{} : candidates[0]; };
        h.next=[this] { Event("Next");Mutate("Next");++index;return index<candidates.size() ? candidates[index] : Sid{}; };
        h.mayTarget=[this](Sid parent,Sid target) { Event("May,"+std::to_string(parent.value)+','+std::to_string(target.value));Mutate("May");return (may&(1U<<(target.value-60)))!=0; };
        h.watcher=[this](Sid form) { Event("Watch,"+std::to_string(form.value));Mutate("Watch");const auto other=Get(pool.Slot(form),18,2);return other ? Sid{static_cast<std::uint16_t>(Get(pool.Slot(Sid{static_cast<std::uint16_t>(other)}),16,2))} : Sid{}; };
        h.processParent=[this](Sid form) { Event("Process,"+std::to_string(form.value));Mutate("Process");return Sid{static_cast<std::uint16_t>(Get(pool.Slot(form),16,2))}; };
        h.returnDelay=[this](FlyerProcessFields&) { Event("Delay");Mutate("Delay");return 10000000.0; };h.command=[this](Sid sid,std::uint32_t command) { Event("Command,"+std::to_string(sid.value)+','+std::to_string(command));Mutate("Command"); };return h;
    } // 위 콜백은 원본에 선언한 큰 외부 반환/변경만 공급한다.
    std::string Objects() const { std::string result;for (const auto sid:kTargetIds) result+=Hex(pool.Slot(Sid{sid}));return result; } // 원문22객체 전체1100BYTE다.
    std::string GlobalsImage() const { std::array<std::uint8_t,28> raw{};Put(raw,0,server ? 1U : 0U);Put(raw,4,checking ? 1U : 0U);const auto bits=std::bit_cast<std::uint64_t>(now);Put(raw,8,static_cast<std::uint32_t>(bits));Put(raw,12,static_cast<std::uint32_t>(bits>>32));Put(raw,16,sun);Put(raw,20,end);Put(raw,24,depend);return Hex(raw); } // 현재 전역28BYTE다.
};
// 독립 범위별 전수 대조로 한 실패가 다른 원문 증거를 감추지 않게 한다.
void CompareTarget(char kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_FLYER_TARGET_FIXTURE);std::size_t compared{};
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 헤더 이외의 선택한 원본 입력만 실행한다.
        const auto& row=fixture.rows[index];CHECK(row.size()==7);if (row[0]!=std::string(1,kind)) continue;++compared;const auto input=Split(row[1],',');FlyerTargetScene scene(kind,input);std::string result="-";
        if (kind=='F') scene.body->Find(scene.fields);
        else { const Sid target{static_cast<std::uint16_t>(Number(input[0]))},flyer{static_cast<std::uint16_t>(Number(input[1]))};if (kind=='C') result=std::to_string(scene.body->CloserCount(target,flyer));if (kind=='P') result=std::to_string(scene.body->Capacity(target));if (kind=='I') result=std::to_string(scene.body->SlotIndex(target,flyer));if (kind=='S') { const auto point=scene.body->Scatter(target,flyer);Put(scene.fields.bytes,16,std::bit_cast<std::uint32_t>(point[0]));Put(scene.fields.bytes,20,std::bit_cast<std::uint32_t>(point[1])); } }
        const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (Hex(scene.fields.bytes)!=row[2] || scene.Objects()!=row[3] || scene.GlobalsImage()!=row[4] || result!=row[5] || trace!=row[6]) std::fprintf(stderr,"비행 표적 불일치 %c/%s\n실제%s\n기대%s\n",kind,row[1].c_str(),trace.c_str(),row[6].c_str());
        CHECK(Hex(scene.fields.bytes)==row[2]);CHECK(scene.Objects()==row[3]);CHECK(scene.GlobalsImage()==row[4]);CHECK(result==row[5]);CHECK(trace==row[6]);
    }
    CHECK(compared==expected);
}
}
TEST_CASE(FlyerTarget_NativeCloser334) { CompareTarget('C',334); } // 현재 거리/상태/종속 타입과 선저장 next다.
TEST_CASE(FlyerTarget_NativeCapacity60) { CompareTarget('P',60); } // 원문의 free/dead·1×1/그 밖 경계다.
TEST_CASE(FlyerTarget_NativeIndex334) { CompareTarget('I',334); } // 의존 순서와 SID 수치의 차이다.
TEST_CASE(FlyerTarget_NativeScatter334) { CompareTarget('S',334); } // 실제 포위 세 위치와 capacity1의0/0이다.
TEST_CASE(FlyerTarget_NativeFind240) { CompareTarget('F',240); } // 출발점·검색 사각형·필터·엄격한 거리·현재 타이머다.
// 선택된 누락 경계는 원문의 앞선 쓰기를 보존하고 아직 선택하지 않은 경계는 요구하지 않는다.
TEST_CASE(FlyerTarget_RequiredBoundaries) {
    FlyerTargetScene scene('F',Split("2,0,1106247680,144,61,15,0,2,1,0,0,0",','));auto hooks=scene.Hooks();hooks.next={};RawFlyerTarget noNext(scene.pool,scene.types,scene.ranges,scene.Globals(),hooks);noNext.Find(scene.fields);CHECK(Get(scene.fields.bytes,12,2)==0);
    FlyerTargetScene fresh('F',Split("2,0,1106247680,144,61,15,0,2,1,0,0,0",','));hooks=fresh.Hooks();hooks.begin={};RawFlyerTarget missing(fresh.pool,fresh.types,fresh.ranges,fresh.Globals(),hooks);const auto before=fresh.fields.bytes;CHECK(Throws([&] { missing.Find(fresh.fields); }));CHECK(fresh.fields.bytes[8]&4U);CHECK(Get(fresh.pool.Slot(Sid{50}),42)!=0);CHECK(fresh.fields.bytes!=before);
}
