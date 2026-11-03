// 실제10.78 CPU 프레임의 현재 색/시각/단계/표시·종료와 도달 불가 진단을 대조한다.
#include "RawSceneSupport.h"
#include "client/PaletteFlash.h"
#include <bit>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 기대값은 CPU fixture만 읽으며 입력 상태/장치만 조립한다.
const auto& PaletteFrameRows() { static const auto rows=LoadFixture(NETSTORM_PALETTE_FRAME_CHECKING_FIXTURE).rows;return rows; } // 원문1670행을 그대로 읽는다.
std::string FrameColors(const std::array<ScreenColor,256>& colors,unsigned count) { return count ? netstorm::test::rawscene::Hex({reinterpret_cast<const std::uint8_t*>(colors.data()),count*4U}) : "-"; } // 예약 바이트도 같은 물리 순서로 출력한다.
// Init 뒤의 명시 상태를 만들며 Frame/색 접근/기한/종료 계산은 실제 제품이 수행한다.
std::vector<std::string> ReplayPaletteFrame(const std::vector<std::string>& row) {
    PaletteFlashState state;state.interval=std::bit_cast<double>(std::stoull(row[1]));state.last=std::bit_cast<double>(std::stoull(row[2]));double now=std::bit_cast<double>(std::stoull(row[3]));const auto increment=std::bit_cast<double>(std::stoull(row[4]));state.steps=std::stoi(row[5]);const auto gate=Number(row[6]),mode=Number(row[7]);state.start=Number(row[8]);state.count=Number(row[9]);const auto pattern=Number(row[10]),steps=Number(row[11]);state.initialized=true;
    auto* original=reinterpret_cast<std::uint8_t*>(state.original.data());auto* current=reinterpret_cast<std::uint8_t*>(state.current.data());
    for (std::uint32_t byte=0;byte<1024;++byte) { original[byte]=static_cast<std::uint8_t>(byte*37U+11U+pattern);current[byte]=static_cast<std::uint8_t>(byte*13U+143U+pattern*17U); } // 실제 CPU의 명시 입력만 복제한다.
    std::uint32_t calls{};std::string trace;PaletteFlashHooks hooks{[&] { return now; },[&] { return (mode&1U)!=0; },[](auto,auto,auto) { throw std::logic_error("프레임의 미선택 Init 장치"); },[&](auto start,auto count,auto colors) { CHECK(colors==state.current.data());trace+="A,"+std::to_string(start)+','+std::to_string(count)+','+FrameColors(state.current,count)+";"; },[&] { trace+="R;"; }};PaletteFlash flash(state,std::move(hooks));
    for (std::uint32_t step=0;step<steps;++step) { ++calls;const auto alive=flash.Frame();now+=increment;if (!alive) { trace+="K,0;";break; } } // 실제 Kill 뒤에는 같은 효과를 다시 실행하지 않는다.
    return {std::to_string(std::bit_cast<std::uint64_t>(state.interval)),std::to_string(std::bit_cast<std::uint64_t>(state.last)),std::to_string(state.steps),state.initialized ? "1" : "0",FrameColors(state.original,state.count),FrameColors(state.current,state.count),std::to_string(calls),std::to_string(gate),"-",trace.empty() ? "-" : trace}; // checking은 미도달 조건이라 값/효과를 바꾸는 제품 writer가 없다.
}
}
// signed 극단/0/65·정확한 간격/NaN/무한/0·음수 간격·전체/끝/빈 색을 CPU와 직접 비교한다.
TEST_CASE(PaletteFrameChecking_ReplaysNativeCpuAndUnreachableAssertion) {
    std::size_t count{};
    for (const auto& row:PaletteFrameRows()) { // 각 행의 첫 불일치 원시 필드를 남긴다.
        CHECK(row.size()==22);const auto actual=ReplayPaletteFrame(row);const std::vector<std::string> expected(row.begin()+12,row.end());CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("팔레트 Frame 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),index,expected[index].c_str(),actual[index].c_str()); } // Native 기대값은 교체하지 않는다.
        CHECK(row[19]==row[6] && row[20]=="-");++count; // checking0/상위비트/ffffffff 모두 진단이0이며 현재 DWORD를 보존한다.
    }
    CHECK(count==1670);
}
