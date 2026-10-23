// 독립PE694행의현재ID/두stream/출력512BYTE/부분복사와Config호출순서를대조한다.
#include "TestSupport.h"
#include "o/RawFortSubscriberLoad.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
// fixture원시구분자를분리하며마지막빈필드도유지한다.
std::vector<std::string> SubscriberFields(std::string_view source,char delimiter) {
    std::vector<std::string> result;std::size_t start{}; //원본입력/독립출력열을가공없이나눈다.
    for (;;) { const auto end=source.find(delimiter,start);result.emplace_back(source.substr(start,end==source.npos ? source.npos : end-start));if (end==source.npos) return result;start=end+1; } //빈끝도하나의열이다.
}
// 명시원시입력과관측출력의DWORD를읽는다.
std::uint32_t SubscriberU(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } //다른필드를섞지않는다.
// 하위장치/입력만현재한DWORD에쓴다.
void SubscriberPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //전체32비트를보존한다.
// 출력전체의원시BYTE를hex로표시해내부NUL과stale꼬리도비교한다.
std::string SubscriberHex(std::span<const std::uint8_t> bytes) { constexpr char digits[]="0123456789abcdef";std::string result;for (auto byte:bytes) { result.push_back(digits[byte>>4]);result.push_back(digits[byte&15]); }return result; } //문자열첫NUL에서잘라비교하지않는다.
// 명시외부중단은물리접근실패와구분한다.
struct SubscriberStop {};
// 원문밖장치의명시입력/현재상태만소유하고제품의cursor/분기를대신계산하지않는다.
struct SubscriberDevices {
    std::array<std::uint32_t,12> input{}; //fixture원시DWORD열이다.뒤출력열로장치를만들지않는다.
    std::uint32_t subscriber{},manager{},selected{},assertions{}; //원문이그때관측하는현재ID/고정this/선택반환과진단순번이다.
    std::array<std::array<std::uint8_t,16>,2> streams{}; //같은선택반환의실제fields다.
    std::array<std::array<std::uint8_t,512>,2> buffers{}; //논리end와별개인두실제물리할당이다.
    std::array<std::uint8_t,512> output{}; //전달된고정출력주소의staleBYTE까지유지한다.
    std::string mutation,trace; //명시외부변형과실제선택경계trace다.
    // PE와같은원시입력만준비한다.결과ID/cursor/이름은계산하지않는다.
    explicit SubscriberDevices(const std::vector<std::string>& fields) {
        for (std::size_t index=0;index<input.size();++index) input[index]=std::stoul(fields[index]);mutation=fields[12];manager=input[2];selected=input[3];subscriber=input[0] ? input[7] : 0x12345678U;output.fill(0xa5); //실제출력초깃값이다.
        std::vector<std::uint8_t> raw(6);SubscriberPut(raw,0,input[7]);raw[4]=static_cast<std::uint8_t>(input[8]);raw[5]=static_cast<std::uint8_t>(input[8]>>8); //파일ID와signedWORD의원시저장폭이다.
        for (std::uint32_t byte=0;byte<std::min(input[8],300U);++byte) raw.push_back(static_cast<std::uint8_t>(byte*37U+65U)); //내부NUL도있는명시입력이다.
        for (std::size_t index=0;index<streams.size();++index) { //두반환노드의실제data/pos/end를독립적으로설정한다.
            SubscriberPut(streams[index],0,input[4] ? static_cast<std::uint32_t>(index+1) : 0U);SubscriberPut(streams[index],4,input[6]);SubscriberPut(streams[index],8,input[5]);SubscriberPut(streams[index],12,512);
            for (std::size_t byte=0;byte<buffers[index].size();++byte) buffers[index][byte]=static_cast<std::uint8_t>(byte*17+index*91); //나머지물리메모리도PE와같은독립입력이다.
            for (const auto position:{std::size_t{0},std::size_t{64}}) std::copy(raw.begin(),raw.end(),buffers[index].begin()+static_cast<std::ptrdiff_t>(position)); //callback이옮긴pos도명시원문을읽는다.
        }
    }
    // 실제경계진입순서를기록한다.
    void Record(std::string event) { if (!trace.empty()) trace+=';';trace+=event; }
    // 입력이명시한외부효과만바꾸며그뒤처리는원문대응제품이한다.
    void Mutate(std::string_view boundary) {
        const auto separator=mutation.find('-');if (separator==std::string::npos) return;const auto trigger=mutation.substr(0,separator),action=mutation.substr(separator+1);std::string base(boundary);while (!base.empty() && base.back()>='0' && base.back()<='9') base.pop_back(); //하위장치별1기반진단과단일경계를구별한다.
        if (trigger!=boundary && trigger!=base) return;auto& fields=streams[selected]; //외부효과가진입했을때의선택반환노드다.
        if (action=="stop") throw SubscriberStop{};
        if (action=="manager22") manager=22;else if (action=="manager0") manager=0;else if (action=="subscriber9") subscriber=9;else if (action=="stream1") selected=1;
        else if (action.starts_with("pos")) SubscriberPut(fields,4,std::stoul(action.substr(3)));else if (action=="data0") SubscriberPut(fields,0,0);else if (action=="dataB") SubscriberPut(fields,0,2);else if (action=="endffffffff") SubscriberPut(fields,8,0xffffffffU);else throw std::logic_error("Subscriber검사변형미해석");
    }
    // 실제물리저장소/Config/새ID장치만연결한다.
    FortSubscriberLoadHooks Hooks() {
        FortSubscriberLoadHooks hooks; //문자열길이/복사/현재cursor/등록판정은본문이소유한다.
        hooks.manager=[this](auto slot) { CHECK(std::bit_cast<std::uint32_t>(slot)==input[1]);return manager; };
        hooks.select=[this](auto current,auto name,auto writable) { Record("S:"+std::to_string(current)+':'+std::string(name)+':'+std::to_string(writable));Mutate("select1");return selected+1; };
        hooks.access=[this](auto token,const auto& operation) { if (!token || token>streams.size()) throw std::out_of_range("Subscriber검사stream물리범위");MemStreamHooks memory;memory.buffer=[this](auto pointer)->std::span<std::uint8_t> { if (!pointer || pointer>buffers.size()) throw std::out_of_range("Subscriber검사data물리범위");return buffers[pointer-1]; };operation(streams[token-1],memory); };
        hooks.assertion=[this](auto line) { Record("A:"+std::to_string(static_cast<std::uint32_t>(line)));Mutate("assert"+std::to_string(++assertions)); };
        hooks.readRegistered=[this](auto key,auto& target) { CHECK(&target==&subscriber);Record("R:"+std::string(key));target=input[10];Mutate("read1");return input[9]!=0; };
        hooks.generate=[this] { Record("G");Mutate("generate1");return input[11]; };
        hooks.configure=[this](auto expression) { Record("C:"+std::string(expression));Mutate("config1"); };return hooks;
    }
};
// 호스트선택장치/물리범위예외만확인한다.
template<class Function> bool SubscriberRejects(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; }
}

// 최초PE694행의정상/중단/부분실패와모든현재물리상태를대조한다.
TEST_CASE(FortSubscriberLoad_IndependentNativeBodyAndRegistration) {
    std::ifstream fixture(NETSTORM_FORT_SUBSCRIBER_LOAD_FIXTURE);CHECK(fixture.good());std::string line;std::size_t compared{}; //원문행수만집계한다.
    while (std::getline(fixture,line)) { //앞두열의입력만장치에주고나머지는제품실행뒤비교한다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=SubscriberFields(line,'\t');CHECK(row.size()==7);if (row.size()!=7) continue;SubscriberDevices d(SubscriberFields(row[1],','));auto hooks=d.Hooks();std::string status="normal";
        try { if (d.input[0]) FinishFortSubscriberRegistration({d.subscriber},hooks);else LoadFortSubscriber(std::bit_cast<std::int32_t>(d.input[1]),d.output,{d.subscriber},hooks); }catch (const SubscriberStop&) { status="stop"; }catch (const std::out_of_range&) { status="fault"; }
        CHECK(status==row[2]);const auto state=SubscriberFields(row[3],',');CHECK(state.size()==2);CHECK(d.subscriber==std::stoul(state[0]) && d.manager==std::stoul(state[1]));
        const auto raw=SubscriberFields(row[4],',');CHECK(raw.size()==8);for (std::size_t node=0;node<2;++node) for (std::size_t field=0;field<4;++field) CHECK(SubscriberU(d.streams[node],field*4)==std::stoul(raw[node*4+field])); //양쪽현재stream을비교한다.
        CHECK(SubscriberHex(d.output)==row[5]);CHECK((d.trace.empty() ? "-" : d.trace)==row[6]);++compared;
    }
    CHECK(compared==694);
}

// 현재ID관문의미선택장치/부족한실제handle부분쓰기/선택장치누락효과를구별한다.
TEST_CASE(FortSubscriberLoad_SelectedDevicesAndPhysicalPrefix) {
    std::uint32_t known=1;FinishFortSubscriberRegistration({known},{});CHECK(known==1); //이미등록된ID는Config/생성장치가없어도정상이다.
    SubscriberDevices d({"0","1","11","0","1","512","0","1","127","0","77","88","none"});auto hooks=d.Hooks();std::array<std::uint8_t,64> shortHandle;shortHandle.fill(0xa5);
    CHECK(SubscriberRejects([&] { LoadFortSubscriber(1,shortHandle,{d.subscriber},hooks); }));CHECK(d.subscriber==1 && SubscriberU(d.streams[0],4)==6 && d.trace=="S:11:Subscriber:0");CHECK(std::equal(shortHandle.begin(),shortHandle.end(),d.buffers[0].begin()+6)); //뒤실패가앞선64BYTE를되돌리지않는다.
    SubscriberDevices missing({"0","1","11","0","1","512","0","4294967295","3","0","77","88","none"});auto selected=missing.Hooks();selected.generate={};CHECK(SubscriberRejects([&] { LoadFortSubscriber(1,missing.output,{missing.subscriber},selected); }));CHECK(missing.subscriber==77 && SubscriberU(missing.streams[0],4)==9 && missing.output[3]==0 && missing.trace=="S:11:Subscriber:0;R:registeredSubId"); //파일이름복사가끝난뒤필수새ID장치위치에서만거부한다.
}
