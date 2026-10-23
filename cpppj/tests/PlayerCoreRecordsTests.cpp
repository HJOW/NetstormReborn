// 신규장부의전체인접물리를제품전에고정한PE값과대조한다.
#include "TestSupport.h"
#include "o/RawPlayerCoreRecords.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
// 순서/빈칸을유지해원시입력과출력칼럼을읽는다.
std::vector<std::string> RecordsSplit(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { // 부모결과를계산하지않고최초문자열만분리한다.
        const auto end=text.find(delimiter,begin);result.emplace_back(text.substr(begin,end==text.npos?text.size()-begin:end-begin));if (end==text.npos) return result;begin=end+1;
    }
}
// 미작성인접BYTE/padding도모두대조하는hex다.
std::string RecordsHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result;
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // NUL에서자르지않는다.
    return result;
}
// 원시행/칼럼을남겨최초불일치의입력을보존한다.
void CheckRecords(std::size_t row,const char* field,const std::string& actual,const std::string& expected) {
    if (actual!=expected) std::fprintf(stderr,"CoreRecords row%zu %s actual=%s expected=%s\n",row,field,actual.c_str(),expected.c_str());CHECK(actual==expected);
}
}

// 별도owner색표6관측은귀속감사로만읽고Core관련186관측의전체물리를대조한다.
TEST_CASE(PlayerCoreRecords_ReplaysIndependentPe186AndSeparatesColorTable6) {
#ifdef NETSTORM_PLAYER_CORE_RECORDS_FIXTURE
    const std::filesystem::path path=NETSTORM_PLAYER_CORE_RECORDS_FIXTURE; // 공용등록의최초TSV절대경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/player-core-records-x86.tsv"; // 격리빌드도같은최초자료를읽는다.
#endif
    std::ifstream file(path);if (!file) throw std::runtime_error("Core장부최초fixture열기실패");std::string line;std::size_t rows{},compared{},colorAudit{};
    while (std::getline(file,line)) { // 최초192관측을빠짐없이분류한다.
        if (line.empty()||line[0]=='#') continue;if (line.back()=='\r') line.pop_back();++rows;const auto values=RecordsSplit(line,'\t');if (values.size()!=7) throw std::runtime_error("Core장부fixture칸수");const auto input=RecordsSplit(values[1],',');
        const auto mode=std::stoul(input[0]),seed=std::stoul(input[1]);const auto owner=static_cast<std::uint32_t>(std::stoull(input[2])),subscriber=static_cast<std::uint32_t>(std::stoull(input[3]));const auto coreSeed=std::stoul(input[4]);
        std::array<std::uint8_t,256> records{};std::array<std::uint8_t,16> core{};std::array<std::uint8_t,44> colors{};std::array<std::uint8_t,10> argument{};
        for (std::size_t byte=0;byte<records.size();++byte) records[byte]=static_cast<std::uint8_t>(seed+byte*17); // Native의장부앞64/뒤48인접물리도같다.
        for (std::size_t byte=0;byte<core.size();++byte) core[byte]=static_cast<std::uint8_t>(seed+byte*13); // 별도지역Core물리다.
        for (std::size_t byte=0;byte<colors.size();++byte) colors[byte]=static_cast<std::uint8_t>(seed+byte*7); // Core와무관한표를별도로유지한다.
        for (std::size_t byte=0;byte<argument.size();++byte) argument[byte]=static_cast<std::uint8_t>(coreSeed+byte*23); // 이미값으로복제한10BYTE인자다.
        if (mode==0) InitializePlayerCore(core);
        else if (mode==1) WritePlayerCoreMetadata({records,64},owner,subscriber,argument);
        else if (mode==2) InitializePlayerCoreRecords({records,64});
        else if (mode==3) InitializePlayerCore(std::span(records).subspan(208)); // 별도4feaf0은lastKnown주소만선택해같은16BYTE자식으로tail-jump한다.
        else if (mode==4) { ++colorAudit;CHECK(values[5].find("O0:4:0;O4:4:1;")==0);continue; } // 별도43b540을Core제품에서구현했다고주장하지않는다.
        else throw std::runtime_error("Core장부진입모드");
        ++compared;CheckRecords(rows,"records",RecordsHex(records),values[2]);CheckRecords(rows,"core",RecordsHex(core),values[3]);CheckRecords(rows,"unrelatedColors",RecordsHex(colors),values[4]);
    }
    CHECK(rows==192 && compared==186 && colorAudit==6);
}

// 선택된단일store폭부족에서앞선store만유지하며먼저전체10/14BYTE를검사하지않는다.
TEST_CASE(PlayerCoreRecords_ShortPhysicalViewsPreserveOrderedPrefix) {
    for (std::size_t length=0;length<=10;++length) { // Core의4/4/2경계와그사이짧은물리를별도host계약으로검증한다.
        std::array<std::uint8_t,16> bytes;bytes.fill(0xa5);bool rejected{};try { InitializePlayerCore(std::span(bytes).first(length)); }catch(const std::out_of_range&) { rejected=true; }
        CHECK(rejected==(length<10));const auto zeroEnd=length>=10?10U:length>=8?8U:length>=4?4U:0U;
        for (std::size_t byte=0;byte<bytes.size();++byte) CHECK(bytes[byte]==(byte<zeroEnd?0:0xa5)); // 성공한store뒤의물리는그대로다.
    }
    std::array<std::uint8_t,32> records;records.fill(0xa5);bool rejected{};
    try { InitializePlayerCoreRecords({std::span(records).first(13),0}); }catch(const std::out_of_range&) { rejected=true; }
    CHECK(rejected && records[0]==0xa5 && records[3]==0xa5 && records[4]==0 && records[11]==0 && records[12]==0xa5); // 첫Core완료전에subId0이앞당겨지지않는다.
    records.fill(0xa5);rejected=false;try { InitializePlayerCoreRecords({std::span(records).first(14),0}); }catch(const std::out_of_range&) { rejected=true; }
    CHECK(rejected && std::all_of(records.begin(),records.begin()+14,[](auto byte) { return byte==0; }) && records[14]==0xa5 && records[16]==0xa5); // 첫record완료효과는다음자식중단에도남는다.
}

// metadata는입력값복제와padding2를보존하며정상장부밖주소를선택접근에서만거부한다.
TEST_CASE(PlayerCoreRecords_MetadataPreservesPaddingAndCopiesInputValues) {
    std::array<std::uint8_t,144> records;records.fill(0xa5);std::array<std::uint8_t,10> core{1,2,3,4,5,6,7,8,9,10};WritePlayerCoreMetadata({records,0},8,0x12345678,core);core.fill(0);
    CHECK(records[128]==0x78 && records[131]==0x12 && records[132]==1 && records[141]==10 && records[142]==0xa5 && records[143]==0xa5);
    const auto before=records;bool rejected{};try { WritePlayerCoreMetadata({records,0},9,0,core); }catch(const std::out_of_range&) { rejected=true; }CHECK(rejected && records==before); // 원문owner관문이아닌호스트물리144BYTE의계약이다.
}
