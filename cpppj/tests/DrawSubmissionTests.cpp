// 원본 상위 Renderer의 저장 관찰을 실제 공통 병합/CRT/분배와 비교한다. 게임/장치는 실행하지 않는다.
#include "ConstructionSupport.h"
#include "client/DrawSubmission.h"
#include "client/DrawSort.h"
#include <cstring>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 독립 관찰의 완성 입력/병합 전후 배열/분배 인자만 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_DRAWSUBMISSION_FIXTURE).rows;return rows; }
// 16진수 입력을 원본의 전체 16바이트 항목으로 읽는다. 기대 병합/정렬/분배 순서는 만들지 않는다.
std::vector<RenderCollectionEntry> Entries(const std::string& hex) {
    if (hex=="-") return {};CHECK(hex.size()%32==0);std::vector<RenderCollectionEntry> result;
    // 숫자로 변환하지 않아 float/WORD/미사용 바이트의 비트를 그대로 유지한다.
    for (std::size_t offset=0;offset<hex.size();offset+=32) {
        std::array<std::uint8_t,16> bytes{};
        // little endian 바이트를 입력 순서로 공급한다.
        for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(hex.substr(offset+index*2,2),nullptr,16));
        result.push_back(std::bit_cast<RenderCollectionEntry>(bytes));
    }
    return result;
}
// 출력/보호 바이트를 원본 관찰과 같은 문자열로 직렬화한다.
std::string Packed(std::span<const RenderCollectionEntry> entries) {
    return entries.empty() ? "-" : Hex({reinterpret_cast<const std::uint8_t*>(entries.data()),entries.size_bytes()});
}
// 네 좌표를 원본 경계의 비트 관찰과 같은 순서로 기록한다.
std::string Rectangle(RenderWorldRect r) { return std::to_string(Bits(r.left))+","+std::to_string(Bits(r.top))+","+std::to_string(Bits(r.right))+","+std::to_string(Bits(r.bottom)); }
// UI는 float이 아닌 화면 정수 네 개를 전달한다.
std::string Rectangle(ScreenRect r) { return std::to_string(r.left)+","+std::to_string(r.top)+","+std::to_string(r.right)+","+std::to_string(r.bottom); }
// 문자열 관찰은 비어 있는 순서를 '-'로 표시한다.
void Add(std::string& text,const std::string& event) { if (!text.empty()) text+=';';text+=event; }
// 원본 묶음은 헤더/개수/항목 배열을 별도로 소유한다. count 주소는 전체 호출 동안 고정한다.
struct GroupStorage { std::uint32_t token{};std::int32_t count{};std::vector<RenderCollectionEntry> entries; };
// 한 판본의 모든 옵션 조합을 원본이 결정한 배열/호출 순서와 대조한다.
void Replay(std::string_view edition) {
    std::size_t replayed=0;
    // 관찰 행마다 수집기 응답 저장소를 새로 만들며 기대 결과는 fixture에서만 읽는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==13);const auto input=Inputs(row[1]);const auto supplied=Split(row[2],'|');
        std::array<std::vector<RenderCollectionEntry>,4> source;
        // 네 수집기의 완성 입력 바이트를 읽는다.
        for (std::size_t index=0;index<source.size();++index) source[index]=Entries(supplied[index]);
        std::vector<GroupStorage> storage;
        // 헤더 토큰과 최초 개수/전체 내부 저장소를 읽는다.
        for (std::size_t index=4;index<supplied.size();++index) {
            const auto fields=Split(supplied[index],'=');storage.push_back({Number(fields[0]),std::stoi(fields[1]),Entries(fields[2])});
        }
        std::vector<DrawGroupView> groups;
        // storage의 마지막 확장 뒤에만 count 포인터를 만든다.
        for (const auto& group:storage) groups.push_back({group.token,group.entries,&group.count});
        DrawCollectionRequest request;request.dirtyFlags=input[1];
        // 좌표 환산은 복원 범위 밖이다. 실제 루트가 계산한 수집기 인자를 경계 입력으로 쓴다.
        for (const auto& event:Split(row[7],';')) {
            const auto fields=Split(event,':');const auto coordinates=Inputs(fields.back());
            if (fields[0]=="ui") request.screen={static_cast<int>(coordinates[0]),static_cast<int>(coordinates[1]),static_cast<int>(coordinates[2]),static_cast<int>(coordinates[3])};
            if (fields[0]=="construction") request.visible={Float(coordinates[0]),Float(coordinates[1]),Float(coordinates[2]),Float(coordinates[3])};
            if (fields[0]=="preview") request.unclipped={Float(coordinates[0]),Float(coordinates[1]),Float(coordinates[2]),Float(coordinates[3])};
            if (fields[0]=="group") request.groupTime=std::stoi(fields[1]);
        }
        std::vector<RenderCollectionEntry> guarded(static_cast<std::size_t>(kDrawSubmissionCapacity)+2);
        std::memset(guarded.data(),0xa5,guarded.size()*sizeof(RenderCollectionEntry));auto work=std::span(guarded).subspan(1,kDrawSubmissionCapacity);
        std::string collected,drawn;DrawCollectionHooks collect;
        // 하나의 명시 수집 응답을 복사하고 원본과 같은 포인터 위치/남은 용량/좌표를 관찰한다.
        const auto copy=[&](std::size_t index,std::string_view name,auto rectangle,std::span<RenderCollectionEntry> output,std::int32_t limit) {
            Add(collected,std::string(name)+":"+std::to_string(output.data()-work.data())+":"+std::to_string(limit)+":"+Rectangle(rectangle));
            CHECK(source[index].size()<=output.size());std::copy(source[index].begin(),source[index].end(),output.begin());return static_cast<std::int32_t>(source[index].size());
        };
        collect.ui=[&](ScreenRect r,auto output,auto limit) { return copy(0,"ui",r,output,limit); };
        collect.construction=[&](RenderWorldRect r,auto output,auto limit) { return copy(1,"construction",r,output,limit); };
        collect.objects=[&](RenderWorldRect r,auto output,auto limit) { return copy(2,"objects",r,output,limit); };
        collect.preview=[&](RenderWorldRect r,auto output,auto limit) { return copy(3,"preview",r,output,limit); };
        collect.groups=[&](RenderWorldRect r,std::int32_t time,bool& covered)->std::span<const DrawGroupView> {
            Add(collected,"group:"+std::to_string(time)+":"+Rectangle(r));covered=(input[0]%2)!=0;return groups;
        };
        const auto result=CollectDrawEntries(request,work,collect);auto entries=work.first(static_cast<std::size_t>(result.count));
        CHECK(result.count==std::stoi(row[3]) && result.backgroundCovered==(row[4]=="1"));CHECK(collected==row[7]);CHECK(Packed(entries)==row[5]);
        if (input[2]) SortDrawEntries(entries,edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
        CHECK(Packed(entries)==row[6]);DrawDispatchState state{input[3]!=0,input[4]!=0};DrawDispatchHooks dispatch;bool toggled=false;
        dispatch.sprite=[&](const RenderCollectionEntry& entry) {
            Add(drawn,"S:"+Packed({&entry,1}));
            if (!storage.empty() && &entry==storage[0].entries.data() && (input[0]==4 || input[0]==5)) storage[0].count=input[0]==4 ? 1 : 4;
        };
        dispatch.group=[&](std::uint32_t token) {
            const auto found=std::find_if(groups.begin(),groups.end(),[&](const auto& group) { return group.token==token; });
            if (found==groups.end()) throw std::runtime_error("원본 묶음 토큰 누락");return *found;
        };
        dispatch.ui=[&](std::uint32_t token) {
            Add(drawn,"U:"+std::to_string(token));
            if (input[0]==9 && !toggled) { state.hideGroups=!state.hideGroups;state.debugAfterEach=!state.debugAfterEach;toggled=true; }
        };
        dispatch.construction=[&](std::uint32_t token) { Add(drawn,"C:"+std::to_string(token)); };
        dispatch.preview=[&](const RenderCollectionEntry& entry) { Add(drawn,"P:"+Packed({&entry,1})); };
        dispatch.afterEach=[&] { Add(drawn,"D"); };DispatchDrawEntries(entries,state,dispatch);
        const auto guard=Packed(std::span(guarded).first(1))+Packed(std::span(guarded).subspan(static_cast<std::size_t>(result.count)+1,1));
        const bool same=(drawn.empty() ? "-" : drawn)==row[8] && guard==row[12];CHECK(same);
        if (!same) { std::printf("공통 표시 제출 %s %s 불일치\n",std::string(edition).c_str(),row[1].c_str());break; }++replayed;
    }
    CHECK(replayed==322);
}
}
// 패치의 상위 switch/실제 CRT/묶음 재조회와 원본 바이트를 모두 대조한다.
TEST_CASE(DrawSubmission_PatchNative) { Replay("originals"); }
// CD의 별도 묶음 몸체/CRT도 독립 PE 관찰과 대조한다.
TEST_CASE(DrawSubmission_CdNative) { Replay("originalCD"); }
// 추가 PE에서 얻은 별도 관찰도 같은 복원 경로에 대조한다.
TEST_CASE(DrawSubmission_AdditionalNative) { Replay("original1037"); }

// 손상된 수집 반환이나 묶음 초과를 다음 단계에 전달하지 않는다. 작은 버퍼에서도 같은 남은 용량 계약이다.
TEST_CASE(DrawSubmission_RejectsInvalidCountsAndGroupOverflow) {
    std::array<RenderCollectionEntry,2> work{};DrawCollectionHooks hooks;bool called=false;
    hooks.objects=[&](auto,auto,auto) { called=true;return 0; };
    hooks.ui=[](auto,auto,auto) { return -1; };CHECK(Throws([&] { CollectDrawEntries({},work,hooks); }));CHECK(!called);
    hooks.ui=[](auto,auto,auto) { return 3; };CHECK(Throws([&] { CollectDrawEntries({},work,hooks); }));CHECK(!called);
    std::int32_t count=1;std::array<RenderCollectionEntry,1> member{};std::array<DrawGroupView,1> group{{{7,member,&count}}};
    hooks.ui=[](auto,auto,auto) { return 2; };hooks.groups=[&](auto,auto,bool&)->std::span<const DrawGroupView> { return group; };
    CHECK(Throws([&] { CollectDrawEntries({},work,hooks); }));CHECK(!called);
    hooks.ui={};group[0].entries={};CHECK(Throws([&] { CollectDrawEntries({},work,hooks); }));
    group[0].entries=member;group[0].count=nullptr;CHECK(Throws([&] { CollectDrawEntries({},work,hooks); }));
}

// 사용하는 종류만 필수 경계를 요구하며 알려지지 않은 종류도 상위 debug 관찰은 유지한다.
TEST_CASE(DrawSubmission_DispatchContractsAndUnknownKinds) {
    RenderCollectionEntry entry{};DrawDispatchState state;DrawDispatchHooks hooks;
    CHECK(Throws([&] { DispatchDrawEntries({&entry,1},state,hooks); }));entry.kind=1;state.hideGroups=true;
    CHECK(!Throws([&] { DispatchDrawEntries({&entry,1},state,hooks); }));state.hideGroups=false;
    CHECK(Throws([&] { DispatchDrawEntries({&entry,1},state,hooks); }));
    std::int32_t count=2;std::array<RenderCollectionEntry,1> members{};hooks.group=[&](auto) { return DrawGroupView{0,members,&count}; };
    int drawn=0;hooks.sprite=[&](const auto&) { ++drawn; };CHECK(Throws([&] { DispatchDrawEntries({&entry,1},state,hooks); }));CHECK(drawn==1);
    count=-1;CHECK(!Throws([&] { DispatchDrawEntries({&entry,1},state,hooks); }));CHECK(drawn==1);
    entry.kind=255;state.debugAfterEach=true;CHECK(Throws([&] { DispatchDrawEntries({&entry,1},state,hooks); }));
    int after=0;hooks.afterEach=[&] { ++after; };DispatchDrawEntries({&entry,1},state,hooks);CHECK(after==1 && drawn==1);
}

// 5,000초과 버퍼도 원본 배열 용량까지만 공급하며 용량 0에서도 연결된 수집기를 원본 순서대로 부른다.
TEST_CASE(DrawSubmission_CapacityAndEmptyBuffer) {
    std::vector<RenderCollectionEntry> work(5001);work.back().tail=71;DrawCollectionHooks hooks;std::vector<std::int32_t> limits;
    hooks.ui=[&](auto,auto output,auto limit) { limits.push_back(limit);CHECK(output.size()==static_cast<std::size_t>(limit));return limit; };
    hooks.construction=[&](auto,auto output,auto limit) { limits.push_back(limit);CHECK(output.empty());return 0; };
    hooks.objects=hooks.preview=hooks.construction;const auto result=CollectDrawEntries({},work,hooks);
    CHECK(result.count==5000 && work.back().tail==71 && limits==std::vector<std::int32_t>({5000,0,0,0}));
    limits.clear();CHECK(CollectDrawEntries({}, {},hooks).count==0);CHECK(limits==std::vector<std::int32_t>({0,0,0,0}));
}
