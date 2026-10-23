// 제품 작성 전250관측의 인접 전체 표와 복사 뒤 count 변경·부분중단을 대조한다.
#include "TestSupport.h"
#include "client/RawDotCommandRegister.h"
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace netstorm::client;
constexpr std::uint32_t kDotCountOffset=0xa04; //원문count의같은인접표오프셋이다.
// 빈 마지막 trace 칸을 포함한 원래 TSV 필드를 분리한다.
std::vector<std::string> DotFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; // 아직 소비하지 않은 첫 위치다.
    for (;;) { // 마지막 빈칸도 한 번 추가한 뒤 반환한다.
        const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos?text.npos:end-start));if (end==text.npos) return result;start=end+1;
    }
}
// 검사 입력 DWORD만 실제 little endian 표에 쓴다.
void DotPut(std::span<std::uint8_t> state,std::uint32_t offset,std::uint32_t value) {
    for (std::uint32_t index=0;index<4;++index) state[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); //입력초기화이며부모의쓰기결과를만들지않는다.
}
// 전체 표의 미작성 BYTE도 원문과 같은 hex로 표시한다.
std::string DotHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; //한BYTE당두자리를보존한다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } //미사용이름/함수/표시포인터도생략하지않는다.
    return result;
}
struct DotStop {}; //명시CRT복사뒤의부분중단이다.
}

// 원문의 count63 끝쓰기와 callback이count64를쓴뒤 인접count를0으로덮는 경우까지 대조한다.
TEST_CASE(DotCommandRegister_All250IndependentPeObservations) {
#ifdef NETSTORM_DOT_COMMAND_REGISTER_FIXTURE
    const std::filesystem::path path=NETSTORM_DOT_COMMAND_REGISTER_FIXTURE; //공용등록이공급할최초관측의절대경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/dot-command-register-x86.tsv"; //격리검사도같은최초파일만읽는다.
#endif
    std::ifstream input(path);CHECK(input.good());if (!input) return;std::string line;std::size_t compared{}; //실제로읽어대조한행수다.
    while (std::getline(input,line)) { //두정밀도원문을각각같은명시입력으로대조한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=DotFields(line,'\t');if (row.size()!=6) throw std::logic_error("점 명령 원문 TSV 칸 수");
        const auto fields=DotFields(row[1],',');if (fields.size()!=5) throw std::logic_error("점 명령 원문 입력 칸 수");
        std::array<std::uint8_t,kDotCommandStateBytes> state{}; //아래명시비영입력으로매BYTE를채운다.
        for (std::size_t index=0;index<state.size();++index) state[index]=static_cast<std::uint8_t>(165+index*17); //최초Native와같은입력이며기대결과가아니다.
        DotPut(state,kDotCountOffset,static_cast<std::uint32_t>(std::stoul(fields[0])));
        std::vector<std::uint8_t> name(static_cast<std::size_t>(std::stoul(fields[1])),std::uint8_t{'Q'});name.push_back(0); //정상NUL포함원문이름입력이다.
        std::string trace,completion="return";DotCommandRegisterHooks hooks; //복사장치의현재count변경과중단만공급한다.
        hooks.copy=[&](auto destination,auto source,auto count) {
            trace="COPY:"+std::to_string(destination.data()-state.data())+":0:"+std::to_string(count);CopyDotCommandName(destination,source,count);
            if (fields[4].starts_with("copy-count")) DotPut(state,kDotCountOffset,static_cast<std::uint32_t>(std::stoul(fields[4].substr(10))));
            else if (fields[4]=="copy-stop") throw DotStop{};
        };
        std::uint32_t result{}; //정상ret에서만EAX와대조한다.중단시hostABI레지스터는원문과다르다.
        try { result=RegisterDotCommand(state,name,static_cast<std::uint32_t>(std::stoul(fields[3])),static_cast<std::uint32_t>(std::stoul(fields[2])),hooks); }catch (const DotStop&) { completion="copy-stop"; }
        if (completion!=row[2] || DotHex(state)!=row[4] || trace!=row[5] || (completion=="return" && result!=std::stoul(row[3]))) {
            std::fprintf(stderr,"Dot command input=%s completion=%s expected=%s trace=%s expectedTrace=%s\n",row[1].c_str(),completion.c_str(),row[2].c_str(),trace.c_str(),row[5].c_str());
            CHECK(completion==row[2]);CHECK(DotHex(state)==row[4]);CHECK(trace==row[5]);if (completion=="return") CHECK(result==std::stoul(row[3]));return;
        }
        ++compared;
    }
    CHECK(compared==250);
}

// 상한의 자연return은 미선택복사장치가 없어도 같은 물리를 유지한다.
TEST_CASE(DotCommandRegister_UpperLimitAndSelectedCopyContract) {
    std::array<std::uint8_t,kDotCommandStateBytes> state{};DotPut(state,kDotCountOffset,64);const auto before=state;DotCommandRegisterHooks hooks;hooks.copy={}; //실제선택누락과자연생략을구별한다.
    CHECK(RegisterDotCommand(state,{},0xffffffffU,1,hooks)==0 && state==before);
    DotPut(state,kDotCountOffset,0);bool missing{};try { RegisterDotCommand(state,{},1,1,hooks); }catch (const std::logic_error&) { missing=true; }CHECK(missing);
    std::array<std::uint8_t,31> destination;destination.fill(0xa5);const std::array<std::uint8_t,2> source{'A',0};CopyDotCommandName(destination,source,31);CHECK(destination[0]=='A');
    for (std::size_t index=1;index<destination.size();++index) CHECK(destination[index]==0); //NUL뒤남은count는CRT처럼0으로채운다.
}

// 교정 전에 고정한 v2의 count64 정상과 count65 종단 실패 전 전체 표를 비교한다.
TEST_CASE(DotCommandRegister_V2CapturedCountPartialEffects) {
#ifdef NETSTORM_DOT_COMMAND_REGISTER_V2_FIXTURE
    const std::filesystem::path path=NETSTORM_DOT_COMMAND_REGISTER_V2_FIXTURE; //첫250과 다른 보충 관측의 고정경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/dot-command-register-v2-x86.tsv"; //독립 격리검사의 같은 자료다.
#endif
    std::ifstream input(path);CHECK(input.good());if (!input) return;
    std::string line;std::size_t compared{},negativeBoundaries{}; //지원된부분효과와음수외부경계를별도로센다.
    while (std::getline(input,line)) { //최초v2두정밀도의전체행을분리해대조한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();
        const auto row=DotFields(line,'\t');if (row.size()!=7) throw std::logic_error("점 명령 v2 TSV 칸 수");
        const auto fields=DotFields(row[1],',');if (fields.size()!=5) throw std::logic_error("점 명령 v2 입력 칸 수");
        if (row[2]=="name-copy-stop") { //원문이표앞CRT인자에도달한기록이며host성공/실제CRT실행으로세지않는다.
            CHECK(fields[0]=="4294967295" && row[5]=="COPY:-32:0:31" && row[6]=="COPY:531b10:31");++negativeBoundaries;continue;
        }
        std::array<std::uint8_t,kDotCommandStateBytes> state{}; //아래원시초기입력을모든BYTE에작성한다.
        for (std::size_t index=0;index<state.size();++index) state[index]=static_cast<std::uint8_t>(165+index*17); //부모기대가아닌입력패턴이다.
        DotPut(state,kDotCountOffset,static_cast<std::uint32_t>(std::stoul(fields[0])));
        std::vector<std::uint8_t> name(static_cast<std::size_t>(std::stoul(fields[1])),std::uint8_t{'Q'});name.push_back(0); //명시NUL문자열입력이다.
        std::string trace,completion="return";DotCommandRegisterHooks hooks; //복사뒤현재count변경만공급한다.
        hooks.copy=[&](auto destination,auto source,auto count) {
            trace="COPY:"+std::to_string(destination.data()-state.data())+":0:"+std::to_string(count);CopyDotCommandName(destination,source,count);
            DotPut(state,kDotCountOffset,static_cast<std::uint32_t>(std::stoul(fields[4].substr(10))));
        };
        std::uint32_t result{}; //물리밖호스트중단에서EAX를성공반환값으로대조하지않는다.
        try { result=RegisterDotCommand(state,name,static_cast<std::uint32_t>(std::stoul(fields[3])),static_cast<std::uint32_t>(std::stoul(fields[2])),hooks); }
        catch (const std::out_of_range&) { completion="physical-store-stop"; }
        CHECK(completion==row[2] && DotHex(state)==row[4] && trace==row[5]);
        if (completion=="return") CHECK(result==std::stoul(row[3]) && row[6]=="-");
        else CHECK(row[6]=="STORE:532538:4:0" && state[kDotCountOffset]==66 && state[kDotCountOffset+1]==0 && state[kDotCountOffset+2]==0 && state[kDotCountOffset+3]==0);
        ++compared;
    }
    CHECK(compared==8 && negativeBoundaries==4);
}

// 음수DWORD는signed관문을통과하지만미지원원시주소산술이므로host는0반환과구별해거부한다.
TEST_CASE(DotCommandRegister_NegativeCountIsExplicitPhysicalContract) {
    for (const auto count:{0xffffffffU,0x80000000U,0x80000001U}) { //원문접근위반/32비트주소감김을지원호스트성공으로가장하지않는다.
        std::array<std::uint8_t,kDotCommandStateBytes> state{};DotPut(state,kDotCountOffset,count);const auto before=state;std::size_t copies{};DotCommandRegisterHooks hooks;
        hooks.copy=[&](auto,auto,auto) { ++copies; }; //host명시거부가외부CRT진입보다앞임을관측한다.
        bool rejected{};try { RegisterDotCommand(state,{},1,1,hooks); }catch (const std::out_of_range&) { rejected=true; }
        CHECK(rejected && copies==0 && state==before);
    }
    std::array<std::uint8_t,kDotCommandStateBytes> state{};DotPut(state,kDotCountOffset,0x7fffffffU);const auto before=state;DotCommandRegisterHooks hooks;hooks.copy={}; //signed양수상한초과는선택장치없이자연0이다.
    CHECK(RegisterDotCommand(state,{},1,1,hooks)==0 && state==before);
}
