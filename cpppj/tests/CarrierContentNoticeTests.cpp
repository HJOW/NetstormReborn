// 실제 Carrier 가상 내용물 통지의 전체 효과·현재 전역·두 슬롯50바이트를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierContentNotice.h"

using namespace netstorm::test::rawscene;
namespace {
// 원문 고정 입력21열의 의미다.
enum Column : std::size_t { Saved,Loading,Blocked,Owner,Local,Kind,Flags,Dead,Word20,Reason,X,Y,Allocate,Create,Genus,TypeAt,Effect,Sound,Mutation,CameraX,CameraY };
// 결과 계산 없이 unsigned 입력 DWORD만 읽는다.
std::vector<std::uint32_t> Values(const std::string& text) {
    std::vector<std::uint32_t> values;
    // 원문 열 순서와 비트를 유지한다.
    for (const auto& word:Split(text,',')) values.push_back(Number(word));return values;
}
// 명시 경계 효과와 실제 공유 raw/전역 입력만 소유한다.
struct NoticeScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 원문과 같은 두50바이트 슬롯이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 kind별 flags2 표다.
    std::array<std::uint32_t,188> effects{}; // 원본 TYPE+6c의 별도 입력이다.
    std::array<std::int32_t,2> camera{}; // Native21열의 현재 정수 화면 원점이다.
    bool saved{};std::uint32_t loading{},blocked{},local{1},priestType{158},blockerType{156}; // 원본 현재 전역이다.
    std::vector<std::uint32_t> input;std::string trace; // 고정 입력과 실제 외부 효과 순서다.
    // 원문 입력/명시 효과에만 쓰는 raw 접근이며 실제 생성의 대체가 아니다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 매 행 전체 입력을 다시 공급한다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        input=values;trace.clear();saved=input[Saved]!=0;loading=input[Loading];blocked=input[Blocked];local=input[Local];camera={std::bit_cast<std::int32_t>(input[CameraX]),std::bit_cast<std::int32_t>(input[CameraY])};
        auto parent=Raw({50});auto form=Raw({60});std::fill(parent.begin(),parent.end(),std::uint8_t{0xa5});std::fill(form.begin(),form.end(),std::uint8_t{0x5a});
        Put(parent,0,0x50f210);parent[10]=158;parent[34]=static_cast<std::uint8_t>(input[Owner]);Put(parent,14,input[X]);Put(parent,18,input[Y]);
        form[11]=static_cast<std::uint8_t>(input[Dead]);Put(form,18,input[Kind],2);Put(form,20,input[Word20],2);
        // 모든 미지정 TYPE은 고정 입력0이며 추가DWORD만 지정한 서로 다른 값을 갖는다.
        for (std::size_t type=0;type<types.size();++type) { types[type].flags2=0;effects[type]=0x10000000U+static_cast<std::uint32_t>(type); }
        types[input[Kind]].flags2=input[Flags];effects[input[Kind]]=input[Effect];
    }
    // 원본 경계가 받았던 DWORD만 같은 순서로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 음수/float를 다시 산술 변환하지 않는다.
        for (const auto value:values) trace+=','+std::to_string(value);
    }
    // 원문 경계에 공급한 재진입 변경만 수행한다.
    void Mutate(std::string_view event) {
        const auto mode=input[Mutation];auto parent=Raw({50});auto form=Raw({60});
        if (event=="B" && mode==1) { Put(form,18,158,2);form[11]=2; }
        if (event=="S" && mode==2) { parent[34]=2;Put(parent,14,std::bit_cast<std::uint32_t>(99.0f));Put(parent,18,std::bit_cast<std::uint32_t>(77.0f)); }
        if (event=="A" && mode==3) { local=2;Put(form,18,123,2); }
        if (event=="N" && mode==4) loading=1;
        if (event=="H" && mode==5) { Put(form,18,158,2);Put(parent,14,std::bit_cast<std::uint32_t>(25.5f));Put(parent,18,std::bit_cast<std::uint32_t>(30.25f)); }
        if (event=="G" && mode==6) { Put(parent,14,std::bit_cast<std::uint32_t>(55.5f));Put(parent,18,std::bit_cast<std::uint32_t>(66.25f)); }
        if (event=="P" && mode==7) { form[11]=2;loading=1; }
        if (event=="B" && mode==8) { blocked=1;Put(form,18,123,2);types[123].flags2=0; }
    }
    // 실제 외부 인수를 기록하며 가상 통지의 게임 분기는 제품이 수행한다.
    CarrierContentNoticeHooks Hooks() {
        CarrierContentNoticeHooks hooks;
        hooks.center=[this](Sid parent) { Event("C",{parent.value});return std::array{std::bit_cast<float>(Get(pool.Slot(parent),14)),std::bit_cast<float>(Get(pool.Slot(parent),18))}; };
        hooks.bolt=[this](Sid parent,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float time) { Event("B",{parent.value,std::bit_cast<std::uint32_t>(from[0]),std::bit_cast<std::uint32_t>(from[1]),std::bit_cast<std::uint32_t>(to[0]),std::bit_cast<std::uint32_t>(to[1]),std::bit_cast<std::uint32_t>(time)});Mutate("B"); };
        hooks.soundAt=[this](float x,float y,std::string_view name) { CHECK(name=="thunderCrack.wav");Event("S",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});Mutate("S"); };
        hooks.typeWord=[this](std::uint32_t type,std::size_t offset) { CHECK(type<types.size() && (offset==0x6c || offset==0x7c));return offset==0x6c ? effects[type] : input[Sound]; };
        hooks.allocate=[this](std::uint32_t bytes) { Event("A",{bytes});Mutate("A");return input[Allocate] ? 1U : 0U; };
        hooks.create=[this](std::uint32_t token,Sid parent,std::uint32_t value,std::uint32_t first,std::uint32_t second) { Event("N",{token,parent.value,value,first,second});Mutate("N");return input[Create] ? 1U : 0U; };
        hooks.duration=[this](std::uint32_t token,float value) { Event("T",{token,std::bit_cast<std::uint32_t>(value)}); };
        hooks.sound=[this](std::uint32_t value) { Event("P",{value});Mutate("P"); };
        hooks.checkCarrier=[this](Sid parent) { Event("H",{parent.value});Mutate("H"); };
        hooks.genusAt=[this](float x,float y,std::uint32_t genus) { Event("G",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),genus});Mutate("G");return Sid{static_cast<std::uint16_t>(input[Genus] ? 60 : 0)}; };
        hooks.typeAt=[this](float x,float y,std::uint32_t type) { Event("I",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),type});return Sid{static_cast<std::uint16_t>(input[TypeAt] ? 61 : 0)}; };
        hooks.freeSound=[this](std::string_view name) { CHECK(name=="priestFree.wav");Event("F"); };return hooks;
    }
    // 같은 현재 전역의 참조만 빌린다.
    CarrierContentNoticeState State() const { return {saved,loading,blocked,local,priestType,camera[0],camera[1],blockerType}; }
    // 원문의9DWORD를 계산 없이 현재 값으로 직렬화한다.
    std::string Snapshot() const {
        const std::array values{saved ? 1U : 0U,loading,blocked,local,types[123].flags2,types[158].flags2,types[168].flags2,static_cast<std::uint32_t>(camera[0]),static_cast<std::uint32_t>(camera[1])};std::string text;
        // 원문 열 순서를 유지한다.
        for (const auto value:values) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text;
    }
};
}
// 전체 원문 효과·재진입 뒤 현재 전역·두 슬롯의 모든 바이트를 대조한다.
TEST_CASE(CarrierContentNotice_NativeComplete) {
    NoticeScene scene;std::size_t count{};
    // 이전 호출의 상태를 새 행에 넘기지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIER_CONTENT_NOTICE_FIXTURE).rows) {
        scene.Prepare(Values(row[0]));RawCarrierContentNotice notice(scene.pool,scene.types,scene.State(),scene.Hooks());CHECK(notice.Handles({50}));notice.Notice({50},{60},scene.input[Reason]);
        const bool same=(scene.trace.empty() ? "-" : scene.trace)==row[1] && scene.Snapshot()==row[2] && Hex(scene.pool.Slot({50}))==row[3] && Hex(scene.pool.Slot({60}))==row[4];CHECK(same);
        if (!same) { std::printf("Carrier 통지 입력=%s\n실제=%s\n원문=%s\n",row[0].c_str(),scene.trace.c_str(),row[1].c_str());break; }++count;
    }
    CHECK(count==1731);
}
// 실제 파생VT와 읽지 않아야 할 분기·선택된 필수 효과의 누락을 구별한다.
TEST_CASE(CarrierContentNotice_ContractsAndActualDispatch) {
    NoticeScene scene;scene.Prepare(Values("0,0,0,1,1,123,0,0,32,4,1109450752,1112211456,1,1,0,0,1,2,0,4294967279,31"));
    RawCarrierContentNotice empty(scene.pool,scene.types,scene.State(),{});empty.Notice({50},{60},4);
    scene.saved=true;scene.types[123].flags2=0x100;empty.Notice({50},{60},1);scene.saved=false;scene.loading=1;empty.Notice({50},{60},1);
    scene.loading=0;CHECK(Throws([&] { empty.Notice({50},{60},1); }));Put(scene.Raw({50}),0,0x501dd0);CHECK(!empty.Handles({50}));
    Put(scene.Raw({50}),0,0xdeadbeef);CHECK(Throws([&] { empty.Handles({50}); }));
    SidPool cd(OriginalEdition::Cd1072,24000,false);CHECK(Throws([&] { RawCarrierContentNotice invalid(cd,scene.types,scene.State(),{}); }));
}
