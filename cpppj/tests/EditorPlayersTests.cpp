// 원본Editor Player273BYTE의최초666관측과실제필드/현재값/직접생성trace를대조한다.
#include "TestSupport.h"
#include "o/RawEditorPlayers.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;

namespace {
constexpr std::size_t kEditorPlayerBytes=9*180; //두현재Player표각각의실제전체물리범위다.
// fixture의마지막빈필드까지보존한다.
std::vector<std::string> EditorFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; //현재분리위치와소유입력이다.
    // 모든구분자뒤마지막칸까지원래순서를유지한다.
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; }
}
// 원문은little endian이므로비정렬네BYTE만조립한다.
std::uint32_t EditorRead(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result{}; //지정DWORD의바이트만포함한다.
    // 현재네바이트를낮은비트부터합친다.
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8);return result;
}
// 실제Player기록에명시callback의네BYTE효과만쓴다.
void EditorPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    // 원본필드폭밖바이트는유지한다.
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 모든raw바이트를NUL뒤도자르지않고고정소문자hex로관측한다.
std::string EditorHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //한BYTE를두글자로표시한다.
    // 전체물리범위를원문순서로표현한다.
    for (const auto byte:bytes) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; }return result;
}
// 원본외부callback에서지정한중단을정상반환과구분한다.
struct EditorStop { std::string cause; }; //알수없는제품예외는이표식으로삼키지않는다.
// 원문기대필드를읽지않고최초입력/명시하위응답만준비한다.
struct EditorDevices {
    std::int32_t count{}; //00541348의현재signedcount이며callback이바꾼다.
    std::uint32_t variant{},language{},initialFlags{},parentMode{},playerBase{},handleBase{},poolBase{}; //독립초기입력과현재주소선택이다.
    std::uint32_t owner{},parentCalls{},creates{},finishCalls{},countReads{},recordCalls{},typeIndex{},capturedBase{},playerReads{},handleReads{}; //외부경계순번과캡처레코드주소다.
    std::string mutation,trace; //외부효과입력과실제관측순서다.
    std::array<std::array<std::uint8_t,kEditorPlayerBytes>,2> players{}; //한번선택한레코드는다른표가현재가되어도같은주소에남는다.
    std::array<std::array<std::uint8_t,64>,2> handles{}; //현재handle의NUL뒤원시바이트도보존한다.
    std::array<RiftTypeRecord,32> types{}; //TYPEgetter뒤필터가현재group/theme를읽는실제기록이다.
    std::array<std::vector<std::uint8_t>,3> translations; //번역슬롯마다NUL포함수명이서로구별된다.
    std::vector<std::uint8_t> formatted; //동기이름복사때까지유지하는명시서식결과다.

    // 원문과같은입력패턴을쓰되생성될기술목록은계산하지않는다.
    explicit EditorDevices(std::string_view input) {
        const auto fields=EditorFields(input,',');if (fields.size()!=6) throw std::runtime_error("Editor Player fixture입력칸수오류");count=std::stoi(fields[0]);variant=static_cast<std::uint32_t>(std::stoul(fields[1]));language=static_cast<std::uint32_t>(std::stoul(fields[2]));initialFlags=static_cast<std::uint32_t>(std::stoul(fields[3]));parentMode=static_cast<std::uint32_t>(std::stoul(fields[4]));mutation=fields[5];
        // 두표의미변경바이트와각owner상위flags를서로구별한다.
        for (std::size_t base=0;base<players.size();++base) {
            for (std::size_t offset=0;offset<kEditorPlayerBytes;++offset) players[base][offset]=static_cast<std::uint8_t>(0xa5+offset*13+base*7); //원시주변입력패턴이다.
            for (std::uint32_t selected=0;selected<9;++selected) EditorPut(players[base],selected*180+0x50,initialFlags^(selected<<20)^(static_cast<std::uint32_t>(base)<<28)); //flags입력은제품이실제마스크한다.
            for (std::size_t offset=0;offset<handles[base].size();++offset) handles[base][offset]=static_cast<std::uint8_t>(0xd0+offset+base*7); //이름NUL뒤보존검사용입력이다.
        }
        // 독립TYPE입력만채우며eligible판단은제품/원문에맡긴다.
        for (std::size_t number=0;number<types.size();++number) {
            auto& kind=types[number];if (!variant) { kind.group=number%5==4 ? 10 : static_cast<std::int32_t>(number%4);kind.theme=number%4==3 ? 5 : static_cast<std::int32_t>(number%4); }
            else if (variant==1) { kind.group=number%2 ? 10 : static_cast<std::int32_t>(number%4);kind.theme=number%3==0 ? 5 : static_cast<std::int32_t>(number%4); }else { kind.group=10;kind.theme=0; }
        }
        constexpr std::array<std::string_view,3> kEnglish{"Human Player","Player","Computer #%d"}; //명시영문번역의NUL앞원문이다.
        const std::array<std::string,3> other{std::string("J\xf6ueur"),"Moi","Bot [%d]"}; //UTF8로변환하지않는원시CP1252입력이다.
        // 빈번역과각언어의NUL도실제입력바이트로구성한다.
        for (std::size_t index=0;index<translations.size();++index) { const auto text=language==0 ? std::string(kEnglish[index]) : language==1 ? std::string() : other[index];translations[index].assign(text.begin(),text.end());translations[index].push_back(0); }
    }
    // 원본하위진입순서를그대로기록한다.
    void Record(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 현재전역또는이미캡처한실제레코드의명시효과만쓴다.
    void Mutate(std::string_view boundary) {
        if (mutation==std::string(boundary)+"-players1") playerBase=1;
        else if (mutation=="player-handle1" && boundary=="player") handleBase=1;
        else if (mutation=="format2-flags" && boundary=="format2") EditorPut(players.at(capturedBase),owner*180+0x50,0x5a5a01fcU);
        else if (mutation=="technology2-count3" && boundary=="technology2") count=3;
        else if (mutation=="format2-count0" && boundary=="format2") count=0;
        else if (mutation=="record1-count0" && boundary=="record1") count=0;
        else if ((mutation=="record1-count12" || mutation=="create1-count12") && boundary==mutation.substr(0,mutation.find('-'))) count=12;
        else if (mutation=="record1-eligible" && boundary=="record1") { types.at(typeIndex).group=0;types.at(typeIndex).theme=0; }
        else if (mutation=="create1-pool1" && boundary=="create1") poolBase=1;
        else if (mutation=="create1-parentrecord" && boundary=="create1") EditorPut(players.at(playerBase),owner*180+0x9c,999);
        else if (mutation==std::string(boundary)+"-stop") throw EditorStop{std::string(boundary)};
    }
    // 실제번역/기술부모/서식/TYPE/Contained경계만연결하며필드쓰기나eligible판단은복제하지않는다.
    EditorPlayersHooks Hooks() {
        EditorPlayersHooks hooks;
        hooks.translate=[this](std::string_view key)->std::span<const std::uint8_t> {
            const auto index=key=="Human Player" ? 0U : key=="Player" ? 1U : key=="Computer #%d" ? 2U : 3U;if (index>=3) throw std::logic_error("Editor미등록번역키");const auto& source=translations[index];Record("T:"+std::to_string(index)+':'+EditorHex(std::span(source).first(source.size()-1)));if (index==2) capturedBase=playerBase;
            Mutate(index==0 ? std::string("human") : index==1 ? std::string("player") : "computer"+std::to_string(owner));return source;
        };
        hooks.players=[this]()->std::span<std::uint8_t> { ++playerReads;return players.at(playerBase); };
        hooks.handle=[this]()->std::span<std::uint8_t> { ++handleReads;return handles.at(handleBase); };
        hooks.technology=[this](std::uint32_t selected) { owner=selected;++parentCalls;const auto parent=parentMode ? owner==2 ? 0U : 500+owner : 100+owner;Record("P:"+std::to_string(owner)+':'+std::to_string(parent));EditorPut(players.at(playerBase),owner*180+0x9c,parent);Mutate("technology"+std::to_string(owner));return Sid{static_cast<std::uint16_t>(parent)}; };
        hooks.format=[this](std::uint32_t capacity,std::span<const std::uint8_t> source,std::int32_t number)->std::span<const std::uint8_t> {
            const auto end=std::find(source.begin(),source.end(),std::uint8_t{});if (end==source.end() || !capacity) throw std::out_of_range("Editor서식명시입력범위");const auto length=static_cast<std::size_t>(end-source.begin());Record("M:"+std::to_string(capacity)+':'+std::to_string(number)+':'+EditorHex(source.first(length)));Mutate("format"+std::to_string(owner));
            std::string result(reinterpret_cast<const char*>(source.data()),length);const auto marker=result.find("%d");if (marker!=result.npos) result.replace(marker,2,std::to_string(number));if (result.size()>=capacity) result.resize(capacity-1);formatted.assign(result.begin(),result.end());formatted.push_back(0);return formatted;
        };
        hooks.typeCount=[this] { ++countReads;Record("N:"+std::to_string(owner)+':'+std::to_string(count));return count; };
        hooks.typeAt=[this](std::int32_t index)->const RiftTypeRecord& { typeIndex=static_cast<std::uint32_t>(index);auto& kind=types.at(typeIndex);++recordCalls;Record("R:"+std::to_string(index)+':'+std::to_string(kind.group)+':'+std::to_string(kind.theme));Mutate("record"+std::to_string(recordCalls));return kind; };
        hooks.createContained=[this](Sid parent,std::uint32_t type,std::uint32_t argument,std::uint32_t flags,std::uint32_t attach,std::uint32_t quality) { ++creates;Record("C:"+std::to_string(poolBase)+':'+std::to_string(parent.value)+':'+std::to_string(type)+':'+std::to_string(argument)+':'+std::to_string(flags)+':'+std::to_string(attach)+':'+std::to_string(quality));Mutate("create"+std::to_string(creates));return Sid{static_cast<std::uint16_t>(700+creates)}; };
        hooks.finish=[this] { ++finishCalls;Record("F");Mutate("finish"); };return hooks;
    }
    // 본체실행뒤현재값만fixture와같은십진형식으로읽는다.
    std::string State() const { return std::to_string(count)+','+std::to_string(playerBase)+','+std::to_string(handleBase)+','+std::to_string(poolBase)+','+std::to_string(parentCalls)+','+std::to_string(creates)+','+std::to_string(finishCalls)+','+std::to_string(countReads); }
    // 두전체Player표와두handle의모든바이트를비교한다.
    std::string Raw() const { return EditorHex(players[0])+':'+EditorHex(players[1])+':'+EditorHex(handles[0])+':'+EditorHex(handles[1]); }
};
}

// 최초333입력의두정밀도666관측·명시중단40까지모든현재효과를직접대조한다.
TEST_CASE(EditorPlayers_IndependentNative666) {
    std::ifstream input(NETSTORM_EDITOR_PLAYERS_FIXTURE);if (!input) throw std::runtime_error("Editor Player최초fixture없음");std::string line;std::size_t compared{},stopped{};std::set<std::pair<std::string,std::string>> unique;
    // 앞입력열만으로제품을실행하고그뒤관측기대값을읽는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=EditorFields(line,'\t');if (row.size()!=6) throw std::runtime_error("Editor Player관측칸수오류");CHECK(unique.emplace(row[0],row[1]).second);
        EditorDevices devices(row[1]);std::string completion="1";try { InitializeEditorPlayers(devices.Hooks()); }catch (const EditorStop& stop) { completion=stop.cause;++stopped; }
        CHECK(completion==row[2]);CHECK(devices.State()==row[3]);CHECK(devices.Raw()==row[4]);CHECK(devices.trace==row[5]);++compared;
    }
    CHECK(compared==666 && stopped==40);
}

// 중복지급방지가없는직접Create와선택되지않는경계/부분효과를별도검사한다.
TEST_CASE(EditorPlayers_DirectRepeatedAndSelectedBoundaries) {
    EditorDevices repeated("1,0,0,4294967295,0,none");InitializeEditorPlayers(repeated.Hooks());InitializeEditorPlayers(repeated.Hooks());CHECK(repeated.creates==14 && repeated.parentCalls==14 && repeated.finishCalls==2 && repeated.playerReads==16 && repeated.handleReads==2); // 같은parent/type도매호출직접생성한다.
    EditorDevices excluded("16,2,0,0,0,none");auto hooks=excluded.Hooks();hooks.createContained={};InitializeEditorPlayers(hooks);CHECK(excluded.creates==0 && excluded.parentCalls==7 && excluded.finishCalls==1);
    EditorDevices missing("0,0,0,0,0,none");hooks=missing.Hooks();hooks.format={};bool rejected=false;try { InitializeEditorPlayers(hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && missing.parentCalls==1 && EditorRead(missing.players[0],2*180+0x2c)==2 && missing.finishCalls==0);
    EditorPlayersHooks physical;const std::array<std::uint8_t,3> source{'A','B',0};physical.translate=[&](auto)->std::span<const std::uint8_t> { return source; };std::array<std::uint8_t,0xc2> smallPlayers{};physical.players=[&]()->std::span<std::uint8_t> { return smallPlayers; };rejected=false;
    try { InitializeEditorPlayers(physical); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && smallPlayers[0xc0]=='A' && smallPlayers[0xc1]=='B'); // 원본BYTE복사의선행부분효과를되돌리지않는다.
}
