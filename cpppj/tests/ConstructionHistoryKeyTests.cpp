// 최초 원문511입력의 전체40BYTE/순번·호출과 실제 Client 수명을 확인한다.
#include "RawSceneSupport.h"
#include "client/ConstructionHistoryKey.h"
#include "client/ClientGameplayGlobals.h"
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { //fixture 좌표는 부호/NaN을 변환하지 않고 원시DWORD로 옮긴다.
void ReadHistoryPoints(netstorm::o::ConstructionHistoryState& history,std::string_view text) { //다섯 좌표를 원문 입력의 실제 비트로 준비한다.
    std::array<std::uint8_t,40> bytes{};CHECK(text.size()==80);if (text.size()!=80) return;
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index*2,2)),nullptr,16)); //fixture의 각 원문 바이트를 그대로 읽는다.
    for (std::size_t index=0;index<5;++index) { //각 축의 실제 네 바이트만 읽는다.
        history.points[index]={std::bit_cast<float>(Get(bytes,index*8)),std::bit_cast<float>(Get(bytes,index*8+4))};
    }
}
std::string HistoryPoints(const netstorm::o::ConstructionHistoryState& history) { //이력 전체를 명시40BYTE로 직렬화해 미사용 칸도 비교한다.
    std::array<std::uint8_t,40> bytes{};
    for (std::size_t index=0;index<5;++index) { Put(bytes,index*8,std::bit_cast<std::uint32_t>(history.points[index].x));Put(bytes,index*8+4,std::bit_cast<std::uint32_t>(history.points[index].y)); } //실제 좌표 비트만 순서대로 직렬화한다.
    return Hex(bytes);
}
}

//같은 형식의 전체 입력을 방문하며 제품 함수 안에 기대 결과를 복제하지 않는다.
TEST_CASE(ConstructionHistoryKey_IndependentNative511Inputs) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/construction-history-key-x86.tsv").string().c_str());std::size_t count{};
    for (const auto& row:fixture.rows) { //초기화3행과 실제 Tab508행을 모두 같은 제품 함수에 전달한다.
        CHECK(row.size()==16);if (row.size()!=16) continue;netstorm::o::ConstructionHistoryState history;ReadHistoryPoints(history,row[2]);history.index=Number(row[3]);std::string trace;bool completed=true;
        auto cells=std::bit_cast<std::int32_t>(Number(row[4]));ConstructionHistoryKeyHooks hooks;
        hooks.paused=[&] { trace="P:"+row[5];return Number(row[5])!=0; };
        hooks.center=[&](float x,float y) { trace+=";C:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y));if (Number(row[6])) { history.index=Number(row[7]);history.points[0]={-10000.0F,250.25F}; }if (Number(row[8])) throw std::runtime_error("원문 Center 명시 중단 입력"); };
        try { if (row[0]=="Init") InitializeConstructionHistory(history);else CHECK(DispatchConstructionHistoryKey({Number(row[1]),0,0},history,cells,hooks)); }catch (const std::runtime_error&) { completed=false; }
        if (trace.empty()) trace="-";CHECK(HistoryPoints(history)==row[9] && history.index==Number(row[10]) && trace==row[11] && completed==(Number(row[12])!=0));++count;
    }
    CHECK(count==511 && count==fixture.rows.size());
}

//다른 사건/뗌/정지는 장부나 카메라를 요구하지 않고 물리 범위는 임의 modulo로 보정하지 않는다.
TEST_CASE(ConstructionHistoryKey_LazyGatesAndPartialFallback) {
    netstorm::o::ConstructionHistoryState history;history.index=0xffffffffU;std::int32_t cells=256;ConstructionHistoryKeyHooks hooks;
    for (const auto code:std::array{InputCode::kRelease|9U,InputCode::kOutside|9U,InputCode::kCommand|9U,0x90000U,std::uint32_t('C')}) CHECK(!DispatchConstructionHistoryKey({code,0,0},history,cells,hooks)); //정수 명령/가상Tab을 실제 문자로 오인하지 않는다.
    hooks.paused=[] { return true; };CHECK(DispatchConstructionHistoryKey({9,0,0},history,cells,hooks) && history.index==0xffffffffU);
    hooks.paused=[] { return false; };bool threw=false;try { static_cast<void>(DispatchConstructionHistoryKey({9,0,0},history,cells,hooks)); }catch (const std::out_of_range&) { threw=true; }CHECK(threw);
    InitializeConstructionHistory(history);history.index=3;history.points[0]={20,30};threw=false;try { static_cast<void>(DispatchConstructionHistoryKey({9,0,0},history,cells,hooks)); }catch (const std::logic_error&) { threw=true; }CHECK(threw && history.index==0); //카메라 미연결 이전의 첫 칸 복귀 쓰기는 보존한다.
}

//같은 Client owner의 주소와 기록은 일반 World 대여/반납 및 Renderer 초기화로 소실되지 않는다.
TEST_CASE(ConstructionHistoryKey_ClientOwnerAcrossWorldAndRenderer) {
    ClientGameplayGlobals gameplay;auto& history=gameplay.Camera().ConstructionHistory();InitializeConstructionHistory(history);history.index=4;history.points[0]={123.25F,145.5F};const auto* owner=&history;
    { auto use=gameplay.Acquire();CHECK(&gameplay.Camera().ConstructionHistory()==owner && history.index==4 && history.points[0].x==123.25F); }
    CHECK(history.index==4 && history.points[0].y==145.5F);std::uint32_t redraw{},revision{};gameplay.Camera().InitializeNativeRenderer(1024,768,256,redraw,revision);CHECK(&gameplay.Camera().ConstructionHistory()==owner && history.index==4 && history.points[0].x==123.25F);
}
