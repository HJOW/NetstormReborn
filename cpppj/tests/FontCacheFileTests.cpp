// 원본 순차 글꼴 캐시 부모의 부분 설치/실패/정리·쓴BYTE를 고정 관측과 대조한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "client/FontCacheFile.h"
#include <array>
#include <bit>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 최초 fixture의 입력과 장치만 재생하며 기대 출력은 계산하지 않는다.
constexpr std::uintptr_t kFile=0x12200000,kKey=0x12300100; // 독립 CPU의 불투명 파일/키 토큰이다.
struct InputFont { // 원문 owner와 입력인 세 작은 글리프를 묶는다.
    std::array<std::uint8_t,kFontCacheOwnerBytes> owner{}; // 네 머리 DWORD와 네 표다.
    std::array<std::vector<std::uint8_t>,256> glyphs; // 각 code의 입력 바이트다.
    InputFont() { // 독립 CPU에 제공한 동일 입력만 구성한다.
        using namespace netstorm::test::rawscene;Put(owner,0,1);Put(owner,4,14);Put(owner,8,11);Put(owner,12,3);
        for (std::uint32_t code=0;code<256;++code) { // 네 표와 각 글리프의 입력을 코드 순서로 만든다.
            const auto length=code==0 ? 2U:code==65 ? 3U:code==255 ? 5U:0U;
            Put(owner,16+code*4,5);Put(owner,0x410+code*4,0xfffffffe);Put(owner,0x810+code*4,6);Put(owner,0xc10+code*4,length);
            for (std::uint32_t offset=0;offset<length;++offset) glyphs[code].push_back(static_cast<std::uint8_t>(code+offset*7)); // 입력 글리프 BYTE이며 예상 출력이 아니다.
        }
    }
};
std::string Fields(const RawBaseFileState& state) { // 여섯 원문 필드의 현재 값이다.
    const std::array<std::uintptr_t,6> values{{state.file,state.flags,std::uint32_t(state.offset),std::uint32_t(state.length),state.key,state.keyLength}};std::string result;
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result; // 포인터를32비트로 절단하지 않는다.
}
void CompareNative(bool writing) { // 두 실제 부모를 각각 별도 등록 검사로 실행한다.
    using namespace netstorm::test::rawscene;std::ifstream input(NETSTORM_FONT_CACHE_FILE_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { // 최초 원본 결과를 읽기만 하며 기대값을 재생성하지 않는다.
        const auto row=Split(line,'\t');CHECK(row.size()==8);if (row.size()!=8) return;const auto p=Split(row[0],',');CHECK(p.size()==6);if (p.size()!=6) return;
        if (bool(Number(p[0]))!=writing) continue;
        const auto key=Number(p[1]),sub=Number(p[2]),scenario=Number(p[3]),fail=Number(p[4]),returnAssert=Number(p[5]);InputFont source;
        std::vector<std::uint8_t> stored{'B','i','t','m','a','p','F','o','n','t','D','a','t','a',26,0};stored.insert(stored.end(),source.owner.begin(),source.owner.begin()+16);stored.insert(stored.end(),{0,1,0,0});stored.insert(stored.end(),source.owner.begin()+16,source.owner.end());
        for (const auto& glyph:source.glyphs) stored.insert(stored.end(),glyph.begin(),glyph.end()); // 순차 파일의 입력 payload다.
        if (scenario==7) Put(stored,32,255);if (scenario==8) Put(stored,16,2);if (scenario==9) stored[0]='X';if (scenario==10) stored[15]=255;
        if (key) for (std::size_t index=0;index<stored.size();++index) stored[index]^=static_cast<std::uint8_t>(std::string_view("abc")[index%3]); // 저장 입력에만 키를 적용한다.
        std::array<std::uint8_t,kFontCacheOwnerBytes> owner{};if (writing) owner=source.owner;std::array<std::optional<std::vector<std::uint8_t>>,256> glyphs;
        if (writing) for (std::size_t code=0;code<256;++code) glyphs[code]=source.glyphs[code]; // writer 입력의 존재하는0길이 포인터도 구별한다.
        std::array<std::uint8_t,16> signature;signature.fill(0x65);std::array<std::uint8_t,4> charset;charset.fill(0x65);std::array<std::uint8_t,4096> scratch{};
        RawBaseFileState state{0x65656565,0x65656565,std::bit_cast<std::int32_t>(0x65656565U),std::bit_cast<std::int32_t>(0x65656565U),0x65656565,0x65656565};RawBaseFileIOHooks io;FontCacheFileHooks hooks;std::string events,outcome="ok",returned="-";std::vector<std::uint8_t> output;std::int32_t position{};std::uint32_t calls{},allocations{};
        const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 장치 호출열만 기록한다.
        hooks.exists=[&](auto name) { CHECK(name=="font.chfnt");event("P");return scenario!=1; };
        hooks.open=[&](auto& current,auto name,auto mode,auto create) { CHECK(name=="font.chfnt" && mode==(writing ? 2U:0U) && create==1 && !current.key && !current.keyLength);event("O");current={scenario==3 ? 0U:kFile,0x80U|sub,sub ? 37:0,sub ? static_cast<std::int32_t>(stored.size()):0,key ? kKey:0,key ? 3U:0U};position=sub ? 37:0;if (scenario==2) throw std::runtime_error("open"); };
        io.tell=[&](auto file) { CHECK(file==kFile);event("T");return position; };
        io.seek=[&](auto file,auto offset,auto origin) { CHECK(file==kFile);event("S:"+std::to_string(offset)+":"+std::to_string(origin));position=offset; };
        io.read=[&](auto file,auto bytes,auto size,auto count) { CHECK(file==kFile);++calls;const auto start=position-(sub ? 37:0);CHECK(start>=0);const auto actual=std::min<std::size_t>(std::size_t(size)*count,static_cast<std::size_t>(std::max<std::int64_t>(0,std::int64_t(stored.size())-(scenario==11 ? 2:0)-start)));event("R:"+std::to_string(size)+":"+std::to_string(count)+":"+std::to_string(actual));if (scenario==4 && calls==fail) throw std::runtime_error("read");if (actual) std::copy_n(stored.begin()+start,actual,bytes.begin());position+=static_cast<std::int32_t>(actual); };
        io.write=[&](auto file,auto bytes,auto size,auto count) { CHECK(file==kFile);++calls;const auto sent=std::size_t(size)*count;CHECK(bytes.size()>=sent);event("W:"+std::to_string(size)+":"+std::to_string(count)+":"+std::to_string(sent));if (scenario==4 && calls==fail) throw std::runtime_error("write");const auto actual=scenario==11 ? sent/2:sent;output.insert(output.end(),bytes.begin(),bytes.begin()+actual);position+=static_cast<std::int32_t>(actual); };
        io.key=[](auto token,auto length) { CHECK(token==kKey && length==3);return std::span(reinterpret_cast<const std::uint8_t*>("abc"),3); };
        io.close=[&](auto file) { CHECK(file==kFile);event("C");if (scenario==6) throw std::runtime_error("close"); };io.freeKey=[&](auto token) { CHECK(token==kKey);event("K"); };
        hooks.allocate=[&](auto code,auto length)->std::span<std::uint8_t> { CHECK(code==allocations);++allocations;event("N:"+std::to_string(code)+":"+std::to_string(length));if (scenario==5 && allocations==fail) throw std::runtime_error("allocation");glyphs[code].emplace(length);return *glyphs[code]; };
        hooks.glyph=[&](auto code,auto length)->std::span<const std::uint8_t> { CHECK(glyphs[code] && glyphs[code]->size()==length);return *glyphs[code]; };
        hooks.assertion=[&](auto sourceLine) { event("D:"+std::to_string(sourceLine));if (!returnAssert) throw std::runtime_error("assertion"); };io.assertion=hooks.assertion;
        try { returned=std::to_string(writing ? WriteFontCacheFile(state,"font.chfnt",owner,io,hooks):ReadFontCacheFile(state,"font.chfnt",owner,io,hooks,scratch,signature,charset)); }catch (const std::exception& failure) { outcome=failure.what(); }
        std::string glyphText;for (const auto& glyph:glyphs) { if (!glyphText.empty()) glyphText+=',';glyphText+=!glyph ? "-":glyph->empty() ? "0":Hex(*glyph); } // 설치 여부와 실제 BYTE를 함께 비교한다.
        CHECK(outcome==row[1] && returned==row[2] && Fields(state)==row[3] && Hex(owner)==row[4] && glyphText==row[5] && events==row[6] && (output.empty() ? "-":Hex(output))==row[7]);++rows;
    }
    CHECK(rows==(writing ? 20U:80U));
}
}
TEST_CASE(FontCacheFile_Native80ReaderPartialInstallationAndClosePrefix) { CompareNative(false); } // 원문 읽기80행의 모든 기록을 대조한다.
TEST_CASE(FontCacheFile_Native20WriterOrderedBytesAndClosePrefix) { CompareNative(true); } // 원문 쓰기20행의 모든 기록을 대조한다.
