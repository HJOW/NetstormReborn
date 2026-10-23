// 같은원문ColorSort를유지한6관측과고정초기입력주소/값을대조한다.
#include "TestSupport.h"
#include "client/ClientColorSortInput.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
// 원문TSV의모든칸을순서대로분리한다.
std::vector<std::string> Fields(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t start=0; //다음칸의첫바이트다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==text.npos?text.npos:end-start));if (end==text.npos) return result;start=end+1; } //마지막빈칸도남긴다.
}
// 전체256/1024BYTE입력을NUL과관계없이읽는다.
std::vector<std::uint8_t> Bytes(const std::string& hex) {
    CHECK(hex.size()%2==0);std::vector<std::uint8_t> result(hex.size()/2); //원문바이트폭만큼소유한다.
    for (std::size_t index=0;index<result.size();++index) result[index]=static_cast<std::uint8_t>(std::stoul(hex.substr(index*2,2),nullptr,16)); //각원시BYTE를독립해석한다.
    return result;
}
// little endian RGBX256개를정규화없이원문입력으로만읽는다.
std::array<std::uint32_t,256> Logical(const std::string& hex) {
    const auto raw=Bytes(hex);CHECK(raw.size()==1024);std::array<std::uint32_t,256> result{}; //원시최초/현재입력을구별한다.
    for (std::size_t index=0;index<result.size();++index) for (std::size_t byte=0;byte<4;++byte) result[index]|=std::uint32_t(raw.at(index*4+byte))<<(byte*8); //호스트포인터재해석을하지않는다.
    return result;
}
}

//53비트의전체9표와64비트의별도명도차이를보존하며화면두번변경뒤같은초기입력을쓴다.
TEST_CASE(ColorSortFixedInput_WholeNativeNineMapsAndStableOwner) {
#ifdef NETSTORM_COLORSORT_FIXED_INPUT_FIXTURE
    const std::filesystem::path path=NETSTORM_COLORSORT_FIXED_INPUT_FIXTURE; //root공용등록의명시자료경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/colorsort-fixed-input-x86.tsv"; //격리검사도제품전같은자료를읽는다.
#endif
    std::ifstream input(path);CHECK(input.good());std::string line;std::size_t rows=0,normal53=0,extended64=0,different64=0; //두정밀도범위를서로섞지않는다.
    netstorm::client::ClientColorSortInput owner;const std::uint32_t* same=nullptr;std::array<std::uint32_t,256> initial{};std::string nativeHash53,nativeHash64; //실제고정주소/값의수명이다.
    while (std::getline(input,line)) { //6관측을모두읽고각각지정정밀도계약을검사한다.
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto fields=Fields(line,'\t');CHECK(fields.size()==7);const auto source=Logical(fields.at(3));auto currentScreen=Logical(fields[4]); //같은화면입력배열이바뀌어도초기입력과구별한다.
        if (!owner.Captured()) { owner.Capture(source);initial=source;same=owner.Logical().data(); }
        CHECK(owner.Logical().data()==same&&std::equal(initial.begin(),initial.end(),owner.Logical().begin()));
        const auto& maps=owner.Rebuild(static_cast<std::uint32_t>(std::stoul(fields[2])));const auto expected=Fields(fields[6],':');CHECK(expected.size()==9); //현재빨강만의도적으로새입력이다.
        for (std::size_t index=0;index<9;++index) { //64비트명도표만원문에서별도차이가난다.
            const auto bytes=Bytes(expected[index]);CHECK(bytes.size()==256);const bool equal=std::equal(maps[index].begin(),maps[index].end(),bytes.begin(),bytes.end());
            if (fields[0]=="639"||index!=8) { if (!equal) std::fprintf(stderr,"ColorSortFixed row=%zu map=%zu\n",rows,index);CHECK(equal); }
            else { CHECK(!equal);++different64; }
        }
        if (fields[0]=="639") { ++normal53;if (nativeHash53.empty()) nativeHash53=fields[5];CHECK(nativeHash53==fields[5]); }
        else { CHECK(fields[0]=="895");++extended64;if (nativeHash64.empty()) nativeHash64=fields[5];CHECK(nativeHash64==fields[5]); }
        currentScreen.fill(0x12345678U); //동일caller현재화면입력의실제쓰기다.초기소유자에동기화하지않는다.
        CHECK(owner.Logical().data()==same&&std::equal(initial.begin(),initial.end(),owner.Logical().begin()));++rows;
    }
    CHECK(rows==6&&normal53==3&&extended64==3&&different64==3&&nativeHash53!=nativeHash64);
}

//미캡처/미생성을빈현재ScreenRGB성공으로바꾸지않으며명시Capture만입력값을교체한다.
TEST_CASE(ColorSortFixedInput_ExplicitCaptureAndNoImplicitCurrentPalette) {
    using netstorm::client::ClientColorSortInput;
    static_assert(!std::is_copy_constructible_v<ClientColorSortInput>&&!std::is_move_constructible_v<ClientColorSortInput>); //빌린입력주소를보존한다.
    ClientColorSortInput owner;CHECK(!owner.Captured());bool rejected=false; //아직실제초기입력이없다.
    try { static_cast<void>(owner.Logical()); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    std::array<std::uint32_t,256> first{}; //원문출력과무관한명시입력이다.
    for (std::size_t index=0;index<first.size();++index) first[index]=static_cast<std::uint32_t>(index)*0x010101U; //중복없는회색RGB다.
    owner.Capture(first);const auto* same=owner.Logical().data();first.fill(0xffffffU); //caller값변경은복사한초기입력에전파하지않는다.
    CHECK(owner.Logical().data()==same&&owner.Logical()[17]==0x111111U);rejected=false;
    try { static_cast<void>(owner.Maps()); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    const auto& maps=owner.Rebuild(0x123U);CHECK(maps[1][17]==0x23);CHECK(&owner.Maps()==&maps); //현재빨강의하위BYTE만읽는다.
    owner.Capture(first);CHECK(owner.Logical().data()==same&&owner.Logical()[17]==0xffffffU); //명시새생성위치에서만입력이바뀐다.
    CHECK(netstorm::client::ColorSortLogicalInput({},first).data()==first.data()); //소유자없는독립검사만명시현재입력fallback을쓴다.
    CHECK(netstorm::client::ColorSortLogicalInput(owner.Logical(),first).data()==same); //실제Client의같은고정입력주소를유지한다.
    rejected=false;try { static_cast<void>(netstorm::client::ColorSortLogicalInput(std::span(first).first(255),first)); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected); //잘못된물리폭을조용히현재RGB로대체하지않는다.
}
