// 원문41baa0의 선택적존재조회/열기/길이/추가할당/읽기/출력/성공ctor 수명을 복원한다.
#pragma once
#include "o/RawBaseFileIO.h"
#include <functional>
#include <span>

namespace netstorm::o { // VFS snapshot/직접 OS 읽기 API와 혼합하지 않는 명시적 전체읽기 부모다.
struct BaseFileWholeAllocation { // 원문 operator_new가 반환한 소유 토큰과 현재 접근 가능한 물리를 빌린다.
    std::uintptr_t token{}; // 호출자가 최종 반납할 할당이며0 반환도 원문처럼 별도로 실패 처리하지 않는다.
    std::span<std::uint8_t> bytes; // 최소4/추가폭까지 실제 할당한 물리다. 미작성 BYTE를 본체가0으로 채우지 않는다.
};
struct BaseFileWholeReadHooks { // callback 소유자/입력/할당은 동기 부모 호출보다 오래 살아야 한다.
    std::function<std::uint32_t(std::span<const std::uint8_t>)> probe; // param2가0이 아닐 때만 원문41ab10의 정확한0/비0 출처를 조회한다.
    std::function<void(RawBaseFileState&,std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> open; // same지역File에41b4c0(name,0,1)을 호출한다. 예외가 나면 부모의ctor완료상태에 들어가지 않는다.
    std::function<BaseFileWholeAllocation(std::uint32_t)> allocate; // signed32(length+extra)가4보다 작으면4를 요청한다. 본체는 할당반납/초기화를 합성하지 않는다.
};
std::uintptr_t ReadWholeBaseFile(RawBaseFileState& state,std::span<const std::uint8_t> name,std::uint32_t checkExists,std::int32_t* lengthOut,std::int32_t extra,const RawBaseFileIOHooks& io,const BaseFileWholeReadHooks& hooks,std::span<std::uint8_t> scratch={}); // 처음두key필드만0으로 하고open성공뒤의 File만 정상/예외에서 반납한다. 실제24BYTE/SEH ABI와 부족/음수물리의원문미정쓰기는호스트경계다.
}
