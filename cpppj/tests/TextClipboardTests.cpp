// 원문 clipboard880관측과 실제 TextEntryStorage callback을 창/실제 클립보드 없이 대조한다.
#include "DudeSpawnSupport.h"
#include "client/TextClipboard.h"
#include "client/RawTextEntryContext.h"
#include "client/TextEntryStorage.h"
#include <map>
#include <sstream>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 명시 장치 입력/trace와 Native BYTE 출력만 준비한다.
constexpr std::uintptr_t kClipboardHandle=0x12345678; // Native 장치의 독립 HANDLE 입력이다.
std::string ClipboardDecode(std::string_view hex) { // fixture BYTE를 정규화 없이 복원한다.
    if (hex=="-") return {};std::string value;
    for (std::size_t index=0;index<hex.size();index+=2) value.push_back(static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)));return value; // 두 hex 글자가 실제 한 BYTE다.
}
std::string ClipboardEncode(std::span<const char> bytes) { return Hex({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()}); } // NUL 이후 가드까지 그대로 관찰한다.
struct ClipboardScene { // 실제 OS 대신 명시 반환과 동일 메모리만 제공한다.
    std::map<std::string,std::uint32_t> flags; // 각 OS 장치의 독립 반환 입력이다.
    std::vector<char> memory=std::vector<char>(1024,static_cast<char>(0xa5)); // GlobalLock이 빌려 주는 동일 물리다.
    std::string trace; // 실제 선택된 OS 순서/인자다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 반환이나 기대값을 계산하지 않는다.
    std::uint32_t Flag(const std::string& name) const { const auto found=flags.find(name);return found==flags.end() ? 1U : found->second; } // fixture가 명시하지 않은 장치는 비영이다.
    TextClipboardDevices Devices() { // 원문 IAT와 같은 인자/반환/빌린 메모리만 연결한다.
        TextClipboardDevices result;
        result.formatAvailable=[this](auto format) { Event("available:"+std::to_string(format));return Flag("available"); };
        result.open=[this](auto window) { Event("open:"+std::to_string(window));return Flag("open"); };
        result.get=[this](auto format) { Event("get:"+std::to_string(format));return Flag("get") ? kClipboardHandle : 0U; };
        result.lock=[this](auto handle)->char* { Event("lock:"+std::to_string(handle));return Flag("lock") ? memory.data() : nullptr; };
        result.unlock=[this](auto handle) { Event("unlock:"+std::to_string(handle));return Flag("unlock"); };
        result.empty=[this] { Event("empty");return Flag("empty"); };
        result.allocate=[this](auto options,auto count) { Event("allocate:"+std::to_string(options)+','+std::to_string(count));return Flag("allocate") ? kClipboardHandle : 0U; };
        result.set=[this](auto format,auto handle) { Event("set:"+std::to_string(format)+','+std::to_string(handle));return Flag("set") ? kClipboardHandle : 0U; };
        result.close=[this] { Event("close");return Flag("close"); };return result;
    }
};
}
TEST_CASE(TextClipboard_ReplaysNative880ByteCapacityAndFailureOrders) { // 분기/복사/패딩의 기대는 Native fixture만 사용한다.
    const auto fixture=LoadFixture(NETSTORM_TEXT_CLIPBOARD_FIXTURE);std::size_t checked=0,faults=0;
    for (const auto& row:fixture.rows) { // 모든 read240/write640 원문 관측을 빠짐없이 재생한다.
        CHECK(row.size()==8);ClipboardScene scene;std::istringstream assignments(row[3]);std::string assignment;
        while (std::getline(assignments,assignment,',')) { const auto equals=assignment.find('=');scene.flags[assignment.substr(0,equals)]=Number(assignment.substr(equals+1)); } // 원시 OS 반환 입력만 읽는다.
        const auto source=ClipboardDecode(row[1]);TextClipboard clipboard(scene.Devices());const auto capacity=Number(row[2]);std::vector<char> output(capacity+8,static_cast<char>(0xa5));bool completed=true;
        if (row[0]=="read") { std::copy(source.begin(),source.end(),scene.memory.begin());scene.memory[source.size()]=0; }
        try { if (row[0]=="read") clipboard.Read(std::span(output).first(capacity));else clipboard.Write({source.data(),source.size()}); } catch (const std::out_of_range&) { completed=false;++faults; }
        CHECK(completed==(Number(row[4])!=0));CHECK(ClipboardEncode(row[0]=="read" ? std::span<const char>(output) : std::span<const char>(scene.memory).first(source.size()+9))==row[6]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[7]);++checked;
    }
    CHECK(checked==880 && faults==80);
}
TEST_CASE(TextClipboard_BindsActualTextEntryCopyPasteAndPreservesOtherContext) { // 기존0x28 버퍼가 같은 실제 callback을 소비한다.
    ClipboardScene scene;RawTextEntryContext context;context.now=[] { return 12.5; };std::uint32_t submits=0;context.submit=[&](auto,auto) { ++submits; };
    auto clipboard=std::make_shared<TextClipboard>(scene.Devices());BindTextClipboard(context,clipboard);clipboard.reset(); // UI의 두 callback이 부모 수명을 직접 보유한다.
    TextEntryStorage storage(context.clipboardRead,context.clipboardWrite);auto& buffer=storage.Create(1);buffer.Construct();buffer.Reset(std::span("Alpha",6));buffer.Copy();CHECK(std::string(scene.memory.data())=="Alpha");CHECK(scene.trace=="open:0;empty;allocate:8192,6;lock:305419896;unlock:305419896;set:1,305419896;close");
    scene.trace.clear();buffer.Reset(std::span("",1));buffer.Paste(std::span("\r\n",3));CHECK(buffer.Text()=="Alpha");CHECK(scene.trace=="available:1;open:0;get:1;lock:305419896;unlock:305419896;close");CHECK(context.now()==12.5);context.submit(1,"x");CHECK(submits==1);buffer.Destroy();storage.ReleaseBuffer(1);CHECK(storage.AllocationCount()==0);
}
