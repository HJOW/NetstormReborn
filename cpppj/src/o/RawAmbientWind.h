// Client004dd330의 공유 바람과 기존 게임 난수/각도 부모를 연결한다.
#pragma once
#include "o/RawBoardAngle.h"
#include "o/SpStore.h"
namespace netstorm::o { // 같은 Client 수명과 현재 입자가 사용하는 원문 전역이다.
// PATCH005e18a8/5435b0/5e18b0와 CD0051fbf8/fbfc/fc00의 세 float다.
struct AmbientWindState {
    float x{}; // X 바람은 실제 PE BSS0이며 연소 입자의 가속에 사용한다.
    float y{1.0F}; // Y 바람의 실제 PE 최초값1이며 각도 계산 전에 매번 감쇠한다.
    float angle{}; // 마지막 잡음 분기에서 갱신한 도 각도이며 나머지 프레임에는 보존한다.
};
// 게임 시계 정지와 별개인 Movie 갱신 잠금 없는 프레임마다 호출한다. 시간 배율/재시드/독립 난수를 추가하지 않는다.
void StepAmbientWind(AmbientWindState& state,GameRandom& random,OriginalEdition edition,const BoardAngleDiagnostics& diagnostics={});
}
