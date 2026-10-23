// 내용물 저장의 원본 PE 관측과 재귀별 외부 장치 수명을 대조한다.
#include "TestSupport.h"
#include "o/RawFortContentSave.h"
#include <array>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
using namespace netstorm::o;
namespace {
// 마지막 빈 필드까지 지정 구분자로 나눈다.
std::vector<std::string> Split(std::string_view source,char separator) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { const auto end=source.find(separator,start);result.emplace_back(source.substr(start,end==source.npos ? source.npos : end-start));if (end==source.npos) return result;start=end+1; } // 원문 경계만 찾아 순서대로 보관한다.
}
// 실제 raw 필드의 little endian 값을 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } // 지정 폭만 조합한다.
// 원시 장치의 지정 폭만 덮어쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } // 인접 필드를 보존한다.
// 두 물리 버퍼의 실제 자료를 독립 TSV와 같은 소문자 hex로 만든다.
std::string Hex(std::span<const std::uint8_t> bytes) { constexpr std::string_view digits="0123456789abcdef";std::string result;for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; } // 한 BYTE당 두 자리를 쓴다.
// 명시 중단 경계만 나타내며 원본의 SEH unwind를 흉내내지 않는다.
struct Stop {};
// 독립 PE에 공급한 장치 입력만 소유한다. 직렬화 결과는 제품 본체가 계산한다.
struct Devices {
    // 한 Begin 호출의 독립 검색 위치다. 후보열과 현재 후보는 callback으로 따로 바뀔 수 있다.
    struct Cursor { std::uint32_t parent{},current{};std::size_t index{};std::vector<std::uint32_t> sequence; }; // 고정 부모/현재/다음 위치/외부 후보열이다.
    std::uint32_t parent{},limit{}; // 진입 부모와 현재 Pool 물리 개수다.
    std::array<std::uint8_t,16> stream{};std::array<std::array<std::uint8_t,16384>,2> buffers{}; // 실제 stream과 data 변경을 구별하는 두 물리 버퍼다.
    std::array<std::array<std::uint8_t,50>,16> forms{};std::map<std::uint32_t,std::uint32_t> robust; // 원시 Form 필드와 bool로 축약하지 않은 가상60 반환 DWORD다.
    std::map<std::uint32_t,FortContentSaveType> types;std::map<std::uint32_t,std::vector<std::uint32_t>> children; // 외부 타입/후보열 장치다.
    std::vector<std::uint32_t> formOrder,typeOrder;std::vector<std::vector<std::string>> mutations; // 입력의 출력 순서와 명시 callback 변경이다.
    std::map<std::string,std::size_t> calls;std::weak_ptr<Cursor> current;std::string trace; // 현재 경계 횟수/활성 검색기/실제 외부 호출 기록이다.
    // 외부 경계를 기록하고 명시 입력 변경만 순서대로 적용한다.
    void Boundary(std::string_view name,std::string text) {
        if (!trace.empty()) trace+=';';trace+=text;const auto event=std::string(name)+std::to_string(++calls[std::string(name)]);
        for (const auto& mutation:mutations) { // 같은 경계의 여러 입력 효과는 작성 순서대로 적용한다.
            if (mutation[0]!=event) continue;const auto& target=mutation[1];const auto& field=mutation[2];const auto value=static_cast<std::uint32_t>(std::stoul(mutation[3]));
            if (target.starts_with("form")) { const auto sid=std::stoul(target.substr(4));if (field=="robust") robust[sid]=value;else Put(forms.at(sid),field=="kind" ? 18 : field=="quantity" ? 12 : 20,value,2); }
            else if (target.starts_with("type")) { auto& type=types.at(std::stoul(target.substr(4)));if (field=="flags1") type.flags1=value;else if (field=="content") type.contentListFlags=value;else type.containerListFlags=value; }
            else if (target=="stream") Put(stream,field=="data" ? 0 : field=="position" ? 4 : field=="end" ? 8 : 12,value);
            else if (target=="global") { CHECK(field=="poollimit");limit=value; }
            else if (target=="finder") { CHECK(field=="current");current.lock()->current=value; }
            else { CHECK(target=="control" && field=="stop" && value==1);throw Stop{}; }
        }
    }
    // 원문이 호출하는 외부 장치만 연결한다. 재귀/형식/쓰기는 제품 구현이 실행한다.
    FortContentSaveHooks Hooks() {
        FortContentSaveHooks hooks;hooks.begin=[this](auto selected) {
            auto cursor=std::make_shared<Cursor>();cursor->parent=selected;cursor->sequence=children[selected];cursor->current=cursor->sequence.empty() ? 0 : cursor->sequence[0];current=cursor;Boundary("begin","begin:"+std::to_string(selected));
            return FortContentSaveCursor{[this,cursor] { current=cursor;return cursor->current; },[this,cursor] { current=cursor;const auto before=cursor->current;++cursor->index;cursor->current=cursor->index<cursor->sequence.size() ? cursor->sequence[cursor->index] : 0;Boundary("next","next:"+std::to_string(cursor->parent)+':'+std::to_string(before)); }};
        };
        hooks.poolLimit=[this] { return limit; };hooks.form=[this](auto sid)->std::span<const std::uint8_t> { return forms.at(sid); };
        hooks.robust=[this](auto sid) { const auto answer=robust[sid];Boundary("robust","robust:"+std::to_string(sid));return answer!=0; };
        hooks.type=[this](auto kind) { Boundary("type","type:"+std::to_string(kind));return types.at(kind); };
        hooks.parentType=[this](auto selected) { Boundary("parent","parent:"+std::to_string(selected));return types.at(Read(forms.at(selected),18,2)); }; // 독립 ParentType 장치는 지정 raw kind를 경계 변경 뒤 읽는다.
        hooks.assertion=[this](auto kind) { Boundary("assert","assert:"+std::to_string(kind==FortContentSaveAssertion::WriteCapacity ? 36 : 278)); };
        hooks.access=[this](auto token,const auto& operation) { CHECK(token==1);MemStreamHooks memory;memory.buffer=[this](auto data)->std::span<std::uint8_t> { return buffers.at(data); };operation(stream,memory); };return hooks;
    }
    // TSV 앞10열의 장치 입력을 그대로 배치한다.
    void Initialize(const std::vector<std::string>& row) {
        parent=std::stoul(row[1]);limit=std::stoul(row[2]);Put(stream,4,std::stoul(row[3]));Put(stream,8,std::stoul(row[4]));Put(stream,12,std::stoul(row[5]));
        for (auto& buffer:buffers) buffer.fill(0xcc); // 원본 관측과 같은 초기 물리 자료다.
        if (row[6]!="-") for (const auto& record:Split(row[6],';')) { const auto fields=Split(record,':');const auto sid=static_cast<std::uint32_t>(std::stoul(fields[0]));formOrder.push_back(sid);Put(forms.at(sid),18,std::stoul(fields[1]),2);Put(forms.at(sid),12,std::stoul(fields[2]),2);Put(forms.at(sid),20,std::stoul(fields[3]),2);robust[sid]=std::stoul(fields[4]); } // 모든 Form 입력을 실제 폭으로 배치한다.
        if (row[7]!="-") for (const auto& record:Split(row[7],';')) { const auto fields=Split(record,':');const auto kind=static_cast<std::uint32_t>(std::stoul(fields[0]));typeOrder.push_back(kind);types[kind]={static_cast<std::uint32_t>(std::stoul(fields[1])),static_cast<std::uint32_t>(std::stoul(fields[2])),static_cast<std::uint32_t>(std::stoul(fields[3]))}; } // 외부 타입 세 DWORD를 배치한다.
        if (row[8]!="-") for (const auto& record:Split(row[8],';')) { const auto fields=Split(record,':');auto& sequence=children[std::stoul(fields[0])];if (fields[1]!="-") for (const auto& child:Split(fields[1],',')) sequence.push_back(std::stoul(child)); } // 후보 반복/경계 밖 SID도 제외하지 않는다.
        if (row[9]!="-") for (const auto& record:Split(row[9],';')) mutations.push_back(Split(record,'/')); // 명시 효과만 파싱한다.
    }
    // 현재 Form 필드와 외부 Robust 값을 입력 순서대로 출력한다.
    std::string FormOutput() { std::string result;for (const auto sid:formOrder) { if (!result.empty()) result+=';';result+=std::to_string(sid)+':'+std::to_string(Read(forms[sid],18,2))+':'+std::to_string(Read(forms[sid],12,2))+':'+std::to_string(Read(forms[sid],20,2))+':'+std::to_string(robust[sid]); }return result; } // 바뀌지 않은 인접 필드도 함께 대조한다.
    // 현재 타입 세 DWORD를 입력 순서대로 출력한다.
    std::string TypeOutput() { std::string result;for (const auto kind:typeOrder) { const auto& type=types.at(kind);if (!result.empty()) result+=';';result+=std::to_string(kind)+':'+std::to_string(type.flags1)+':'+std::to_string(type.contentListFlags)+':'+std::to_string(type.containerListFlags); }return result; } // callback 변경 결과를 포함한다.
};
// 정상 저장으로 진행할 수 없는 물리/장치 계약 위반을 확인한다.
template<class Function> bool Rejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// 실제 원문이 만든 전체 출력·경계 순서·부분 쓰기를 제품 결과와 비교한다.
TEST_CASE(FortContentSave_IndependentNative) {
    std::ifstream input(NETSTORM_FORT_CONTENT_SAVE_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 앞10열만 입력이며 뒤12열은 독립 PE 결과다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=Split(line,'\t');CHECK(row.size()==22);if (row.size()!=22) continue;Devices d;d.Initialize(row);bool completed=true;const auto failures=netstorm::test::FailureCount();
        try { SaveFortContents(d.parent,1,d.Hooks()); } catch (const Stop&) { completed=false; }
        CHECK(completed==(row[10]=="1"));CHECK(Read(d.stream,4)==std::stoul(row[11]));CHECK(Read(d.stream,8)==std::stoul(row[12]));CHECK(Read(d.stream,0)==std::stoul(row[13]));
        CHECK(Hex(std::span(d.buffers[0]).first(Read(d.stream,8)))==row[14]);CHECK(Hex(std::span(d.buffers[1]).first(Read(d.stream,8)))==row[15]);CHECK(d.trace==row[16]);
        CHECK(completed ? std::stoi(row[17])==0 : std::stoi(row[17])<0);CHECK(d.limit==std::stoul(row[18]));CHECK(d.FormOutput()==row[19]);CHECK(d.TypeOutput()==row[20]);CHECK(std::stoul(row[21])>0);++compared;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"내용물 관측 입력: %s\n",row[0].c_str());
    }CHECK(compared==706);
}
// 빈 목록은 헤더만 쓰며 선택하지 않은 외부 장치를 요구하지 않는다. 순환은 성공으로 처리하지 않는다.
TEST_CASE(FortContentSave_EmptyMissingDevicesAndCycle) {
    Devices d;d.limit=8;Put(d.stream,12,0);auto hooks=d.Hooks();hooks.robust={};hooks.type={};hooks.form={};hooks.parentType={};hooks.assertion={};hooks.poolLimit={};SaveFortContents(1,1,hooks);
    CHECK(Read(d.stream,4)==1 && Read(d.stream,8)==1 && d.buffers[0][0]==0 && d.trace=="begin:1");
    Devices cycle;cycle.limit=8;Put(cycle.stream,12,4096);cycle.children[1]={2};cycle.children[2]={1};cycle.robust[1]=true;cycle.robust[2]=true;Put(cycle.forms[1],18,71,2);Put(cycle.forms[2],18,71,2);cycle.types[71]={0x20000,0,0};CHECK(Rejects([&] { SaveFortContents(1,1,cycle.Hooks()); }));CHECK(Read(cycle.stream,4)>1);
    Devices missing;missing.limit=8;missing.children[1]={2};auto absent=missing.Hooks();absent.robust={};CHECK(Rejects([&] { SaveFortContents(1,1,absent); }));CHECK(missing.buffers[0][0]==0 && Read(missing.stream,4)==1);
}
