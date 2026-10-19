// 실제 네 해시를 포함한 독립 원본300입력의 검색 기하/순서/전체 배열과 현재 상태를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawBombTargets.h"
#include <filesystem>
#include <iostream>

using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kBombTable=0x502370,kAlternateTable=0x20000800; // 원본 실행기의 실제/교체 가상 표 토큰이다.
constexpr std::array<std::uint16_t,12> kTargetIds{1,4,5,14999,15000,15001,15002,15003,15004,15005,15006,15007}; // 예약/클라이언트/서버 대역의 명시 입력이다.
constexpr std::array<std::array<float,2>,12> kPositions{{{50.75F,52.5F},{51,53},{51.5F,53},{52,53},{54.5F,53},{54.500003814697266F,53},{53.5F,55},{47.5F,53},{50.75F,48},{57,53},{52,55},{51,52}}}; // 독립 원문에 제공한 후보 좌표이며 선택 결과가 아니다.
constexpr std::array<std::array<float,2>,4> kEdgePositions{{{1.5F,1.5F},{255,255},{1,255},{255,1}}}; // 경계 장면의 네 입력 좌표다.
constexpr std::array<std::uint8_t,12> kStates{0,1,2,4,8,0,0,0,0,0,0,0},kExtras{0,0,0,0,0,1,8,16,32,0,1,0}; // 원문 상태/extra 패턴이다.
// 입력 장면의 실제Pool/Hash/TYPE와 명시 파생 효과만 준비한다.
struct TargetScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원본 번호를 바꾸지 않는 물리 저장소다.
    SquidHash hash; // 현재 네 단계의 실제 머리 배열이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 후보의 현재 발자국이다.
    std::vector<float> ranges=std::vector<float>(188); // 부모TYPE+44의 현재 원문 입력이다.
    BombTargetState state; // 현재 권한 전역이다.
    std::array<std::uint32_t,100> storage{}; // 원문과 동일한 실제 DWORD100개 버퍼다.
    BombTargetList output{storage}; // 파생 효과가 바꿀 수 있는현재 배열 헤더다.
    RawBombTargets* active{}; // 이번 실제 몸체의 기본radius helper를 호출하는 비소유 참조다.
    std::vector<std::uint32_t> input; // 독립 원문의10개DWORD 입력이다.
    std::string trace; // 실제 파생 자격/복귀 진단의 순서와 인자다.
    std::uint32_t queries{}; // 명시 재진입의 첫 자격 호출을 식별한다.
    // 검증의 명시 입력/외부 효과만 물리 슬롯에 쓴다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 관측된 큰 외부 경계의 DWORD 인자만 기록한다.
    void Event(std::string_view name,std::uint32_t value) { if (!trace.empty()) trace+=';';trace+=std::string(name)+':'+std::to_string(value); }
    // 같은 버킷의 머리 삽입 입력만 작성한다. Pop/탐색 결과를 계산하지 않는다.
    void Object(std::size_t index) {
        const auto scene=input[0];const Sid sid{kTargetIds[index]};auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kBombTable);
        raw[10]=static_cast<std::uint8_t>(70+index);raw[11]=scene==2 ? kStates[index] : 0;raw[40]=(scene==2 || scene==3) ? kExtras[index] : 0;
        const auto position=scene==4 && index<4 ? kEdgePositions[index] : kPositions[index];Put(raw,14,std::bit_cast<std::uint32_t>(position[0]));Put(raw,18,std::bit_cast<std::uint32_t>(position[1]));
        types[70+index].footX=scene==1 ? static_cast<int>(index%4)+1 : 1;types[70+index].footY=scene==1 ? static_cast<int>(index%3)+1 : 1;
        const auto level=static_cast<int>(index%4),scale=SquidHash::kScales[index%4];auto& head=hash.Cell(level,static_cast<int>(position[0])/scale,static_cast<int>(position[1])/scale);Put(raw,4,head,2);head=sid.value;
    }
    // 입력만 설치하며 검색/거리/출력 개수의 기대값을 계산하지 않는다.
    void Prepare(std::string_view text) {
        input.clear();trace.clear();queries=0;hash.Reset();types.assign(188,{});ranges.assign(188,0.0F);
        // 원문의10개DWORD 입력을 그대로 읽는다.
        for (const auto& field:Split(text,',')) input.push_back(Number(field));
        CHECK(input.size()==10);state.boss=input[1];output.capacity=std::bit_cast<std::int32_t>(input[5]);output.count=std::bit_cast<std::int32_t>(input[6]);
        // 실제 등록 순서만 바꿔 같은 버킷의 후보 순서를 재현한다.
        for (std::size_t i=0;i<kTargetIds.size();++i) Object(input[9] ? kTargetIds.size()-1-i : i);
        auto raw=Raw(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kBombTable);raw[10]=168;Put(raw,14,input[3]);Put(raw,18,input[4]);ranges[168]=std::bit_cast<float>(input[2]);
        // 배열의 미사용 범위도 동일 입력 표식으로 비교한다.
        for (std::size_t i=0;i<storage.size();++i) storage[i]=0x11110000U+static_cast<std::uint32_t>(i);
    }
    // 독립 원문에 선언한 파생 자격과 재진입 변경만 공급한다.
    BombTargetHooks Hooks() {
        BombTargetHooks hooks;
        // 실제41e3c0 helper는제품이실행하며가상분배토큰만확인한다.
        hooks.radius=[this](Sid parent,std::uint32_t table) { CHECK(table==kBombTable || table==kAlternateTable);if (!active) throw std::logic_error("검사 대상 몸체 미설치");return active->DefaultRadius(parent); };
        // 현재 가상 표의파생 자격 경계이며기하 판단을대체하지않는다.
        hooks.eligible=[this](Sid parent,std::uint32_t table,Sid candidate) {
            CHECK(parent.value==50);CHECK(table==kBombTable || table==kAlternateTable);Event(table==kBombTable ? "eligible" : "eligible-alt",candidate.value);
            if (++queries==1) {
                if (input[8]==1) Put(Raw(parent),0,kAlternateTable);
                if (input[8]==2) Raw(Sid{15003})[40]=1;
                if (input[8]==3) Put(Raw(Sid{15003}),14,std::bit_cast<std::uint32_t>(200.0F));
                if (input[8]==4) Put(Raw(candidate),4,0,2);
                if (input[8]==5) { Put(Raw(parent),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(parent),18,std::bit_cast<std::uint32_t>(200.0F)); }
                if (input[8]==6) output.capacity=0;
                if (input[8]==7) output.count=output.capacity;
                if (input[8]==8) types[77].footX=8;
            }
            return input[7]==1 || (input[7]==2 && candidate.value%2==0);
        };
        // 원본의 복귀 가능한 진단만 기록한다.
        hooks.assertion=[this](std::uint32_t line) { Event("assert",line); };return hooks;
    }
    // 실제13슬롯 전체를 원문과 같은 순서로 읽는다.
    std::string Raws() {
        std::string result=Hex(Raw(Sid{50}));
        // 예약 번호도 제외하지 않고 물리 바이트를 비교한다.
        for (const auto sid:kTargetIds) result+=','+Hex(Raw(Sid{sid}));
        return result;
    }
    // 실제DWORD 배열의전체400바이트를 원문 형식으로직렬화한다.
    std::string Array() const {
        std::array<std::uint8_t,400> bytes{};
        // 논리선택목록을재계산하지않고 실제물리값만기록한다.
        for (std::size_t i=0;i<storage.size();++i) Put(bytes,4*i,storage[i]);
        return Hex(bytes);
    }
};
// 원본 관측 순서로 중심2/정수반경/float사각형4를 기록한다.
std::string Geometry(const BombTargetGeometry& geometry) {
    std::string result=std::to_string(std::bit_cast<std::uint32_t>(geometry.center[0]))+','+std::to_string(std::bit_cast<std::uint32_t>(geometry.center[1]))+','+std::to_string(std::bit_cast<std::uint32_t>(geometry.radius));
    // 원본41def0에 전달된 네float 비트를 비교한다.
    for (const auto value:geometry.area) result+=','+std::to_string(std::bit_cast<std::uint32_t>(value));
    return result;
}
}
// 실제네해시/원본보조함수를포함한300행의전체결과와 비교한다.
TEST_CASE(BombTargets_Native300Inputs) {
    TargetScene scene;std::size_t count{},diagnostics{};const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-targets-x86.tsv").string();
    // 각독립입력을새몸체로실행하며기대값은 동결원문만사용한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==15);if (row.size()!=15) continue;scene.Prepare(row[0]);RawBombTargets targets(scene.pool,scene.hash,scene.types,scene.ranges,scene.state,scene.Hooks());scene.active=&targets;
        const auto geometry=targets.Find(Sid{50},scene.output);
        const std::array<std::string,7> actual{Geometry(geometry),scene.trace.empty() ? "-" : scene.trace,scene.Array(),std::to_string(std::bit_cast<std::uint32_t>(scene.output.capacity)),std::to_string(std::bit_cast<std::uint32_t>(scene.output.count)),scene.Raws(),std::to_string(scene.types[77].footX)};
        // 두정밀도의원문과기하/효과/전체배열/헤더/전체raw/현재발자국을각각대조한다.
        for (std::size_t field=0;field<actual.size();++field) {
            if (actual[field]!=row[1+field] && diagnostics++<3) std::cout<<"Bomb대상 입력 "<<row[0]<<" 필드 "<<field<<"\n실제 "<<actual[field]<<"\n원본 "<<row[1+field]<<'\n';
            CHECK(actual[field]==row[1+field]);CHECK(row[1+field]==row[8+field]);
        }
        ++count;
    }
    CHECK(count==300);
}
// 미도달 자격을 요구하지 않으며 실제 선택 경계와 물리 버퍼 범위를 확인한다.
TEST_CASE(BombTargets_LazyHooksAndOutputBounds) {
    TargetScene scene;scene.Prepare("0,1,1084227584,1112211456,1112670208,100,0,1,0,0");
    RawBombTargets missing(scene.pool,scene.hash,scene.types,scene.ranges,scene.state,{});CHECK(&missing.Pool()==&scene.pool);CHECK(Throws([&] { missing.Find(Sid{50},scene.output); }));
    BombTargetHooks hooks;hooks.radius=[](Sid,std::uint32_t) { return 5; }; // 실제자격경계누락을 성공으로처리하지않는다.
    RawBombTargets qualification(scene.pool,scene.hash,scene.types,scene.ranges,scene.state,hooks);CHECK(Throws([&] { qualification.Find(Sid{50},scene.output); }));
    scene.hash.Reset();RawBombTargets empty(scene.pool,scene.hash,scene.types,scene.ranges,scene.state,hooks);CHECK(empty.Find(Sid{50},scene.output).radius==5); // 후보가 없는 검색에서는 자격을 호출하지 않는다.
    scene.Prepare("0,1,1084227584,1112211456,1112670208,100,0,1,0,0");hooks.eligible=[](Sid,std::uint32_t,Sid) { return true; }; // 물리 배열보다 큰 헤더 용량도 실제 버퍼 밖에 쓰지는 않는다.
    BombTargetList shortBuffer{std::span<std::uint32_t>{},100,0};RawBombTargets bounded(scene.pool,scene.hash,scene.types,scene.ranges,scene.state,hooks);CHECK(Throws([&] { bounded.Find(Sid{50},shortBuffer); }));
}
