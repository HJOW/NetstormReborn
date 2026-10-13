// o/Burning 프레임의 캡처 경계를 실제 client ShapeToBuffer 몸체에 연결한다.
#pragma once
#include "client/RawShapeToBuffer.h"
#include "o/RawBurningFrame.h"

namespace netstorm::client {
// 같은 SID 풀의 실제 캡처만 설치하고 호출자가 준 nonzero 소리 소비자는 유지한다.
o::BurningFrameHooks MakeBurningFrameCapture(const RawShapeToBuffer& capture,const o::SidPool& pool,o::BurningFrameHooks hooks={});
}
