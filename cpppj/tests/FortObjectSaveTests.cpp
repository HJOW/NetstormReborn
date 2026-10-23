// 단일 객체/청크 저장의 실제 PE·CRT 관측을 입력 전용 외부 장치와 대조한다.
#include "TestSupport.h"
#include "o/RawFortObjectSave.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace netstorm::o;
namespace {
constexpr std::array<std::string_view,10> kGlobalNames{"battle","mission","editor","scope","version","localOwner","bridgeType","daisType","battleMask","ownerMask"}; // 입력 및 원시 출력의 전역 DWORD 순서다.
constexpr std::array<std::string_view,6> kDeviceNames{"flags1","flags2","robust","island","hasLevel","level"}; // 명시 getter/가상 함수의 현재 반환 장치 순서다.
constexpr std::array<std::uint32_t,2> kBufferPointers{0x01001000U,0x01006000U}; // 원본 관측과 같은 두 비영 물리 주소를 불투명 토큰으로 사용한다.
constexpr std::uint32_t kStreamPointer=0x01000100U; // 객체와 청크가 공유하는 고정 stream 토큰이다.
constexpr std::size_t kBufferSize=16384; // 독립 관측의 실제 물리 출력 상한이다.

// 마지막 빈 문자열까지 보존하면서 지정한 구분자로 나눈다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> parts;std::size_t begin{};
    for (;;) { // 명시 TSV와 효과 문법의 각 경계를 순서대로 보관한다.
        const auto end=text.find(separator,begin);parts.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return parts;begin=end+1;
    }
}
// signed 입력과 전체 DWORD 입력을 같은 원시 비트로 읽는다.
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
// DWORD의 비트를 변경하지 않고 원본 signed 비교에 사용한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 비정렬 필드를 little endian의 지정 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("객체 관측 raw 읽기 범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8); // 지정 BYTE만 조합하고 이웃 자료는 읽지 않는다.
    return value;
}
// 지정 raw 필드의 폭만 덮어써 인접 필드가 보존되는지 확인한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("객체 관측 raw 쓰기 범위");
    for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 지정 폭만 little endian으로 기록한다.
}
// 관측 payload와 raw 슬롯을 소문자 hex로 표현한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef"; // 독립 도구와 같은 두 자리 BYTE 표현이다.
    std::string text;text.reserve(bytes.size()*2);
    for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15U]; } // 전체 관측 범위의 각 BYTE를 빠짐없이 출력한다.
    return text;
}
// 독립 입력에 적힌 hex BYTE를 변경 없이 복원한다.
std::vector<std::uint8_t> Bytes(std::string_view text) {
    if (text.size()%2) throw std::invalid_argument("관측 hex BYTE 폭");std::vector<std::uint8_t> bytes;bytes.reserve(text.size()/2);
    for (std::size_t at=0;at<text.size();at+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16))); // 명시 두 자리마다 BYTE 하나만 생성한다.
    return bytes;
}
// 명명한 입력 필드만 찾아 알 수 없는 효과가 조용히 다른 필드를 바꾸지 않게 한다.
template<std::size_t Count> std::size_t Field(const std::array<std::string_view,Count>& names,std::string_view name) {
    const auto found=std::find(names.begin(),names.end(),name);if (found==names.end()) throw std::invalid_argument("미명시 객체 관측 필드");return static_cast<std::size_t>(found-names.begin());
}
// 현재 DWORD 목록을 독립 출력과 같은 순서의 십진수 문자열로 만든다.
template<std::size_t Count> std::string Words(const std::array<std::uint32_t,Count>& values) {
    std::string text;
    for (const auto value:values) { if (!text.empty()) text+=':';text+=std::to_string(value); } // 원시 DWORD를 signed 축소 없이 전부 비교한다.
    return text;
}
// 명시 경계에서 원문 실행을 중단한 입력을 C++에서도 같은 시점에 멈춘다.
struct Stop {};

// 기대 출력은 보관하지 않고 입력의 외부 장치·현재 자료·변경 효과만 소유한다.
struct Devices {
    std::array<std::uint8_t,50> raw{}; // 고정 SID의 전체 raw50이며 첫 vptr DWORD는 격리 주소를 넣지 않은0이다.
    std::array<std::uint32_t,10> globals{}; // 각 분기에서 다시 조회할 실제 전역 DWORD다.
    std::array<std::uint32_t,6> devices{}; // flags/가상/섬 함수의 명시 현재 반환값이다.
    std::array<std::uint8_t,16> stream{}; // data/pos/end/capacity를 보관하는 실제16BYTE 헤더다.
    std::array<std::array<std::uint8_t,kBufferSize>,2> buffers{}; // data 변경과 고정 count 주소를 구별하는 두 물리 출력이다.
    std::vector<std::vector<std::string>> effects; // 입력에 적힌 경계별 순서 효과이며 기대 결과를 사용하지 않는다.
    std::map<std::string,std::size_t> calls; // flags11과 flags12처럼 종류별 호출 번호를 만든다.
    std::string trace; // 실제로 호출된 외부 경계만 순서대로 보관한다.
    std::uint32_t sid{},poolLimit{},current{}; // 고정 객체 SID, 현재 Pool 개수, 독립 Finder 현재 후보다.
    std::vector<std::uint32_t> sequence; // 명시 Begin/Next 장치가 공급하는 후보열이다.
    std::size_t sequenceIndex{}; // current 효과와 독립적으로 Next가 진행하는 후보열 위치다.
    std::map<std::uint32_t,std::uint32_t> returns; // SaveObject 외부 장치의 명시 DWORD 반환표다.
    std::map<std::uint32_t,std::vector<std::uint8_t>> childPayloads; // 명시한 비영 반환 때만 쓰는 하위 장치 payload다.

    // 비영 물리 토큰을 두 버퍼 중 하나로 해석하며 임의 대체 버퍼는 만들지 않는다.
    std::span<std::uint8_t> Buffer(std::uint32_t pointer) {
        const auto found=std::find(kBufferPointers.begin(),kBufferPointers.end(),pointer);if (found==kBufferPointers.end()) throw std::out_of_range("관측 물리 버퍼 토큰");return buffers.at(static_cast<std::size_t>(found-kBufferPointers.begin()));
    }
    // stream과 두 CC 초기 버퍼, 입력에 적힌 효과만 준비한다.
    void InitializeStream(const std::vector<std::string>& row,std::size_t at) {
        Put(stream,0,kBufferPointers[0]);Put(stream,4,Number(row[at]));Put(stream,8,Number(row[at+1]));Put(stream,12,Number(row[at+2]));
        for (auto& buffer:buffers) buffer.fill(0xcc); // 원문 출력 밖의 초기 BYTE도 구별할 수 있게 놓는다.
        if (row[at+3]!="-") for (const auto& effect:Split(row[at+3],';')) { // 앞쪽 입력 효과만 순서대로 읽고 결과 열은 참조하지 않는다.
            auto fields=Split(effect,'/');if (fields.size()!=4) throw std::invalid_argument("관측 효과 열 수");effects.push_back(std::move(fields));
        }
    }
    // 객체 TSV의 입력25열만 raw/전역/현재 장치에 설치한다.
    void InitializeObject(const std::vector<std::string>& row) {
        sid=Number(row[1]);const auto bytes=Bytes(row[4]);if (bytes.size()!=raw.size()) throw std::invalid_argument("관측 객체 raw50 폭");std::copy(bytes.begin(),bytes.end(),raw.begin());
        devices[0]=Number(row[5]);devices[1]=Number(row[6]);
        for (std::size_t index=0;index<globals.size();++index) globals[index]=Number(row[7+index]); // 원시 전역 입력의 전체 DWORD 폭을 유지한다.
        for (std::size_t index=2;index<devices.size();++index) devices[index]=Number(row[15+index]); // robust/island/hasLevel/level 네 입력을 같은 장치 배열에 놓는다.
        InitializeStream(row,21);
    }
    // 청크 TSV의 입력10열만 Finder/SaveObject 반환/stream에 설치한다.
    void InitializeChunk(const std::vector<std::string>& row) {
        poolLimit=Number(row[3]);if (row[4]!="-") for (const auto& value:Split(row[4],',')) sequence.push_back(Number(value)); // 후보 개수에는 실제65536 반환1 감김 입력도 포함한다.
        if (row[5]!="-") for (const auto& text:Split(row[5],';')) { // 반환표에 적힌 하위 장치만 공급하고 나머지는 기본 반환1이다.
            const auto fields=Split(text,':');if (fields.size()<2 || fields.size()>3) throw std::invalid_argument("관측 SaveObject 반환 열 수");const auto candidate=Number(fields[0]);returns[candidate]=Number(fields[1]);
            if (fields.size()==3) childPayloads[candidate]=fields[2]=="-" ? std::vector<std::uint8_t>{} : Bytes(fields[2]);
        }
        InitializeStream(row,6);
    }
    // 실제 경계의 trace와 종류별 호출 번호를 기록하되 효과 시점은 호출자가 정한다.
    std::string Event(std::string_view kind,std::string text) {
        if (!trace.empty()) trace+=';';trace+=text;const auto key=std::string(kind);return key+std::to_string(++calls[key]);
    }
    // 같은 경계에 적힌 현재 자료 변경을 입력 순서대로 수행한다.
    void Mutate(std::string_view event) {
        for (const auto& effect:effects) { // 일치한 경계의 입력 효과만 적용하며 저장 분기 자체는 구현하지 않는다.
            if (effect[0]!=event) continue;const auto& target=effect[1];const auto& field=effect[2];const auto value=Number(effect[3]);
            if (target=="global") { if (field=="poollimit") poolLimit=value;else globals.at(Field(kGlobalNames,field))=value; }
            else if (target=="device") devices.at(Field(kDeviceNames,field))=value;
            else if (target=="form") {
                if (field=="type") Put(raw,10,value,1);else if (field=="owner") Put(raw,34,value,1);else if (field=="flags") Put(raw,40,value,1);
                else if (field=="qa") Put(raw,12,value,2);else if (field=="frame") Put(raw,36,value);else if (field=="x") Put(raw,14,value);else if (field=="y") Put(raw,18,value);else throw std::invalid_argument("미명시 raw 효과");
            } else if (target=="stream") {
                if (field=="data") Put(stream,0,kBufferPointers.at(value));else if (field=="position") Put(stream,4,value);else if (field=="end") Put(stream,8,value);else if (field=="capacity") Put(stream,12,value);else throw std::invalid_argument("미명시 stream 효과");
            } else if (target=="finder" && field=="current") current=value;
            else if (target=="control" && field=="stop" && value==1) throw Stop{};
            else throw std::invalid_argument("미명시 관측 효과 대상");
        }
    }
    // 원문 진단에서 명시한 복귀/변경/중단만 공급한다.
    void Assertion(std::uint32_t line) { Mutate(Event("assert","assert:"+std::to_string(line))); }
    // 외부 장치의 명시 bytes만 현재 data/pos에 쓰고 unsigned 논리끝을 진행한다.
    void Append(std::span<const std::uint8_t> bytes) {
        const auto position=Read(stream,4);auto output=Buffer(Read(stream,0));if (position>output.size() || bytes.size()>output.size()-position) throw std::out_of_range("관측 외부 장치 출력 범위");
        std::copy(bytes.begin(),bytes.end(),output.begin()+position);const auto next=position+static_cast<std::uint32_t>(bytes.size());Put(stream,4,next);Put(stream,8,std::max(Read(stream,8),next));
    }
    // flags getter는 trace/효과 뒤의 현재 장치 DWORD를 반환한다.
    std::uint32_t Flags(std::uint32_t selected,std::size_t index) {
        CHECK(selected==sid);const auto name=std::string(kDeviceNames.at(index));Mutate(Event(name,name+':'+std::to_string(selected)+':'+std::to_string(raw[10])));return devices.at(index);
    }
    // 가상 함수와 hasLevel은 반환값을 효과 전에 캡처한다.
    std::uint32_t CapturedDevice(std::uint32_t selected,std::size_t index) {
        CHECK(selected==sid);const auto answer=devices.at(index);const auto name=std::string(kDeviceNames.at(index));Mutate(Event(name,name+':'+std::to_string(selected)));return answer;
    }
    // 저장 필터/필드 선택은 제품에 맡기고 명시한 객체 외부 함수만 연결한다.
    FortObjectSaveHooks ObjectHooks() {
        FortObjectSaveHooks hooks;
        hooks.global=[this](auto key) { return globals.at(static_cast<std::size_t>(key)); }; // 각 소비 위치에서 현재 DWORD를 조회한다.
        hooks.form=[this](auto selected)->std::span<const std::uint8_t> { CHECK(selected==sid);return raw; }; // raw 조회 자체는 효과를 발생시키지 않는다.
        hooks.flags1=[this](auto selected) { return Flags(selected,0); };hooks.flags2=[this](auto selected) { return Flags(selected,1); }; // 두 getter의 호출 순서와 현재 type을 기록한다.
        hooks.robust=[this](auto selected) { return CapturedDevice(selected,2); };hooks.hasLevel=[this](auto selected) { return CapturedDevice(selected,4); };hooks.level=[this](auto selected) { return CapturedDevice(selected,5); }; // 가상 반환을 bool이나 BYTE로 미리 줄이지 않는다.
        hooks.island=[this](auto x,auto y) { const auto answer=devices[3];Mutate(Event("island","island:"+std::to_string(x)+':'+std::to_string(y)));return answer; }; // 전달된 실제 float32 비트를 그대로 기록한다.
        hooks.assertion=[this](auto kind) { Assertion(kind==FortObjectSaveAssertion::XOffset ? 667U : kind==FortObjectSaveAssertion::YOffset ? 668U : 707U); }; // 원문의 세 진단 행을 구별한다.
        hooks.write=[this](auto selected,auto source) { // 실제 caller bytes는 용량 진단 및 입력 효과 전에 캡처한다.
            CHECK(selected==kStreamPointer);const std::vector<std::uint8_t> bytes(source.begin(),source.end());const auto event=Event("write","write:"+std::to_string(bytes.size())+':'+Hex(bytes));
            if (Signed(Read(stream,12))<Signed(Read(stream,4)+static_cast<std::uint32_t>(bytes.size()))) Assertion(36);Append(bytes);Mutate(event);
        };
        hooks.contents=[this](auto selected,auto output) { // 내용물 자체는 독립 관측에 명시한 EE 한 BYTE 장치다.
            CHECK(selected==sid && output==kStreamPointer);const auto event=Event("contents","contents:"+std::to_string(selected));const std::array<std::uint8_t,1> bytes{0xee};Append(bytes);Mutate(event);
        };return hooks;
    }
    // 원본 Finder/SaveObject 외부 경계만 공급하며 marker/count 쓰기는 제품이 수행한다.
    FortChunkSaveHooks ChunkHooks() {
        FortChunkSaveHooks hooks;
        hooks.poolLimit=[this] { return poolLimit; }; // 각 후보에서 현재 물리 개수를 다시 읽는다.
        hooks.begin=[this](const FortChunkSaveArea& area) { // 네 단정도 인자의 실제 비트와 고정 flags0을 기록한다.
            sequenceIndex=0;current=sequence.empty() ? 0 : sequence[0];Mutate(Event("begin","begin:"+std::to_string(std::bit_cast<std::uint32_t>(area.left))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.top))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.right))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.bottom))+":0"));
            FortChunkSaveCursor cursor;cursor.current=[this] { return current; }; // current 효과가 후보열 위치를 바꾸지 않도록 별도 조회한다.
            cursor.next=[this] { const auto before=current;++sequenceIndex;current=sequenceIndex<sequence.size() ? sequence[sequenceIndex] : 0;Mutate(Event("next","next:"+std::to_string(before))); }; // 원문 Next의 위치 이동 뒤 효과를 적용한다.
            return cursor;
        };
        hooks.object=[this](auto output,auto candidate,auto originX,auto originY) { // 반환 DWORD와 payload는 명시 입력표에서만 읽는다.
            CHECK(output==kStreamPointer);const auto answer=returns.contains(candidate) ? returns.at(candidate) : 1U;const auto event=Event("object","object:"+std::to_string(candidate)+':'+std::to_string(static_cast<std::uint32_t>(originX))+':'+std::to_string(static_cast<std::uint32_t>(originY)));
            if (answer) { if (childPayloads.contains(candidate)) Append(childPayloads.at(candidate));else { const std::array<std::uint8_t,1> bytes{static_cast<std::uint8_t>(candidate)};Append(bytes); } }Mutate(event);return answer;
        };
        hooks.access=[this](auto selected,const auto& operation) { // 순수 stream 빌림은 외부 trace나 입력 효과를 추가하지 않는다.
            CHECK(selected==kStreamPointer);MemStreamHooks memory;memory.buffer=[this](auto pointer) { return Buffer(pointer); }; // marker와 마지막 count 쓰기가 같은 물리 버퍼를 사용한다.
            memory.assertion=[this](auto kind) { CHECK(kind==MemStreamAssertion::WriteCapacity);Assertion(36); }; // marker의 실제 용량 진단만 원문 경계와 연결한다.
            operation(stream,memory);
        };return hooks;
    }
    // 물리 주소만 독립 TSV의 data0/1 토큰으로 정규화하고 나머지16BYTE를 전부 비교한다.
    std::string StreamOutput() const {
        const auto pointer=Read(stream,0);const auto found=std::find(kBufferPointers.begin(),kBufferPointers.end(),pointer);if (found==kBufferPointers.end()) throw std::out_of_range("관측 출력 data 토큰");return std::to_string(found-kBufferPointers.begin())+':'+std::to_string(Read(stream,4))+':'+std::to_string(Read(stream,8))+':'+std::to_string(Read(stream,12));
    }
    // 현재 논리끝까지 두 물리 출력의 전체 BYTE와 stream의 네 DWORD를 함께 비교한다.
    void CheckStream(const std::vector<std::string>& row,std::size_t at) const {
        CHECK(StreamOutput()==row[at]);const auto end=Read(stream,8);CHECK(end<=kBufferSize);if (end>kBufferSize) return;
        CHECK(Hex(std::span(buffers[0]).first(end))==row[at+1]);CHECK(Hex(std::span(buffers[1]).first(end))==row[at+2]);
    }
};

// 청크 marker의 실제 MemStream 진입 관측 한 항목만 제거한다. 제품 access는 순수 빌림이라 이 진입을 hook으로 노출하지 않는다.
// marker의 BYTE99 및 pos/end/capacity/용량 진단/고정 count 쓰기는 정규화하지 않고 위 전체 메모리 비교에 남긴다.
std::string NativeChunkTrace(std::string_view source) {
    std::string trace;std::size_t removed{};
    for (const auto& item:Split(source,';')) { // 실제 외부 호출 순서를 보존하고 정확한 내부 관측 항목 하나만 생략한다.
        if (item=="write:1:63") { ++removed;continue; }if (!trace.empty()) trace+=';';trace+=item;
    }
    CHECK(removed==1);return trace;
}
}

// 모든 모드/필드 폭/실제 CRT 좌표/현재 재조회/진단 중단의 원문3294행을 대조한다.
TEST_CASE(FortObjectSave_IndependentNativeObject) {
    std::ifstream input(NETSTORM_FORT_OBJECT_SAVE_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 입력25열만 장치를 구성하고 뒤11열은 제품 실행 후에만 비교한다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=Split(line,'\t');CHECK(row.size()==36);if (row.size()!=36) continue;Devices d;d.InitializeObject(row);const auto failures=netstorm::test::FailureCount();bool completed=true;std::uint32_t result{};
        try { result=SaveFortObject(kStreamPointer,d.sid,Signed(Number(row[2])),Signed(Number(row[3])),d.ObjectHooks()); } catch (const Stop&) { completed=false; }
        CHECK(completed==(row[25]=="1"));if (completed) { CHECK(result<=1);CHECK(result==Number(row[26])); } // 중단된 native EAX는 의미 있는 반환값이 아니므로 비교하지 않는다.
        d.CheckStream(row,27);CHECK(Hex(d.raw)==row[30]);CHECK(Words(d.globals)==row[31]);CHECK(Words(d.devices)==row[32]);CHECK(d.trace==row[33]);CHECK(completed ? std::stoi(row[34])==0 : std::stoi(row[34])<0);CHECK(std::stoull(row[35])>0);++compared;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"Fort 객체 저장 입력: %s\n",row[0].c_str());
    }
    CHECK(compared==3294);
}

// 후보 범위/빈 청크/고정 count 주소/WORD 감김/부분 중단의 원문672행을 대조한다.
TEST_CASE(FortObjectSave_IndependentNativeChunk) {
    std::ifstream input(NETSTORM_FORT_CHUNK_SAVE_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 입력10열만 외부 장치로 사용하고 뒤9열은 전체 메모리와 호출 순서 검증에 쓴다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=Split(line,'\t');CHECK(row.size()==19);if (row.size()!=19) continue;Devices d;d.InitializeChunk(row);const auto failures=netstorm::test::FailureCount();bool completed=true;
        try { SaveFortChunk(kStreamPointer,Signed(Number(row[1])),Signed(Number(row[2])),d.ChunkHooks()); } catch (const Stop&) { completed=false; }
        CHECK(completed==(row[10]=="1"));d.CheckStream(row,11);CHECK(d.poolLimit==Number(row[14]));CHECK(d.trace==NativeChunkTrace(row[15]));CHECK(completed ? std::stoi(row[17])==0 : std::stoi(row[17])<0);CHECK(std::stoull(row[18])>0);++compared; // void 청크의 native EAX 열16은 반환 계약으로 비교하지 않는다.
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"Fort 청크 저장 입력: %s\n",row[0].c_str());
    }
    CHECK(compared==672);
}
