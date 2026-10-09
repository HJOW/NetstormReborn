// 특수 타입의 선행 비교 순서와 번호별 72바이트 패턴을 보존한다.
#include "o/CanonTypeDecoder.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
#include "o/CanonTypePatterns.inc"
}
// 표 범위 밖 접근은 원본의 잘못된 메모리 조회 대신 명시적인 예외로 보고한다.
std::span<const CanonPattern> CanonSpecialPatterns(std::size_t group) {
    if (group==0) return kIslandPatterns;
    if (group==1) return kSupportPatterns;
    if (group==2) return kBattlePatterns;
    throw std::out_of_range("CanonDecoder 특수 패턴 그룹 오류");
}
// 실제 생성자의 타입 선택은 explicitFrame보다 앞서므로 패턴 타입에서는 플래그를 무시한다.
CanonDecoder DecodeCanonType(const RiftTypeFrames& frames,int defaultFrame,std::span<const std::uint32_t,8> patternTypes,
    CanonTypeQuery query,OriginalEdition edition) {
    if (!std::isfinite(query.x) || !std::isfinite(query.y)) throw std::invalid_argument("CanonDecoder 좌표 오류");
    if (edition==OriginalEdition::Cd1072 && (query.direction&1)) throw std::out_of_range("CD CanonDecoder 홀수 방향 오류");
    std::span<const CanonPattern> patterns;
    if (query.type==patternTypes[0]) patterns=TerritoryPatterns();
    else if (query.type==patternTypes[1]) patterns=BridgePatterns(edition);
    else if (query.type==patternTypes[2]) patterns=CanonSpecialPatterns(0);
    else if (query.type==patternTypes[3]) patterns=CanonSpecialPatterns(1);
    else if (std::find(patternTypes.begin()+4,patternTypes.end(),query.type)!=patternTypes.end()) patterns=CanonSpecialPatterns(2);
    else return CanonDecoder(frames,defaultFrame,query.argument,query.direction,query.x,query.y,query.explicitFrame,edition);
    if (query.argument<0 || static_cast<std::size_t>(query.argument)>=patterns.size()) throw std::out_of_range("CanonDecoder 패턴 번호 오류");
    return CanonDecoder(frames,patterns[static_cast<std::size_t>(query.argument)],query.direction,query.x,query.y);
}
}
