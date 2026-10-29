// 10.78 CRT의 __dosmaperr가 쓰는 Win32→errno 선택을 파일 장치에서 공유한다.
#pragma once
#include <cstdint>

namespace netstorm::o { // 원문 오류 표는 현대 CRT의 변경 가능성과 분리한다.
// 원문의 첫 일치 항목→구간→EINVAL 순서다. 호출자는 원문처럼 비영 OS 오류에서만 사용한다.
std::int32_t Win32FileErrorCode(std::uint32_t error);
}
