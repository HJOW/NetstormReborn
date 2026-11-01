// 제품 표시 버전과 Windows 버전 리소스가 같은 값을 사용한다. 원본 분석 판본과는 별개다.
#ifndef NETSTORM_PRODUCT_VERSION_H
#define NETSTORM_PRODUCT_VERSION_H // 리소스/C++가 함께 읽는 버전 헤더의 중복 정의를 막는다.
// 사용자 지시에 따른 새 제품의 정수 주 버전이다.
#define NETSTORM_PRODUCT_VERSION_MAJOR 11
// 표기11.01의 정수 부 버전이며 내부 원본 데이터 판본 번호를 변경하지 않는다.
#define NETSTORM_PRODUCT_VERSION_MINOR 1
// 선행0이 필요한 제품 표시 문자열이다.
#define NETSTORM_PRODUCT_VERSION_TEXT "11.01"

#if !defined(RC_INVOKED)
#include <string_view>
namespace netstorm::client { // GUI/CLI가 공유하는 제품 버전 상수다.
inline constexpr int kProductMajorVersion=NETSTORM_PRODUCT_VERSION_MAJOR; // 현재 제품의 설정 주 버전이다.
inline constexpr int kProductMinorVersion=NETSTORM_PRODUCT_VERSION_MINOR; // 현재 제품의 설정 부 버전이다.
inline constexpr std::string_view kProductVersion=NETSTORM_PRODUCT_VERSION_TEXT; // GUI/CLI/리소스의 표기11.01을 유지한다.
}
#endif

#endif
