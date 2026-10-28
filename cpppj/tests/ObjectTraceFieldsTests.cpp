// 원문 Base/내장Form의 모든 서식 바이트와 파생 필드 재조회 시점을 비교한다.
#include "TestSupport.h"
#include "o/RawObjectTraceFields.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 기대 출력은 고정 원문에서만 읽고 합성 입력/장치 기록을 별도로 만든다.
using namespace netstorm::o;
// fixture 한 행의 탭 구분 입력과 마지막 기대 열을 분리한다.
std::vector<std::string> Columns(const std::string& value) {
    std::stringstream stream(value);std::vector<std::string> result;std::string field; // 원문 순서의 현재 열이다.
    while (std::getline(stream,field,'\t')) result.push_back(field); // 모든 입력/기대 열을 빠짐없이 읽는다.
    return result;
}
// %s 바이트의 이름/슬롯을16진수 열에서 복원한다. '-'는 빈 이름의 명시 입력이다.
std::string Unhex(const std::string& text) {
    if (text=="-") return {};std::string bytes; // NUL도 그대로 담는 입력 바이트다.
    for (std::size_t i=0;i<text.size();i+=2) bytes+=static_cast<char>(std::stoul(text.substr(i,2),nullptr,16)); // 두 자리씩만 원시 바이트로 바꾼다.
    return bytes;
}
// 로그 장치의 최종 바이트를 인코딩 변환 없이16진수로 기록한다.
std::string Hex(std::string_view bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result; // 낮은8비트 값의 두 자리 표다.
    for (const unsigned char value:bytes) { result+=digits[value>>4];result+=digits[value&15]; } // 실제 출력 바이트의 순서를 유지한다.
    return result;
}
// 명시 진입점 이름을 제품의 다섯 원문 본체 선택으로 바꾼다.
ObjectTraceFieldKind Kind(const std::string& name) {
    if (name=="Base") return ObjectTraceFieldKind::Base;if (name=="Form") return ObjectTraceFieldKind::Form;
    if (name=="Process") return ObjectTraceFieldKind::Process;if (name=="Depend") return ObjectTraceFieldKind::Depend;
    if (name=="Content") return ObjectTraceFieldKind::Content;throw std::logic_error("검사 진입 본체 이름 오류");
}
// 원문 도구와 같은 입력 슬롯/이름/콜백 변경을 구성하고 기대 계산은 하지 않는다.
struct FieldsScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원본50BYTE의 물리 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 전체TYPE이름 표다.
    ObjectTraceFieldKind kind{}; // fixture가 지정한 직접 원문 진입점이다.
    std::uint32_t sid{},suppress{},mutation{},calls{}; // 현재 슬롯/덤프 모드/명시 변경/로그 횟수다.
    std::string events; // 실제 장치가 받은 level/서식완료 바이트다.
    // 검사만 물리 슬롯의 raw 바이트를 설치한다.
    std::span<std::uint8_t> Raw() { const auto bytes=pool.Bytes();return std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size()).subspan(sid*50,50); }
    // 지정한 원문 물리 폭만 변경한다.
    void Put(std::size_t offset,std::uint32_t value,std::size_t width) {
        for (std::size_t i=0;i<width;++i) Raw()[offset+i]=static_cast<std::uint8_t>(value>>(i*8)); // 인접 필드는 보존한다.
    }
    // 모든 입력 열을 설치하며 원문 출력 열은 읽지 않는다.
    explicit FieldsScene(const std::vector<std::string>& columns) {
        kind=Kind(columns.at(0));sid=static_cast<std::uint32_t>(std::stoul(columns.at(1)));suppress=static_cast<std::uint32_t>(std::stoul(columns.at(2)));
        mutation=static_cast<std::uint32_t>(std::stoul(columns.at(3)));const auto name=Unhex(columns.at(4)),raw=Unhex(columns.at(5));
        CHECK(raw.size()==50);std::copy(raw.begin(),raw.end(),Raw().begin());types[46].name=name;types[82].name=name;types[94].name="islandAfterCallback";
    }
    // 첫 공통 로그 반환 때 현재 파생 필드를 바꿔 캐시된 사본 사용을 검출한다.
    ObjectTraceFieldLog Log() {
        return [this](std::int32_t level,std::string_view bytes) {
            if (!events.empty()) events+=';';events+=std::to_string(level)+":"+Hex(bytes);
            if (++calls==1 && mutation) { Put(14,65535,2);Put(16,4321,2);Put(18,94,4);Put(12,65534,2);Put(22,0,2); }
        };
    }
};
}

// 상태16조합/모드3종/이름/정수경계/현재콜백의308개독립원문출력과완전히같아야한다.
TEST_CASE(ObjectTraceFields_IndependentNative308BaseAndFourForms) {
    std::ifstream input(NETSTORM_OBJECT_TRACE_FIELDS_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{}; // 원문 전체 행을 센다.
    while (std::getline(input,line)) { // 두 정밀도가 같은 모든 고유 입력을 제품 함수에 적용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto columns=Columns(line);FieldsScene scene(columns);
        TraceRawObjectFields(scene.pool,scene.types,scene.suppress,scene.sid,scene.kind,scene.Log());CHECK(scene.events==columns.at(6));++rows;
    }
    CHECK(rows==308);
}

// 잘못된TYPE/미정의CRT/임시버퍼초과를거부해도앞선Base로그는보존한다.
TEST_CASE(ObjectTraceFields_HostBoundariesKeepBaseOutputAndRejectMissingLog) {
    const std::string raw="000000000000000000002e000000000000005200000000000000000000000000000000000000000000000000000000000000"; // TYPE46/ContentTYPE82의50BYTE입력이다.
    FieldsScene scene(Columns("Content\t123\t0\t0\t41\t"+raw));scene.types[82].name="%s/%s";bool threw=false;
    try { TraceRawObjectFields(scene.pool,scene.types,scene.suppress,scene.sid,scene.kind,scene.Log()); }catch (const std::logic_error&) { threw=true; }
    CHECK(threw && scene.calls==1);
    FieldsScene large(Columns("Content\t123\t0\t0\t41\t"+raw));large.types[82].name=std::string(80,'x');threw=false;
    try { TraceRawObjectFields(large.pool,large.types,large.suppress,large.sid,large.kind,large.Log()); }catch (const std::length_error&) { threw=true; }
    CHECK(threw && large.calls==1);
    FieldsScene absent(Columns("Base\t123\t0\t0\t41\t"+raw));threw=false;
    try { TraceRawObjectFields(absent.pool,absent.types,absent.suppress,absent.sid,absent.kind,{}); }catch (const std::logic_error&) { threw=true; }
    CHECK(threw && absent.calls==0);
}
