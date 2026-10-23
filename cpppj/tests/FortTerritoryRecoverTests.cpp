// 제품보다먼저고정한Territory회복974관측의현재값/주변원시/호출순서를독립장치로대조한다.
#include "TestSupport.h"
#include "o/RawFortTerritoryRecover.h"
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
constexpr std::array<std::uint32_t,3> kRecoverManagers{0U,0x12001000U,0x12002000U}; //native입력0/11/22의실제manager주소토큰이다.
constexpr std::array<std::uint32_t,3> kRecoverStreams{0x12003000U,0x12003020U,0x12003040U}; //Create반환stream2와manager.current0/1을구별한다.
constexpr std::array<std::uint32_t,3> kRecoverBuffers{0x12004000U,0x12004200U,0x12004400U}; //진단상capacity와별개인실제256BYTE출력주소토큰이다.
constexpr std::string_view kRecoverMessage="Territory recovered from Battle!\n"; //native로그의NUL제외문자열이다.
// 마지막빈칸도보존해입력과관측열을구별한다.
std::vector<std::string> RecoverFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; //현재분리위치이며원시hex를정규화하지않는다.
    // 마지막문자열까지원문순서로그대로반환한다.
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; }
}
// 비정렬DWORD를little endian으로읽는다.
std::uint32_t RecoverGet(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t result{}; //지정한네BYTE만합친다.
    // 원본메모리순서와같이낮은BYTE부터읽는다.
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result;
}
// 같은네BYTE만갱신해주변필드손상을관측한다.
void RecoverPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 원본little endian순서로해당필드만쓴다.
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 모든주변BYTE를고정소문자hex로관측한다.
std::string RecoverHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //NUL도포함하여BYTE당두글자다.
    // 실제범위를잘라내거나문자열로해석하지않는다.
    for (const auto byte:bytes) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; }return result;
}
// 입력flags조합만공급하며로그여부와회복결과는제품이계산한다.
std::array<std::uint8_t,120> RecoverRecords(std::uint32_t slot,std::uint32_t pattern) {
    std::array<std::uint8_t,120> result{}; //native와독립적으로같은초기물리입력을구성한다.
    // flags이외BYTE도슬롯마다달라현재owner재조회누락을검출한다.
    for (std::size_t byte=0;byte<result.size();++byte) result[byte]=static_cast<std::uint8_t>(byte*13+7+slot*11);
    // 각6BYTE기록의두번째BYTE만입력패턴에따라지정한다.
    for (std::size_t index=0;index<20;++index) {
        auto& value=result[index*6+1];value&=0xfbU; //기존다른flags비트는그대로둔다.
        if (pattern==4) value=255;else if (pattern==1 || (pattern==2 && index%2==0) || (pattern==3 && index==19)) value|=4U;
    }
    return result;
}
// 명시callback중단과원본0주소물리접근을정상반환에서구별한다.
struct RecoverStop { std::string cause; }; //기대관측에실제도달한중단원인이다.
// 명시getter/State/Create/로그/진단과실제MemStream을소유하는fixture입력장치다.
struct RecoverDevices {
    std::string kind,mutation,trace; //독립진입점/외부효과/실제관측순서다.
    std::uint32_t owner{},other{},fixed{},nullInput{}; //현재owner와고정writer슬롯/실제NULL입력조건이다.
    std::array<std::uint32_t,6> calls{}; //read/log/create/write/state/assert각실제호출수다.
    std::array<std::uint8_t,180> headers{}; //9×20BYTE헤더이며manager이외주변BYTE도비교한다.
    std::array<std::array<std::uint8_t,16>,2> managers{}; //원본manager11/22의현재stream+8을포함한다.
    std::array<std::array<std::uint8_t,16>,3> streams{}; //현재data/pos/end/capacity의실제16BYTE다.
    std::array<std::array<std::uint8_t,256>,3> buffers{}; //capacity진단뒤에도분리된실제물리범위를쓴다.
    std::array<std::array<std::uint8_t,120>,9> sources{}; //getter가선택한현재슬롯의독립입력이다.
    std::array<std::array<std::uint8_t,28>,9> states{}; //반환State주소의첫BYTE이외주변을함께관측한다.
    std::array<std::uint8_t,120> input{},localSnapshot{}; //직접writer원문과부모지역수명안에복사한최종원시다.
    std::span<std::uint8_t> local; //동기callback동안만유효한실제부모지역버퍼다.반환뒤역참조하지않는다.
    std::size_t outputStream{},returnedState{};bool stateNull{}; //현재일반writer와Statecallback의물리반환주소를구별한다.

    // 기대관측을읽지않고입력열의값만으로모든물리장치를초기화한다.
    explicit RecoverDevices(std::string_view description) {
        const auto fields=RecoverFields(description,',');if (fields.size()!=10) throw std::runtime_error("Territory fixture 입력칸수오류");
        kind=fields[0];owner=static_cast<std::uint32_t>(std::stoul(fields[1]));const auto manager=static_cast<std::uint32_t>(std::stoul(fields[2]));const auto pattern=static_cast<std::uint32_t>(std::stoul(fields[3]));const auto state=static_cast<std::uint8_t>(std::stoul(fields[4])); //owner별입력은서로구별한다.
        const auto position=static_cast<std::uint32_t>(std::stoul(fields[5]));const auto end=static_cast<std::uint32_t>(std::stoul(fields[6]));const auto capacity=static_cast<std::uint32_t>(std::stoul(fields[7]));nullInput=static_cast<std::uint32_t>(std::stoul(fields[8]));mutation=fields[9];other=(owner+1)%9;fixed=owner;
        // manager필드밖의헤더BYTE를각위치별로그대로초기화한다.
        for (std::size_t byte=0;byte<headers.size();++byte) headers[byte]=static_cast<std::uint8_t>(0x70+byte);
        // 슬롯마다별개입력/State를두고현재두manager만명시설정한다.
        for (std::size_t slot=0;slot<sources.size();++slot) {
            RecoverPut(headers,slot*20,0);sources[slot]=RecoverRecords(static_cast<std::uint32_t>(slot),pattern);
            // State의첫BYTE뒤27BYTE도변하지않는지검사한다.
            for (std::size_t byte=0;byte<states[slot].size();++byte) states[slot][byte]=static_cast<std::uint8_t>(0x80+slot+byte*7);states[slot][0]=state;
        }
        RecoverPut(headers,other*20,kRecoverManagers[2]);RecoverPut(headers,owner*20,kRecoverManagers.at(manager/11));
        // Create반환2와별개로두manager의현재stream은0/1부터시작한다.
        for (std::size_t index=0;index<managers.size();++index) {
            // manager+8이외주변필드도원문관측과대조한다.
            for (std::size_t byte=0;byte<managers[index].size();++byte) managers[index][byte]=static_cast<std::uint8_t>(0x50+(index+1)*11+byte);RecoverPut(managers[index],8,kRecoverStreams[index]);
        }
        // 같은값의필드를가져도실제stream/출력주소세개를독립소유한다.
        for (std::size_t index=0;index<streams.size();++index) {
            RecoverPut(streams[index],0,kRecoverBuffers[index]);RecoverPut(streams[index],4,position);RecoverPut(streams[index],8,end);RecoverPut(streams[index],12,capacity);
            // 쓰지않는출력까지전부비교하기위한주변입력이다.
            for (std::size_t byte=0;byte<buffers[index].size();++byte) buffers[index][byte]=static_cast<std::uint8_t>(0xb0+index*7+byte);
        }
        input=RecoverRecords(owner,pattern);localSnapshot.fill(0xcd);
    }
    // 주소토큰을관측용manager번호로만변환한다.
    std::size_t ManagerIndex(std::uint32_t token) const {
        //0포인터도native의manager0진단에서별개로기록한다.
        for (std::size_t index=0;index<kRecoverManagers.size();++index) if (kRecoverManagers[index]==token) return index;throw std::out_of_range("Territory manager 토큰");
    }
    // 주소토큰이실제세stream중어느것인지선택한다.
    std::size_t StreamIndex(std::uint32_t token) const {
        //명시입력밖주소를기본stream으로대체하지않는다.
        for (std::size_t index=0;index<kRecoverStreams.size();++index) if (kRecoverStreams[index]==token) return index;throw std::out_of_range("Territory stream 토큰");
    }
    // 현재관측한경계만순서대로추가한다.
    void Record(std::string event) { if (!trace.empty()) trace+=';';trace+=event; }
    // 부모의지역수명이끝나기전에원시120BYTE를소유배열에복사한다.
    void SnapshotLocal() { if (kind=="recover" && !local.empty()) std::copy(local.begin(),local.end(),localSnapshot.begin()); }
    // 현재부분효과를보존하고명시중단한다.
    [[noreturn]] void Stop(std::string cause) { SnapshotLocal();throw RecoverStop{cause}; }
    // callback입력의외부효과만실행하고본문분기/재조회/flags쓰기에는관여하지않는다.
    void Mutate(std::string_view boundary) {
        const auto name=std::string(boundary); //fixture의명시경계이름이다.
        if (mutation==name+"-owner") owner=other;
        else if (mutation==name+"-manager22") RecoverPut(headers,(boundary=="create" || boundary=="assert521" || boundary=="assert522" ? fixed : owner)*20,kRecoverManagers[2]);
        else if (mutation=="create-current2" && boundary=="create") { const auto selected=ManagerIndex(RecoverGet(headers,fixed*20));if (selected) RecoverPut(managers.at(selected-1),8,kRecoverStreams[2]); }
        else if (mutation=="log1-byte1-80" && boundary=="log1") local[1]=128;
        else if ((mutation=="log1-byte1-255" && boundary=="log1") || (mutation=="write-byte1-255" && boundary=="write")) local[1]=255;
        else if (mutation=="log1-byte7-0" && boundary=="log1") local[7]=0;
        else if (mutation=="state-returnother" && boundary=="state") returnedState=other;
        else if (mutation=="state-byte0-255" && boundary=="state") states.at(returnedState)[0]=255;
        else if (mutation=="state-zero" && boundary=="state") stateNull=true;
        else if (mutation=="assert36-data1" && boundary=="assert36") RecoverPut(streams.at(outputStream),0,kRecoverBuffers[1]);
        else if (mutation=="assert36-pos7" && boundary=="assert36") RecoverPut(streams.at(outputStream),4,7);
        else if (mutation==name+"-stop" || (mutation=="read-partial-stop" && boundary=="read")) Stop(name);
    }
    // 각진단을원문번호로관측하며복귀뒤실제현재값을제품이재조회한다.
    void Assert(std::uint32_t line) { ++calls[5];Record("A:"+std::to_string(line));Mutate("assert"+std::to_string(line)); }
    // 실제0044cce0을공유하며NULL입력의물리접근만원문명시중단에대응한다.
    void Write(std::uint32_t token,std::optional<FortTerritoryRecords> records,std::uint32_t length) {
        CHECK(length==120);outputStream=StreamIndex(token);++calls[3];Record("W:"+std::to_string(outputStream)+":120:"+(records ? RecoverHex(*records) : "null"));Mutate("write");
        MemStreamHooks hooks; //일반쓰기에서는allocate/release가선택되지않는다.
        hooks.buffer=[this](std::uint32_t selected)->std::span<std::uint8_t> {
            // 진단뒤현재data를실제출력주소에대응한다.
            for (std::size_t index=0;index<kRecoverBuffers.size();++index) if (selected==kRecoverBuffers[index]) return buffers[index];throw std::out_of_range("Territory 실제출력토큰");
        };
        hooks.assertion=[this](MemStreamAssertion kind) { CHECK(kind==MemStreamAssertion::WriteCapacity);Assert(36); };
        try { WriteMemStream(streams.at(outputStream),records ? std::span<const std::uint8_t>(*records) : std::span<const std::uint8_t>{},length,hooks); }
        catch (const std::out_of_range&) { if (!records) Stop("source0");throw; }
    }
    // 현재manager진단/Create/current/일반writer만실제하위로연결한다.
    FortTerritoryWriteHooks WriterHooks() {
        FortTerritoryWriteHooks hooks; //부모가주는고정슬롯과현재owner를혼동하지않는다.
        hooks.manager=[this](std::uint32_t slot) { const auto value=RecoverGet(headers,slot*20);Record("M:"+std::to_string(slot)+':'+std::to_string(ManagerIndex(value)*11));return value; };
        hooks.assertion=[this](std::uint32_t line) { Assert(line); };
        hooks.create=[this](std::uint32_t token,std::string_view name) { CHECK(name=="Territory");++calls[2];Record("K:"+std::to_string(ManagerIndex(token)*11)+":Territory");Mutate("create"); };
        hooks.current=[this](std::uint32_t token) { const auto index=ManagerIndex(token);if (!index) Stop("manager0");const auto selected=RecoverGet(managers.at(index-1),8);Record("C:"+std::to_string(index*11)+':'+std::to_string(StreamIndex(selected)));return selected; };
        hooks.write=[this](auto token,auto records,auto length) { Write(token,records,length); };return hooks;
    }
    // 원본004bd600의고정this를기록하고제품writer를호출한다.
    void Writer(std::uint32_t slot,std::optional<FortTerritoryRecords> records) { fixed=slot;Record("F:"+std::to_string(slot));WriteFortTerritoryRecords(slot,records,WriterHooks()); }
    // getter/State는명시입력경계이며읽기/회복/write본문은제품만실행한다.
    FortTerritoryRecoverHooks RecoverHooks() {
        FortTerritoryRecoverHooks hooks; //동기부모지역span은callback수명안에서만빌린다.
        hooks.owner=[this] { return owner; };
        hooks.read=[this](std::uint32_t slot,std::span<std::uint8_t,120> result) { local=result;++calls[0];Record("R:"+std::to_string(slot)+':'+RecoverHex(result));const auto length=mutation=="read-partial-stop" ? 37U : 120U;std::copy_n(sources.at(slot).begin(),length,result.begin());Mutate("read"); };
        hooks.log=[this](std::uint32_t type,std::string_view text) { CHECK(type==29 && text==kRecoverMessage);++calls[1];Record("L:29:"+RecoverHex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())));Mutate("log"+std::to_string(calls[1])); };
        hooks.write=[this](auto slot,auto records) { Writer(slot,records); };
        hooks.state=[this](std::uint32_t slot)->std::span<std::uint8_t> { returnedState=slot;++calls[4];Record("S:"+std::to_string(slot));Mutate("state");SnapshotLocal();return stateNull ? std::span<std::uint8_t>{} : std::span<std::uint8_t>(states.at(returnedState)); };return hooks;
    }
    // 독립진입점두개만선택하고명시0State물리거부만native중단으로표시한다.
    void Run() {
        if (kind=="write") { local=input;Writer(owner,nullInput ? std::nullopt : std::optional<FortTerritoryRecords>(input)); }
        else if (kind=="recover") { try { RecoverFortTerritory(RecoverHooks()); }catch (const std::out_of_range&) { if (stateNull) throw RecoverStop{"state0"};throw; } }
        else throw std::runtime_error("Territory fixture 미등록진입점");
    }
    // 현재owner와각하위호출수를원문일곱칸으로직렬화한다.
    std::string State() const {
        auto result=std::to_string(owner); //현재owner는다른callback에서도바뀔수있다.
        // 원문read/log/create/write/state/assert순서를유지한다.
        for (const auto count:calls) result+=','+std::to_string(count);return result;
    }
    // 지역수명이끝난span을역참조하지않고소유스냅샷과모든주변원시를비교한다.
    std::string Raw() const {
        auto result=RecoverHex(headers); //9헤더의미사용필드까지포함한다.
        // 두manager의+8현재값과주변을모두포함한다.
        for (const auto& value:managers) result+=':'+RecoverHex(value);
        // 세stream의현재data/pos/end/capacity를모두포함한다.
        for (const auto& value:streams) result+=':'+RecoverHex(value);
        // 미선택출력도주변256BYTE전체를관측한다.
        for (const auto& value:buffers) result+=':'+RecoverHex(value);result+=':';
        // 원문의연속9×28State와같은순서로붙인다.
        for (const auto& value:states) result+=RecoverHex(value);return result+':'+RecoverHex(input)+':'+(kind=="recover" ? RecoverHex(localSnapshot) : "-");
    }
};
}

// 원문974관측의정상/명시중단과전체원시/trace를각입력에서제품실행뒤대조한다.
TEST_CASE(FortTerritoryRecover_IndependentNative974) {
    std::ifstream input(NETSTORM_FORT_TERRITORY_RECOVER_FIXTURE);if (!input) throw std::runtime_error("Territory 회복최초fixture없음");std::string line;std::size_t compared{},stopped{};std::set<std::pair<std::string,std::string>> unique; //정밀도/입력중복도거부한다.
    // 입력열만먼저사용하고기대관측은실행뒤읽는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=RecoverFields(line,'\t');if (row.size()!=6) throw std::runtime_error("Territory fixture 관측칸수오류");CHECK(unique.emplace(row[0],row[1]).second);
        RecoverDevices devices(row[1]);std::string completion="1";try { devices.Run(); }catch (const RecoverStop& stop) { completion=stop.cause;++stopped; }
        const bool equal=completion==row[2] && devices.State()==row[3] && devices.Raw()==row[4] && devices.trace==row[5]; //실패입력만추가표시해독립관측을찾을수있게한다.
        if (!equal) std::fprintf(stderr,"Territory fixture input=%s completion=%s\n",row[1].c_str(),completion.c_str());
        CHECK(completion==row[2]);CHECK(devices.State()==row[3]);CHECK(devices.Raw()==row[4]);CHECK(devices.trace==row[5]);++compared;
    }
    CHECK(compared==974 && stopped==510);
}

// 미선택로그는필수가아니며선택된경계누락/0State가앞선쓰기효과를롤백하지않는다.
TEST_CASE(FortTerritoryRecover_SelectedAndPhysicalBoundaries) {
    FortTerritoryRecoverHooks hooks;std::array<std::uint8_t,3> state{255,17,33};std::uint32_t ownerQueries{},reads{},writes{}; //State첫BYTE이외는유지되어야한다.
    hooks.owner=[&] { ++ownerQueries;return 8U; };
    hooks.read=[&](std::uint32_t slot,std::span<std::uint8_t,120> bytes) { CHECK(slot==8);++reads;CHECK(std::all_of(bytes.begin(),bytes.end(),[](auto value) { return value==0; })); };
    hooks.write=[&](std::uint32_t slot,FortTerritoryRecords bytes) { CHECK(slot==8);++writes;CHECK(bytes.size()==120); };
    hooks.state=[&](std::uint32_t slot)->std::span<std::uint8_t> { CHECK(slot==8);return state; };
    RecoverFortTerritory(hooks);CHECK(ownerQueries==3 && reads==1 && writes==1 && state[0]==251 && state[1]==17 && state[2]==33);
    hooks.state={};bool rejected=false;try { RecoverFortTerritory(hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && reads==2 && writes==2); //선택된State누락전실제쓰기를보존한다.
    hooks.state=[](std::uint32_t)->std::span<std::uint8_t> { return {}; };rejected=false;try { RecoverFortTerritory(hooks); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && reads==3 && writes==3);
    RecoverDevices changed("recover,1,11,1,255,0,0,256,0,create-manager22");changed.Run();CHECK(RecoverGet(changed.streams[0],4)==0 && RecoverGet(changed.streams[1],4)==120 && changed.calls[2]==1); //Create반환stream2를쓰지않는다.
    RecoverDevices noFlags("recover,1,11,0,255,0,0,256,0,none");noFlags.Run();CHECK(noFlags.calls[1]==0 && noFlags.calls[3]==1 && noFlags.calls[4]==1);
}
