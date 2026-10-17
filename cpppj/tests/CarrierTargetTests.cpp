// 실제 접근점과 이동 상위 본문을 원본 보호 PE의 전체 입력으로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierTarget.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원문22DWORD의 입력 비트를 읽는다.
std::vector<std::uint32_t> TargetNumbers(std::string_view text) { std::vector<std::uint32_t> result;
    // 각 숫자를 signed로 축소하지 않는다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// 발자국/실제 경로 시작/귀환 후보만 외부 경계로 제공한다.
struct CarrierTargetScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 두 물리 raw 입력의 원본 폭이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 대상 genus 입력이다.
    std::array<std::uint8_t,9*0xb4> players{}; // 실제 현재 AI 포인터를 저장한다.
    bool boss{}; // 현재 귀환 권한이다.
    BoardTargetState state; // 진단 뒤 현재 실패점이다.
    std::vector<std::uint32_t> row; // 보호 PE에 제공한22DWORD 입력이다.
    std::array<std::uint32_t,2> out{}; // 기하 연산의 직접 결과만 기록한다.
    std::uint32_t attempts{}; // 실제 시작 callback의 호출 수다.
    std::string trace; // 원래 인자 순서의 외부 효과다.
    // 원문은 할당 필터 없이 물리 슬롯을 읽는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 외부 효과 인자를 원래 DWORD 순서대로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) { if (!trace.empty()) trace+=';';trace+=name;
        // 포인터/float/flags의 비트를 정규화하지 않는다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 입력 사각형만 읽고 접근점은 계산하지 않는다.
    SquidFootprint Rectangle() const { return {std::bit_cast<float>(row[1]),std::bit_cast<float>(row[2]),std::bit_cast<float>(row[3]),std::bit_cast<float>(row[4])}; }
    // 현재 raw/AI/genus/실패점 입력을 원문과 같게 설치한다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);CHECK(row.size()==22);boss=row[16]!=0;state.invalid={std::bit_cast<float>(row[19]),std::bit_cast<float>(row[20])};out={0xa5a5a5a5,0xa5a5a5a5};attempts=0;trace.clear();players.fill(0);Put(players,row[12]*0xb4+0xb0,row[13]);types[122].flags2=row[15];
        // 원문 부모와 대상의 전체 초기 raw를 보존한다.
        for (const auto sid:{Sid{50},Sid{60}}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});raw[10]=sid==Sid{50} ? 133 : 122;raw[11]=static_cast<std::uint8_t>(sid==Sid{60} ? row[14] : 0U);raw[34]=static_cast<std::uint8_t>(sid==Sid{50} ? row[12] : 2U);Put(raw,14,row[5]);Put(raw,18,row[6]); }
    }
    // 호출 시에만 진단을 요구하며 원본의 현재 실패점 재조회를 관측한다.
    auto Diagnostic() { return [this](std::uint32_t line) { Event("!",{line});if (row[21]) state.invalid={42.125F,std::bit_cast<float>(0x7f800001U)}; }; }
    // 반환을 대신 계산하지 않고 명시 큰 하위 효과만 제공한다.
    CarrierTargetHooks Hooks() {
        CarrierTargetHooks hooks;
        hooks.footprint=[this](Sid sid) { Event("R",{sid.value,0});if (row[18]==1) { Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(42.25F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(43.5F)); }return Rectangle(); };
        hooks.start=[this](RawPathStartRequest request) { Event("S",{request.parent.value,std::bit_cast<std::uint32_t>(request.destination.x),std::bit_cast<std::uint32_t>(request.destination.y),request.replaceStart,request.helper,request.tailWord,request.tailByte,request.reserved});++attempts;if (row[18]==2) { Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(100.0F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(101.0F));Put(Raw(Sid{60}),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(Sid{60}),18,std::bit_cast<std::uint32_t>(201.0F)); }return row[11]==attempts ? CarrierPathResult{0x14600000U+attempts*40U} : CarrierPathResult{}; };
        hooks.chooseReturn=[this](Sid sid) { Event("C",{sid.value});if (row[18]==3) { boss=false;Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(21.75F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(8.5F)); }return Sid{static_cast<std::uint16_t>(row[17])}; };return hooks;
    }
    // 원문 순서의 현재 결과와 전역/횟수다.
    std::string State(CarrierPathResult result) const { std::string text;const std::array<std::uint64_t,7> values{result,out[0],out[1],std::bit_cast<std::uint32_t>(state.invalid.x),std::bit_cast<std::uint32_t>(state.invalid.y),std::uint32_t(boss),attempts};
        // 실제 DWORD와 호스트 반환 주소를 숫자로 비교한다.
        for (const auto value:values) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text;
    }
};
}
// 새 원문2245입력 전체의 반환/호출 인자/최종 두 raw를 재생한다.
TEST_CASE(CarrierTarget_NativeGeometryAndCommands) {
    CarrierTargetScene scene;std::size_t count{},geometry{},diagnostics{}; // 전체/실제 CPU 기하 행 수와 불일치 진단 한도다.
    std::map<std::pair<std::size_t,std::uint32_t>,std::vector<std::string>> hardware; // 같은 입력의 두 실제 x87 관측 원문이다.
    // 실제 CPU 자료를 별도로 읽으며 에뮬레이터 원문을 덮어쓰지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIER_TARGET_HARDWARE).rows) { CHECK(row.size()==5);if (row.size()==5) hardware.emplace(std::pair{static_cast<std::size_t>(Number(row[0])),Number(row[1])},row); }
    CHECK(hardware.size()==2040);
    // 두 정밀도 원문을 보존하며 기하는 실제 CPU의53비트 결과로 대조한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_TARGET_FIXTURE).rows) {
        CHECK(fields.size()==7);if (fields.size()!=7) continue;scene.Prepare(TargetNumbers(fields[0]));RawBoardTarget board(scene.state,scene.Diagnostic());RawCarrierTarget target(scene.pool,scene.types,scene.players,scene.boss,board,scene.Hooks());CarrierPathResult result{};
        if (scene.row[0]==6) { const BoardTargetState initialized;const auto native=TargetNumbers(fields[2]);CHECK(native[3]==std::bit_cast<std::uint32_t>(initialized.invalid.x) && native[4]==std::bit_cast<std::uint32_t>(initialized.invalid.y));++count;continue; } // 세계 초기화 전체를 가장하지 않고4af170의 실패점 쓰기와 기본값만 대조한다.
        if (scene.row[0]<2) { const auto point=scene.row[0]==0 ? board.Side(scene.Rectangle(),scene.row[8],scene.row[7]) : board.Nearest(scene.Rectangle(),{std::bit_cast<float>(scene.row[5]),std::bit_cast<float>(scene.row[6])},scene.row[7]);scene.out={std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y)}; }
        else if (scene.row[0]==2) result=target.StartTarget(Sid{50},Sid{60},scene.row[9],scene.row[10]);else if (scene.row[0]==3) result=target.GoReturn(Sid{50},Sid{60});else if (scene.row[0]==4) result=target.GoGeyser(Sid{50},Sid{60});else result=target.GoHome(Sid{50});
        const auto trace=scene.trace.empty() ? "-" : scene.trace,state=scene.State(result),raws=Hex(scene.Raw(Sid{50}))+","+Hex(scene.Raw(Sid{60}));
        auto expectedTrace=fields[1],expectedState=fields[2]; // 기하는 실제 CPU, 나머지 상위는 기존 독립 에뮬레이터 관측을 사용한다.
        if (scene.row[0]<2) { const auto& measured=hardware.at({count,0x27f});const auto& extended=hardware.at({count,0x37f});CHECK(measured[2]==extended[2] && measured[3]==extended[3]);const auto oldState=Split(fields[2],',');expectedTrace=measured[2];expectedState=oldState[0]+","+measured[3]+","+oldState[5]+","+oldState[6];++geometry; }
        if ((trace!=expectedTrace || state!=expectedState || raws!=fields[3]) && diagnostics++<6) std::cout<<"운반 대상 입력 "<<fields[0]<<"\n실제 "<<trace<<' '<<state<<' '<<raws<<"\n원본 "<<expectedTrace<<' '<<expectedState<<' '<<fields[3]<<'\n';
        CHECK(trace==expectedTrace);CHECK(state==expectedState);CHECK(raws==fields[3]);++count;
    }
    CHECK(count==2245 && geometry==1020);
}
// 조기 반환은 필요 없는 하위를 요구하지 않으며 AI 판정은 현재 Player 포인터를 직접 읽는다.
TEST_CASE(CarrierTarget_CurrentAiAndLazyBoundaries) {
    CarrierTargetScene scene;BoardTargetState pointState;RawBoardTarget board(pointState);RawCarrierTarget target(scene.pool,scene.types,scene.players,scene.boss,board,{});CHECK(target.GoHome(Sid{50})==0);scene.Raw(Sid{50})[34]=1;Put(scene.players,0xb4+0xb0,1);CHECK(target.GoReturn(Sid{50},Sid{60})==0);CHECK(target.GoGeyser(Sid{50},Sid{60})==0);Put(scene.players,0xb4+0xb0,0);scene.Raw(Sid{60})[11]=1;CHECK(target.GoGeyser(Sid{50},Sid{60})==0);CHECK(&target.Pool()==&scene.pool);
    bool rejected{};try { static_cast<void>(target.GoReturn(Sid{50},Sid{60})); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);rejected=false;try { static_cast<void>(board.Side({1,2,3,4},1,1)); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);rejected=false;try { RawCarrierTarget bad(scene.pool,scene.types,std::span<const std::uint8_t>{},scene.boss,board,{}); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
