// 실제 DWORD 감김·현재 참조·원문 행 초기화/나머지와 선택된 장치 호출을 유지한다.
#include "client/RawRendererPrelude.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 원본 Renderer 앞단은 OS/월드 수집이나 게임 실행에 의존하지 않는다.
namespace { // 원문 DWORD 산술과 물리 범위/선택된 하위 요구를 공통 처리한다.
constexpr std::size_t kInitialized=17,kPreviousMode=18; // index와16행 뒤의 인접 DWORD 위치다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Renderer 앞단의 선택된 현재 장치 미연결");return function; } //실제로 도달한 하위만 요구한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } //signed CMP/IDIV를 현재 DWORD 비트로 실행한다.
std::int32_t Add(std::int32_t value,std::int32_t delta) { return Signed(static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(delta)); } //ADD/LEA의 하위32비트 결과다.
std::int32_t Row(RendererPreludeState state,std::int32_t index) {
    const auto physical=static_cast<std::int64_t>(index)+1;
    if (physical<0 || physical>=static_cast<std::int64_t>(state.scan.size())) throw std::out_of_range("Renderer 행 표의 현재 인접 물리 범위");
    return Signed(state.scan[static_cast<std::size_t>(physical)]);
} //index-1/16/17의 실제 인접 alias는 허용하며 임의로16칸에 정규화하지 않는다.
ScreenRect Full(const RendererPreludeGlobals& g) { return {0,0,Signed(g.width),Signed(g.height)}; } //호출 직전 현재 화면 크기다.
ScreenRect View(const RendererPreludeGlobals& g) { return {Signed(g.left),Signed(g.top),Signed(g.right),Signed(g.bottom)}; } //행 조회 뒤 바뀐 현재 지도 clip도 반영한다.
void InitializeRows(RendererPreludeState state,std::uint32_t height) {
    state.scan[kInitialized]|=1U;
    state.scan[2]=height-30U;state.scan[4]=height-150U;state.scan[6]=height-60U;state.scan[8]=height-180U;
    state.scan[10]=height-90U;state.scan[12]=height-210U;
    state.scan[3]=120;state.scan[5]=30;state.scan[7]=150;state.scan[9]=60;state.scan[11]=180;state.scan[13]=90;state.scan[15]=210;
    state.scan[14]=height-120U;state.scan[16]=height-240U;
} //원문은0번 행을 쓰지 않는다. 기존값과 실제 store 순서를 유지하며 최초 높이만 한 번 참조한다.
}

// Draw의 수집/표시 전에 현재 화면 모드·세계 요청·override와 구름 행 스캔을 순서대로 처리한다.
void PrepareRendererFrame(RendererPreludeState state,const RendererPreludeGlobals& g,const RendererPreludeHooks& h) {
    if (g.mode!=state.scan[kPreviousMode]) {
        Need(h.markCloudDirty)();Need(h.fullInvalidate)(Full(g));state.scan[kPreviousMode]=g.mode;
    }
    if (g.force) { Need(h.markCloudDirty)();Need(h.fullInvalidate)(Full(g)); }
    if ((g.mode&8U) && !Need(h.overrideActive)()) {
        auto flags=g.mode&255U; //override getter 효과 뒤 현재 하위 BYTE를 캡처한다.
        if (flags&32U) {
            Need(h.updateClouds)();flags=g.mode&255U;
            if (!(flags&16U)) {
                if (!(state.scan[kInitialized]&1U)) InitializeRows(state,g.initialHeight);
                auto index=Signed(state.scan[0]);auto row=Row(state,index);
                if (row<Add(row,30)) {
                    // 원문은 현재30행 구간의 첫 dirty에서 멈추고 매 callback 뒤 현재 index/상한을 재조회한다.
                    for (;;) {
                        const auto dirty=Need(h.rowDirty)(row,Signed(g.cameraY));index=Signed(state.scan[0]);
                        if (dirty) {
                            const auto start=Row(state,index);const ScreenRect rect{0,start,Signed(g.width),Add(start,30)};
                            Need(h.invalidate)(ClipScreenRect(rect,View(g)),0);break;
                        }
                        row=Add(row,1);if (row>=Add(Row(state,index),30)) break;
                    }
                    flags=g.mode&255U;
                }
                state.scan[0]=static_cast<std::uint32_t>(Add(index,1)%16); //signed 나머지이며 음수 index를 양수로 바꾸지 않는다.
            }
        }
        Need(h.markCloudDirty)();
        if ((flags&16U) && (flags&1U) && !(flags&2U)) { Need(h.invalidateBackground)();state.backgroundRedraw=1; }
    }
    state.drawCounter=0;
    if (g.force) { state.dirtyCount=0;Need(h.invalidate)(Full(g),0); }
}
}
