// 제품전에고정한SP writer198관측의전체물리·공유RNG·진단/중단순서를독립대조한다.
#include "TestSupport.h"
#include "o/RawFortSpWrite.h"
#include "o/SpStore.h"
#include <array>
#include <bit>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
constexpr std::array<std::uint32_t,3> kSpBuffers{0x12001000,0x12002000,0x12003000}; //원문과동일물리를나타내는opaque식별자다.
// 원시입력/관측칸의빈마지막문자열까지분리한다.
std::vector<std::string> SpFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; //아직읽지않은칸의위치다.
    for (;;) { //마지막빈칸도반환하고종료한다.
        const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos?text.npos:end-start));if (end==text.npos) return result;start=end+1;
    }
}
// 명시초기입력/하위callback의같은DWORD만littleendian으로쓴다.
void SpPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); //부모출력기대를생성하지않는다.
}
// 실제현재출력폭을검사관측용DWORD로읽는다.
std::uint32_t SpRead(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; //관측폭밖의BYTE는보지않는다.
}
// 미작성인접BYTE도포함하는전체물리출력이다.
std::string SpHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; //한BYTE두자리의고정표현이다.
    for (const auto value:bytes) { result+=digits[value>>4];result+=digits[value&15U]; }return result; //NUL이나미사용구간을버리지않는다.
}
struct SpStop {}; //명시하위에서부분중단하는Native입력의같은표식이다.
// 실제stream16BYTE와버퍼/RNG를소유하며원문출력기대가아닌입력/외부효과만구성한다.
struct SpDevice {
    std::array<std::array<std::uint8_t,256>,3> buffers{}; //셋째배열앞16BYTE가바로같은실제stream이다.
    GameRandom random; //Native실제4558c0에대응하는기존제품공유난수다.
    std::uint32_t bits{},randoms{},assertions{},equal{}; //원시float입력과실제호출관측이다.
    std::string mutation,trace; //명시하위변경/중단입력과실제경계순서다.
    // 기록의원시입력만해석하고Native출력을역산하지않는다.
    explicit SpDevice(const std::vector<std::string>& input):random(0) {
        if (input.size()!=8) throw std::logic_error("SP writer 입력 칸 수");bits=static_cast<std::uint32_t>(std::stoul(input[0]));random.SetState(static_cast<std::uint32_t>(std::stoul(input[1])));mutation=input[7];
        const auto pattern=std::stoul(input[5]); //원시물리전체를먼저명시초기화한다.
        for (std::size_t index=0;index<buffers.size();++index) for (std::size_t byte=0;byte<256;++byte) buffers[index][byte]=static_cast<std::uint8_t>(pattern+index*37+byte*11); //세배열의인접BYTE도정의한다.
        SpPut(Stream(),0,kSpBuffers.at(std::stoul(input[6])));
        for (std::size_t field=1;field<4;++field) SpPut(Stream(),field*4,static_cast<std::uint32_t>(std::stoul(input[field+1]))); //pos/size/alloc의같은실제필드다.
    }
    // 별도헤더사본없이셋째출력물리의같은첫16BYTE를빌린다.
    std::span<std::uint8_t> Stream() { return std::span(buffers[2]).first(16); }
    // 외부하위의현재필드변경만제공하며부모분기나원문기대는계산하지않는다.
    void Mutate(const std::string& boundary) {
        const auto split=mutation.find('-');if (split==mutation.npos || mutation.substr(0,split)!=boundary) return;
        const auto action=mutation.substr(split+1); //이번선택하위의명시효과다.
        if (action=="stop") throw SpStop{};
        if (action=="seed0") random.SetState(0);
        else if (action=="baseA" || action=="baseB") { SpPut(Stream(),0,kSpBuffers[action=="baseB"]);SpPut(Stream(),4,5);SpPut(Stream(),8,1);SpPut(Stream(),12,128); }
        else if (action=="stream") { SpPut(Stream(),0,kSpBuffers[2]);SpPut(Stream(),4,8); }
        else if (action=="position240") { SpPut(Stream(),4,240);SpPut(Stream(),12,256); }
        else if (action=="capacity0") SpPut(Stream(),12,0);
        else if (action=="endHigh") SpPut(Stream(),8,0x80000000U);
        else if (action=="position17") { SpPut(Stream(),4,17);SpPut(Stream(),12,256); }
        else throw std::logic_error("SP writer 미해석 입력 변경");
    }
    // 같은GameRandom과현재물리/선택진단을실제새부모에연결한다.
    FortSpWriteHooks Hooks() {
        FortSpWriteHooks hooks; //할당/free/일반write를제공하지않아도실제부모는요구하지않는다.
        hooks.nextRandom=[this](auto limit) {
            CHECK(limit==32);const auto value=random.Next(limit);++randoms;equal+=value==(randoms-1)/16;
            if (!trace.empty()) trace+=';';trace+="R:"+std::to_string(randoms)+':'+std::to_string(value)+':'+std::to_string(random.State());Mutate("r"+std::to_string(randoms));return value;
        };
        hooks.stream.buffer=[this](auto token)->std::span<std::uint8_t> {
            for (std::size_t index=0;index<kSpBuffers.size();++index) if (token==kSpBuffers[index]) return buffers[index]; //같은배열물리만빌리며사본동기화하지않는다.
            throw std::out_of_range("SP writer 검사 미등록 버퍼");
        };
        hooks.stream.assertion=[this](auto kind) { CHECK(kind==MemStreamAssertion::WriteCapacity);++assertions;if (!trace.empty()) trace+=';';trace+="A:36";Mutate("a"+std::to_string(assertions)); };return hooks;
    }
};
}

// Native178정상/20부분중단과실제_ftol/RNG/같은stream자체alias를모두대조한다.
TEST_CASE(FortSpWrite_ReplaysIndependentNative198) {
#ifdef NETSTORM_FORT_SP_WRITE_FIXTURE
    const std::filesystem::path path=NETSTORM_FORT_SP_WRITE_FIXTURE; //공용등록은최초원문자료의절대경로를공급한다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/fort-sp-write-x86.tsv"; //격리검사도같은자료만읽는다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("SP writer 최초 fixture 없음");std::string line;std::size_t rows{},normal{},stops{}; //부분중단을정상반환에합치지않는다.
    while (std::getline(input,line)) { //두정밀도의전체원시행을같은독립입력으로대조한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=SpFields(line,'\t');if (row.size()!=13) throw std::logic_error("SP writer TSV 칸 수");
        const auto fields=SpFields(row[1],',');SpDevice device(fields);std::string status="normal";
        try { WriteFortSp(device.Stream(),std::bit_cast<float>(device.bits),device.Hooks()); }catch (const SpStop&) { status="stop"; }
        if (status!=row[2] || SpHex(device.buffers[0])!=row[8] || SpHex(device.buffers[1])!=row[9] || SpHex(device.buffers[2])!=row[10]) std::fprintf(stderr,"FortSp row=%zu input=%s status=%s expected=%s\n",rows+1,row[1].c_str(),status.c_str(),row[2].c_str());
        CHECK(status==row[2]);CHECK(device.random.State()==std::stoul(row[4]));CHECK(device.randoms==std::stoul(row[5]));CHECK(device.assertions==std::stoul(row[6]));CHECK(device.equal==std::stoul(row[7]));
        CHECK(SpHex(device.buffers[0])==row[8]);CHECK(SpHex(device.buffers[1])==row[9]);CHECK(SpHex(device.buffers[2])==row[10]);
        const auto conversion="F:"+row[3]+';';CHECK(row[11].starts_with(conversion));CHECK(device.trace==row[11].substr(conversion.size())); //내부변환은출력으로검증하며가짜C++변환hook를만들지않는다.
        if (status=="normal" && fields[6]=="0" && fields[7]=="none") {
            std::uint32_t decoded{};std::size_t offset=std::stoul(fields[2]); //보호bit를실제88BYTE출력에서만모아_ftol원문EAX와대조한다.
            for (std::uint32_t bit=0;bit<32;++bit) { const auto width=bit<8?1U:bit<16?2U:4U;decoded|=SpRead(device.buffers[0],offset,width)&(1U<<bit);offset+=width; } //잡음기대는계산하지않는다.
            CHECK(decoded==std::stoul(row[3]));
        }
        ++rows;if (status=="normal") { ++normal;CHECK(device.randoms==512); }else ++stops;
    }
    CHECK(rows==198 && normal==178 && stops==20);
}

// 해당bit만반환하는명시난수입력으로잡음생략/8BYTE·8WORD·16DWORD/총88BYTE를독립확인한다.
TEST_CASE(FortSpWrite_EqualNoiseKeepsOwnBitAndExactWidths) {
    std::array<std::uint8_t,16> stream{};std::array<std::uint8_t,96> payload{};payload.fill(0xa5);SpPut(stream,0,1);SpPut(stream,12,88);std::uint32_t calls{};FortSpWriteHooks hooks;
    hooks.nextRandom=[&](auto limit) { CHECK(limit==32);return calls++/16; }; //512회모두현재bit와같아OR만생략해야한다.
    hooks.stream.buffer=[&](auto token)->std::span<std::uint8_t> { CHECK(token==1);return payload; };
    WriteFortSp(stream,65535.0f,hooks);CHECK(calls==512 && SpRead(stream,4)==88 && SpRead(stream,8)==88);
    std::size_t offset{}; //각인라인쓰기의실제연속위치다.
    for (std::uint32_t bit=0;bit<32;++bit) { const auto width=bit<8?1U:bit<16?2U:4U;CHECK(SpRead(payload,offset,width)==(bit<16?1U<<bit:0U));offset+=width; } //원문보호bit외잡음이추가되면실패한다.
    for (std::size_t byte=88;byte<payload.size();++byte) CHECK(payload[byte]==0xa5); //88BYTE밖은쓰지않는다.
}

// 선택한buffer/진단이없으면첫16난수효과는남고할당/일반writer로우회하지않는다.
TEST_CASE(FortSpWrite_MissingSelectedDevicesPreserveRandomPrefix) {
    for (const bool missingAssertion:{false,true}) { //자연정상용량과실제선택진단의누락을구별한다.
        SpDevice device(SpFields(missingAssertion?"1199570688,55,3,9,0,165,0,none":"1199570688,55,3,9,128,165,0,none",','));auto hooks=device.Hooks();const auto before=device.buffers;
        if (missingAssertion) hooks.stream.assertion={};else hooks.stream.buffer={};bool rejected{};
        try { WriteFortSp(device.Stream(),65535.0f,hooks); }catch (const std::logic_error&) { rejected=true; }
        CHECK(rejected && device.randoms==16 && device.buffers==before);
    }
    SpDevice device(SpFields("1199570688,55,3,9,128,165,0,none",','));const auto before=device.buffers;auto hooks=device.Hooks();hooks.nextRandom={};bool rejected{};
    try { WriteFortSp(device.Stream(),65535.0f,hooks); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && device.randoms==0 && device.buffers==before);
}

// 버퍼물리가짧으면이미쓴앞두BYTE와48회난수를유지하고그뒤위치/size를자동복구하지않는다.
TEST_CASE(FortSpWrite_PhysicalFailurePreservesPriorWrites) {
    std::array<std::uint8_t,16> stream{};std::array<std::uint8_t,2> payload{0xa5,0xa5};SpPut(stream,0,1);SpPut(stream,12,88);std::uint32_t calls{};FortSpWriteHooks hooks;
    hooks.nextRandom=[&](auto limit) { CHECK(limit==32);return calls++/16; }; //외부난수효과는실제선택횟수만관측한다.
    hooks.stream.buffer=[&](auto)->std::span<std::uint8_t> { return payload; };bool rejected{};
    try { WriteFortSp(stream,1.0f,hooks); }catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && calls==48 && payload[0]==1 && payload[1]==0 && SpRead(stream,4)==2 && SpRead(stream,8)==2);
}
