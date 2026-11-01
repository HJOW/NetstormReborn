// 원본 파일을 바꾸지 않고 사용자 요청의 표시 버전/추가 Credits를 현재 UI에 제공한다.
#pragma once
#include "client/ProductVersion.h"
#include "o/ConfigInterface.h"
#include <memory>
#include <string>
#include <string_view>

namespace netstorm::client { // 원본 기반 UI와 제품 고유 표기를 연결하는 경계다.
inline constexpr std::string_view kProductCreditsObject="Patch1101"; // 원문 Tell의 등록 객체 조회를 그대로 사용하는 전용 이름이다.
inline constexpr std::string_view kProductCreditsTarget="Patch1101.Credits"; // 추가 Credits 항목이 실제 입력 큐에 전달할 대상이다.
// 현재 제품의 세 표시 키를 설정한다. 원본 파일/OriginalEdition/패킷 형식은 변경하지 않는다.
void ApplyProductVersion(o::ConfigInterface& config);
// 기존 Config 등록/섹션 수명을 사용하는 제품 Credits 객체를 만든다. 호출자가 Client UI 수명 동안 소유한다.
std::unique_ptr<o::Config> MakeProductCreditsConfig(o::ConfigRegistry& registry);
// 메인 Credits 목록에 세 번째 항목만 덧붙이며 다른 Tell/원문 Credits 내용은 보존한다.
std::string ProductTellBody(std::string_view target,std::string_view original);
}
