// 원본4794b0의 현재 프레임 선저장·현재 TYPE+114·signed IDIV·실제 Frame.Set을 복원한다.
#pragma once
#include "o/SquidFrame.h"

namespace netstorm::o { //실제 장기/Session이 같은 현재 풀/TYPE/프레임 지정기를 공유한다.
float RunSpriteRegular(const SidPool& pool,std::span<const RiftTypeRecord> types,const SquidFrame& setter,Sid parent,float payload); //event/count와 무관하며 지정 뒤 원래payload 비트를 반환한다.
}
