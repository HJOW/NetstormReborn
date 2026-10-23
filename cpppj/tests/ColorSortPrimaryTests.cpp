// 제품 전에 고정한 raw 생성자80행의 실제 BYTE/순서/부분효과와 두 원문 정밀도를 구별한다.
#include "TestSupport.h"
#include "o/RawColorSortPrimary.h"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
using namespace netstorm::o;
// 원문 fixture의 마지막 빈 칸도 버리지 않는 구분자 파서다.
std::vector<std::string> PrimaryFields(std::string_view line) {
    std::vector<std::string> result;std::size_t begin=0; // 현재 칸의 시작 위치다.
    for (;;) { // 모든 칸을 한 번씩 원시 순서로 읽는다.
        const auto end=line.find('\t',begin); // 다음 구분자다.
        result.emplace_back(line.substr(begin,end==line.npos?line.npos:end-begin));
        if (end==line.npos) return result;
        begin=end+1;
    }
}
// 원시 hex의 한 자리만 해석하며 제품 수학을 기대값으로 쓰지 않는다.
std::uint8_t PrimaryNibble(char value) {
    if (value>='0' && value<='9') return static_cast<std::uint8_t>(value-'0');
    if (value>='a' && value<='f') return static_cast<std::uint8_t>(value-'a'+10);
    throw std::logic_error("ColorSort primary 최초 hex 문자");
}
// Native가 저장한 전체 물리 BYTE를 손실 없이 읽는다.
std::vector<std::uint8_t> PrimaryBytes(std::string_view hex) {
    if (hex.size()%2) throw std::logic_error("ColorSort primary 최초 hex 길이");
    std::vector<std::uint8_t> bytes(hex.size()/2); // 원시 물리 폭을 유지한다.
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>((PrimaryNibble(hex[index*2])<<4)|PrimaryNibble(hex[index*2+1])); // 원시 BYTE 하나다.
    return bytes;
}
// 관측 경계에서 같은 실제 raw store의 현재 값만 Native trace와 비교한다.
std::string PrimaryHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef"; // 원문과 같은 소문자 hex 표기다.
    std::string result;result.reserve(bytes.size()*2); // store4/8BYTE의 출력만 만든다.
    for (const auto value:bytes) { result+=digits[value>>4];result+=digits[value&15]; } // 현재 BYTE의 두 자리다.
    return result;
}
// 공용 등록 전에도 동일한 최초 자료만 읽는다.
std::vector<std::vector<std::string>> PrimaryRows() {
#ifdef NETSTORM_COLORSORT_PRIMARY_FIXTURE
    const std::filesystem::path path=NETSTORM_COLORSORT_PRIMARY_FIXTURE; // root 공용 등록이 선택할 절대 최초 fixture다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/colorsort-primary-x86.tsv"; // 격리 빌드도 같은 원시 자료를 쓴다.
#endif
    std::ifstream input(path); // 실제 Native 최초 자료만 읽는다.
    if (!input) throw std::runtime_error("ColorSort primary 최초 fixture 없음");
    std::vector<std::vector<std::string>> rows;std::string line; // 원문80행의 입력과 기대를 별도 보관한다.
    while (std::getline(input,line)) { // 주석을 제외한 모든 물리 행을 읽는다.
        if (line.empty() || line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        rows.push_back(PrimaryFields(line));
        if (rows.back().size()!=14) throw std::logic_error("ColorSort primary 최초 칼럼 수");
    }
    return rows;
}
// Native가 N개 store 완료 뒤 다음 명령 전에 멈춘 것과 같은 외부 관측 중단이다.
struct PrimaryObservationStop {};
}

// 지원하는027f/53비트만 raw 전체·실제 alias·모든 store/명시 중단 위치와 대조한다.
TEST_CASE(ColorSortPrimary_ReplaysFrozen53BitRecordsAndPartialEffects) {
    static_assert(!std::is_copy_constructible_v<RawColorSortPrimary> && !std::is_move_constructible_v<RawColorSortPrimary>); // 빌린 raw 주소를 이전하지 않는다.
    const auto rows=PrimaryRows();std::size_t count=0,normal=0,stops=0,lightness=0; // 제품 대조와 미지원037f 증거를 섞지 않는다.
    for (const auto& row:rows) { // 최초80행 중027f40행만 제품53비트와 비교한다.
        if (row[1]!="027f") continue;
        const auto logical=PrimaryBytes(row[4]),initial=PrimaryBytes(row[5]),expected=PrimaryBytes(row[6]),logicalAfter=PrimaryBytes(row[7]); // 모두 제품 전 Native 원시 BYTE다.
        RawColorSortPrimary owner(initial);const auto* same=owner.Bytes().data(); // 초기 할당/미작성padding을 그대로 소유한다.
        const auto alias=std::stoi(row[2]),stop=std::stoi(row[3]); // 실제 물리 alias와 완료 store 수다.
        std::span<const std::uint8_t> input=logical; // 외부 입력이 기본이며 alias행은 같은 raw를 빌린다.
        if (alias>=0) input=owner.Bytes().subspan(static_cast<std::size_t>(alias),1024);
        std::string trace;std::uint32_t writes=0;bool stopped=false; // 콜백의 외부 관측만 보관한다.
        ColorSortPrimaryHooks hooks;hooks.checkpoint=[&](std::uint32_t completed,std::uint32_t offset,std::uint32_t size) {
            if (completed) { if (!trace.empty()) trace+=';';trace+=std::to_string(offset)+':'+std::to_string(size)+':'+PrimaryHex(owner.Bytes().subspan(offset,size)); }
            writes=completed;
            if (stop>=0 && completed==static_cast<std::uint32_t>(stop)) throw PrimaryObservationStop{};
        }; // 관측자는 입력/기록을 수정하지 않으며 예외만 명시 중단에 쓴다.
        try { owner.Construct(input,hooks); } catch (const PrimaryObservationStop&) { stopped=true; }
        CHECK(owner.Bytes().data()==same && std::equal(expected.begin(),expected.end(),owner.Bytes().begin()));
        CHECK(std::equal(logicalAfter.begin(),logicalAfter.end(),input.begin()) && trace==row[12]);
        CHECK(stopped==(row[8]=="0") && owner.Constructed()==(row[8]=="1"));
        if (row[8]=="1") {
            ++normal;CHECK(writes==1792);
            for (std::size_t index=0;index<256;++index) { // getter는 Native +8의 비트를 직접 반환해야 한다.
                std::uint64_t bits=0; // 원문 little endian8BYTE를 별도 해석한다.
                for (std::size_t byte=0;byte<8;++byte) bits|=std::uint64_t(expected[index*56+8+byte])<<(byte*8); // RGB 재계산을 기대에 쓰지 않는다.
                CHECK(std::bit_cast<std::uint64_t>(owner.Lightness(index))==bits);++lightness;
            }
        } else { ++stops;CHECK(writes==static_cast<std::uint32_t>(stop)); }
        ++count;
    }
    CHECK(rows.size()==80 && count==40 && normal==18 && stops==22 && lightness==4608);
}

//037f는 독립 원문 출력으로만 보존하며 제품 기대를 맞추려고53비트 출력으로 치환하지 않는다.
TEST_CASE(ColorSortPrimary_PreservesBothNativePrecisionFieldDifferences) {
    const auto rows=PrimaryRows(); // 두 원문 제어어의 같은 입력을 비교한다.
    std::map<std::string,std::array<std::vector<std::uint8_t>,2>> pairs; // 정상 입력만 같은 id로 짝짓는다.
    std::array<std::size_t,8> differences{}; // H/L/S/R/G/B/index/padding의 차이를 분리한다.
    std::size_t extended=0,normal=0,stopped=0; // 비지원64비트 자료를 제품 성공 수로 세지 않는다.
    for (const auto& row:rows) { // 전체 원문80행은 보존/형식 감사만 한다.
        CHECK(row[1]=="027f" || row[1]=="037f");
        if (row[1]=="037f") ++extended;
        if (row[8]=="1") { ++normal;pairs[row[0]][row[1]=="027f"?0:1]=PrimaryBytes(row[6]); } else ++stopped;
    }
    constexpr std::array<std::size_t,8> offsets{0,8,16,24,32,40,48,52},sizes{8,8,8,8,8,8,4,4}; // 실제56BYTE 기록의 독립 필드다.
    for (const auto& [key,pair]:pairs) { // 같은 실제 입력의 두 원문 결과만 비교한다.
        static_cast<void>(key);CHECK(pair[0].size()==kColorSortPrimaryBytes && pair[1].size()==kColorSortPrimaryBytes);
        for (std::size_t index=0;index<256;++index) { // 전체 항목의 모든 기록 필드를 본다.
            for (std::size_t field=0;field<offsets.size();++field) { // 원문 padding도 별도로 보존한다.
                const auto offset=index*56+offsets[field]; // 현재 필드의 원시 BYTE 시작이다.
                if (!std::equal(pair[0].begin()+offset,pair[0].begin()+offset+sizes[field],pair[1].begin()+offset)) ++differences[field];
            }
        }
    }
    CHECK(rows.size()==80 && extended==40 && normal==36 && stopped==44 && pairs.size()==18);
    CHECK(differences==(std::array<std::size_t,8>{409,0,451,0,0,0,0,0}));
}

// 잘못된 초기 물리를0으로 가장하지 않고 같은 owner의 재생성과 getter 범위를 구별한다.
TEST_CASE(ColorSortPrimary_RequiresAllocationAndCompletedCurrentStorage) {
    std::array<std::uint8_t,kColorSortPrimaryBytes> initial;initial.fill(0xa5); // 명시 할당 물리이며 padding도 비영이다.
    std::array<std::uint8_t,1024> logical{}; // 원문에 유효한 검정 RGBX 입력이다.
    RawColorSortPrimary owner(initial);bool missing=false; // 생성 전 raw BYTE 자체는 그대로 빌릴 수 있다.
    try { static_cast<void>(owner.Lightness(0)); } catch (const std::logic_error&) { missing=true; }
    CHECK(missing && !owner.Constructed());owner.Construct(logical);
    const auto* same=owner.Bytes().data(); // 두번째 명시 생성도 같은 payload 주소다.
    owner.Construct(logical);CHECK(owner.Bytes().data()==same && owner.Constructed() && owner.Lightness(0)==0.0);
    bool badSize=false,badIndex=false; // host 물리/범위 오류를 원문 진단 후 계속과 혼합하지 않는다.
    try { RawColorSortPrimary invalid(std::span<const std::uint8_t>(initial).first(8)); } catch (const std::invalid_argument&) { badSize=true; }
    try { static_cast<void>(owner.Lightness(256)); } catch (const std::out_of_range&) { badIndex=true; }
    CHECK(badSize && badIndex);
    for (std::size_t index=0;index<256;++index) CHECK(std::all_of(owner.Bytes().begin()+index*56+52,owner.Bytes().begin()+index*56+56,[](auto value) { return value==0xa5; })); // 재생성도 미작성padding을 유지한다.
}
