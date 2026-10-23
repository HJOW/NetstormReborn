// 최초 Mission 저장 x86 관측과 같은 입력·실제 MemStream 출력을 직접 대조한다.
#include "TestSupport.h"
#include "o/RawFortMissionSave.h"
#include "o/RawMemStream.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;

namespace {
constexpr std::array<std::uint32_t,2> kMissionBuffers{0x12040000U,0x12060000U}; //최초 native 물리버퍼 주소이며호스트포인터로역참조하지않는다.
constexpr std::size_t kMissionBufferBytes=0x11000; //65537BYTE 원문과출력주변까지비교할실제물리범위다.
// 마지막 빈 칸을 보존해 fixture의 입력/관측 열을 구별한다.
std::vector<std::string> MissionFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; //현재 분리 위치이며 원문을 정규화하지 않는다.
    // 모든 구분자와 마지막 빈 문자열을 원래 순서로 유지한다.
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; }
}
// 실제 stream의 네 BYTE를 little endian DWORD로 읽는다.
std::uint32_t MissionGet(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t result{}; //지정한DWORD밖은읽지않는다.
    // 원본 네 바이트만 낮은 비트부터 조립한다.
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result;
}
// 실제 stream의 지정한 DWORD만 갱신한다.
void MissionPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 낮은 BYTE부터 네 바이트만 기록한다.
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 주변 바이트를 포함한 관측을 고정 소문자 hex로 반환한다.
std::string MissionHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //원본 BYTE마다 두 글자를 만든다.
    // 물리 관측 순서를 바꾸거나 NUL에서 자르지 않는다.
    for (const auto byte:bytes) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; }return result;
}
// 모든 실제 출력 바이트를 원문 도구와 독립적인 C++ FNV-1a 계산으로 비교한다.
std::string MissionHash(std::span<const std::uint8_t> bytes) {
    std::uint64_t value=0xcbf29ce484222325ULL; //원문 관측 형식의64비트 offset basis다.
    // unsigned64 곱셈의 wrap을 포함하여 전체 물리 범위를 읽는다.
    for (const auto byte:bytes) value=(value^byte)*0x100000001b3ULL;
    constexpr std::string_view kDigits="0123456789abcdef";std::string result(16,'0'); //고정16글자의관측형식이다.
    // 높은 nibble부터 고정 길이로 표시한다.
    for (std::size_t index=0;index<16;++index) result[index]=kDigits[(value>>((15-index)*4))&15U];return result;
}
// 원본 callback 중단과 명시 물리 접근 한계를 정상 반환과 구별한다.
struct MissionStop { std::string cause; }; //기대된도달점만보관하고제품예외를임의성공으로바꾸지않는다.
// 최초 fixture의 입력열만 사용하며 기대 관측은 제품 실행 뒤 읽는다.
struct MissionDevices {
    std::string mode,mutation,trace; //진입몸체·외부효과입력과실제일반쓰기/진단trace다.
    std::uint32_t current{},nameCase{},manager{},writes{},currentReads{},nameReads{}; //현재Guide필드와독립입력·조회수다.
    std::array<std::uint32_t,3> sequence{},flags{}; //토큰1/2가구별하는실제두Mission필드다.
    std::vector<std::uint8_t> name; //같은주소의이름내용은callback이바꿀수있으며재할당하지않는다.
    std::array<std::uint8_t,16> stream{}; //실제일반WriteMemStream의현재data/pos/end/alloc이다.
    std::array<std::vector<std::uint8_t>,2> buffers; //진단상alloc보다큰물리출력도명시입력이다.

    // 이름 NUL 뒤 바이트까지 별도 저장하되 실제 문자열 길이는 제품이 계산한다.
    explicit MissionDevices(std::string_view input) {
        const auto fields=MissionFields(input,',');if (fields.size()!=12) throw std::runtime_error("Mission fixture 입력 칸 수 오류");
        mode=fields[0];current=static_cast<std::uint32_t>(std::stoul(fields[1]));nameCase=static_cast<std::uint32_t>(std::stoul(fields[2]));
        MissionPut(stream,0,kMissionBuffers[0]);MissionPut(stream,4,static_cast<std::uint32_t>(std::stoul(fields[3])));MissionPut(stream,8,static_cast<std::uint32_t>(std::stoul(fields[4])));MissionPut(stream,12,static_cast<std::uint32_t>(std::stoul(fields[5])));
        sequence[1]=static_cast<std::uint32_t>(std::stoul(fields[6]));flags[1]=static_cast<std::uint32_t>(std::stoul(fields[7]));sequence[2]=static_cast<std::uint32_t>(std::stoul(fields[8]));flags[2]=static_cast<std::uint32_t>(std::stoul(fields[9]));manager=static_cast<std::uint32_t>(std::stoul(fields[10]));mutation=fields[11];
        constexpr std::array<std::size_t,8> kLengths{0,3,0,31,65535,65536,65537,0}; //WORD경계는독립이름writer입력에서만사용한다.
        if (nameCase==2) name={'A',0,'B','C',0};else { name.resize(kLengths.at(nameCase)+1);for (std::size_t index=0;index+1<name.size();++index) name[index]=static_cast<std::uint8_t>(65+index%23); } //마지막 NUL은vector초기값0을유지한다.
        // 미사용 물리버퍼도 주변cc바이트 전체를 보존하는지 확인한다.
        for (auto& output:buffers) output.assign(kMissionBufferBytes,0xcc);
    }
    // 실제 경계 진입을 문자열 하나에 순서대로 기록한다.
    void Record(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 부모의 분기/재조회는 제품에 맡기고 지정 callback의 외부 효과만 적용한다.
    void Mutate(std::string_view boundary) {
        if (mutation==std::string(boundary)+"-current2") current=2;
        else if (mutation==std::string(boundary)+"-current0") current=0;
        else if (mutation==std::string(boundary)+"-name") { name[0]='Z';name[1]=0;name[2]='Y'; }
        else if (mutation=="write1-sequence" && boundary=="write1") sequence[1]=0x13579bdfU;
        else if (mutation=="write3-flags" && boundary=="write3") flags[1]=0x2468ace0U;
        else if (mutation=="write4-flags" && boundary=="write4") flags[1]=0x01020304U;
        else if (mutation=="capacity-current" && boundary=="capacity") { MissionPut(stream,0,kMissionBuffers[1]);MissionPut(stream,4,5);MissionPut(stream,8,2);MissionPut(stream,12,static_cast<std::uint32_t>(kMissionBufferBytes));sequence[1]=0x11223344U; }
        else if (mutation=="write1-physical" && boundary=="write1") MissionPut(stream,0,0);
        else if (mutation==std::string(boundary)+"-stop") throw MissionStop{std::string(boundary)};
    }
    // 같은 실제 일반 writer로 현재 용량 진단/현재data/pos/end 쓰기를 수행한다.
    void Write(std::uint32_t selected,std::span<const std::uint8_t> source) {
        CHECK(selected==1);++writes;Record("W:"+std::to_string(writes)+':'+std::to_string(source.size())+':'+MissionHash(source));Mutate("write"+std::to_string(writes));
        MemStreamHooks hooks; //할당/해제는일반write에서선택되지않는다.
        hooks.buffer=[this](std::uint32_t pointer)->std::span<std::uint8_t> { if (!pointer) throw MissionStop{"buffer0"};for (std::size_t index=0;index<kMissionBuffers.size();++index) if (pointer==kMissionBuffers[index]) return buffers[index];throw std::out_of_range("Mission 현재 물리 버퍼 토큰"); }; //순수조회가포인터별실제범위만반환한다.
        hooks.assertion=[this](MemStreamAssertion value) { CHECK(value==MemStreamAssertion::WriteCapacity);Record("A:36");Mutate("capacity"); };
        try { WriteMemStream(stream,source,static_cast<std::uint32_t>(source.size()),hooks); }
        catch (const std::logic_error&) { if (!MissionGet(stream,0) && !source.empty()) throw MissionStop{"buffer0"};throw; } //기존MemStream의명시null물리거부만native접근직전중단과대응한다.
    }
    // Guide/Mission 필드는 순수현재조회이며 일반쓰기만 외부효과를 낸다.
    FortMissionSaveHooks Hooks() {
        FortMissionSaveHooks hooks;
        hooks.current=[this] { ++currentReads;return current; };
        hooks.name=[this]()->std::optional<std::span<const std::uint8_t>> { ++nameReads;if (nameCase==7) return {};return std::span(name); };
        hooks.field=[this](std::uint32_t selected,std::uint32_t offset) { if (!selected) throw MissionStop{"mission0"};if (selected>2 || (offset!=0x54 && offset!=0x58)) throw std::out_of_range("Mission 현재 필드 범위");return offset==0x58 ? sequence.at(selected) : flags.at(selected); };
        hooks.write=[this](auto selected,auto source) { Write(selected,source); };
        hooks.assertion=[this](FortMissionSaveAssertion value) { CHECK(value==FortMissionSaveAssertion::NullName);Record("A:155");Mutate("null"); };return hooks;
    }
    // 독립 진입점에 맞춰 제품 함수만 실행한다. 전체Template부모는 별도 모듈의 소유다.
    void Run() {
        auto hooks=Hooks();if (mode=="guide") SaveFortMission(1,hooks);else if (mode=="name") SaveFortMissionName(1,nameCase==7 ? std::nullopt : std::optional<std::span<const std::uint8_t>>(std::span(name)),hooks);else if (mode=="state") SaveFortMissionState(1,current,hooks);else throw std::runtime_error("Mission fixture 미등록 진입점");
    }
    // 제품 실행 뒤 Guide/Mission의 모든 입력 필드와 일반쓰기 횟수를 관측한다.
    std::string State() const { return std::to_string(manager)+','+std::to_string(current)+','+std::to_string(sequence[1])+','+std::to_string(flags[1])+','+std::to_string(sequence[2])+','+std::to_string(flags[2])+','+std::to_string(writes); }
    // 실제16BYTE헤더·두전체물리해시·앞뒤주변바이트를 원문과 대조한다.
    std::string Raw() const {
        auto result=MissionHex(stream); //현재 data 포인터 변이도 원시주소로 비교한다.
        // 미사용 출력도 원래cc바이트로 남는지 확인한다.
        for (const auto& buffer:buffers) { const auto bytes=std::span(buffer);result+='|'+MissionHash(bytes)+':'+MissionHex(bytes.first(64))+':'+MissionHex(bytes.last(16)); }return result;
    }
};
}

// 최초115입력/두정밀도230관측의 정상반환과 명시중단까지 모든 제품 효과를 비교한다.
TEST_CASE(FortMissionSave_IndependentNative230) {
    std::ifstream input(NETSTORM_FORT_MISSION_SAVE_FIXTURE);if (!input) throw std::runtime_error("Mission 최초 x86 fixture 없음");std::string line;std::size_t compared{},stopped{};std::set<std::pair<std::string,std::string>> unique;
    // 각 행의 앞 입력만으로 제품을 실행한 뒤 관측열을 사용한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=MissionFields(line,'\t');if (row.size()!=6) throw std::runtime_error("Mission fixture 관측 칸 수 오류");CHECK(unique.emplace(row[0],row[1]).second);
        MissionDevices devices(row[1]);std::string completion="1";try { devices.Run(); }catch (const MissionStop& stop) { completion=stop.cause;++stopped; }
        CHECK(completion==row[2]);CHECK(devices.State()==row[3]);CHECK(devices.Raw()==row[4]);CHECK(devices.trace==row[5]);++compared;
    }
    CHECK(compared==230 && stopped==12);
}

// 선택하지 않는 장치는 요구하지 않고 없는 실제 경계/원문 물리범위는 정상 성공으로 만들지 않는다.
TEST_CASE(FortMissionSave_CurrentAndPhysicalBoundaries) {
    FortMissionSaveHooks empty;std::size_t queries{};empty.current=[&] { ++queries;return 0U; };SaveFortMission(99,empty);CHECK(queries==1);
    MissionDevices devices("guide,1,1,0,0,64,1,2,3,4,11,write3-current2");devices.Run();CHECK(devices.currentReads==2 && devices.nameReads==1 && devices.current==2);CHECK(MissionGet(devices.stream,4)==13 && MissionGet(devices.buffers[0],5)==1 && MissionGet(devices.buffers[0],9)==2); //flags는선택한Mission1에남는다.
    const std::array<std::uint8_t,2> noNul{'A','B'};std::size_t writes{};FortMissionSaveHooks bounds;bounds.write=[&](auto,auto) { ++writes; };bool rejected=false;
    try { SaveFortMissionName(1,std::span(noNul),bounds); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && writes==0);
    rejected=false;try { SaveFortMissionName(1,std::nullopt,bounds); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && writes==0);
    const std::array<std::uint8_t,1> blank{};std::vector<std::size_t> lengths;bounds.write=[&](auto selected,auto source) { CHECK(selected==77);lengths.push_back(source.size()); };SaveFortMissionName(77,std::span(blank),bounds);CHECK((lengths==std::vector<std::size_t>{2,0}));
}
