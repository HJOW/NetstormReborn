// 원문 x87 출력16BYTE·현재 모드/옵션 쓰기·청크 조회 순서와 Player 예산을 대조한다.
#include "ConstructionSupport.h"
#include "o/RawWorldGeyserBounds.h"

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace { // 원문과 같은 입력만 구성하며 기대 좌표나 예산은 계산하지 않는다.
constexpr std::uint32_t kWorldBoundsCases=820; // 독립 두 emulator의 같은 최초 입력 수다.
// 음수/감긴 DWORD를 원본 signed 좌표로 읽는다.
std::int32_t WorldBoundsSigned(const std::string& value) { return static_cast<std::int32_t>(std::stoll(value)); }
// 원문과 같은16BYTE 출력을 보존하며 구조체 padding을 읽지 않는다.
std::string WorldBoundsHex(FortRandomRectangle rectangle) {
    std::array<std::uint8_t,16> bytes{};
    Put(bytes,0,Bits(rectangle.left));Put(bytes,4,Bits(rectangle.top));Put(bytes,8,Bits(rectangle.right));Put(bytes,12,Bits(rectangle.bottom));return Hex(bytes);
}
}
// 출력은 최초 원문 fixture에서만 가져오며 동일 입력의 전체 필드/외부 순서를 비교한다.
TEST_CASE(WorldGeyserBounds_IndependentNative820) {
    std::ifstream input(NETSTORM_WORLD_GEYSER_BOUNDS_FIXTURE);if (!input) throw std::runtime_error("세계 간헐천 범위 원문 fixture 없음");
    std::string line;std::uint32_t count{};
    while (std::getline(input,line)) { // 사례마다 모드/옵션/Player/여백을 독립 초기화한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=23) throw std::runtime_error("세계 간헐천 범위 원문 열 수 오류");++count;
        bool mission=Number(row[1])!=0,battle=Number(row[16])!=0;std::uint8_t size=static_cast<std::uint8_t>(Number(row[2]));
        std::int32_t margin=0x12345678,mapLimit=256;std::uint32_t calls{};const auto alter=Number(row[13]);std::string trace;
        WorldGeyserBoundsHooks hooks;
        hooks.chunkBounds=[&](std::int32_t region) { // 큰 ChunkMap의 명시 입력과 앞선 하위의 현재 모드/옵션 변경만 공급한다.
            if (!trace.empty()) trace+=';';trace+="C,"+std::to_string(region);++calls;
            if (calls==1) {
                if (alter==1 || alter==5) mission=true;else if (alter==2) mission=false;else if (alter==3) size=3;else if (alter==4) size=0;
            } else if (calls==2 && alter==5) mission=false;
            const auto at=region==0 ? 5U : 9U;
            return ChunkRect{WorldBoundsSigned(row[at]),WorldBoundsSigned(row[at+1]),WorldBoundsSigned(row[at+2]),WorldBoundsSigned(row[at+3])};
        };
        hooks.spacing=[&] { if (!trace.empty()) trace+=';';trace+='S';return WorldGeyserSpacing(size); }; // 원본 getter 본문과 같은 실제 표를 실행한다.
        RawWorldGeyserBounds body(OriginalEdition::Patch1078,{mission,mapLimit,margin},std::move(hooks));
        std::string rectangle="a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5";std::int32_t budget{};
        if (row[0]=="B") {
            std::vector<std::uint8_t> players(9*0xb4);std::array<std::uint8_t,87> options{};const auto present=Number(row[14]),hidden=Number(row[15]);
            for (std::uint32_t owner=0;owner<9;++owner) { // owner0/flags50 비영도 같은 원문 입력이며 예산 관문으로 추정하지 않는다.
                Put(players,owner*0xb4+0x2c,owner==0 || (present&(1U<<(owner-1))) ? 0x80000000U : 0U);Put(players,owner*0xb4+0x50,0xff,1);
                if (owner) options[62+owner]=hidden&(1U<<(owner-1)) ? 0x80 : 0;
            }
            budget=WorldGeyserBudget(OriginalEdition::Patch1078,battle,players,options);
        } else rectangle=WorldBoundsHex(row[0]=="W" ? body.Whole() : body.Partition(WorldBoundsSigned(row[3]),WorldBoundsSigned(row[4])));
        const std::array<std::string,6> actual{rectangle,std::to_string(std::bit_cast<std::uint32_t>(margin)),std::to_string(budget),trace.empty() ? "-" : trace,mission ? "1" : "0",std::to_string(size)};
        for (std::size_t field=0;field<actual.size();++field) { // 실패 시 최초 입력 번호와 해당 출력만 기록해 기대 fixture를 바꾸지 않는다.
            if (actual[field]!=row[17+field]) throw std::runtime_error("세계 간헐천 원문 불일치 사례"+std::to_string(count)+" 필드"+std::to_string(field)+" 실제="+actual[field]+" 원문="+row[17+field]);
        }
    }
    CHECK(count==kWorldBoundsCases);
}
// 미선택 spacing은 요구하지 않고 실제 분기 도달 시 앞선 청크 조회를 보존한다.
TEST_CASE(WorldGeyserBounds_SelectedHooksAndPhysicalGuards) {
    bool mission=true;std::int32_t mapLimit=256,margin=17;std::vector<std::int32_t> regions;
    WorldGeyserBoundsHooks hooks;hooks.chunkBounds=[&](auto region) { regions.push_back(region);return ChunkRect{2,3,13,12}; }; // 실제 호출된 지역 순서만 관측한다.
    RawWorldGeyserBounds body(OriginalEdition::Patch1078,{mission,mapLimit,margin},std::move(hooks));
    static_cast<void>(body.Whole());CHECK((regions==std::vector<std::int32_t>{0,1}));CHECK(margin==17);
    regions.clear();static_cast<void>(body.Partition(1,0));CHECK((regions==std::vector<std::int32_t>{0,0,1}));
    mission=false;regions.clear();bool failed=false;
    try { static_cast<void>(body.Whole()); } catch (const std::logic_error&) { failed=true; }
    CHECK(failed);CHECK((regions==std::vector<std::int32_t>{0}));CHECK(margin==17);
    failed=false;try { static_cast<void>(body.Partition(0,0)); } catch (const std::out_of_range&) { failed=true; }CHECK(failed);
    failed=false;try { static_cast<void>(WorldGeyserSpacing(4)); } catch (const std::out_of_range&) { failed=true; }CHECK(failed);
    std::array<std::uint8_t,1620> players{};std::array<std::uint8_t,87> options{};
    failed=false;try { static_cast<void>(WorldGeyserBudget(OriginalEdition::Patch1078,false,std::span(players).first(1619),options)); } catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
}
