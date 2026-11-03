// 원문 fmod 뒤 float 저장/현재 checking/진단 복귀와 판본별 반환을 유지한다.
#include "o/RawBoardAngle.h"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace netstorm::o { // Boardcoord의 세 부모와 실제 방향 helper가 공유하는 원문 부품이다.
// PATCH의 x87 53비트 저장 계약과 CD의 binary32 중간 저장을 구별한다.
double NormalizeBoardAngle(float angle,OriginalEdition edition,const BoardAngleDiagnostics& diagnostics,double modulus) {
    if (angle==0.0F) return 0.0; // 실제 두 CRT의0입력 꼬리는 부호와 무관하게 fldz로+0을 반환한다.
    const double remainder=std::fmod(static_cast<double>(angle),modulus);
    if (edition==OriginalEdition::Cd1072) {
        float stored=static_cast<float>(remainder);
        if (remainder<0.0) stored=stored+360.0F;
        if (std::bit_cast<std::int32_t>(stored)>0x43b3ffff) stored=359.9999F;
        return stored;
    }
    const double result=remainder<0.0 ? remainder+360.0 : remainder;
    if (result>=360.0) {
        if (diagnostics.checking && result>=361.0) {
            if (!diagnostics.assertion) throw std::logic_error("Boardcoord 각도 원문 진단 장치 미연결");
            diagnostics.assertion("angle < 361.0f","\\Ns\\O\\Boardcoord.cpp",0xb3);
        }
        return 359.9999F; // 실제 PATCH005022a0의 float 비트43b3fffd를 반환한다.
    }
    return result;
}
}
