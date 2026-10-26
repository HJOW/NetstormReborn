// 최초 Subscriber 저장90관측과 같은 실제 등록/문자열/MemStream 및 현재 manager 수명을 대조한다.
#include "TestSupport.h"
#include "DudeSpawnSupport.h"
#include "o/RawFortSubscriberWrite.h"
#include "o/RawFortSubscriberLoad.h"
#include "o/RawMemStream.h"
#include "client/CampaignTemplateStorage.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { //원문 fixture의 입력/현재 장치/독립 BYTE 관측만 준비한다.
using namespace netstorm::o; //원문 부모와 기존 실제 등록/쓰기 몸체를 그대로 호출한다.
constexpr std::array<std::uint32_t,2> kSubWriteBuffers{0x12040000U,0x12060000U}; //CPU fixture의 두 물리 버퍼 토큰이며 호스트 주소가 아니다.
constexpr std::size_t kSubWritePhysical=0x11000; //전체 output/이름의 명시 물리 용량이다.
// TSV의 마지막 빈 값도 보존하여 입력과 관측을 분리한다.
std::vector<std::string> SubWriteFields(std::string_view text,char delimiter) { std::vector<std::string> fields;std::size_t at{};for (;;) { const auto end=text.find(delimiter,at);fields.emplace_back(text.substr(at,end==text.npos ? text.npos : end-at));if (end==text.npos) return fields;at=end+1; } } //구분자가 끝나야 반환한다.
// 같은16BYTE stream의 지정 DWORD만 읽는다.
std::uint32_t SubWriteGet(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (unsigned index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(8*index);return value; } //낮은 BYTE부터 원문 순서로 조립한다.
// 같은16BYTE stream의 지정 DWORD만 쓴다.
void SubWritePut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (unsigned index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index)); } //주변 필드/바이트는 유지한다.
// 모든 물리 BYTE를 독립 FNV1a로 계산해 원문 전체 출력 관측과 대조한다.
std::string SubWriteHash(std::span<const std::uint8_t> bytes) {
    std::uint64_t value=0xcbf29ce484222325ULL;for (const auto byte:bytes) value=(value^byte)*0x100000001b3ULL; //unsigned64 곱셈의 wrap까지 동일한 관측 형식이다.
    constexpr std::string_view digits="0123456789abcdef";std::string result(16,'0');for (std::size_t index=0;index<16;++index) result[index]=digits[(value>>(4*(15-index)))&15U];return result; //고정16글자다.
}
// 출력64BYTE prefix는 NUL에서 자르지 않고 원시 hex로 대조한다.
std::string SubWriteHex(std::span<const std::uint8_t> bytes) { constexpr std::string_view digits="0123456789abcdef";std::string result;for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; } //실제 순서의 각 BYTE만 표시한다.
// 명시 callback/잘못된 manager 경계를 원문의 정상 반환과 구별한다.
struct SubWriteStop { std::string status; }; //fixture가 관측한 도달 상태만 담는다.
// fixture 위치는 등록된 매크로를 우선하고 staging의 독립 준비도 같은 내용을 읽는다.
std::filesystem::path SubWriteFixture() {
#ifdef NETSTORM_FORT_SUBSCRIBER_WRITE_FIXTURE
    return NETSTORM_FORT_SUBSCRIBER_WRITE_FIXTURE;
#else
    auto path=std::filesystem::path(__FILE__).parent_path();while (!path.empty()) { //원본/현재 작업 디렉터리를 추측하지 않고 저장소 marker까지 오른다.
        if (std::filesystem::exists(path/"AGENTS.md")) { const auto target=path/"cpppj/tests/fixtures/fort-subscriber-write-x86.tsv";return std::filesystem::exists(target) ? target : path/"extracted/cpp-fort-subscriber-write-469-staging/work/cpppj/tests/fixtures/fort-subscriber-write-x86.tsv"; }const auto parent=path.parent_path();if (parent==path) break;path=parent;
    }throw std::runtime_error("Subscriber 저장 fixture 저장소 경로 누락");
#endif
}
// 기대는 fixture에서만 읽고 이 구조체는 원문과 같은 외부 입력/현재 장치만 제공한다.
struct SubWriteScene {
    std::array<std::uint32_t,12> input{};std::string mutation,trace; //slot~alloc 입력과 지정 외부효과/실제 순서다.
    std::uint32_t manager{},subscriber{},currentHandle{},writes{}; //실제 helper가 바꾸는 현재 전역/헤더와 선택된 write 순번이다.
    std::array<std::vector<std::uint8_t>,2> names,output;std::array<std::array<std::uint8_t,16>,2> stream{}; //캡처 포인터/대체 포인터/stream/물리 output은 서로 다르다.
    explicit SubWriteScene(std::string_view text) {
        const auto fields=SubWriteFields(text,',');if (fields.size()!=13) throw std::runtime_error("Subscriber 저장 입력 열 수 오류");for (std::size_t index=0;index<input.size();++index) input[index]=static_cast<std::uint32_t>(std::stoull(fields[index]));mutation=fields[12];manager=input[1];subscriber=input[3];
        for (std::size_t index=0;index<2;++index) { //원문과 같은 초기 물리 guard/stream을 명시 입력한다.
            names[index].assign(kSubWritePhysical,0xcc);output[index].assign(kSubWritePhysical,0xcc);SubWritePut(stream[index],0,kSubWriteBuffers[index]);SubWritePut(stream[index],4,input[9]);SubWritePut(stream[index],8,input[10]);SubWritePut(stream[index],12,input[11]);
        }
        std::copy_n("Other",6,names[1].begin());
        if (input[4]>=4) { const auto length=input[4]==4 ? 255U : input[4]==5 ? 256U : 65536U;for (std::uint32_t index=0;index<length;++index) names[0][index]=static_cast<std::uint8_t>(65+index%23);names[0][length]=0; } //길이WORD wrap의 실제 전체 입력을 유지한다.
        else if (input[4]==2) names[0][0]=0;
        else if (input[4]==3) { const std::array<std::uint8_t,5> value{'A',0,'B','C',0};std::copy(value.begin(),value.end(),names[0].begin()); }
        else std::copy_n("Alpha",6,names[0].begin());
    }
    // 실제 경계의 외부 응답/내용만 바꾸고 부모의 조건/재조회는 계산하지 않는다.
    void Event(std::string_view boundary,std::string text) {
        if (!trace.empty()) trace+=';';trace+=text;const auto dash=mutation.find('-');if (dash==std::string::npos || mutation.substr(0,dash)!=boundary) return;const auto action=mutation.substr(dash+1);
        if (action=="stop") throw SubWriteStop{"stop:"+std::string(boundary)};
        if (action=="manager22") manager=22;else if (action=="manager0") manager=0;else if (action=="id9") subscriber=9;else if (action=="pointer2") currentHandle=1;
        else if (action=="name") { names[0][0]='Z';names[0][1]=0;names[0][2]='Y'; }
        else if (action=="grow") std::copy_n("Grown Word",11,names[0].begin());
        else if (action=="streamB") { auto& selected=stream.at(input[8]);SubWritePut(selected,0,kSubWriteBuffers[1]);SubWritePut(selected,4,4);SubWritePut(selected,8,2);SubWritePut(selected,12,kSubWritePhysical); }
        else throw std::logic_error("Subscriber 저장 미해석 외부 변경");
    }
    // 실제 일반 MemStream 몸체에 물리 버퍼만 제공한다.
    MemStreamHooks Memory() {
        MemStreamHooks hooks;hooks.buffer=[&](auto token)->std::span<std::uint8_t> { const auto at=std::find(kSubWriteBuffers.begin(),kSubWriteBuffers.end(),token);if (at==kSubWriteBuffers.end()) throw std::out_of_range("Subscriber 저장 물리 output 토큰");return output[static_cast<std::size_t>(at-kSubWriteBuffers.begin())]; };
        hooks.assertion=[&](auto value) { if (value!=MemStreamAssertion::WriteCapacity) throw std::logic_error("Subscriber 저장 예상 밖 할당 진단");Event("capacity","A:36"); };return hooks;
    }
    // actual 등록helper/문자열/일반writer를 연결하고 Create/Config만 명시 장치로 제공한다.
    FortSubscriberWriteHooks Hooks() {
        FortSubscriberWriteHooks hooks;hooks.manager=[&](auto slot) { CHECK(slot==input[0]);return manager; };hooks.assertion=[&](auto line) { Event(line==0x1cfU ? "assert" : "null","A:"+std::to_string(line)); };
        hooks.registerSubscriber=[&] { Event("register","H");FortSubscriberLoadHooks registration;registration.readRegistered=[&](auto key,auto& target) { target=input[6];Event("read","R:"+std::string(key));return input[5]!=0; };registration.generate=[&] { Event("generate","G");return input[7]; };registration.configure=[&](auto value) { Event("config","F:"+std::string(value)); };FinishFortSubscriberRegistration({subscriber},registration); };
        hooks.create=[&](auto selected,auto section) { CHECK(section=="Subscriber");Event("create","C:"+std::to_string(selected));if (!selected) throw SubWriteStop{"fault:manager0"};return input[8]+1; };
        hooks.write=[&](auto selected,auto bytes) { ++writes;Event("write"+std::to_string(writes),"W:"+std::to_string(writes)+':'+std::to_string(bytes.size())+':'+SubWriteHash(bytes));WriteMemStream(stream.at(selected-1),bytes,static_cast<std::uint32_t>(bytes.size()),Memory()); };return hooks;
    }
    // 실제 두 stream의 필드를 포인터만 fixture 번호로 바꾸어 관측한다.
    std::string Streams() const {
        std::string result;for (const auto& selected:stream) { //다른 stream이 바뀌지 않았는지도 같은8DWORD에 포함한다.
            const auto pointer=SubWriteGet(selected,0);for (unsigned index=0;index<4;++index) { if (!result.empty()) result+=',';result+=std::to_string(index ? SubWriteGet(selected,index*4) : pointer==kSubWriteBuffers[0] ? 1U : pointer==kSubWriteBuffers[1] ? 2U : 0U); }
        }return result;
    }
};
}

// 부모/등록/기존 문자열/실제 MemStream의 상태와 전체 물리 output을 최초 Native 관측에 대조한다.
TEST_CASE(FortSubscriberWrite_ReplaysNative90CapturedIdAndDelayedNameWrites) {
    std::ifstream file(SubWriteFixture(),std::ios::binary);if (!file) throw std::runtime_error("Subscriber 저장 최초 fixture 열기 실패");std::string line;std::size_t checked{};
    while (std::getline(file,line)) { //첫 독립 관측의 두 x87 precision90행을 빠짐없이 재생한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=SubWriteFields(line,'\t');if (row.size()!=9) throw std::runtime_error("Subscriber 저장 fixture 열 수 오류");SubWriteScene scene(row[2]);std::string status="normal";
        try { SaveFortSubscriber(scene.input[0],scene.input[2],scene.input[4]==1 ? std::nullopt : std::optional<std::span<const std::uint8_t>>(scene.names[0]),scene.Hooks()); }catch (const SubWriteStop& stopped) { status=stopped.status; }
        CHECK(status==row[3]);CHECK(std::to_string(scene.subscriber)+','+std::to_string(scene.manager)+','+std::to_string(scene.currentHandle)==row[4]);CHECK(scene.Streams()==row[5]);CHECK(SubWriteHash(scene.output[0])+':'+SubWriteHash(scene.output[1])==row[6]);CHECK(SubWriteHex(std::span(scene.output[0]).first(64))+':'+SubWriteHex(std::span(scene.output[1]).first(64))==row[7]);CHECK(scene.trace==row[8]);++checked;
    }
    CHECK(checked==90);
}

// 실제 보호 Fort의 기존 manager/Subscriber를 사용하고 helper/ID쓰기 뒤 header가 바뀌어도 선택 stream을 유지한다.
TEST_CASE(FortSubscriberWrite_RealStorageKeepsChosenManagerAndCapturedPhysicalName) {
    using namespace netstorm::client;const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/TEST01.fort");CampaignTemplateStorage storage;storage.InstallFile(0,FortTemplate::Parse(original,assets.game.TypeTable()),original,"first.fort");storage.InstallFile(2,FortTemplate::Parse(original,assets.game.TypeTable()),original,"second.fort");storage.Borrow(1,0);const auto first=storage.ReadSubscriberName(0);std::uint32_t subscriber=0xffffffffU,writes{};TemplateWriteStream selected;std::array<std::uint8_t,64> name{};std::copy_n("Original",9,name.begin());
    FortSubscriberWriteHooks hooks;hooks.manager=[&](auto slot) { return SubWriteGet(storage.Headers(),slot*20); };hooks.assertion=[](auto) { throw std::logic_error("실제 Subscriber 저장 예상 밖 진단"); };
    hooks.registerSubscriber=[&] { FortSubscriberLoadHooks registration;registration.readRegistered=[](auto,auto& value) { value=7;return true; };FinishFortSubscriberRegistration({subscriber},registration);storage.Borrow(1,2);std::copy_n("Helper",7,name.begin()); };
    hooks.create=[&](auto manager,auto section) { selected=storage.CreateManagerSection(manager,section);return 1U; };hooks.write=[&](auto stream,auto bytes) { CHECK(stream==1);storage.WriteSection(selected,bytes);if (++writes==1) { storage.Borrow(1,0);std::copy_n("AfterID",8,name.begin()); } };
    SaveFortSubscriber(1,0xffffffffU,std::span<const std::uint8_t>(name),hooks);CHECK(subscriber==7 && writes==3 && storage.ReadSubscriberName(2)=="AfterID" && storage.ReadSubscriberName(0)==first);CHECK(SubWriteGet(storage.Template(2).Section("Subscriber"),0)==0xffffffffU && assets.files.Read("d/TEST01.fort")==original);
}
