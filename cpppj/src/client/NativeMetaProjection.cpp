// Native PE의 FILD32·DWORD 감김·float 저장과 배율/클리핑 필드 순서를 보존한다.
#include "client/NativeMetaProjection.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client { // 실제 CS/객체를 만드는 부모가 이 순수 계산을 같은 Client 전역에 연결한다.
namespace { // 32비트 덧셈과 비정렬 raw 저장만 호스트 연산에서 분리한다.
// Native ADD의 하위 DWORD를 signed32로 해석하며 C++ signed overflow를 실행하지 않는다.
std::int32_t Add(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)+std::bit_cast<std::uint32_t>(second));
}
// 현재 Renderer의 정수 픽셀/단정도 칸 크기로 나누고 원문 FSTP32 한 번만 수행한다.
std::array<float,2> Center(const RawCameraViewState& view,std::int32_t x,std::int32_t y) {
    return {static_cast<float>(static_cast<double>(Add(x,view.pixels[0]))/std::bit_cast<float>(view.cellFloatBits[0])),
            static_cast<float>(static_cast<double>(Add(y,view.pixels[1]))/std::bit_cast<float>(view.cellFloatBits[1]))};
}
// 같은 raw 필드의 물리를 확인한 뒤 낮은 지정 BYTE부터 기록한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value,std::size_t width) {
    if (at>raw.size() || width>raw.size()-at) throw std::out_of_range("메타 투영의 실제 raw 저장 범위 부족");
    // WORD 분수 또는 DWORD 상위 배율/클리핑 필드만 바꾸고 인접 바이트는 보존한다.
    for (std::size_t byte=0;byte<width;++byte) raw[at+byte]=static_cast<std::uint8_t>(value>>(8*byte));
}
}
// 보호PE00501b18의 double 각도를 사용하며 x87 FSIN/FCOS의 저장 위치에 맞춰 float로 축소한다.
MetaProjectionCamera CreateMetaProjectionCamera() {
    constexpr double angle=0x1.921fb6p-1; // 원문0.7853981852531433이며 수학 pi/4로 바꾸지 않는다.
    return {static_cast<float>(std::sin(angle)),static_cast<float>(std::cos(angle)),0.0F,0U};
}
// 원문 setter는 높이의 단정도 비트를 보존하며 기울기/애니메이션 개수·화면 갱신을 바꾸지 않는다.
void SetMetaProjectionHeight(MetaProjectionCamera& camera,float height) { camera.height=height; }
// C 디컴파일에 생략된 FPU 계산을 실제00418a00 기계어에서 복원한다.
std::int32_t MetaProjectionDenominator(const MetaProjectionCamera& camera) {
    return camera.height<20.0F ? CameraFtolLow((20.0-static_cast<double>(camera.height))*camera.sine+1.0):1;
}
// 두 경계의32비트 합을 먼저 만들고 signed /2의0방향 절삭을 적용한다.
std::array<float,2> MetaViewportCenter(const MetaProjectionView& view) {
    return Center(view.renderer,Add(view.viewport.left,view.viewport.right)/2,Add(view.viewport.top,view.viewport.bottom)/2);
}
// 화면 중심과 viewport 중심은 Native에서도 별개 함수/지역값이다.
std::array<float,2> MetaScreenCenter(const MetaProjectionView& view) { return Center(view.renderer,view.width/2,view.height/2); }
// 입력 F32에서 뺄셈을 시작하고 실제 FSTP 위치에서만 F32로 저장한다. 클리핑 분모는0/NaN으로 교체하지 않는다.
MetaProjectedPoint ProjectMetaCoordinates(const MetaProjectionCamera& camera,const MetaProjectionView& view,float x,float y,float z) {
    const auto viewport=MetaViewportCenter(view);const auto screen=MetaScreenCenter(view);
    const double dx=static_cast<double>(x)-viewport[0];const double dz=static_cast<double>(z)-camera.height;
    const float dy=static_cast<float>((static_cast<double>(y)-screen[1])+dz*camera.cosine);
    const double depth=dz*camera.sine;MetaProjectedPoint result;
    if (depth<=-1.0) { // 원문 FCOM의 ordered 이하에서만 클리핑한다. NaN은 뒤쪽 나눗셈으로 진행한다.
        result.clipped=1;result.scale=0.0F;result.position={static_cast<float>(static_cast<double>(viewport[0])+dx),static_cast<float>(static_cast<double>(dy)+screen[1])};
    } else {
        const double divisor=depth+1.0;
        result.position={static_cast<float>(dx/divisor+viewport[0]),static_cast<float>(static_cast<double>(dy)/divisor+screen[1])};
        result.scale=static_cast<float>(1.0/divisor);
    }
    return result;
}
// 최초 한 번만 보정하며 복귀형 진단이 상태를 바꿔도 이후 원문 쓰기 순서를 계속 따른다.
void CalibrateMetaProjection(MetaProjectionState& state,const MetaProjectionView& view,const CameraViewAssertion& assertion) {
    if (state.calibrated) return;
    auto temporary=CreateMetaProjectionCamera();SetMetaProjectionHeight(temporary,0.0F);
    const auto projected=ProjectMetaCoordinates(temporary,view,0.0F,0.0F,4.0F);
    if (projected.clipped) { if (!assertion) throw std::logic_error("메타 기준 투영의 실제 clipped 진단 미연결");assertion("!clipped",0x439); }
    state.referenceScale=projected.scale;state.referenceHeight=60.0F;state.calibrated=1;
}
// 먼저 투영/선택 정규화→raw 분수/상위배율을 기록하고 그 뒤 반환용 clipped만 추가 변경한다.
MetaProjectedPoint ProjectMetaObject(std::span<std::uint8_t> raw,const MetaProjectionState& state,const MetaProjectionView& view,const std::uint32_t& screenFlags,float x,float y,float z,bool normalize) {
    auto result=ProjectMetaCoordinates(state.camera,view,x,y,z);
    if (normalize && (screenFlags&0x10U)) result.scale=static_cast<float>(static_cast<double>(result.scale)/state.referenceScale);
    constexpr double fixedScale=65536.0; //00501ad0 float65536을 곱한 뒤 원문 FTOL의 같은 하위DWORD를 두 번 소비한다.
    const auto fixed=std::bit_cast<std::uint32_t>(CameraFtolLow(static_cast<double>(result.scale)*fixedScale));
    Put(raw,8,fixed,2);Put(raw,0x1a,((fixed>>16)&0x3fffU)|(result.clipped ? 0x4000U:0U),4);
    if ((result.scale<0.9 || result.scale>1.1) && result.scale!=0.0F) result.clipped=1; // 이 후행 판정은 이미 쓴 raw4000h를 다시 수정하지 않는다.
    return result;
}
}
