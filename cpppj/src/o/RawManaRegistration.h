// 마나 공급원 목록의 현재 checking·중복 진단 복귀·생산 카운터를 복원한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 같은 원문 목록/카운터와 외부 진단 수명만 빌린다.
struct SquidPostPopList; // 기존 현재 포인터/용량/개수 뷰이며 별도 목록을 만들지 않는다.
// 같은 Client의 현재 checking과 선택 진단 수명을 빌리는 공급원 등록 전용 경계다.
struct ManaRegistrationDiagnostics {
    RawCheckingFlag checking{}; // 패치의 현재 전체 DWORD이며 CD는 읽지 않는다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 중복 원문 진단 후 정상 복귀하거나 선택 장치 실패를 전달한다.
};
// 00472f70/CD0048a360: 중복 여부는 저장하고, 진단 복귀 뒤 현재 생산 카운터를 한 번 감김 증가한다.
// value는 원문 DWORD 전체이며 실제 postPop caller만 유효 SID 번호를 전달한다.
void RegisterManaProvider(OriginalEdition edition,SquidPostPopList& list,std::uint32_t& productionDirty,
    std::uint32_t value,const ManaRegistrationDiagnostics& diagnostics);
}
