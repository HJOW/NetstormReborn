// 원문45사례의 전체 callback 순서와 중단 뒤 두 Config 부분효과를 비교한다.
#include "TestSupport.h"
#include "o/RawNewTechnology.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>

namespace { // 원문 경계의 독립 응답만 주고 제품 함수가 순서·조건을 선택하게 한다.
using namespace netstorm::o; // 실제 기술 획득 부모와 같은 판본을 사용한다.
// 빈 마지막 필드도 보존하는 최초 TSV의 분리다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 원래 필드 경계를 유지한다.
}
// 원시 byte를 Native fixture와 같은 소문자16진 문자열로 표현한다.
std::string Hex(std::string_view value) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 같은 원래 BYTE를 UTF-8 변환하지 않는다.
    for (const unsigned char byte:value) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; // 첫NUL도 입력에서는 보존한다.
}
// '-'는 입력 빈 문자열이며 출력 필드의 빈 값과 구별한다.
std::string Bytes(std::string_view value) {
    std::string result;if (value=="-") return result;
    for (std::size_t index=0;index<value.size();index+=2) result+=static_cast<char>(std::stoul(std::string(value.substr(index,2)),nullptr,16));return result; // 원시 바이트를 복구한다.
}
struct Stop {}; // 지정한 callback 진입 뒤 실제 원문 prefix에서 중단한다.
struct Probe { // 게임 계산 대신 원문 관측의 반환값과 문자열만 공급한다.
    std::uint32_t found{},stop{},parents{},events{};std::string caption,trace; // 명시 입력과 현재 호출 순서다.
    std::map<std::string,std::string> config; // 최초 원문과 비교할 실제 부분 설정이다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value;++events;if (stop && events==stop) throw Stop{}; } // 효과보다 먼저 callback 진입을 기록한다.
    NewTechnologyHooks Hooks() { // 부모가 선택한 실제 외부 효과만 노출한다.
        NewTechnologyHooks h;
        h.technologyParent=[this] { const auto value=std::array<std::uint32_t,3>{10,20,30}[parents<3 ? parents : 2];++parents;Event("G,"+std::to_string(value));return value; };
        h.find=[this](auto parent,auto mask,auto kind,auto index) { Event("Q,"+std::to_string(parent)+','+std::to_string(mask)+','+std::to_string(kind)+','+std::to_string(index));return found; };
        h.create=[this](auto parent,auto kind,auto argument,auto flags,auto attach,auto quality) { Event("I,"+std::to_string(parent)+','+std::to_string(kind)+','+std::to_string(argument)+','+std::to_string(flags)+','+std::to_string(attach)+','+std::to_string(quality)); };
        h.snapshot=[this] { Event("S"); };h.type=[this](auto type) { Event("T,"+std::to_string(type));return 0x12345678U; };
        h.name=[this](auto record) { Event("N,"+std::to_string(record));return caption; };
        h.configure=[this](auto key,auto value) { Event("C,"+Hex(key)+','+Hex(value));config[std::string(key)]=value; };
        h.format=[this](auto pattern,std::int32_t value) { Event("F,"+Hex(pattern)+','+std::to_string(value));std::array<char,256> bytes{};const std::string format(pattern);std::snprintf(bytes.data(),bytes.size(),format.c_str(),value);return std::string(bytes.data()); };
        h.tell=[this](auto key) { Event("L,"+Hex(key)); };return h;
    }
};
}

// 같은 최초 입력과 명시 중단으로 원문에서 동결한45개 전체 순서/두 Config 결과를 비교한다.
TEST_CASE(NewTechnology_Native45CurrentParentDuplicateAndPartialEffects) {
    std::ifstream input(NETSTORM_NEW_TECHNOLOGY_FIXTURE);CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { // 각 원문 관측에 새 장치를 제공해 앞 사례의 상태를 섞지 않는다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto fields=Fields(line);CHECK(fields.size()==8);if (fields.size()!=8) return;
        Probe probe;probe.found=static_cast<std::uint32_t>(std::stoul(fields[1]));probe.caption=Bytes(fields[2]);probe.stop=static_cast<std::uint32_t>(std::stoul(fields[3]));bool completed=true;
        try { AcquireNewTechnology(OriginalEdition::Patch1078,static_cast<std::uint32_t>(std::stoul(fields[0])),probe.Hooks()); }catch (const Stop&) { completed=false; }
        CHECK(completed==(fields[4]=="1"));CHECK(probe.trace==fields[5]);
        for (const auto& [key,column]:std::array<std::pair<std::string_view,std::size_t>,2>{{{"global.newTechName",6},{"global.newTechPic",7}}}) { // 미작성과 작성한 빈 값을 별개로 비교한다.
            const auto it=probe.config.find(std::string(key));CHECK((it==probe.config.end())==(fields[column]=="-"));if (it!=probe.config.end()) CHECK(Hex(it->second)==fields[column]);
        }
        ++count;
    }CHECK(count==45);
}

// 선택된 저장 장치 누락은 기술 추가까지의 부분효과를 유지하며 설정/안내 성공으로 바꾸지 않는다.
TEST_CASE(NewTechnology_MissingSnapshotPreservesPrefixAndRejectsCd) {
    Probe probe;probe.caption="Sun Cannon";auto hooks=probe.Hooks();hooks.snapshot={};bool missing{};
    try { AcquireNewTechnology(OriginalEdition::Patch1078,83,hooks); }catch (const std::logic_error&) { missing=true; }
    CHECK(missing && probe.trace=="G,10;G,20;Q,20,4,83,0;G,30;I,30,83,0,4,16,0" && probe.config.empty());
    probe.trace.clear();bool cd{};try { AcquireNewTechnology(OriginalEdition::Cd1072,83,probe.Hooks()); }catch (const std::invalid_argument&) { cd=true; }CHECK(cd && probe.trace.empty());
}
