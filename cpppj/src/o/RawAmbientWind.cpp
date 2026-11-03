//004dd2b0/CD004503f0의 감쇠 두 저장→100난수→잡음 난수→X 저장→현재 각도 저장을 유지한다.
#include "o/RawAmbientWind.h"
#include <cmath>
namespace netstorm::o { // 입자와 Client의 동일 장기 바람을 직접 갱신한다.
namespace { // 보호 PE의 double/float 상수이며 delta에 따라 바꾸지 않는다.
constexpr double kDamping=0.9,kDegreesPerRadian=57.295827908797776; // 실제 double 감쇠/각도 환산이다.
constexpr float kNoiseMinimum=-0.06F,kNoiseMaximum=0.06F; // 두 판본의 bd75c28f/3d75c28f 입력이다.
}
// 두 판본의 잡음 곱셈 순서를 구별하고 잡음 없는 경우 기존 angle과 두 번째 난수를 보존한다.
void StepAmbientWind(AmbientWindState& state,GameRandom& random,OriginalEdition edition,const BoardAngleDiagnostics& diagnostics) {
    state.x=static_cast<float>(static_cast<double>(state.x)*kDamping);
    state.y=static_cast<float>(static_cast<double>(state.y)*kDamping);
    if (random.Next(100)>=10) return;
    const auto value=random.Next(65536);const double width=std::abs(static_cast<double>(kNoiseMaximum)-kNoiseMinimum);
    const double offset=edition==OriginalEdition::Patch1078 ? static_cast<double>(value)*(1.0/65536.0)*width+kNoiseMinimum : width*value*(1.0/65536.0)+kNoiseMinimum;
    state.x=static_cast<float>(offset+state.x);
    const float degrees=static_cast<float>(std::atan2(-static_cast<double>(state.y),static_cast<double>(state.x))*kDegreesPerRadian);
    state.angle=static_cast<float>(NormalizeBoardAngle(degrees,edition,diagnostics));
}
}
