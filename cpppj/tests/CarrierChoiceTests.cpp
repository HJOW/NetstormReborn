// 원본 후보 수집·평가의 전체 효과 순서/선택/거리 WORD를 독립 PE 원문과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierChoice.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 명시 입력과 실제 상태 DWORD를 읽는다.
std::vector<std::uint32_t> ChoiceNumbers(std::string_view text) {
    std::vector<std::uint32_t> result;
    // 비트 패턴을 signed 값으로 축소하지 않는다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// 원래 source 목록의 구멍/중복/역순 입력이다.
std::uint32_t ChoiceValue(const std::vector<std::uint32_t>& row,std::size_t index) {
    return row[1]==1 && index%3==0 ? 0U : row[1]==2 ? 60U+static_cast<std::uint32_t>(index%2) : row[1]==3 ? 67U-static_cast<std::uint32_t>(index%8) : 60U+static_cast<std::uint32_t>(index);
}
// 경로 선택 결과를 계산하지 않는 BFS 경계의 명시 꼭짓점/방향 입력이다.
void ChoiceRoute(RawPathRuntime& output,std::uint32_t pattern,std::uint32_t candidate) {
    std::vector<RawPathPoint> points;std::vector<std::int32_t> directions;
    if (pattern!=0 && !(pattern==2 && candidate%2==0)) {
        if (pattern==4) { points={{0,0}};directions={7}; }
        else if (pattern==2) { points={{0,0},{30,40},{31,41}};directions={0,2,4}; }
        else if (pattern==3) { points={{0,0},{static_cast<float>(3+candidate%3),static_cast<float>(4+candidate%5)}};directions={0,2}; }
        else if (pattern==5) { points={{0,0},{std::bit_cast<float>(0x7fc00000U),4}};directions={0,2}; }
        else { points={{0,0},{3,4}};directions={0,2}; }
    }
    output.SetPath(points,directions);
}
// 같은 물리 입력과 큰 하위 경계만 공급하며 선택·정렬·거리·Prepare는 실제 구현이다.
struct CarrierChoiceScene {
    SidPool pool{OriginalEdition::Patch1078,24000}; // 원본 전체 물리 풀의 실제 폭이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 타입 플래그 입력이다.
    std::array<std::array<float,2>,188> movement{}; // 원본 TYPE+e0/e4의 입력 비트다.
    std::array<std::uint8_t,9*0xb4> players{}; // 현재 AI와 귀환점 입력이다.
    SquidPostPopList returns,geysers,candidates; // 현재 원래 두 목록과 직접 평가 입력이다.
    CarrierChoiceState state;BoardTargetState boardState; // 실제 후보 전역과 일반 접근점 실패점이다.
    double now{10.5};bool checking{}; // 실제 현재 시계와 진단 허용 전역이다.
    std::vector<std::uint32_t> row; // 원본에 제공한19DWORD 입력이다.
    std::uint32_t footprints{},searches{},validations{},candidate{60}; // 실제 효과 호출 수와 BFS 경계 입력 인덱스다.
    std::string trace; // 현재 효과의 원래 호출 순서다.
    // 원본 관측은 free/dead 필터 없이 물리 raw를 초기 입력으로 제공한다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 효과 인자는 비트 패턴과 원래 순서로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 현재 DWORD 인자만 연결한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 두 float의 현재 위치를 같은 물리 슬롯에서 읽는다.
    RawPathPoint Point(Sid sid) { return {std::bit_cast<float>(Get(Raw(sid),14)),std::bit_cast<float>(Get(Raw(sid),18))}; }
    // PE에 제공한 초기 입력을 설치하며 예상 선택이나 거리를 계산하지 않는다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);trace.clear();footprints=searches=validations=0;candidate=60;now=10.5;checking=row[10]!=0;state={};state.comparePaths=row[9]!=0;state.baselineBits=row[16];state.currentMetricBits=0x12345678;
        std::fill(types.begin(),types.end(),RiftTypeRecord{});movement={};players={};movement[133]={std::bit_cast<float>(row[7]),std::bit_cast<float>(row[8])};movement[137]={2,180};movement[156]={4,45};types[133].flags2=row[6];types[121].flags1=0x800;Put(players,row[4]*0xb4+0xb0,row[5]);Put(players,row[4]*0xb4+0x7c,row[12]);
        auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),pool.Bytes().size());std::fill(raw.begin(),raw.end(),std::uint8_t{0x2a});
        // 슬롯0/부모50과512개 원본 후보의 바이트 입력을 그대로 넣는다.
        for (std::uint32_t sid=0;sid<572;++sid) {
            if (sid!=0 && sid!=50 && sid<60) continue;auto slot=Raw(Sid{static_cast<std::uint16_t>(sid)});
            slot[10]=static_cast<std::uint8_t>(sid==0 || sid==50 ? 133 : sid==60 && row[1]==5 ? 121 : 122);slot[11]=row[1]==2 ? 3 : 0;slot[34]=static_cast<std::uint8_t>(sid==0 || sid==50 ? row[4] : 2);
            float x=sid==0 || sid==50 ? 20.25F : static_cast<float>(25+(sid-60)%8*3),y=sid==0 || sid==50 ? 21.5F : static_cast<float>(20+(sid-60)/8*3);if (row[17]) { x*=65536;y*=65536; }Put(slot,14,std::bit_cast<std::uint32_t>(x));Put(slot,18,std::bit_cast<std::uint32_t>(y));Put(slot,8,(sid*31+111)&65535U,2);
        }
        returns.entries.resize(row[3]);geysers.entries.resize(row[3]);candidates.entries.resize(row[3]);returns.count=geysers.count=candidates.count=row[2];
        // 논리 capacity만큼 원래 source 배열을 채운다.
        for (std::size_t i=0;i<row[3];++i) returns.entries[i]=geysers.entries[i]=candidates.entries[i]=ChoiceValue(row,i);
    }
    // 실제 새 임시 경로의 시각 배열은 BFS 호출 전5000개 모두0이어야 한다.
    CarrierChoiceHooks Hooks() {
        CarrierChoiceHooks hooks;
        hooks.movement=[this](Sid sid) { return movement[Raw(sid)[10]]; };
        hooks.footprint=[this](Sid sid) {
            Event("R",{sid.value,0});++footprints;if (sid.value!=50) candidate=sid.value;const auto point=Point(sid);const SquidFootprint rectangle{point.x-0.5F,point.y-0.5F,point.x+0.5F,point.y+0.5F};
            if (footprints==1) {
                if (row[11]==1) { Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(42.25F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(43.5F));Put(Raw(Sid{60}),14,std::bit_cast<std::uint32_t>(50.25F));Put(Raw(Sid{60}),18,std::bit_cast<std::uint32_t>(51.5F)); }
                if (row[11]==2) Raw(Sid{50})[10]=137;
                if (row[11]==7) candidates.count=2;
            }
            return rectangle;
        };
        hooks.search=[this](RawPathRuntime& output,Sid,RawPathPoint start,RawPathPoint destination,std::uint32_t flags) {
            ++searches;Event("B",{std::bit_cast<std::uint32_t>(start.x),std::bit_cast<std::uint32_t>(start.y),std::bit_cast<std::uint32_t>(destination.x),std::bit_cast<std::uint32_t>(destination.y),flags});CHECK(output.Count()==0);const auto layout=RawPathRuntime::Layout(OriginalEdition::Patch1078);const auto times=output.Bytes().subspan(layout.times,layout.capacity*8);CHECK(std::all_of(times.begin(),times.end(),[](std::uint8_t byte) { return byte==0; }));ChoiceRoute(output,row[14],candidate);
            if (searches==1 && row[11]==3) { now=13.75;state.comparePaths=false;state.baselineBits=0x7fa12345; }
            if (searches==1 && row[11]==9) Raw(Sid{50})[10]=156;
        };
        hooks.validReturn=[this](Sid parent,Sid target) { Event("V",{parent.value,target.value});++validations;if (validations==1 && row[11]==5) { returns.count=2;Put(players,row[4]*0xb4+0x7c,61); }return (row[13]&(1U<<(target.value%3)))!=0; };
        hooks.validGeyser=[this](Sid parent,Sid target) { Event("G",{parent.value,target.value});++validations;if (row[11]==6) geysers.count=2;return (row[13]&(1U<<(target.value%3)))!=0; };
        hooks.graphFor=[this](Sid sid) { Event("Q",{sid.value});return 7U; };
        hooks.assertion=[this](std::uint32_t line) { Event("!",{line});CHECK(line==0xace || line==0xb03);if (row[11]==4) types[121].flags1=0;if (row[11]==4 || row[11]==8) state.baselineBits=0x7fa12345; };
        hooks.relocate=[this](std::span<const std::uint32_t> entries,const void* pointer) {
            if (pointer==pool.Bytes().data()) return 0x14000000U;const auto address=reinterpret_cast<std::uintptr_t>(pointer),base=reinterpret_cast<std::uintptr_t>(entries.data());if (address<base || address-base>=entries.size()*4) throw std::out_of_range("운반 후보 원문 주소 재배치 범위 오류");return row[15]+static_cast<std::uint32_t>(address-base);
        };return hooks;
    }
    // 선택/현재 전역/실제 호출 수와 전체 물리 raw Adler를 원문 순서로 기록한다.
    std::string State(Sid selected) const {
        const std::array<std::uint32_t,11> values{selected.value,state.currentMetricBits,state.baselineBits,std::uint32_t(state.comparePaths),returns.count,geysers.count,Get(players,row[4]*0xb4+0x7c),footprints,searches,validations,Adler(pool.Bytes())};std::string text;
        // 모든 DWORD를 그대로 대조한다.
        for (const auto value:values) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text;
    }
};
}
// 큰 하위 경계를 제외한 세 실제 상위의 전체 정상 반환 원문을 재생한다.
TEST_CASE(CarrierChoice_NativeCollectionEvaluationAndPrepare) {
    CarrierChoiceScene scene;std::size_t count{},diagnostics{};
    // 원본53비트와 대조하고64비트 결과도 별도 원문으로 보존한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_CHOICE_FIXTURE).rows) {
        CHECK(fields.size()==7);if (fields.size()!=7) continue;scene.Prepare(ChoiceNumbers(fields[0]));RawBoardTarget board(scene.boardState);RawCarrierChoice choice(scene.pool,scene.types,scene.players,scene.returns,scene.geysers,scene.now,scene.checking,board,scene.state,scene.Hooks());Sid selected{};
        if (scene.row[0]==0) selected=choice.Evaluate(scene.candidates,Sid{50},Sid{static_cast<std::uint16_t>(scene.row[18])});else if (scene.row[0]==1) selected=choice.ChooseReturn(Sid{50});else selected=choice.ChooseGeyser(Sid{50});
        const auto trace=scene.trace.empty() ? "-" : scene.trace,state=scene.State(selected);std::string array="-";if (scene.row[0]==0 && scene.row[2]) array=Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(scene.candidates.entries.data()),scene.row[2]*4));
        if ((trace!=fields[1] || state!=fields[2] || array!=fields[3]) && diagnostics++<8) std::cout<<"운반 후보 입력 "<<fields[0]<<"\n실제 "<<trace<<' '<<state<<' '<<array<<"\n원본 "<<fields[1]<<' '<<fields[2]<<' '<<fields[3]<<'\n';
        CHECK(trace==fields[1]);CHECK(state==fields[2]);CHECK(array==fields[3]);++count;
    }
    CHECK(count==721); // 독립 도구의 모든 원문 행을 빠짐없이 재생한다.
}
// 빈 목록도 실제 metric DWORD를 복사하고 선택되지 않은 큰 하위를 요구하지 않는다.
TEST_CASE(CarrierChoice_EmptyCurrentBitsAndContracts) {
    CarrierChoiceScene scene;scene.players={};scene.returns={};scene.geysers={};scene.state.baselineBits=0x7fa12345;RawBoardTarget board(scene.boardState);RawCarrierChoice choice(scene.pool,scene.types,scene.players,scene.returns,scene.geysers,scene.now,scene.checking,board,scene.state,{});SquidPostPopList empty;
    CHECK(!choice.Evaluate(empty,Sid{50},Sid{}).value);CHECK(scene.state.currentMetricBits==0x7fa12345);CHECK(&choice.Pool()==&scene.pool);
    bool rejected{};try { RawCarrierChoice bad(scene.pool,scene.types,{},scene.returns,scene.geysers,scene.now,scene.checking,board,scene.state,{}); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
