// Boardcoord0041cdb0/CD0043f4e0의 현재 진단과 판본별 끝각 처리를 공유한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 실제 방향 계산과 각도 상태가 같은 현재 Client 진단을 소비한다.
// 자체 checking 사본 대신 같은 owner 뷰와 원문 식/파일/행의 장치를 전달한다.
struct BoardAngleDiagnostics {
    RawCheckingFlag checking{}; //005e4794의 현재 전체 DWORD다. CD는 읽지 않는다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 선택한 진단이 복귀하면 원문은 반드시 현재 끝각 상수를 반환한다.
};
// 원문은 float 인자를 CRT fmod로 계산한 뒤 두 판본 모두359.9999로 끝각을 처리하되 중간 float 저장이 다르다.
// modulus는 원문의 별도 double 전역이며 일반 소비자는 실제360을 사용한다.
double NormalizeBoardAngle(float angle,OriginalEdition edition,const BoardAngleDiagnostics& diagnostics={},double modulus=360.0);
}
