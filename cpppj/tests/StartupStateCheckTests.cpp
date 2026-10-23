// 제품보다 먼저 고정한390개 부모 관측을 대조하며 실제772BYTE reader/파일/GUI는 실행하지 않는다.
#include "TestSupport.h"
#include "o/RawStartupStateCheck.h"
#include <bit>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
// 마지막 빈 trace 칸까지 보존한다.
std::vector<std::string> StartupFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; //아직소비하지않은첫문자다.
    for (;;) { //구분자가없어진뒤에도마지막칸을한번추가한다.
        const auto at=text.find(separator,start);result.emplace_back(text.substr(start,at==text.npos?text.npos:at-start));if (at==text.npos) return result;start=at+1;
    }
}
// 원문 BYTE 또는 double 메모리와 같은 소문자hex를 만든다.
std::string StartupHex(std::string_view text) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; //같은BYTE의두자리이며문자코드변환을하지않는다.
    for (const auto value:text) { const auto byte=static_cast<unsigned char>(value);result+=digits[byte>>4];result+=digits[byte&15]; } //NUL제외C문자열의실제바이트만표시한다.
    return result;
}
// 명시 double 입력의 little endian8BYTE를 손실 없이 읽는다.
double StartupDouble(std::string_view hex) {
    if (hex.size()!=16) throw std::logic_error("startup 검사 double 폭");std::uint64_t bits{}; //소수문자열재해석으로경계비트를바꾸지않는다.
    for (std::size_t byte=0;byte<8;++byte) bits|=std::uint64_t(std::stoul(std::string(hex.substr(byte*2,2)),nullptr,16))<<(byte*8); //낮은BYTE부터원래double을구성한다.
    return std::bit_cast<double>(bits);
}
// double 승격 인자를 같은 원문trace의 메모리 순서로 표시한다.
std::string StartupDoubleHex(double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);std::string bytes; //실제double의8BYTE다.
    for (std::size_t byte=0;byte<8;++byte) bytes+=static_cast<char>(bits>>(byte*8)); //정확한little endian인자만기록한다.
    return StartupHex(bytes);
}
struct StartupStop { std::string cause; }; //명시하위중단이며부모정상반환과구별한다.
// 계산/분기를 대신하지 않고 원문에서 경계화했던 장치와 부분효과만 제공한다.
struct StartupDevice {
    std::uint32_t option{};std::int32_t saved{};double now{}; //파일/설정/시각의명시입력이다.
    std::string mutation,trace,pending{"BeforeSavedState"}; //reader의부분효과와실제호출순서다.
    std::uint32_t translations{},formats{}; //같은하위의두번째중단을구별하는횟수다.
    // 효과를 먼저 기록하고 같은 원문 위치에서만 부분 중단한다.
    void Emit(std::string_view boundary,std::string value,std::uint32_t count=1) {
        if (!trace.empty()) trace+=';';trace+=value;
        if (mutation=="saved-pending" && boundary=="saved") pending="RestoredPending";
        if (mutation==std::string(boundary)+"-stop") throw StartupStop{std::string(boundary)};
        if (count==2 && mutation==std::string(boundary)+"2-stop") throw StartupStop{std::string(boundary)+"2"};
    }
    // 실제 부모에만 계산을 맡기고 명시 format 결과의 주소 수명을 유지한다.
    StartupStateCheckHooks Hooks() {
        StartupStateCheckHooks hooks; //도달한callback만호출되는지아래trace로확인한다.
        hooks.configInt=[this](auto key) { CHECK(key=="startupStateCheck");Emit("startup-option","STARTUP-OPTION:"+std::to_string(option));return std::bit_cast<std::int32_t>(option); };
        hooks.readSavedState=[this] { Emit("saved","SAVED-STATE:"+std::to_string(saved));return saved; };
        hooks.wallSeconds=[this] { Emit("time","TIME:"+StartupDoubleHex(now));return now; };
        hooks.translate=[this](std::string_view text) { Emit("translate","TRANSLATE:"+StartupHex(text),++translations);return text; };
        hooks.formatButtons=[this](auto format,auto yes,auto no)->std::string_view { CHECK(yes=="Yes" && no=="No");Emit("format","FORMAT:"+StartupHex(format),++formats);return "ExplicitButtons"; };
        hooks.formatWarning=[this](auto format,double minutes,auto buttons)->std::string_view { CHECK(buttons=="ExplicitButtons");Emit("format","FORMAT:"+StartupHex(format)+":elapsed64="+StartupDoubleHex(minutes),++formats);return "ExplicitWarning"; };
        hooks.tell=[this](auto text,auto first,auto second) { CHECK(first==0 && second==0);Emit("tell","TELL:"+StartupHex(text)+":0:0"); };return hooks;
    }
};
}

// 정상ret EAX와 모든 호출/부분pending을 대조한다. 중단시EAX는호스트ABI가달라분석자료로만보존한다.
TEST_CASE(StartupStateCheck_All390OriginalParentObservations) {
    std::ifstream input(NETSTORM_STARTUP_STATE_CHECK_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t compared{}; //실제로대조한관측수이며이전외곽관측을더하지않는다.
    while (std::getline(input,line)) { //최초두x87정밀도자료를변경하지않고각각같은부모에대조한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=StartupFields(line,'\t');if (row.size()!=6) throw std::logic_error("startup 최초자료 칸 수");
        const auto fields=StartupFields(row[1],',');if (fields.size()!=4) throw std::logic_error("startup 최초입력 칸 수");
        StartupDevice device;device.option=static_cast<std::uint32_t>(std::stoull(fields[0]));device.saved=static_cast<std::int32_t>(std::stoll(fields[1]));device.now=StartupDouble(fields[2]);device.mutation=fields[3];
        std::string completion="return";std::uint32_t result{}; //부모정상반환과명시하위중단을별도로대조한다.
        try { result=CheckStartupState(device.Hooks()); }catch (const StartupStop& stop) { completion=stop.cause; }
        auto expectedTrace=row[5];const std::string annotation="STARTUP:editor=1;"; //기존oracle의진입debug항목이며원문함수의외부호출은아니다.
        CHECK(expectedTrace.starts_with(annotation));if (expectedTrace.starts_with(annotation)) expectedTrace.erase(0,annotation.size());
        if (completion!=row[2] || device.trace!=expectedTrace || StartupHex(device.pending)!=row[4] || (completion=="return" && result!=std::stoul(row[3]))) {
            std::fprintf(stderr,"Startup input=%s completion=%s expected=%s\ntrace=%s\nexpected=%s\n",row[1].c_str(),completion.c_str(),row[2].c_str(),device.trace.c_str(),expectedTrace.c_str());
            CHECK(completion==row[2]);CHECK(device.trace==expectedTrace);CHECK(StartupHex(device.pending)==row[4]);if (completion=="return") CHECK(result==std::stoul(row[3]));return;
        }
        ++compared;
    }
    CHECK(compared==390);
}

// 설정0은미선택장치를요구하지않고비영에서실제로누락된reader만거부한다.
TEST_CASE(StartupStateCheck_SelectedDevicesAreRequired) {
    StartupStateCheckHooks hooks;hooks.configInt=[](auto) { return 0; };CHECK(CheckStartupState(hooks)==0);
    hooks.configInt=[](auto) { return 1; };bool rejected=false;try { CheckStartupState(hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    std::uint32_t walls{};hooks.readSavedState=[] { return 0; };hooks.wallSeconds=[&] { ++walls;return 700.0; };CHECK(CheckStartupState(hooks)==0 && walls==1);
}
