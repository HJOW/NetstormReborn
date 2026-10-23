// 원문편집버퍼570행의현재10DWORD·할당전체·출력·clipboard·부분중단을대조한다.
#include "TestSupport.h"
#include "client/TextEntryBuffer.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr std::uint32_t kEditBlockBase=0x01010000,kEditBlockStride=0x10000; // 독립장치의고정가상할당주소다.
constexpr std::size_t kEditRows=570,kEditColumns=22; // 실제285입력의두정밀도관측수다.
struct EditExplicitStop {}; // 명시callback중단을원문정상RET와구별한다.

// 마지막빈칸을포함하여입력/관측의구분자를읽는다.
std::vector<std::string> EditSplit(std::string_view text,char separator) {
    std::vector<std::string> output;std::size_t begin{}; // 현재원문필드와복사목록이다.
    for (;;) { // 구분자사이의원문만순서대로복사한다.
        const auto end=text.find(separator,begin);output.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return output;begin=end+1;
    }
}
// signed입력도하위DWORD비트를그대로유지한다.
std::uint32_t EditNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
// DWORD를실제signed32필드로읽는다.
std::int32_t EditSigned(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 기대값계산없이원문16진바이트를그대로읽는다.
std::vector<std::uint8_t> EditBytes(std::string_view hex) {
    if (hex.size()%2) throw std::runtime_error("편집버퍼16진폭");std::vector<std::uint8_t> result;
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(i,2)),nullptr,16))); // 두문자씩한바이트다.
    return result;
}
// 현재메모리를원시바이트로만관측한다.
std::string EditHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef"; // fixture와같은소문자16진표다.
    std::string output;output.reserve(bytes.size()*2);
    for (const auto byte:bytes) { output+=digits[byte>>4];output+=digits[byte&15]; } // 메모리순서대로관측한다.
    return output;
}
// 원시DWORD목록의관측순서를유지한다.
template<std::size_t Size> std::string EditPacked(const std::array<std::uint32_t,Size>& values) {
    std::string output;for (auto value:values) { if (!output.empty()) output+=':';output+=std::to_string(value); } return output; // 고정필드순서다.
}

// 실제본문과줄표를소유하며가드/반납메모리까지독립관측한다.
struct EditBlock {
    std::uint32_t nominal{},live{}; // 원문요청폭과현재반납여부다.
    std::vector<std::uint8_t> bytes; // nominal+16의실제소유바이트다.
};
// 기대자료를입력으로사용하지않는현재버퍼와명시allocator/clipboard장치다.
struct EditScene {
    TextEntryBufferState state; // 제품처리기와TextEntry가공유하는실제10DWORD다.
    std::map<std::uint32_t,EditBlock> blocks; // 가상주소순서로소유블록을보관한다.
    std::uint32_t nextBlock{},fill{}; // 명시allocator의다음주소/초기바이트다.
    std::set<std::string> failures; // 입력이명시한할당실패호출번호다.
    std::map<std::string,std::size_t> calls; // 실제경계별1기반호출번호다.
    std::vector<std::vector<std::string>> effects; // 앞11열의콜백변경목록이다.
    std::vector<char> text,excluded,clipboardInput; // 독립입력NUL문자열과clipboard다.
    std::array<char,2048> output; // 원문출력2048BYTE의실제관측범위다.
    std::array<std::int32_t,2> selection; // 원문CopyLine의별도선택쌍출력이다.
    std::vector<std::uint8_t> clipboardOutput; // 실제clipboardWrite가받은바이트만기록한다.
    std::string trace; // 명시외부효과의전체순서/인자다.
    std::unique_ptr<TextEntryBuffer> buffer; // 입력설치뒤같은상태와메모리에연결한다.

    // 현재10DWORD만원문필드순서로관측한다.
    std::array<std::uint32_t,10> Fields() const {
        return {state.vtable,state.offsets,std::uint32_t(state.offsetCapacity),std::uint32_t(state.lineCount),state.dirty,std::uint32_t(state.selectionBegin),std::uint32_t(state.selectionEnd),std::uint32_t(state.length),std::uint32_t(state.capacity),state.data};
    }
    // 현재원시필드를같은상태에쓴다.
    void SetFields(const std::array<std::uint32_t,10>& fields) {
        state={fields[0],fields[1],EditSigned(fields[2]),EditSigned(fields[3]),fields[4],EditSigned(fields[5]),EditSigned(fields[6]),EditSigned(fields[7]),EditSigned(fields[8]),fields[9]};
    }
    // base또는base+offset에대응하는같은실제소유메모리의남은폭이다.
    std::span<std::uint8_t> Memory(std::uint32_t address) {
        auto found=blocks.upper_bound(address);if (found==blocks.begin()) throw std::runtime_error("편집버퍼장치주소");--found;const auto offset=std::size_t(address-found->first);
        if (offset>=found->second.bytes.size()) throw std::runtime_error("편집버퍼장치물리범위");return std::span<std::uint8_t>(found->second.bytes).subspan(offset);
    }
    // 앞11열의현재state변경만적용한다.
    void Effect(const std::vector<std::string>& effect) {
        if (effect[1]=="control") return;if (effect[1]!="state") throw std::runtime_error("편집버퍼효과대상");
        static const std::array<std::string_view,10> names{"vtable","offsets","offsetCapacity","lineCount","dirty","selectionBegin","selectionEnd","length","capacity","data"}; // 원문필드명의단순표다.
        const auto found=std::find(names.begin(),names.end(),effect[2]);if (found==names.end()) throw std::runtime_error("편집버퍼효과필드");auto fields=Fields();fields[static_cast<std::size_t>(found-names.begin())]=EditNumber(effect[3]);SetFields(fields);
    }
    // 장치기본효과뒤동일호출번호의변경과명시중단을실행한다.
    void Event(std::string name,std::vector<std::string> values) {
        if (!trace.empty()) trace+=';';trace+=name;
        for (const auto& value:values) trace+=':'+value; // 실제장치인자만기록한다.
        const auto trigger=name+std::to_string(++calls[name]);bool stopped{};
        for (const auto& effect:effects) { // 같은경계의여러변경을원문입력순서대로적용한다.
            if (effect[0]!=trigger) continue;Effect(effect);if (effect[1]=="control" && effect[2]=="stop" && effect[3]=="1") stopped=true;
        }
        if (stopped) throw EditExplicitStop{};
    }
    // allocator호출전의실패입력과가드가있는실제메모리만공급한다.
    std::uint32_t Allocate(std::uint32_t size,bool failed) {
        if (failed) return 0;if (size>16384) throw std::runtime_error("편집버퍼명시할당한도");const auto token=kEditBlockBase+nextBlock++*kEditBlockStride;
        blocks.emplace(token,EditBlock{size,1,std::vector<std::uint8_t>(static_cast<std::size_t>(size)+16,static_cast<std::uint8_t>(fill))});return token;
    }
    // pure조회와관측가능한실제소유메모리/clipboard효과를구별한다.
    TextEntryBufferHooks Hooks() {
        TextEntryBufferHooks hooks;hooks.bytes=[this](auto address) { return Memory(address); };
        hooks.allocate=[this](auto size) { const auto token=Allocate(size,failures.contains("allocate"+std::to_string(calls["allocate"]+1)));Event("allocate",{std::to_string(size),std::to_string(token)});return token; };
        hooks.release=[this](auto token) { if (token) blocks.at(token).live=0;Event("release",{std::to_string(token)}); };
        hooks.resize=[this](auto old,auto size) {
            const auto token=Allocate(size,failures.contains("resize"+std::to_string(calls["resize"]+1)));
            if (token && old) { auto& previous=blocks.at(old);auto& current=blocks.at(token);std::copy_n(previous.bytes.begin(),std::min(previous.nominal,size),current.bytes.begin());previous.live=0; }
            Event("resize",{std::to_string(old),std::to_string(size),std::to_string(token)});return token;
        };
        hooks.clipboardRead=[this](std::span<char> target) { std::fill(target.begin(),target.end(),char{});std::copy_n(clipboardInput.begin(),std::min(clipboardInput.size(),target.size()),target.begin());Event("clipboardRead",{std::to_string(target.size())}); };
        hooks.clipboardWrite=[this](std::span<const char> source) { clipboardOutput.assign(reinterpret_cast<const std::uint8_t*>(source.data()),reinterpret_cast<const std::uint8_t*>(source.data())+source.size());Event("clipboardWrite",{clipboardOutput.empty() ? "-" : EditHex(clipboardOutput)}); };return hooks;
    }
    // 원문입력의raw바이트를NUL범위와같은수명으로보관한다.
    static std::vector<char> Characters(std::string_view hex,bool terminate) {
        const auto bytes=EditBytes(hex);std::vector<char> output(bytes.begin(),bytes.end());if (terminate) output.push_back(0);return output;
    }
    // 앞11열만읽으며후반관측자료를장치입력으로쓰지않는다.
    explicit EditScene(const std::vector<std::string>& row):text(Characters(row[5],true)),excluded(Characters(row[6],true)),clipboardInput(Characters(row[7],false)) {
        fill=EditNumber(row[10]);output.fill(0x6d);selection={EditSigned(0x11223344),EditSigned(0x55667788)};const auto fields=EditSplit(row[2],':');if (fields.size()!=10) throw std::runtime_error("편집버퍼state폭");std::array<std::uint32_t,10> values{};
        for (std::size_t i=0;i<values.size();++i) values[i]=EditNumber(fields[i]); // 원시현재10필드를설치한다.
        SetFields(values);
        if (row[3]!="-") for (const auto& item:EditSplit(row[3],';')) { const auto parts=EditSplit(item,':');if (parts.size()!=4) throw std::runtime_error("편집버퍼할당입력폭");blocks.emplace(EditNumber(parts[0]),EditBlock{EditNumber(parts[1]),EditNumber(parts[2]),EditBytes(parts[3])}); } // 기존소유메모리입력이다.
        nextBlock=static_cast<std::uint32_t>(blocks.size());if (row[8]!="-") for (const auto& failure:EditSplit(row[8],',')) failures.insert(failure); // allocator실패도앞부분입력이다.
        if (row[9]!="-") for (const auto& effect:EditSplit(row[9],';')) effects.push_back(EditSplit(effect,'/')); // 관측값과무관한callback입력이다.
        buffer=std::make_unique<TextEntryBuffer>(state,Hooks());
    }
    // 반납메모리와NUL바깥가드까지전부원문주소순서로관측한다.
    std::string BlocksOutput() const {
        std::string serialized;
        for (const auto& [address,block]:blocks) { if (!serialized.empty()) serialized+=';';serialized+=std::to_string(address)+':'+std::to_string(block.nominal)+':'+std::to_string(block.live)+':'+EditHex(block.bytes); } // 실제할당전체순회다.
        return serialized.empty() ? "-" : serialized;
    }
};
}

//13개의실제진입과중첩삽입/줄생성을전체메모리와외부순서로대조한다.
TEST_CASE(TextEntryBuffer_IndependentNativeAllOperations) {
    std::ifstream input(NETSTORM_TEXT_ENTRY_BUFFER_FIXTURE);CHECK(input.good());std::string line;std::size_t rows{},stopped{};
    while (std::getline(input,line)) { // 앞11열은입력,뒤11열은기대관측으로만사용한다.
        if (line.empty() || line.starts_with("name\t")) continue;const auto row=EditSplit(line,'\t');CHECK(row.size()==kEditColumns);if (row.size()!=kEditColumns) continue;const auto failures=netstorm::test::FailureCount();EditScene scene(row);std::vector<std::uint32_t> args;
        if (row[4]!="-") for (const auto& value:EditSplit(row[4],':')) args.push_back(EditNumber(value)); // 원문의실제진입인자다.
        std::uint32_t result{};bool completed=true,meaningful{};std::size_t parameters=args.size();
        try {
            const auto& operation=row[1];
            if (operation=="construct") { scene.buffer->Construct(args[0]);result=1;meaningful=true; }
            else if (operation=="destroy") scene.buffer->Destroy();
            else if (operation=="select") scene.buffer->Select(EditSigned(args[0]),EditSigned(args[1]));
            else if (operation=="insert") scene.buffer->Insert(args[0] ? std::span<const char>(scene.text) : std::span<const char>{});
            else if (operation=="reset") scene.buffer->Reset(args[0] ? std::span<const char>(scene.text) : std::span<const char>{});
            else if (operation=="lines") { result=scene.buffer->EnsureLines();meaningful=true; }
            else if (operation=="lineCount") { result=std::uint32_t(scene.buffer->LineCount());meaningful=true; }
            else if (operation=="lineLength") { result=std::uint32_t(scene.buffer->LineLength(EditSigned(args[0])));meaningful=true; }
            else if (operation=="copyLine") { result=scene.buffer->CopyLine(EditSigned(args[0]),std::span<char>(scene.output).first(args[1]),scene.selection);meaningful=true;parameters=4; }
            else if (operation=="copyAll") { result=scene.buffer->CopyAll(std::span<char>(scene.output).first(args[0]));meaningful=true;parameters=2; }
            else if (operation=="copy") scene.buffer->Copy();else if (operation=="cut") scene.buffer->Cut();
            else if (operation=="paste") scene.buffer->Paste(args[0] ? std::span<const char>(scene.excluded) : std::span<const char>{});
            else throw std::runtime_error("편집버퍼원문진입이름");
        } catch (const EditExplicitStop&) { completed=false;++stopped; }
        CHECK(completed==(row[11]=="1"));CHECK(EditPacked(scene.Fields())==row[12]);CHECK(scene.BlocksOutput()==row[13]);CHECK(EditHex({reinterpret_cast<const std::uint8_t*>(scene.output.data()),scene.output.size()})==row[14]);
        CHECK(EditPacked(std::array<std::uint32_t,2>{std::uint32_t(scene.selection[0]),std::uint32_t(scene.selection[1])})==row[15]);CHECK((scene.clipboardOutput.empty() ? "-" : EditHex(scene.clipboardOutput))==row[16]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[17]);
        if (completed && meaningful) CHECK(result==EditNumber(row[18]));if (completed) CHECK(EditNumber(row[19])==4+parameters*4);CHECK(EditNumber(row[20])>0 && row[21]=="0");++rows; // void/미완료EAX는의미반환으로비교하지않는다.
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"TextEntryBuffer native 입력: %s\n",row[0].c_str());
    }
    CHECK(rows==kEditRows);CHECK(stopped==10);
}
