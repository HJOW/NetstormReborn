// 원문 Winutil.cpp004dd580의 세 Win32 호출과 선택 진단 순서다.
#pragma once
#include <cstdint>
#include <functional>
#include <string_view>

namespace netstorm::client { // OS 핸들 소유와 원문 매핑 부모를 분리한다.
struct ReadOnlyMapSecurity { // 원문32비트 SECURITY_ATTRIBUTES의 논리 필드다. 호스트 ABI 크기로 사용하지 않는다.
    std::uint32_t length{12}; // 원문 구조체 크기다. 실제64비트 어댑터는 자신의 sizeof를 사용한다.
    std::uintptr_t descriptor{}; // NULL 보안 기술자다.
    std::uint32_t inherit{}; // 원문 FALSE 상속 플래그다.
};
struct ReadOnlyFileMapHooks { // 원문에 없는 Close/캐시/추출 성공을 자동 합성하지 않는다.
    std::function<std::uintptr_t(std::string_view,std::uint32_t,std::uint32_t,ReadOnlyMapSecurity,std::uint32_t,std::uint32_t,std::uintptr_t)> createFile; // 이름/access/share/security/OPEN_EXISTING/attributes/template이다.
    std::function<std::uintptr_t(std::uintptr_t,std::uintptr_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uintptr_t)> createMapping; // file/NULL보안/PAGE_READONLY/크기상위·하위/NULL이름이다.
    std::function<std::uintptr_t(std::uintptr_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uintptr_t)> mapView; // mapping/FILE_MAP_READ/오프셋상위·하위/전체길이0이다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // Winutil.cpp의17e/181/184 진단이며 복귀하면 다음 호출로 진행한다.
};
// invalidFile은 원문의0xffffffff 또는 실제호스트 INVALID_HANDLE_VALUE다. 유효한 토큰을32비트로 절단하지 않는다.
std::uintptr_t MapReadOnlyFile(std::string_view name,std::uintptr_t invalidFile,const ReadOnlyFileMapHooks& hooks);
}
