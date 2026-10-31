// 비로컬 의식 보상의 SP 덧셈과 별도 Core 장부 쓰기를 원문 순서대로 실행한다.
#pragma once
#include "o/RawPlayerCoreRecords.h"
#include <functional>

// 원본 공용 게임 규칙이며 UI나 별도 Player 기술 목록을 소유하지 않는다.
namespace netstorm::o {
//004482b0: getter 결과에 5000을 더하고 기본 x87 53비트 정밀도로 절삭한 EAX를 정책 setter에 넘긴다.
void GrantForeignRitualSp(std::uint32_t owner,const std::function<float(std::uint32_t)>& balance,
    const std::function<void(std::int32_t,std::int32_t)>& set);
//0048f380→0042b280: TYPE 기술 비트가 -1이면 쓰지 않고 그 밖에는 별도 owner Core+6의 비트를 켠다.
// 범위 진단 뒤 정상 복귀하면 마스크 shift를 계속하며 논리 owner 상한을 새로 추가하지 않는다.
void GrantForeignRitualType(PlayerCoreRecordsView records,std::uint32_t owner,std::uint32_t type,
    const std::function<std::int32_t(std::uint32_t)>& technologyBit,const std::function<void(std::uint32_t)>& assertion);
}
