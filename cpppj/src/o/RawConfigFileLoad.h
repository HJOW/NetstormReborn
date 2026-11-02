// 원문40380의 전체읽기·조건XOR·NUL·Config 복귀 뒤 할당 반납을 복원한다.
#pragma once
#include "o/RawBaseFileWholeRead.h"

namespace netstorm::o { // 값형 Config reader와 구별한 실제 파일 부모다.
struct ConfigFileLoadBodyHooks { // 전체읽기가 넘긴 같은 할당을 Append 복귀까지 빌린다.
    std::function<std::span<std::uint8_t>(std::uintptr_t)> buffer; // 최소4/추가1의 실제 할당 물리이며 미작성 바이트도 지우지 않는다.
    std::function<void(std::uintptr_t,std::span<const std::uint8_t>)> append; // NUL을 쓴 같은 원문 바이트를 Config에 더한다. 앞NUL은 C문자열 끝이다.
    std::function<void(std::uintptr_t)> release; // Append 정상 복귀 뒤 고정 지역 포인터를 free한다. Append throw에는 호출하지 않는다.
};
void LoadConfigFile(RawBaseFileState& file,std::span<const std::uint8_t> name,const RawBaseFileIOHooks& io,const BaseFileWholeReadHooks& whole,const ConfigFileLoadBodyHooks& body,std::span<std::uint8_t> scratch={}); //40380처럼 length0/check1/extra1로41baa0을 부른 뒤 실제0/2BYTE 조건과 소유 순서를 적용한다.
}
