// 최초258관측중선택본문에진입한startup옵션0입력만독립C++외곽에투영해대조한다.
#include "TestSupport.h"
#include "o/RawExistingEditorInstallOuter.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;

namespace {
// 마지막빈칸을보존해독립입력/관측을분리한다.
std::vector<std::string> OuterFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; //현재분리위치이며NUL/hex를정규화하지않는다.
    // 마지막원시문자열까지그대로반환한다.
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; }
}
// 물리BYTE전체를고정소문자hex로관측한다.
std::string OuterHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //NUL뒤도각BYTE두글자다.
    // 실제순서와범위를그대로보존한다.
    for (const auto byte:bytes) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; }return result;
}
// 원문헤더의지정DWORD만little endian으로쓴다.
void OuterPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // manager이외주변필드도비교할수있도록네BYTE만바꾼다.
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 실제NUL앞만문자열로빌리고종료문자없는물리는명시거부한다.
std::string_view OuterString(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("Editor 외곽장치NUL없음");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// NUL뒤기존BYTE를유지한채입력이름만같은주소에쓴다.
void OuterName(std::span<std::uint8_t> bytes,std::string_view text) {
    if (text.size()>=bytes.size()) throw std::out_of_range("Editor 외곽장치이름물리범위");std::copy(text.begin(),text.end(),bytes.begin());bytes[text.size()]=0;
}
// 명시하위중단/범위밖분기를전체성공에서구별한다.
struct OuterStop { std::string cause; }; //최초관측의도달점이름이다.
// 초기prefix와startup본문을대신구현하지않고선택본문의실제하위입력을제공한다.
struct OuterDevices {
    std::string mutation,trace;std::uint32_t owner{},other{},battleOwner{7},subscriber{17},mission{},editor{1},exists{},valid{},stateCalls{},notifyCalls{}; //명시prefix완료후현재값과독립입력이다.
    std::array<std::uint8_t,180> headers{};std::array<std::uint8_t,252> states{}; //서로다른9헤더/9State와주변BYTE다.
    std::array<std::uint8_t,256> current{},pending{},handle{}; //native세고정이름주소의실제할당이다.
    std::map<std::string,std::string> configs;bool nullState{}; //실제설정쓰기와반환State0물리중단을보관한다.
    // 기대관측을읽지않고초기입력에서선택본문context만구성한다.
    explicit OuterDevices(std::string_view description) {
        const auto values=OuterFields(description,',');if (values.size()!=9 || values[4]!="0") throw std::invalid_argument("Editor 외곽C++선택입력범위");const auto names=static_cast<std::size_t>(std::stoul(values[1]));owner=static_cast<std::uint32_t>(std::stoul(values[2]));other=owner==1 ? 8U : 1U;mission=static_cast<std::uint32_t>(std::stoul(values[5]));exists=static_cast<std::uint32_t>(std::stoul(values[6]));valid=static_cast<std::uint32_t>(std::stoul(values[7]));mutation=values[8];const auto flags=static_cast<std::uint8_t>(std::stoul(values[3])); //원문prefix는별도최초관측에남기고재구현하지않는다.
        // 헤더의미사용BYTE도전체비교한다.
        for (std::size_t byte=0;byte<headers.size();++byte) headers[byte]=static_cast<std::uint8_t>(0x50+byte);
        // 각State의첫BYTE만입력flags로바꾸고주변은서로구별한다.
        for (std::size_t slot=0;slot<9;++slot) {
            OuterPut(headers,slot*20,0x12007000U+static_cast<std::uint32_t>(slot)*0x20U);
            // 원문명시State장치와같은27BYTE주변을둔다.
            for (std::size_t byte=0;byte<28;++byte) states[slot*28+byte]=static_cast<std::uint8_t>(0x80+slot+byte*7);states[slot*28]=flags;
        }
        current.fill(0xee);pending.fill(0xee);handle.fill(0xee);OuterName(handle,"BeforeSubscriber");
        constexpr std::array<std::string_view,5> kCurrent{"Existing","Existing","Old","",".temp"}; //파일이름비교/빈이름의독립입력이다.
        constexpr std::array<std::string_view,5> kPending{"Existing","eXiStInG","Existing","",".TEMP"}; //대소문자비교는명시CRT장치다.
        OuterName(current,kCurrent.at(names));OuterName(pending,kPending.at(names));
    }
    // 실제선택한하위의순서를기록한다.
    void Record(std::string event) { if (!trace.empty()) trace+=';';trace+=event; }
    // callback외부효과만바꾸며본문의현재값조회/분기를제품에맡긴다.
    void Mutate(std::string_view boundary) {
        const auto name=std::string(boundary); //최초fixture의명시경계이름이다.
        if (mutation==name+"-owner") owner=other;
        else if (mutation==name+"-name") OuterName(current,"ChangedDuringInstall");
        else if (mutation==name+"-editor0") editor=0;
        else if (mutation==name+"-stop") throw OuterStop{name};
    }
    // 도달한게임하위는고유trace와외부효과만공급한다.
    void Effect(std::string_view name,std::string event) { Record(event);Mutate(name); }
    // 원문다른분기를편의상완료로반환하지않는다.
    [[noreturn]] void Outside(ExistingEditorInstallOutside value) {
        const auto cause=value==ExistingEditorInstallOutside::DefaultName ? "outside-new-name" : value==ExistingEditorInstallOutside::NewFort ? "outside-new-fort" : value==ExistingEditorInstallOutside::InvalidFile ? "outside-invalid-file" : "outside-mission";throw OuterStop{cause}; //native는해당분기첫명령전에중단했다.
    }
    // callback이owner를바꾸어도원문이반환한주소를따로유지한다.
    std::span<std::uint8_t> State(std::uint32_t selected) {
        ++stateCalls;Record("STATE:"+std::to_string(selected));Mutate("state"+std::to_string(stateCalls));
        if (stateCalls==2 && mutation=="state2-returnother") selected=other;if (stateCalls==2 && mutation=="state2-zero") { nullState=true;return {}; }return std::span(states).subspan(selected*28,28);
    }
    // State수정뒤현재슬롯으로통지하는각위치를구별한다.
    void Notify(std::uint32_t selected) { ++notifyCalls;Effect("notify"+std::to_string(notifyCalls),"NOTIFY:"+std::to_string(selected)); }
    // 실제현재설정입력만쓰기전에기록하고callback을실행한다.
    void Config(std::string_view key,std::string_view text) {
        configs[std::string(key)]=std::string(text);Effect(key=="lastEditFort" ? "lastedit" : "handleconfig","CONFIG:"+OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(key.data()),key.size()))+':'+OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())));
    }
    // 기존파일부모의하위경계이며State|1/순서/현재owner는제품이소유한다.
    ExistingEditorFileHooks FileHooks() {
        ExistingEditorFileHooks hooks; //현재owner8은native명시owners장치의후속입력이며실제0048f3b0(1,1)결과라고주장하지않는다.
        hooks.path=[this] { Effect("path","PATH:456469746f7246696c65732f4578697374696e672e666f7274");return std::string_view("EditorFiles/Existing.fort"); };
        hooks.exists=[this](auto) { Effect("exists","EXISTS:"+std::to_string(exists));return static_cast<std::int32_t>(exists); };
        hooks.owner=[this] { return owner; };
        hooks.load=[this](auto selected,auto path) { Effect("load","LOAD:"+std::to_string(selected)+':'+OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(path.data()),path.size()))); };
        hooks.valid=[this](auto selected) { Effect("valid","VALID:"+std::to_string(selected)+':'+std::to_string(valid));return static_cast<std::int32_t>(valid); };
        hooks.manager=[this](auto slot) { return std::uint32_t(headers.at(slot*20))|(std::uint32_t(headers.at(slot*20+1))<<8)|(std::uint32_t(headers.at(slot*20+2))<<16)|(std::uint32_t(headers.at(slot*20+3))<<24); };
        hooks.assertion=[this](auto line) { Effect("assert","ASSERT:"+std::to_string(line)); };
        hooks.state=[this](auto selected) { return State(selected); };hooks.notify=[this](auto selected) { Notify(selected); };
        hooks.subscriber=[this](auto selected) { subscriber=0x100+selected;OuterName(handle,"Subscriber"+std::to_string(selected));Effect("subscriber","SUBSCRIBER:"+std::to_string(selected)+":540c74:handle"); };
        hooks.handle=[this] { return OuterString(handle); };hooks.setConfig=[this](auto key,auto text) { Config(key,text); };
        hooks.title=[this] { Effect("title","TITLE:"+OuterHex(std::span(current).first(OuterString(current).size()))+':'+OuterHex(std::span(handle).first(OuterString(handle).size()))); };
        hooks.outside=[this](auto value) { Outside(value); };return hooks;
    }
    // 현재Session이소유할World전체하위와외부UI만분리하며본문State/순서를실행하지않는다.
    ExistingEditorFortHooks FortHooks() {
        ExistingEditorFortHooks hooks; //startup옵션0만이검사의명시입력이며비영본문은258관측의별도분석범위다.
        hooks.currentName=[this] { return OuterString(current); };hooks.pendingName=[this] { return OuterString(pending); };
        hooks.compare=[this](std::string_view left,std::string_view right) { Record("COMPARE:"+OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(left.data()),left.size()))+':'+OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(right.data()),right.size())));auto first=std::string(left),second=std::string(right);std::transform(first.begin(),first.end(),first.begin(),[](unsigned char value) { return static_cast<char>(value>='A' && value<='Z' ? value+32 : value); });std::transform(second.begin(),second.end(),second.begin(),[](unsigned char value) { return static_cast<char>(value>='A' && value<='Z' ? value+32 : value); });return first==second ? 0 : 1; };
        hooks.inMission=[this] { return mission; };hooks.missionName=[this](auto text) { Effect("missionname","MISSIONNAME:"+OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())));return 0; };
        hooks.closeGuide=[this] { if (mutation=="guide-clear") mission=0;Effect("guide","GUIDECLOSE"); };
        hooks.setName=[this](auto text) { const auto name=std::string(text);OuterName(current,name);Effect("name","NAME:"+OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(name.data()),name.size()))); };
        hooks.file=[this] { InstallExistingEditorFile(FileHooks()); }; //최초도구의callsite부가INSTALL오측정은제품의경계로추가하지않는다.
        hooks.world=[this] { Effect("player","PLAYER:"+std::to_string(battleOwner));Effect("coreread","COREREAD:"+std::to_string(owner)+":0");Effect("recover","RECOVER:owner="+std::to_string(owner));Effect("world","WORLD:0"); };
        hooks.owner=[this] { return owner; };hooks.deck=[this](auto selected) { Effect("deck","DECK:"+std::to_string(selected)); };
        hooks.priests=[this] { Effect("priests","PRIESTS"); };hooks.camera=[this] { Effect("camera","CAMERA"); };hooks.production=[this] { Effect("production","PRODUCTION"); };
        hooks.coreWrite=[this](auto selected) { Effect("corewrite","COREWRITE:"+std::to_string(selected)); };hooks.snapshot=[this](auto selected) { Effect("snapshot","SNAPSHOT:"+std::to_string(selected)); };
        hooks.state=[this](auto selected) { return State(selected); };hooks.notify=[this](auto selected) { Notify(selected); };
        hooks.startupStateCheck=[this] { Effect("startup","STARTUP:editor="+std::to_string(editor));Effect("startup-option","STARTUP-OPTION:0"); };
        hooks.editor=[this] { return editor; };hooks.setConfig=[this](auto key,auto text) { Config(key,text); };hooks.editorPlayers=[this] { Effect("editorplayers","EDITORPLAYERS:handle="+OuterHex(std::span(handle).first(OuterString(handle).size()))); };
        hooks.outside=[this](auto value) { Outside(value); };return hooks;
    }
    // 선택본문완료를전체State함수정상반환/loaded로표현하지않는다.
    std::string Run() { try { InstallExistingEditorFort(FortHooks());return "checkpoint-editor-children-returned"; }catch (const OuterStop& stop) { return stop.cause; }catch (const std::out_of_range&) { if (nullState) return "state0";throw; } }
    // 실제선택본문이읽거나바꾸는현재8필드만원문전체관측에서투영한다.
    std::string Values() const { return std::to_string(editor)+','+std::to_string(mission)+','+std::to_string(owner)+','+std::to_string(battleOwner)+','+std::to_string(subscriber)+",306184192,306184704,306185216"; }
    // 초기prefix의144BYTE요청복사는별도원문관측으로남기고선택본문원시만비교한다.
    std::string Raw() const { return OuterHex(headers)+':'+OuterHex(states)+':'+OuterHex(current)+':'+OuterHex(pending)+':'+OuterHex(handle); }
    // 실제설정쓰기만원문정렬형식으로관측한다.
    std::string Configs() const {
        std::string result; //원문byte key의사전순서다.
        // 이름과내용을NUL제외같은hex로보존한다.
        for (const auto& [key,value]:configs) { if (!result.empty()) result+=';';result+=OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(key.data()),key.size()))+'='+OuterHex(std::span(reinterpret_cast<const std::uint8_t*>(value.data()),value.size())); }return result;
    }
};
}

// 원문전체258중prefix도달/옵션0선택범위만대조하고제외행수를명시한다.
TEST_CASE(ExistingEditorInstallOuter_IndependentNativeProjection) {
    std::ifstream input(NETSTORM_EXISTING_EDITOR_INSTALL_OUTER_FIXTURE);if (!input) throw std::runtime_error("Editor 외곽최초fixture없음");std::string line;std::size_t total{},compared{},excluded{};std::set<std::pair<std::string,std::string>> unique; //두정밀도중복도거부한다.
    // 기대관측을실행입력으로사용하지않고필터의입력/원문도달점만선별한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=OuterFields(line,'\t');if (row.size()!=7) throw std::runtime_error("Editor 외곽fixture관측칸수");CHECK(unique.emplace(row[0],row[1]).second);++total;const auto fields=OuterFields(row[1],',');
        if (fields[4]!="0" || fields[8]=="poll-stop" || fields[8]=="cleanup-stop") { ++excluded;continue; }
        OuterDevices device(row[1]);const auto completion=device.Run();const auto values=OuterFields(row[3],',');constexpr std::array<std::size_t,8> kFields{3,5,8,9,10,18,19,20};std::string projected; //모듈이소유한현재필드의고정위치다.
        // 원문prefix가소유한전역은제품에포함한것처럼비교하지않는다.
        for (const auto field:kFields) { if (!projected.empty()) projected+=',';projected+=values.at(field); }
        const auto raw=OuterFields(row[4],':');std::string projectedRaw; //헤더/9State/세현재이름만실제모듈범위다.
        // 원문관측의앞다섯실제할당을순서대로고른다.
        for (std::size_t index=0;index<5;++index) { if (index) projectedRaw+=':';projectedRaw+=raw.at(index); }
        const auto start=row[6].find("COMPARE:");if (start==std::string::npos) throw std::runtime_error("Editor 외곽원문선택진입없음");std::string projectedTrace; //실제선택하위와구별되는원문도구의부가callsite오측정한개를제외한다.
        // 최초fixture는보존하고ESP+4를잘못읽은INSTALL debug항목만대조범위에서명시제외한다.
        for (const auto& event:OuterFields(std::string_view(row[6]).substr(start),';')) { if (event.starts_with("INSTALL:alternate-before=")) continue;if (!projectedTrace.empty()) projectedTrace+=';';projectedTrace+=event; }
        if (completion!=row[2] || device.Values()!=projected || device.Raw()!=projectedRaw || device.Configs()!=row[5] || device.trace!=projectedTrace) std::fprintf(stderr,"Editor outer fixture input=%s\n",row[1].c_str());
        CHECK(completion==row[2]);CHECK(device.Values()==projected);CHECK(device.Raw()==projectedRaw);CHECK(device.Configs()==row[5]);CHECK(device.trace==projectedTrace);++compared;
    }
    CHECK(total==258 && compared+excluded==total);CHECK(compared>100 && excluded>0);std::printf("Editor outer native=%zu compared=%zu excluded=%zu\n",total,compared,excluded);
}

// 필수startup누락은앞선저장/State/Notify효과를보존하며빈분기소비자는성공으로귀환하지않는다.
TEST_CASE(ExistingEditorInstallOuter_RequiredAndPartialBoundaries) {
    OuterDevices device("1,0,1,255,0,0,1,1,none");auto hooks=device.FortHooks();hooks.startupStateCheck={};bool rejected=false;
    try { InstallExistingEditorFort(hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && device.stateCalls==2 && device.notifyCalls==2 && device.states[28]==254 && device.configs.contains("fort.mySubHandle") && !device.configs.contains("lastEditFort"));
    OuterDevices other("1,3,1,255,0,0,1,1,none");hooks=other.FortHooks();hooks.outside=[](auto) {};rejected=false;try { InstallExistingEditorFort(hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && other.stateCalls==0 && other.configs.empty());
}
