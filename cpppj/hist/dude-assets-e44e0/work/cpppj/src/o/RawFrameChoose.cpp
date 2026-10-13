// 이미 대조한 후보 검색·폴백·게임 난수 소비 몸체를 초기 지형과 지역 도장이 공유한다.
#include "o/RawFrameChoose.h"
#include "o/TerrainBuilder.h"
#include <stdexcept>

namespace netstorm::o {
// 기존 RawRegionPaint::Choose의 본문을 보존하며 난수 참조만 함수 인자로 받는다.
std::int32_t ChooseTerrainFrame(const RiftTypeFrames& frames,FrameCode code,std::int32_t theme,GameRandom& random) {
    // 기존 몸체에서 사용한 난수 참조 이름을 유지하여 토큰 보존을 직접 감사한다.
    auto& random_=random;
    std::vector<std::int32_t> candidates;
    // 각 검색은 프레임 코드 저장 순서대로 최대256개를 채운다.
    const auto collect=[&](std::uint8_t side,std::optional<std::uint8_t> variant) {
        candidates.clear();const auto codes=frames.Codes();
        // 후보 수가 한계에 도달해도 원본 코드 배열 순회 의미는 동일하다.
        for (std::size_t i=0;i<codes.size();++i)
            if (codes[i].side==side && (!variant || codes[i].variant==*variant) && candidates.size()<256)
                candidates.push_back(static_cast<std::int32_t>(i));
    };
    collect(code.side,code.variant);
    if (candidates.empty()) { const auto normalized=TerrainBuilder::Normalize(static_cast<char>(code.side),static_cast<char>(code.variant));collect(static_cast<std::uint8_t>(normalized.first),static_cast<std::uint8_t>(normalized.second)); }
    if (candidates.empty()) collect(code.side,static_cast<std::uint8_t>('A'));
    if (candidates.empty()) collect(code.side,{});
    if (candidates.empty()) return -1;
    if (theme==-1) return candidates[random_.Next(static_cast<std::uint32_t>(candidates.size()))];
    const auto quarter=static_cast<std::int32_t>(candidates.size()/4);
    const auto low=quarter*theme+1,high=quarter*(theme+1);
    if (high<=low) throw std::out_of_range("지역 프레임 quartile 난수 범위 오류");
    const auto index=low+static_cast<std::int32_t>(random_.Next(static_cast<std::uint32_t>(high-low)));
    if (index<0 || static_cast<std::size_t>(index)>=candidates.size()) throw std::out_of_range("지역 프레임 후보 범위 오류");
    return candidates[static_cast<std::size_t>(index)];
}
}
