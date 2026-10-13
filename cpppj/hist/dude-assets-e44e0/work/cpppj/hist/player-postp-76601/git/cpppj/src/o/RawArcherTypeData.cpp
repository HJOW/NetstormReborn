// 실제 타입 callback의 directions 쓰기와 클러스터 수를 기존 정의 파서에서 읽는다.
#include "o/RawArcherTypeData.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
// 실제 callback은 숫자를 float AST에서 읽고 CRT가0쪽으로 절삭한다. 누락 시 초기0을 보존한다.
ArcherTypeData ReadRawArcherTypeData(const RiftTypeDefinition& definition) {
    // 원본은 orientations가 명시된 경우 숫자 AST만 허용한다.
    if (definition.Property("orientations") && !definition.Number("orientations"))
        throw std::invalid_argument("Archer directions 숫자 AST 원본 계약 오류");
    // 누락된 방향수는 원본 초기0이며 숫자는 원본 AST의 float폭으로 읽는다.
    const auto property=definition.Number("orientations");
    const auto value=property ? static_cast<float>(*property) : 0.0f;
    if (!std::isfinite(value) || static_cast<double>(value)<std::numeric_limits<std::int32_t>::min() ||
        static_cast<double>(value)>std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("Archer 방향수 CRT 정상 정수 범위 오류");
    if (definition.clusters.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::out_of_range("Archer 물리프레임 정수 범위 오류");
    return {static_cast<std::int32_t>(value),static_cast<std::int32_t>(definition.clusters.size())};
}
}
