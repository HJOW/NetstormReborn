// 실제 원문 매핑 부모의 호출 인자·진단/복귀와 호스트 핸들 폭을 대조한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "client/ReadOnlyFileMap.h"
#include <array>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace { // 원문 fixture 기대값과 명시 입력만 재생한다.
constexpr std::array<std::string_view,3> kNames{{"case.shp","",std::string_view("part\0hidden",11)}}; // Native와 같은 유효 빈/첫NUL 입력이다.
std::string TextHex(std::string_view text) { return netstorm::test::rawscene::Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // 이름/진단의 원래 BYTE를 보존한다.
std::string Fields(std::initializer_list<std::uintptr_t> values) { std::string result;for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result; } // Native 장치 인자를 같은 원문 순서로 직렬화한다.
}
TEST_CASE(ReadOnlyFileMap_Native72ArgumentsAssertionsAndResume) { // 정상·파일0/INVALID·mapping/view0와 진단 복귀의 모든 기록을 비교한다.
    using namespace netstorm::test::rawscene;std::ifstream input(NETSTORM_READONLY_FILE_MAP_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { // 최초 Native 출력은 읽기만 하며 기대값을 제품에 맞춰 다시 쓰지 않는다.
        const auto row=Split(line,'\t');CHECK(row.size()==4);if (row.size()!=4) return;const auto p=Split(row[0],',');CHECK(p.size()==5);if (p.size()!=5) return;
        const auto name=Number(p[0]),file=Number(p[1]),mapping=Number(p[2]),view=Number(p[3]),returnAssert=Number(p[4]);std::string events,outcome="ok",returned="-";ReadOnlyFileMapHooks hooks;
        const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 도달한 효과 경계만 기록한다.
        hooks.createFile=[&](auto path,auto access,auto share,auto security,auto creation,auto flags,auto source) { event("F:"+TextHex(path)+":"+Fields({access,share,security.length,security.descriptor,security.inherit,creation,flags,source}));return file; };
        hooks.createMapping=[&](auto source,auto security,auto protection,auto high,auto low,auto title) { event("M:"+Fields({source,security,protection,high,low,title}));return mapping; };
        hooks.mapView=[&](auto source,auto access,auto high,auto low,auto count) { event("V:"+Fields({source,access,high,low,count}));return view; };
        hooks.assertion=[&](auto condition,auto source,auto sourceLine) { event("D:"+TextHex(condition)+":"+TextHex(source)+":"+std::to_string(sourceLine));if (!returnAssert) throw std::runtime_error("assertion"); };
        try { returned=std::to_string(MapReadOnlyFile(kNames.at(name),0xffffffff,hooks)); }catch (const std::runtime_error& failure) { outcome=failure.what(); }
        CHECK(outcome==row[1] && returned==row[2] && events==row[3]);++rows;
    }
    CHECK(rows==72);
}
TEST_CASE(ReadOnlyFileMap_HostHandlesPreserveAll64Bits) { // 원문의32비트 fixture와 별도로 AMD64 OS 핸들이 절단되지 않는 계약이다.
    CHECK(sizeof(std::uintptr_t)>=8);const auto file=static_cast<std::uintptr_t>(0x123456789abULL),mapping=file+1,view=file+2;std::uint32_t stage{};ReadOnlyFileMapHooks hooks;
    hooks.createFile=[&](auto name,auto access,auto share,auto security,auto creation,auto flags,auto source) { CHECK(stage++==0 && name=="host.shp" && access==0x80000000U && share==1 && security.length==12 && !security.descriptor && !security.inherit && creation==3 && flags==0x80 && !source);return file; };
    hooks.createMapping=[&](auto source,auto security,auto protection,auto high,auto low,auto title) { CHECK(stage++==1 && source==file && !security && protection==2 && !high && !low && !title);return mapping; };
    hooks.mapView=[&](auto source,auto access,auto high,auto low,auto count) { CHECK(stage++==2 && source==mapping && access==4 && !high && !low && !count);return view; };
    hooks.assertion=[](auto,auto,auto) { CHECK(false); };
    CHECK(MapReadOnlyFile("host.shp",std::numeric_limits<std::uintptr_t>::max(),hooks)==view && stage==3);
}
