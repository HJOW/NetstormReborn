// 팔레트 계산을 중복 작성하지 않고 원문 이름·캐시·하위 호출·dirty의 순서만 소유한다.
#include "o/RawFortPaletteTransition.h"
#include <cstddef>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 전체 길이를 미리 계산하지 않고 한 BYTE씩 읽고 써서 겹치는 버퍼와 물리 중단 전 효과를 보존한다.
void CopyNameForward(std::span<std::uint8_t> target,std::span<const std::uint8_t> source) {
    // 원문에는 길이 절단이나 memcpy가 없으며 NUL을 쓴 직후에만 끝난다.
    for (std::size_t index=0;;++index) {
        if (index>=source.size()) throw std::out_of_range("팔레트 이름 입력 물리 범위");
        const auto value=source[index]; // 같은 source 주소의 현재 BYTE이며 앞선 alias 쓰기를 관측한다.
        if (index>=target.size()) throw std::out_of_range("팔레트 이름 출력 물리 범위");
        target[index]=value;
        if (value==0) return;
    }
}
}
// 같은 현재 구름 할당을 해제한 뒤에만 전역 토큰을0으로 쓰며 실패 전 효과를 보존한다.
void ReleaseFortPaletteCloudCache(const FortPaletteTransitionGlobals& globals,const FortPaletteTransitionHooks& hooks) {
    const auto current=globals.cache; // 00470c30이 검사와 free 인자에 함께 사용하는 현재 주소이다.
    if (current!=0) {
        hooks.release(current);
        globals.cache=0;
    }
}
// 실제 구름 이름을 복사하고 필요한 생성 뒤에 한 번 읽은 cloudSpeed만큼 행을 갱신한다.
void SetFortPaletteCloudName(std::span<const std::uint8_t> source,const FortPaletteTransitionGlobals& globals,const FortPaletteTransitionHooks& hooks) {
    CopyNameForward(globals.cloudName,source);
    if (globals.cache==0) hooks.buildCloud();
    auto remaining=globals.count; // build가 바꾼 현재 횟수까지 반영한 뒤 원문 ESI에 한 번 캡처한다.
    // 각 step이 count/cache를 바꾸어도 남은 횟수는 캡처된 DWORD를 사용한다.
    while (remaining!=0) {
        hooks.stepCloud();
        --remaining;
    }
}
// 팔레트 적용 뒤 현재 렌더러가 존재하는 경우에만 dirty를 설정한다.
void MarkFortPaletteDirty(const FortPaletteTransitionGlobals& globals) {
    if (globals.renderer!=0) globals.dirty=1;
}
// 같은 이름 자연반환과 로그·이름·구름·경로·장치·색표·dirty의 실제 순서를 실행한다.
void TransitionFortPalette(const std::span<const std::uint8_t>& argument,const FortPaletteTransitionGlobals& globals,const FortPaletteTransitionHooks& hooks) {
    hooks.log(29,"init palette\n");
    const auto source=argument; // 004b610d는 log 복귀 뒤 인자 포인터를 ESI에 캡처한다.
    if (hooks.compare(globals.paletteName,source)==0) return;
    CopyNameForward(globals.paletteName,source);
    ReleaseFortPaletteCloudCache(globals,hooks);
    SetFortPaletteCloudName(source,globals,hooks);
    const auto path=hooks.pathSpec("GamePalSpec",source,0,0); // 원문 push 1은 PathSpec 인자가 아니라 다음 LoadPalette의 둘째 인자이다.
    hooks.loadPalette(path,1);
    hooks.buildMaps();
    MarkFortPaletteDirty(globals);
}
}
