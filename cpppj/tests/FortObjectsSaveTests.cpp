// 실제004be430의714관측을현재manager/stream/명시하위trace와대조한다.
#include "TestSupport.h"
#include "o/RawFortObjectsSave.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
constexpr std::uint32_t kObjectsBufferBase=0x12008000U; // 최초x86입력의가상버퍼주소이며호스트포인터로역참조하지않는다.
// 마지막 빈 칸도 유지하는 독립 fixture의 필드 분리다.
std::vector<std::string> ObjectsFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{};
    // 각 구분자 뒤 원문을 순서대로 읽는다.
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; }
}
// little endian DWORD 네 BYTE만 읽는다.
std::uint32_t ObjectsGet(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 낮은 BYTE부터 원본 DWORD를 조립한다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value;
}
// 지정 DWORD만 써 주변 raw 필드를 보존한다.
void ObjectsPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 원본 네 BYTE 순서다.
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 전체 stream/물리 출력의 모든 BYTE를 x86과 같은 형식으로 직렬화한다.
std::string ObjectsHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 가상 메모리 관측의 고정 소문자 표현이다.
    // 인접 두 글자가 하나의 실제 BYTE다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result;
}
// 원문 emu_stop에 대응하는 지정 외부 경계의 중단 표식이다.
struct ObjectsStop {};
// 부모의 조건을 대신 계산하지 않고 최초 독립 입력의 하위 응답만 공급한다.
struct ObjectsDevices {
    std::int32_t slot{},region{-1}; // 고정 this 슬롯과 외부 Temp 생성 응답의 순번이다.
    std::uint32_t manager{},mine{},fort{},pattern{},position{},end{},capacity{},version{0xaabbccddU}; // 독립 입력과 부모가 직접쓰는현재version이다.
    std::array<std::array<std::uint8_t,16>,21> nodes{}; // 부모가 직접읽고쓰는21개실제stream헤더다.
    std::array<std::array<std::uint8_t,64>,21> output{}; // 각stream의물리64BYTE출력과주변cc바이트다.
    std::map<std::uint32_t,std::uint32_t> currents{{0,90},{11,91},{22,92}}; // 명시하위manager의현재선택입력이다.
    std::map<std::string,std::uint32_t> calls; // 각 실제 하위 진입의1기반 횟수다.
    std::uint32_t active{},temps{}; // 동기access의현재stream과명시Temp생성횟수다.
    std::string mutation,trace; // 지정callback효과와관측된전체순서다.
    // fixture의입력열만읽어상태를구성하며관측열은읽지않는다.
    explicit ObjectsDevices(std::string_view input) {
        const auto fields=ObjectsFields(input,',');if (fields.size()!=9) throw std::runtime_error("Objects 부모 입력 칸 수 오류");
        slot=std::stoi(fields[0]);manager=std::stoul(fields[1]);mine=std::stoul(fields[2]);fort=std::stoul(fields[3]);pattern=std::stoul(fields[4]);position=std::stoul(fields[5]);end=std::stoul(fields[6]);capacity=std::stoul(fields[7]);mutation=fields[8];
        // 미사용노드/출력도x86최초입력과동일하게초기화한다.
        for (std::uint32_t token=1;token<=21;++token) { auto& node=nodes[token-1];ObjectsPut(node,0,kObjectsBufferBase+token*64);ObjectsPut(node,12,64);output[token-1].fill(0xcc); }
    }
    // 실제현재fort는최종분기에서만getter로읽어하위callback뒤의변경을보존한다.
    FortObjectsSaveGlobals Globals() { return {mine,version,[this] { return fort; }}; }
    // 원본 하위 CALL의 실제순서와 횟수만 기록한다.
    std::uint32_t Record(std::string_view kind,std::string text) { if (!trace.empty()) trace+=';';trace+=text;return ++calls[std::string(kind)]; }
    // 지정된한callback의원시효과만적용한다.다음branch/manager조회는제품몸체가결정한다.
    void Mutate(std::string_view boundary) {
        if (mutation=="assert-manager22" && boundary=="assert") manager=22;
        else if (mutation==std::string(boundary)+"-manager22") manager=22;
        else if (mutation=="temp1-version171" && boundary=="temp1") version=0x123456abU;
        else if (mutation=="territory1-version127" && boundary=="territory1") version=0xff00007fU;
        else if (mutation=="territory20-fort1" && boundary=="territory20") fort=1;
        else if (mutation=="rename1-fort0" && boundary=="rename1") fort=0;
        else if (mutation=="chaff-version255" && boundary=="chaff") version=0x765432ffU;
        else if (mutation=="capacity-current" && boundary=="capacity") { auto& node=nodes.at(active-1);ObjectsPut(node,4,5);ObjectsPut(node,8,2);ObjectsPut(node,12,64);version=0x987654eeU; }
        else if (mutation==std::string(boundary)+"-stop") throw ObjectsStop{};
    }
    // 영역/Chaff하위가명시한1BYTE만현재stream에쓰며부모version쓰기는대신하지않는다.
    void Append(std::uint32_t token,std::uint8_t value) {
        auto& node=nodes.at(token-1);const auto pointer=ObjectsGet(node,0),pos=ObjectsGet(node,4);const auto target=(pointer-kObjectsBufferBase)/64;
        if (pointer<kObjectsBufferBase+64 || target>21 || pos>=64) throw std::out_of_range("Objects 명시 하위 출력 범위");output.at(target-1)[pos]=value;ObjectsPut(node,4,pos+1);
        if (std::bit_cast<std::int32_t>(ObjectsGet(node,8))<static_cast<std::int32_t>(pos+1)) ObjectsPut(node,8,pos+1);
    }
    // 실제부모와동일한입력/하위응답만구성하며DataManager노드알고리즘은복제하지않는다.
    FortObjectsSaveHooks Hooks() {
        FortObjectsSaveHooks hooks;
        hooks.manager=[this](std::int32_t selected) { CHECK(selected==slot);return manager; };
        hooks.assertion=[this](FortObjectsSaveAssertion value) { const auto line=value==FortObjectsSaveAssertion::MissingTemplate ? 957 : value==FortObjectsSaveAssertion::NotMine ? 958 : 36;Record("assert","assert:"+std::to_string(line));Mutate(line==36 ? "capacity" : "assert"); };
        hooks.find=[this](std::uint32_t selected,std::string_view name) { const auto current=std::stoi(std::string(name.substr(4)));const bool found=pattern==1 || (pattern==2 && current%2==0);const auto index=Record("find","find:"+std::to_string(selected)+':'+std::string(name));currents[selected]=found ? static_cast<std::uint32_t>(current+1) : 0U;Mutate("find"+std::to_string(index));return found ? 1U : 0U; };
        hooks.create=[this](std::uint32_t selected,std::string_view name) {
            const auto index=Record("create","create:"+std::to_string(selected)+':'+std::string(name));std::uint32_t token{};
            if (name=="Temp") { ++region;token=static_cast<std::uint32_t>(region+1);++temps; }else token=name=="Chaff" ? 21U : static_cast<std::uint32_t>(std::stoi(std::string(name.substr(4)))+1);
            currents[selected]=token;
            if (name=="Temp" || name=="Chaff") { auto& node=nodes.at(token-1);ObjectsPut(node,0,kObjectsBufferBase+token*64);ObjectsPut(node,4,position);ObjectsPut(node,8,end);ObjectsPut(node,12,capacity); }
            Mutate("create"+std::to_string(index));if (name=="Temp") Mutate("temp"+std::to_string(temps));else if (name=="Chaff") Mutate("chaff");return token;
        };
        hooks.select=[this](std::uint32_t selected,std::string_view name,std::int32_t writable) { CHECK(name=="Temp" && writable==1);const auto token=static_cast<std::uint32_t>(region+1);const auto index=Record("select","select:"+std::to_string(selected)+":Temp:1");currents[selected]=token;if (pattern==2) { if (region%3==0) ObjectsPut(nodes.at(token-1),0,0);else if (region%3==1) ObjectsPut(nodes.at(token-1),8,0); }Mutate("select"+std::to_string(index));return token; };
        hooks.rename=[this](std::uint32_t selected,std::string_view destination,std::string_view source) { CHECK(source=="Temp");const auto index=Record("rename","rename:"+std::to_string(selected)+':'+std::string(destination)+':'+std::string(source));Mutate("rename"+std::to_string(index)); };
        hooks.territory=[this](std::uint32_t stream,std::uint32_t owner,std::int32_t current) { const auto index=Record("territory","territory:"+std::to_string(stream)+':'+std::to_string(owner)+':'+std::to_string(current));Append(stream,static_cast<std::uint8_t>(0x40+current));Mutate("territory"+std::to_string(index)); };
        hooks.chaff=[this](std::uint32_t stream) { Record("chaffbody","chaff:"+std::to_string(stream));Append(stream,0x7f); };
        hooks.access=[this](std::uint32_t token,const auto& operation) { active=token;MemStreamHooks memory;memory.buffer=[this](std::uint32_t pointer)->std::span<std::uint8_t> { if (pointer<kObjectsBufferBase+64 || (pointer-kObjectsBufferBase)%64) throw std::out_of_range("Objects 물리 버퍼 토큰");return output.at((pointer-kObjectsBufferBase)/64-1); };operation(nodes.at(token-1),memory); };return hooks;
    }
    // 부모실행후의현재전역과명시manager선택을원문관측형식으로읽는다.
    std::string State() const { return std::to_string(manager)+','+std::to_string(mine)+','+std::to_string(version)+','+std::to_string(fort)+','+std::to_string(currents.at(0))+','+std::to_string(currents.at(11))+','+std::to_string(currents.at(22)); }
    // 모든21stream과64BYTE전체출력을읽어부분쓰기/주변보존까지비교한다.
    std::string Raw() const {
        std::string result;
        // 생성되지않은노드도원래입력보존을확인한다.
        for (std::size_t index=0;index<nodes.size();++index) { if (!result.empty()) result+=':';result+=ObjectsHex(nodes[index])+ObjectsHex(output[index]); }return result;
    }
};
}
// 최초357고유입력의두정밀도714관측을정상반환/명시중단의모든출력과대조한다.
TEST_CASE(FortObjectsSave_IndependentNative714) {
    std::ifstream input(NETSTORM_FORT_OBJECTS_PARENT_FIXTURE);if (!input) throw std::runtime_error("Objects 부모 최초 x86 fixture 없음");std::string line;std::size_t compared{},stopped{};std::set<std::pair<std::string,std::string>> unique;
    // 기대값은제품본문을실행한뒤에만사용한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=ObjectsFields(line,'\t');if (row.size()!=6) throw std::runtime_error("Objects 부모 fixture 칸 수 오류");CHECK(unique.emplace(row[0],row[1]).second);
        ObjectsDevices devices(row[1]);bool completed=true;try { SaveFortObjects(devices.slot,devices.Globals(),devices.Hooks()); }catch (const ObjectsStop&) { completed=false;++stopped; }
        CHECK(completed==(row[2]=="1"));CHECK(devices.State()==row[3]);CHECK(devices.Raw()==row[4]);CHECK(devices.trace==row[5]);++compared;
    }
    CHECK(compared==714 && stopped==48);
}
// 도달하지않는Chaff경계는요구하지않고실제find누락은선행version쓰기뒤실패한다.
TEST_CASE(FortObjectsSave_ExplicitUnselectedAndMissingBoundary) {
    ObjectsDevices devices("1,11,1,0,0,0,0,64,none");auto hooks=devices.Hooks();hooks.chaff={};SaveFortObjects(1,devices.Globals(),hooks);CHECK(devices.temps==20 && devices.calls["chaffbody"]==0);
    ObjectsDevices missing("1,11,1,0,0,0,0,64,none");hooks=missing.Hooks();hooks.find={};bool rejected=false;try { SaveFortObjects(1,missing.Globals(),hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && missing.version==2 && missing.trace.empty());
}
