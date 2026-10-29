// 원본 설정 바이트를 보존하면서 현대 Windows의 유니코드 설치 경로를 저장한다.
#pragma once
#include "o/ConfigInterface.h"

namespace netstorm::client { // 호환 저장은 Client에만 적용하며 원문 ConfigInterface의 엄격한 인코딩 API를 바꾸지 않는다.
std::vector<std::uint8_t> EncodeClientOptions(std::string_view text); // 원문 CP1252가 가능하면 같은 XOR 바이트, 불가능한 유효UTF-8은 서명 뒤 BOM으로 구분한다.
void SaveClientOptions(o::ConfigInterface& config,const o::ConfigInterface::FileWriter& writer); // d/options.cfg의 정상/실패 changed 수명을 원문 SaveFile과 동일하게 유지한다.
}
