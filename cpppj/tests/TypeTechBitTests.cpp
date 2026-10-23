// 원문 techBit 선관측과 현재 보호 TYPE116개를 실제 parser/동일 Record 필드로 대조한다.
#include "TestSupport.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include "o/RiftType.h"
#include <array>
#include <bit>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <limits>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
// 첫 원시 TSV의 칼럼을 결과 재계산 없이 분리한다.
std::vector<std::string> TechBitFields(std::string_view text,char delimiter) {
    std::vector<std::string> fields;std::size_t first{}; // 현재 칼럼 시작과 실제 칼럼 저장소다.
    for (;;) { // 마지막 빈 칼럼도 그대로 보존한다.
        const auto next=text.find(delimiter,first);fields.emplace_back(text.substr(first,next==text.npos?text.npos:next-first));
        if (next==text.npos) return fields;
        first=next+1;
    }
}
// Native little endian DWORD의 실제 비트만 읽는다.
std::int32_t TechBitSigned(std::string_view hex) {
    if (hex.size()!=8) throw std::logic_error("techBit원시DWORD폭");
    std::uint32_t value{}; // 원래 signed 해석 전 DWORD 비트다.
    for (std::size_t byte=0;byte<4;++byte) value|=static_cast<std::uint32_t>(std::stoul(std::string(hex.substr(byte*2,2)),nullptr,16))<<(byte*8); // 낮은 바이트 순서다.
    return std::bit_cast<std::int32_t>(value);
}
// 첫 관측의 숫자 AST 입력을 기존 TYPE lexer가 읽을 수 있는 십진 문장으로 표현한다.
std::string TechBitInput(std::string_view pairs) {
    const std::string input(pairs); // 정규식 iterator보다 오래 사는 원시 입력이다.
    const std::regex pair(R"pair(\["([^"]+)",([0-9]+)\])pair"); // 첫 Native pair의 키와 float32 비트만 읽는다.
    std::string text="typename sunArcher {"; // parser의 기존 typename/속성 경계를 실제 실행한다.
    for (auto found=std::sregex_iterator(input.begin(),input.end(),pair);found!=std::sregex_iterator();++found) { // 중복 속성도 원문 순서대로 넘긴다.
        const auto bits=static_cast<std::uint32_t>(std::stoull((*found)[2].str())); // 명시 float32 입력 비트다.
        const auto number=std::bit_cast<float>(bits);std::array<char,64> decimal{}; // 동일 float로 왕복 가능한 십진 표기다.
        const auto written=std::to_chars(decimal.data(),decimal.data()+decimal.size(),number,std::chars_format::general,std::numeric_limits<float>::max_digits10);
        if (written.ec!=std::errc{}) throw std::logic_error("techBit숫자입력표기실패");
        text+=(*found)[1].str()+"="+std::string(decimal.data(),written.ptr)+";";
    }
    return text+"} A00:default:\"a.gif\"#0;";
}
// 정밀 관측 파일의 실제 경로이며 제품 등록 때 절대 macro를 공급한다.
std::filesystem::path TechBitFixture(bool assets) {
#if defined(NETSTORM_TYPE_TECH_BIT_FIXTURE) && defined(NETSTORM_TYPE_TECH_BIT_ASSETS_FIXTURE)
    return assets?std::filesystem::path(NETSTORM_TYPE_TECH_BIT_ASSETS_FIXTURE):std::filesystem::path(NETSTORM_TYPE_TECH_BIT_FIXTURE);
#else
    return std::filesystem::path(__FILE__).parent_path()/"fixtures"/(assets?"type-tech-bit-assets-x86.tsv":"type-tech-bit-x86.tsv");
#endif
}
}

// 실제188초기값과27종 숫자/순서 입력의 native DWORD를 기존 lexer/table의 같은 필드와 대조한다.
TEST_CASE(TypeTechBit_ReplaysNativeDefaultAnd27Callbacks) {
    std::ifstream file(TechBitFixture(false)); // 제품 전에 고정된 최초 원시 기대다.
    if (!file) throw std::runtime_error("techBit최초fixture누락");
    const auto empty=RiftTypeDefinition::Parse("typename empty {} A00:default:\"a.gif\"#0;"); // 다른115자산의 명시 빈 속성 입력이다.
    const auto order=TypeLoadOrder(OriginalEdition::Patch1078);std::string line;std::size_t rows{}; // 실제 PE와 같은 로딩 순서다.
    while (std::getline(file,line)) { // 모든 최초27입력을 재생한다.
        if (line.empty() || line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto fields=TechBitFields(line,'\t');CHECK(fields.size()==6);
        const auto definition=RiftTypeDefinition::Parse(TechBitInput(fields[0])); // 새 techBit parser를 만들지 않고 기존 구현을 호출한다.
        std::vector<RiftTypeSource> sources; // 각 source는 위 실제 definition 객체를 빌린다.
        for (std::size_t index=0;index<order.size();++index) sources.push_back({order[index],index==1?&definition:&empty}); // 원문 관측과 같은 실제71번만 변경한다.
        const RiftTypeTable table(OriginalEdition::Patch1078,sources);const auto records=table.Types(); // 같은 Record에서 이후 생산자가 사용할 필드를 읽는다.
        CHECK(records.size()==188 && fields[1].size()==188*8);
        for (std::size_t number=0;number<records.size();++number) { // 원문188초기값과 선택된71번의 마지막 대입을 모두 확인한다.
            const auto expected=number==71?TechBitSigned(fields[3]):TechBitSigned(std::string_view(fields[1]).substr(number*8,8));
            CHECK(records[number].techBitIndex==expected);
        }
        ++rows;
    }
    CHECK(rows==27);
}

// 보호 아카이브의 실제116본문을 기존 parser로 읽고 원문 현재 TYPE188필드와 전부 대조한다.
TEST_CASE(TypeTechBit_Protected116DefinitionsMatchAll188NativeFields) {
    std::ifstream fixture(TechBitFixture(true));std::string line; // 별도 원본 자산대조의188DWORD다.
    if (!std::getline(fixture,line)) throw std::runtime_error("techBit자산최초fixture누락");
    const auto first=TechBitFields(line,'\t');CHECK(first.size()==2 && first[1].size()==188*8);
#ifdef NETSTORM_TYPE_TECH_BIT_ORIGINALS
    const std::filesystem::path root=NETSTORM_TYPE_TECH_BIT_ORIGINALS; // 실제 보호 디렉터리는 읽기만 한다.
#else
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
#endif
    const auto archive=TaffArchive::Open(root/"netstorm.tarc"); // Native와 같은 원본 단일 아카이브 바이트를 읽는다.
    const auto order=TypeLoadOrder(OriginalEdition::Patch1078);std::vector<RiftTypeDefinition> definitions;definitions.reserve(order.size()); // source 주소를 이동하지 않도록 먼저 확보한다.
    for (const auto name:order) { // 각 타입의 실제 전체본문을 기존 lexer에 넘긴다.
        const auto bytes=archive.Read("d/"+std::string(name)+".type");definitions.push_back(RiftTypeDefinition::Parse(DecodeOriginalText(bytes)));
    }
    std::vector<RiftTypeSource> sources; // 같은 실제 definition 목록을 빌리는 원본 순서다.
    for (std::size_t index=0;index<order.size();++index) sources.push_back({order[index],&definitions[index]}); // 별도 type번호-상수 bit계산이 없다.
    const RiftTypeTable table(OriginalEdition::Patch1078,sources);const auto records=table.Types(); // 실제 필드가 같은 Record에 저장됐는지 확인한다.
    CHECK(records.size()==188 && definitions.size()==116);
    for (std::size_t number=0;number<records.size();++number) CHECK(records[number].techBitIndex==TechBitSigned(std::string_view(first[1]).substr(number*8,8))); // 내장/빈타입도 원문 초기값과 비교한다.
    std::size_t assets{},selected{}; // PE이름 순서와 속성 실제선택 수를 추가 확인한다.
    while (std::getline(fixture,line)) { // 최초116행은 실제 PE 로딩 순서이며 제품 목록에서 기대를 만들지 않는다.
        if (line.empty() || line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto row=TechBitFields(line,'\t');CHECK(row.size()==6 && std::stoul(row[0])==assets+70 && row[1]==order[assets]);
        CHECK(records[assets+70].techBitIndex==TechBitSigned(row[4]));if (row[3]!="[]") ++selected;++assets;
    }
    CHECK(assets==116 && selected==23);
}
