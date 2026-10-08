// 복원된 프레임 지정의 외부 훅을 실제 SHP 표시·일반 Pop/Unpop 모듈로 연결한다.
#pragma once
#include "o/SquidFrame.h"
#include "o/SquidDisplay.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"

namespace netstorm::o {
// SHP 목록은 SquidRenderer::Shapes가 공급한다. 참조/타입 표는 반환한 콜백보다 오래 살아야 한다.
// Pop/Unpop은 같은 해시/spot을 사용해야 하고 둘 다 같은 display를 연결해야 한다.
SquidFrameHooks MakeSquidFrameHooks(SidPool& pool,std::span<const RiftTypeRecord> types,
    SquidDisplay& display,SquidUnpop& unpop,SquidPop& pop);
}
