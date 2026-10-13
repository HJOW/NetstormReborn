// 기존 보호 SHP/plain RLE 캡처를 실제 Burning 프레임에 재사용한다.
#include "client/BurningFrameCapture.h"
#include <stdexcept>

namespace netstorm::client {
// 처리기와 월드의 동일성을 확인하고 실제 현재프레임 캡처 호출을 전달한다.
o::BurningFrameHooks MakeBurningFrameCapture(const RawShapeToBuffer& capture,const o::SidPool& pool,o::BurningFrameHooks hooks) {
    if (&capture.Pool()!=&pool) throw std::invalid_argument("Burning frame/ShapeToBuffer SID 풀 불일치");
    hooks.capture=[&capture](o::Sid sid) { return capture.Capture(sid); };return hooks;
}
}
