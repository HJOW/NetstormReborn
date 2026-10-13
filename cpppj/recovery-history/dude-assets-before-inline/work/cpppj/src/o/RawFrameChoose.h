// 초기 지형과 지역 재도장이 사용하는 원본 후보 선택을 하나의 몸체로 공유한다.
#pragma once
#include "o/RiftType.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 원본0049aa90의 번호wildcard(-1) 요청을 선택한다. theme=-1은 전체 후보,0..3은 원소quartile이다.
// frames/code/random은 현재 같은 월드 자료이며 후보 실패는-1을 유지한다.
std::int32_t ChooseTerrainFrame(const RiftTypeFrames& frames,FrameCode code,std::int32_t theme,GameRandom& random);
}
