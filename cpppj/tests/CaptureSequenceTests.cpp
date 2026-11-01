// 연속 저장174입력의 실제 원문 상태/1000슬롯/전체 복사/장치 순서를 대조한다.
#include "TestSupport.h"
#include "client/RawCaptureSequence.h"
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace netstorm::client; // 실제 제품의 상태/화면/부모만 사용한다.
namespace { // 원문 입력의 물리 장치와 출력 직렬화를 제공한다.
constexpr std::uint32_t kSource=0x11000000,kOther=0x11001000,kBuffer=0x11002000,kSecond=0x11003000; //독립원문장치주소이며실제호스트포인터가아니다.
std::vector<std::string> Fields(std::string_view text,char delimiter) { //마지막필드와원시hex를보존한다.
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find(delimiter,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos:end-begin));if (end==text.npos) return result;begin=end+1; } //구분자사이의바이트만옮긴다.
}
std::string Hex(std::span<const std::uint8_t> bytes) { //실제전체버퍼를낮은주소순으로직렬화한다.
    constexpr char digits[]="0123456789abcdef";std::string result;
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } //출력기대값을입력에쓰지않는다.
    return result;
}
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } //파일명은변환없이원문바이트를기록한다.
std::string SlotsHex(const CaptureSequenceState& state) { //1000포인터를원문DWORD폭으로기록한다.
    std::vector<std::uint8_t> bytes;
    for (const auto token:state.frames) for (unsigned shift=0;shift<32;shift+=8) bytes.push_back(static_cast<std::uint8_t>(token>>shift)); //전체표의쓰지않은칸도비교한다.
    return Hex(bytes);
}
struct Scene { //장치는메모리/설정응답만공급하며복사/순회는제품함수가결정한다.
    CaptureSequenceState state; //현재개수/두flag/1000슬롯이다.
    CaptureSequenceScreen screen; //콜백효과와제품이공유하는현재표면값이다.
    std::map<std::uint32_t,std::vector<std::uint8_t>> memory; //할당/화면의선언된실제512BYTE물리다.
    std::map<std::string,std::uint32_t> calls; //독립장치의1기반호출번호다.
    std::string trace; //제품이실제요청한장치/인자순서다.
    unsigned mode{}; //원문과같은명시경계변이번호다.
    explicit Scene(const std::vector<std::string>& row):mode(static_cast<unsigned>(std::stoul(row[7]))) { //앞8열만입력으로사용한다.
        state.count=static_cast<std::int32_t>(std::stol(row[1]));state.active=static_cast<std::uint32_t>(std::stoul(row[2]));state.first=static_cast<std::uint32_t>(std::stoul(row[3]));state.frames[0]=kBuffer;state.frames[1]=state.frames[999]=kSecond;
        screen={static_cast<std::uint32_t>(std::stoul(row[4])),static_cast<std::uint32_t>(std::stoul(row[5])),static_cast<std::uint32_t>(std::stoul(row[6])),kSource,24680};
        for (const auto token:{kSource,kOther,kBuffer,kSecond}) memory[token].resize(512); //원문명시입력과같은물리폭이다.
        for (std::size_t index=0;index<512;++index) { memory[kSource][index]=static_cast<std::uint8_t>(index*37+11);memory[kOther][index]=static_cast<std::uint8_t>(index*19+17);memory[kBuffer][index]=0xa7;memory[kSecond][index]=0x52; } //독립원문시드/할당오염을넣는다.
    }
    void Event(std::string name,std::vector<std::string> args={}) { //진입인자를기록한뒤현재값을바꾼다.
        if (!trace.empty()) trace+=';';trace+=name;for (const auto& value:args) trace+=':'+value; //원문구분자순서를보존한다.
        const auto trigger=name+std::to_string(++calls[name]);
        if (mode==1 && trigger=="lock1") { screen.height=3;screen.pitch=7; }
        if (mode==2 && trigger=="allocate1") screen.pixels=kOther;
        if (mode==3 && trigger=="unlock1") state.count=1;
        if (mode==4 && trigger=="prefix1") { screen.width=5;screen.height=2;screen.palette=12345; }
        if (mode==5 && trigger=="free1") state.count=1;
        if (mode==6 && trigger=="write1") state.frames[0]=kSecond;
        if (mode==7 && trigger=="prefix3") state.count=7;
    }
    CaptureSequenceHooks Hooks() { //필요한원문경계만같은현재상태/메모리에연결한다.
        CaptureSequenceHooks h;h.lock=[this] { Event("lock"); };h.unlock=[this] { Event("unlock"); };
        h.allocate=[this](auto size) { Event("allocate",{std::to_string(size)});return kBuffer; };
        h.memory=[this](auto token,auto length)->std::span<std::uint8_t> { auto& bytes=memory.at(token);if (length>bytes.size()) throw std::out_of_range("검사장치실제물리");return bytes; };
        h.prefix=[this] { Event("prefix");return calls["prefix"]&1U ? std::string("folder/seq"):std::string("next%name"); };
        h.bitmap=[this](auto path,auto frame,auto x,auto y,auto width,auto height,auto pitch,auto palette) { Event("write",{TextHex(path),std::to_string(frame),std::to_string(x),std::to_string(y),std::to_string(width),std::to_string(height),std::to_string(pitch),std::to_string(palette)}); };
        h.release=[this](auto token) { Event("free",{std::to_string(token)}); };
        h.report=[this](auto first,auto last) { Event("report",{TextHex(first),TextHex(last)});return std::string("formatted report"); };
        h.notice=[this](auto message,auto width,auto height) { CHECK(message=="formatted report");Event("notice",{std::to_string(width),std::to_string(height)}); };return h;
    }
};
}

// 복사끝1~3BYTE·1000상한·현재재조회와중간null/파일별반납을전체원문출력에대조한다.
TEST_CASE(CaptureSequence_Native174StartCopyAndExportLifetime) {
    std::ifstream input(NETSTORM_CAPTURE_SEQUENCE_FIXTURE);CHECK(static_cast<bool>(input));std::string line;std::getline(input,line);std::size_t count{};
    while (std::getline(input,line)) { //각입력은새원문과같은초기상태/물리표를사용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=Fields(line,'\t');CHECK(row.size()==12);if (row.size()!=12) return;Scene scene(row);auto hooks=scene.Hooks();const auto source=scene.memory[kSource],other=scene.memory[kOther];
        if (row[0]=="Start") StartCaptureSequence(scene.state);else if (row[0]=="Frame") AppendCaptureSequenceFrame(scene.state,scene.screen,hooks);else if (row[0]=="Stop") StopCaptureSequence(scene.state,scene.screen,hooks);else CHECK(false);
        const auto state=std::to_string(scene.state.count)+':'+std::to_string(scene.state.active)+':'+std::to_string(scene.state.first);
        if (state!=row[8] || scene.trace!=(row[11]=="-" ? "":row[11])) std::printf("capture mismatch row=%zu kind=%s mode=%s trace=%s\n",count,row[0].c_str(),row[7].c_str(),scene.trace.c_str());
        CHECK(state==row[8] && SlotsHex(scene.state)==row[9] && Hex(scene.memory[kBuffer])==row[10] && (scene.trace.empty() ? "-":scene.trace)==row[11]);CHECK(scene.memory[kSource]==source && scene.memory[kOther]==other);++count;
    }
    CHECK(count==174);
}

// 선택하지않은장치를요구하지않으며중단전파일/슬롯/flag를임의성공으로바꾸지않는다.
TEST_CASE(CaptureSequence_SelectedBoundariesAndOriginalNullBreak) {
    CaptureSequenceState state;CaptureSequenceScreen screen;state.count=1000;state.active=7;AppendCaptureSequenceFrame(state,screen,{});CHECK(state.count==1000 && state.active==7);
    state.count=0;bool missing{};try { AppendCaptureSequenceFrame(state,screen,{}); }catch (const std::logic_error&) { missing=true; }CHECK(missing && state.count==0 && state.active==7);
    std::vector<std::string> events;CaptureSequenceHooks h;h.prefix=[&] { events.emplace_back("prefix");return std::string("seq"); };h.report=[&](auto first,auto last) { CHECK(first=="seq000.bmp" && last=="seq002.bmp");events.emplace_back("report");return std::string("report"); };h.notice=[&](auto message,auto width,auto height) { CHECK(message=="report" && width==0 && height==0);events.emplace_back("notice"); };
    state.count=2;state.frames[0]=0;state.frames[1]=123;StopCaptureSequence(state,screen,h);CHECK(state.count==2 && state.frames[1]==123 && state.active==0 && events==std::vector<std::string>({"prefix","prefix","report","notice"}));
    state.active=9;h.notice=[](auto,auto,auto) { throw std::runtime_error("명시안내중단"); };missing=false;try { StopCaptureSequence(state,screen,h); }catch (const std::runtime_error&) { missing=true; }CHECK(missing && state.active==9 && state.count==2 && state.frames[1]==123);
    state.count=-1;missing=false;try { AppendCaptureSequenceFrame(state,screen,{}); }catch (const std::out_of_range&) { missing=true; }CHECK(missing); //Native표밖상태를호스트성공으로처리하지않는물리계약이다.
}
