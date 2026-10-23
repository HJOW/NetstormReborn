// 제품전PE116관측에서확정한부모순서와중단전부분효과를보존한다.
#include "o/RawColorSortStartup.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr double kGray=std::bit_cast<double>(std::uint64_t{0x3fcb9b9b9b9b9b9c}),kWidth=2.4; //원문00506698/00503678의정확한double입력이다.
// 같은 검색객체의 첫 DWORD만 little endian vtable로 쓴다.
void SearchVtable(std::span<std::uint8_t> memory) {
    if (memory.size()<4) throw std::out_of_range("ColorSort 검색 vtable 물리 범위");
    constexpr std::uint32_t value=0x506658; //43b502가직접쓰는파생vtable식별자다.
    for (std::size_t index=0;index<4;++index) memory[index]=static_cast<std::uint8_t>(value>>(index*8)); //인접미작성BYTE는보존한다.
}
}
void InitializeColorSortGradient(ColorSortStartupGlobals globals,const ColorSortStartupHooks& hooks) {
    if (globals.gradient!=0) { hooks.release(globals.gradient);globals.gradient=0; }
    const auto allocated=hooks.allocate(0x5020); //할당callback뒤에만현재count를읽는다.
    if (allocated!=0) {
        const auto count=globals.count; //이후생성자callback중바뀌어도현재인자는캡처값이다.
        globals.gradient=hooks.constructGradient(allocated,{kGray,kGray,kGray},{1.0,0.0,0.0},count,kWidth);
    } else globals.gradient=0;
}
void InitializeColorSort(std::uint32_t logical,ColorSortStartupGlobals globals,const ColorSortStartupHooks& hooks) {
    if (globals.paletteLoaded==0) hooks.assertion(451);
    if (globals.primary!=0) { hooks.release(globals.primary);globals.primary=0; }
    if (globals.search!=0) { hooks.release(globals.search);globals.search=0; }
    const auto primary=hooks.allocate(0x3800); //같은logical포인터의내용은뒤생성자진입에서처음읽는다.
    globals.primary=primary!=0 ? hooks.construct(primary,logical) : 0U;
    const auto search=hooks.allocate(0x5008); //반환주소는이후callback동안캡처한다.
    if (search!=0) {
        hooks.constructSearch(search);
        SearchVtable(hooks.memory(search));
        hooks.sort(search+8U,256,80,0x43a1e0);
    }
    globals.search=search; //qsort뒤현재전역이바뀌었어도캡처한할당주소를쓴다.
    globals.count=10;
    InitializeColorSortGradient(globals,hooks);
}
}
