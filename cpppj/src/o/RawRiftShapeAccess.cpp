// 실제00419850의 낮은 DWORD 감김과 진단 뒤 현재 SHP 접근을 구현한다.
#include "o/RawRiftShapeAccess.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 호스트 포인터를 원본 주소로 호출하지 않고 선택 TYPE 토큰을 보관한다.
namespace { // 선택한 callback이 실행 중 교체돼도 해당 호출의 수명을 보존한다.
template<class F> F ShapeNeed(const F& function) { if (!function) throw std::logic_error("SHP접근의선택된실제장치미연결");return function; } // 도달하지 않은 장치는 요구하지 않는다.
constexpr std::uint32_t kShapeDoubleFrames=0x440000U; //원문TYPE+e8의shadow/abstract두배프레임마스크다.
constexpr std::int32_t kShapeFreed=-0x22222223,kShapeUninitialized=-0x32323233; //원문TYPE첫DWORD의두CRT표식이다.
}
std::array<float,2> ReadRiftShapeFrame(RawSquidDisplayTypeView type,std::int32_t frame,std::uint32_t abstract,const RiftShapeAccessHooks& hooks) { // 부모가 저장한 TYPE/frame을 진단 이후에도 유지한다.
    const auto count=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(type.frameCount)*((type.flags1&kShapeDoubleFrames) ? 2U : 1U));
    const bool range=frame>=0 && frame<count;
    if (!range) { const auto number=ShapeNeed(hooks.typeNumber)(type);ShapeNeed(hooks.frameError)(number,frame,abstract); }
    if (!range || !type.loaded || type.header==kShapeFreed || type.header==kShapeUninitialized)
        ShapeNeed(hooks.assertion)("frameCheck( abstract, frame, false )",R"(..\o\rifttype.h)",0xe1);
    return ShapeNeed(hooks.shape)(type.token,frame); // 실제 물리 범위 밖은 이 소비 지점의 호스트 오류로 남긴다.
}
}
