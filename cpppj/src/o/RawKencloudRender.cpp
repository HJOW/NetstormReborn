// 고정 Native944관측의 현재 재조회·DWORD 감김·부분 효과를 같은 순서로 실행한다.
#include "o/RawKencloudRender.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 기존 Kencloud 소유자의 같은 packed 배열을 복제하지 않는다.
namespace { // 물리 범위와 필수 하위는 실제로 접근하는 시점에만 검사한다.
constexpr std::size_t kCache=515,kRows=516,kAll=2051,kPart=2052; // 원문 연속 전역의 실제 DWORD 위치다.
std::uint32_t& At(std::span<std::uint32_t> state,std::size_t index) { if (index>=state.size()) throw std::out_of_range("구름 표시 현재 DWORD 물리 범위");return state[index]; } // 이전 쓰기를 예외로 되돌리지 않는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 원문 signed 비교만 수행한다.
std::uint32_t Remainder(std::uint32_t value) { return std::bit_cast<std::uint32_t>(Signed(value)%512); } // 음수도 원문처럼 음의 나머지다.
template<class Callback> const Callback& Need(const Callback& callback) { if (!callback) throw std::logic_error("구름 표시 선택된 실제 장치 누락");return callback; } // 자연 생략 경로에는 장치를 요구하지 않는다.
}
void UpdateKencloud(const KencloudRenderGlobals& globals,const KencloudRenderHooks& hooks) {
    auto count=globals.speed;if (!At(globals.state,kCache)) { Need(hooks.build)();count=globals.speed; } // 기존 캐시는 최초 속도만 캡처한다.
    for (;count;--count) Need(hooks.step)(); // 하위가 현재 speed를 바꿔도 이미 캡처한 반복 수는 유지한다.
}
void MarkKencloudDirty(std::span<std::uint32_t> state) { At(state,kAll)=1;At(state,kPart)=0; } // 첫 store 뒤 중단은 그대로 남는다.
void CleanKencloudDirty(std::span<std::uint32_t> state) {
    for (std::size_t index=0;index<512;++index) if (At(state,index)) { At(state,kRows+index)=0;At(state,index)=0;At(state,kAll)=0; } // 실제 이전 표시 행만 지운다.
    if (At(state,kPart)) { At(state,kAll)=0;At(state,kPart)=0; } // 전체 표시가 남긴 부분 플래그도 마지막에 정리한다.
}
std::uint32_t KencloudRowDirty(std::span<std::uint32_t> state,std::uint32_t first,std::uint32_t second) {
    const auto row=Signed(Remainder(first+second));return At(state,static_cast<std::size_t>(static_cast<std::int32_t>(kRows)+row)) || At(state,kAll) ? 1U : 0U; // 음수 행은 앞선 같은 물리 DWORD를 읽는다.
}
void DrawKencloud(const KencloudRenderGlobals& globals,const std::array<std::uint32_t,6>& area,const KencloudRenderHooks& hooks,bool alternate) {
    const auto [left,top,right,bottom,x,y]=area;if ((right-left)*(bottom-top)==0) return; // 원문 곱의 하위32비트가0이면 장치도 조회하지 않는다.
    if (!At(globals.state,kCache)) Need(hooks.build)();const auto previous=Need(hooks.clip)();Need(hooks.setClip)({left,top,right,bottom});
    const auto firstX=(left+x)-(Remainder(left+x)+x),firstY=(top+y)-(Remainder(top+y)+y);
    const auto lastX=(x+511U+right)-(Remainder(x+511U+right)+x),lastY=(bottom+511U+y)-(Remainder(bottom+511U+y)+y); // x86 덧셈/뺄셈의 실제 감김이다.
    Need(hooks.lock)(alternate);
    for (auto tileY=firstY,rowBottom=1U-firstY;Signed(tileY)<Signed(lastY);tileY+=512U,rowBottom-=512U) { // 각 세로 타일에서 현재 allDirty를 다시 읽는다.
        if (!At(globals.state,kAll)) {
            for (std::uint32_t row=0;row<512;++row) if (At(globals.state,kRows+row)) { // dirty 값은 각 실제 행 접근 때 조회한다.
                for (auto tileX=firstX;Signed(tileX)<Signed(lastX);tileX+=512U) Need(hooks.blit)({tileX,tileY+row,At(globals.state,kCache),512,0,row,512,rowBottom+tileY+row}); // 각 복사 전에 현재 cache를 조회한다.
                At(globals.state,row)=1; // 가로 복사가 없어도 선택된 행의 이전 표시 요청을 남긴다.
            }
        } else {
            for (auto tileX=firstX;Signed(tileX)<Signed(lastX);tileX+=512U) Need(hooks.blit)({tileX,tileY,At(globals.state,kCache),512,0,0,512,512}); // 전체 경로도 callback 뒤 현재 cache를 읽는다.
            At(globals.state,kPart)=1; // 현재 세로 타일의 가로 복사가 끝난 뒤에만 저장한다.
        }
    }
    Need(hooks.unlock)(alternate);Need(hooks.setClip)(previous); // 예외 때 이 후속 효과를 가짜로 실행하지 않는다.
}
}
