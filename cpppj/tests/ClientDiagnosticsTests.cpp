// 원문167입력/두 정밀도334관측의 개발자 진단 호출과 현재 물리/모드 참조를 검사한다.
#include "TestSupport.h"
#include "client/ClientDiagnostics.h"
#include "client/ClientMain.h"
#include "o/RawObjectTrace.h"
#include "o/RawGameAuxState.h"
#include <fstream>
#include <sstream>
#include <vector>

namespace { // 기대 호출열은 기존 독립 원문 fixture에서만 읽는다.
using namespace netstorm::client;using namespace netstorm::o; // 같은 실제 입력/풀/로그 타입을 사용한다.
std::vector<std::string> DiagnosticColumns(const std::string& line,char separator) { // 탭과 사건 구분자의 모든 원시 필드를 순서대로 읽는다.
    std::stringstream stream(line);std::string field;std::vector<std::string> result;while (std::getline(stream,field,separator)) result.push_back(field);return result;
}
}

TEST_CASE(ClientDiagnostics_Native167KeyInputs) {
    std::ifstream input(NETSTORM_SCREENSHOT_KEY_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{},diagnostics{};
    while (std::getline(input,line)) { // 같은167원문행의 상위 수정키·뗌·선행 소비·정지를 모두 유지한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;
        const auto row=DiagnosticColumns(line,'\t');CHECK(row.size()==20);if (row.size()!=20) continue;++rows;
        const auto fields=DiagnosticColumns(row[1],':');const auto code=static_cast<std::uint32_t>(std::stoull(fields.at(0)));std::size_t calls{};
        if (row[2]=="0" && row[3]=="0") static_cast<void>(DispatchDiagnosticKey({code,17,23},[&] { ++calls; })); // 실제 Guide/Gump 소비 뒤의 같은 위치만 입력을 받는다.
        const bool expected=row[16].find("diagnostic:0x4437f0;diagnostic:0x4d6020;diagnostic:0x471890")!=std::string::npos;
        CHECK(calls==std::size_t(expected));diagnostics+=calls;
    }
    CHECK(rows==167 && diagnostics==16);
}

TEST_CASE(ClientDiagnostics_CurrentPhysicalCountAndBooleanOwners) {
    SidPool pool{OriginalEdition::Patch1078,24000,true};bool checking=false,metamap=true,boss=false;std::uint32_t depth{},word=7;RawGameAuxState aux;
    ObjectTraceGlobals globals{depth,pool.CapacityValue(),checking,metamap,boss};ObjectTraceFlag native{word};
    CHECK(&globals.count==&pool.CapacityValue());CHECK(globals.count==24000 && !globals.checkingDead && globals.metamap && native);
    pool.Reset(30000);checking=true;metamap=false;word=0;CHECK(globals.count==30000 && globals.checkingDead && !globals.metamap && !native);
    const auto* terrain=&aux.TerrainType();const auto* farm=&aux.EdgeFarmType();CHECK(*terrain==97 && *farm==121);aux.TerrainType()=102;aux.EdgeFarmType()=122;CHECK(*terrain==102 && *farm==122);
}

TEST_CASE(ClientDiagnostics_ConfigChangeUsesSameClientLogLevel) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";Client client{options}; // 장치/창 없이 실제 Config/Client owner만 구성한다.
    auto* state=&client.DiagnosticGlobals();CHECK(state->log.level==29 && state->depth==0 && state->suppress==0);
    client.TraceObject(0);CHECK(state->log.stream==0 && state->depth==0); // 첫 자산/Pool/창이 없어도 원문SID0은 장치 없이 복귀한다.
    client.Configuration().SetInt("tracelevel",7);CHECK(&client.DiagnosticGlobals()==state && state->log.level==7);
    state->log.level=30;client.Configuration().SetInt("tracelevel",11);CHECK(state->log.level==11); // 외부 효과 중 명시 설정 변경도 같은 현재 임시 모드 주소를 갱신한다.
}
