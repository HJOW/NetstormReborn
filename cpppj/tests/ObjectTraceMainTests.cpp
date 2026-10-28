// 원문 Main/Task의 현재 필드/모드/수치 인자와87개 실제 가상 분배를 비교한다.
#include "TestSupport.h"
#include "o/RawObjectTrace.h"
#include "o/RawObjectTraceMain.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 기대 출력은 원문 fixture에서만 읽는다. 장면은 입력과 실제 장치 기록만 보유한다.
using namespace netstorm::o;
// 모든 탭 열을 원문 순서로 분리한다.
std::vector<std::string> Columns(const std::string& line) {
    std::stringstream stream(line);std::string field;std::vector<std::string> result; // 현재 열과 원문 순서의 입력이다.
    while (std::getline(stream,field,'\t')) result.push_back(field); // 모든 열을 읽는다.
    return result;
}
// 문자열/원시 바이트를 인코딩 변환 없이16진수 열로 표현한다.
std::string Hex(std::string_view bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result; // BYTE값의 두 자리 표다.
    for (const unsigned char value:bytes) { result+=digits[value>>4];result+=digits[value&15]; } // 실제 바이트 전체를 보존한다.
    return result;
}
// 원문 little endian double의 전체 비트를16진수로 출력한다.
std::string DoubleHex(double value) { const auto bytes=std::bit_cast<std::array<char,8>>(value);return Hex({bytes.data(),bytes.size()}); }
// 원문50BYTE 풀/현재 TYPE·모드와 명시 로그 콜백 변경을 보유한다.
struct MainScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 같은 물리50BYTE 슬롯이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 TYPE 속성/이름 표다.
    std::uint32_t sid{123},suppress{},mutation{},calls{}; // 대상/현재 모드/명시 변경/로그 횟수다.
    bool task{}; // Main 또는Task 직접 원문 진입점의 선택이다.
    std::string events,arguments; // 실제 최종 로그 바이트와 수치 인자의 별도 기록이다.
    // 검사만 물리 raw 입력을 설치한다.
    std::span<std::uint8_t> Raw(std::uint32_t number) { const auto bytes=pool.Bytes();return std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size()).subspan(number*50,50); }
    // 지정 폭의 현재 원문 필드만 바꾼다.
    void Put(std::uint32_t number,std::size_t offset,std::uint32_t value,std::size_t width) {
        for (std::size_t i=0;i<width;++i) Raw(number)[offset+i]=static_cast<std::uint8_t>(value>>(i*8)); // 인접 필드는 보존한다.
    }
    // 기본 가상 분배 검사에서도 같은 현재 이름 표를 사용한다.
    MainScene() { types[82].name="assetName";types[46].name="Regular";types[6].name="ContentForm"; }
    // 독립 fixture의 입력만 설치하고 기대 열은 읽지 않는다.
    explicit MainScene(const std::vector<std::string>& columns):MainScene() {
        task=columns.at(0)=="Task";sid=static_cast<std::uint32_t>(std::stoul(columns.at(1)));suppress=static_cast<std::uint32_t>(std::stoul(columns.at(2)));
        types[82].flags1=static_cast<std::uint32_t>(std::stoul(columns.at(3)));types[82].flags2=static_cast<std::uint32_t>(std::stoul(columns.at(4)));mutation=static_cast<std::uint32_t>(std::stoul(columns.at(5)));
        const auto& raw=columns.at(6);CHECK(raw.size()==100);
        for (std::size_t i=0;i<50;++i) Raw(sid)[i]=static_cast<std::uint8_t>(std::stoul(raw.substr(i*2,2),nullptr,16)); // 원시 입력50BYTE를 그대로 설치한다.
    }
    // 원문과 같은 로그 반환 시점에만 다음 Main/Task의 현재 필드를 바꾼다.
    ObjectTraceMainLog Log() {
        return [this](std::int32_t level,std::string_view line,const ObjectTraceMainNumbers* numbers) {
            if (!events.empty()) events+=';';events+=std::to_string(level)+":"+Hex(line);
            if (numbers) arguments=DoubleHex(numbers->x)+DoubleHex(numbers->y)+":"+std::to_string(static_cast<std::uint32_t>(numbers->highlight))+","+std::to_string(numbers->quantity)+","+std::to_string(numbers->region)+","+std::to_string(static_cast<std::uint32_t>(numbers->z));
            ++calls;
            if (calls==1 && mutation==1) { Put(sid,40,255,1);Put(sid,26,12345,4);Put(sid,30,254,1);suppress=1;Put(sid,12,0x5106,2); }
            if (calls==2 && mutation==2) Put(sid,12,0x5104,2);
        };
    }
};
}

// 원문 분기/수치 인자와 외부CRT서식의373개 고유 입력을 비교한다.
TEST_CASE(ObjectTraceMain_IndependentNative373FieldsFlagsCoordinatesAndTask) {
    std::ifstream input(NETSTORM_OBJECT_TRACE_MAIN_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{}; // 실제 고유 입력을 센다.
    while (std::getline(input,line)) { // 두 정밀도가 일치한 모든 원문 입력을 실제 제품 함수로 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto columns=Columns(line);MainScene scene(columns);TraceRawObjectMain(scene.pool,scene.types,scene.suppress,scene.sid,scene.task,scene.Log());
        CHECK(scene.events+"|"+scene.arguments==columns.at(7));++rows;
    }
    CHECK(rows==373);
}

// PE에서 독립 추출한+70 주소에 따라 동일한 실제 본체가 호출되는지 모든87개 표를 확인한다.
TEST_CASE(ObjectTraceMain_IndependentNative87VirtualTablesAndUnknownRejection) {
    std::ifstream input(NETSTORM_OBJECT_TRACE_VTABLE_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{},tasks{}; // 실제 분배표와 Dude 예외 수다.
    while (std::getline(input,line)) { //83개자산과네내장Form 표를누락없이비교한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto columns=Columns(line);const auto table=static_cast<std::uint32_t>(std::stoul(columns.at(0),nullptr,16)),entry=static_cast<std::uint32_t>(std::stoul(columns.at(3),nullptr,16));
        MainScene actual,expected;actual.Put(123,0,table,4);expected.Put(123,0,table,4);actual.Raw(123)[10]=expected.Raw(123)[10]=82;
        TraceRawObjectVirtual(actual.pool,actual.types,actual.suppress,123,actual.Log());
        if (entry==0x4b0fb0U || entry==0x450d60U) {
            CHECK(columns.at(1)=="004ad000" && columns.at(2)=="004ad010");const bool task=entry==0x450d60U;tasks+=task;
            TraceRawObjectMain(expected.pool,expected.types,expected.suppress,123,task,expected.Log());
        }
        else {
            ObjectTraceFieldKind kind{}; // 원문 본체 주소가 지정하는 네 독립 파생 서식이다.
            if (entry==0x4ae1e0U) kind=ObjectTraceFieldKind::Form;else if (entry==0x4ae3f0U) kind=ObjectTraceFieldKind::Process;
            else if (entry==0x4ae530U) kind=ObjectTraceFieldKind::Depend;else { CHECK(entry==0x4aea90U);kind=ObjectTraceFieldKind::Content; }
            TraceRawObjectFields(expected.pool,expected.types,expected.suppress,123,kind,[&expected](auto level,auto text) { expected.Log()(level,text,nullptr); });
        }
        CHECK(actual.events==expected.events && actual.arguments==expected.arguments);++rows;
    }
    CHECK(rows==87 && tasks==1);MainScene unknown;bool threw=false;
    try { TraceRawObjectVirtual(unknown.pool,unknown.types,unknown.suppress,123,unknown.Log()); }catch (const std::logic_error&) { threw=true; }
    CHECK(threw && unknown.calls==0);
}

// 0/음의0·반올림동률·전지수·NaN/INF·고정난수의887개float32를원본CRT바이트와비교한다.
TEST_CASE(ObjectTraceMain_IndependentOriginalCrt887CoordinateFormats) {
    std::ifstream input(NETSTORM_OBJECT_TRACE_COORDINATE_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{}; // 원문CRT의실제고유입력을센다.
    while (std::getline(input,line)) { // 전체비트입력을제품의원본호환실수서식에전달한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto columns=Columns(line);const auto bits=static_cast<std::uint32_t>(std::stoul(columns.at(0),nullptr,16));
        const auto actual=Hex(FormatOriginalTraceCoordinate(std::bit_cast<float>(bits)));
        if (actual!=columns.at(1)) std::printf("좌표 진단 불일치 bits=%08x actual=%s expected=%s\n",bits,actual.c_str(),columns.at(1).c_str());
        CHECK(actual==columns.at(1));++rows;
    }
    CHECK(rows==887);
}

// 실제 같은 풀에서 재귀→현재가상분배→공통/파생로그의 전체 경계가 연결되는지 확인한다.
TEST_CASE(ObjectTraceMain_RecursiveVirtualDispatchSharesCurrentPool) {
    MainScene scene;
    for (const auto sid:{10U,11U,12U}) std::fill(scene.Raw(sid).begin(),scene.Raw(sid).end(),std::uint8_t{}); // free 슬롯의 초기 연결/상태를 지운 명시 live 입력을 설치한다.
    scene.Raw(10)[10]=82;scene.Put(10,0,0x501dd0,4);scene.Put(10,6,11,2);
    scene.Raw(11)[10]=46;scene.Put(11,0,0x512098,4);scene.Put(11,4,12,2);scene.Put(11,18,999,4);
    scene.Raw(12)[10]=6;scene.Put(12,0,0x512208,4);scene.Put(12,18,82,2);
    std::uint32_t depth{},indent{},count=24000,checking{},metamap{};bool boss=true; // 동일 재귀에서 빌리는 현재 전역이다.
    ObjectTraceHooks hooks;hooks.trace=[&scene](Sid sid) { TraceRawObjectVirtual(scene.pool,scene.types,scene.suppress,sid.value,scene.Log()); };
    hooks.indent=[&indent](auto value) { indent=value; };hooks.log=[&scene](auto level,auto text) { scene.Log()(level,text,nullptr); };
    TraceRawObject(scene.pool,{depth,count,checking,metamap,boss},hooks,10);
    CHECK(depth==0 && indent==0 && scene.calls==7 && !scene.arguments.empty());
    CHECK(scene.events.ends_with("29:"+Hex("~1~yEND 0010\n")));
}
