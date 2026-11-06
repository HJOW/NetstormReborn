// 신전 missingIsland의 원문 상세 문자열과 진단을 같은 Client 장치에 전달한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 원문 진단 직전의 현재 객체와 IslandList count를 사용한다.
struct VortexOwnerDiagnosticHooks { // 상세 기록 뒤 실제 진단을 호출하며 정상 복귀를 허용한다.
    std::function<void(std::string_view)> detail; // 원문00436d20/CD004875b0의 현재 Client 상세 저장 장치다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 식/파일/행과 정상 복귀를 전달한다.
};
// 현재 위치의 ftol 하위DWORD·현재 owner BYTE와 요청 DWORD를 원문 형식으로 기록한다.
void ReportVortexOwnerMissingIsland(const SidPool& pool,Sid sid,std::int32_t island,std::uint32_t owner,
    std::uint32_t count,const VortexOwnerDiagnosticHooks& hooks);
}
