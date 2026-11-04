// SHP 프레임의 범위/해제 표식·상세 오류·진단 복귀 뒤 물리 접근 순서를 보존한다.
#pragma once
#include "o/RawSquidDisplayParent.h"
#include <array>

namespace netstorm::o { // 현재 TYPE 참조와 저장된 frame/abstract 인자를 구별한다.
struct RiftShapeAccessHooks { // 도달한 상세·진단·현재 SHP 장치만 요구한다.
    std::function<std::uint32_t(RawSquidDisplayTypeView)> typeNumber; //0049adc0의 선택된 TYPE 번호다.
    std::function<void(std::uint32_t,std::int32_t,std::uint32_t)> frameError; //00436d40의 현재 상세 owner에 세 저장 인자를 전달한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //원문rifttype.h:e1이며 checking과무관하다.
    std::function<std::array<float,2>(std::uint32_t,std::int32_t)> shape; //진단복귀뒤선택된TYPE의현재SHP에서저장frame을읽는다.
};
// 원문00419850은 범위를 진단한 뒤 같은 frame으로 계속하며 SHP/TYPE을 다른 정상 입력으로 보정하지 않는다.
std::array<float,2> ReadRiftShapeFrame(RawSquidDisplayTypeView type,std::int32_t frame,std::uint32_t abstract,const RiftShapeAccessHooks& hooks);
}
