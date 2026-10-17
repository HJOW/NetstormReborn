// 보상 내용물 제거 뒤 현재 권한·모드와 지역0의 원본 보충 범위를 계산한다.
#pragma once
#include "o/RawPlayerGeyserBounds.h"

namespace netstorm::o {
// 호출 도중 진단 효과가 바꾸는 전역도 다음 조회에서 직접 읽는다.
struct NuggetReplenishState {
    const bool& boss; //00540bc4의 현재 생성 권한이다.
    const bool& single; //00594fa4의 현재 단독 미션 모드다.
    const bool& fort; //00594fb8의 현재 요새 모드다.
    const std::uint32_t& editor; //005c85a4의 현재 편집 모드다.
    const std::int32_t& archipelagoSpan; //00542654의 현재 지역 확장 단위다.
};
// 원본 진단과 비미션의 실제 무작위 배치는 별도의 큰 하위다.
struct NuggetReplenishHooks {
    std::function<void(std::uint32_t)> assertion; //Nugget.cpp의246 진단 후 현재 boss를 재독한다.
    std::function<void(FortRandomRectangle,std::uint32_t)> place; //0040e050의 사각형 네 float와 수량1이다.
};
// 00485e40 전체를 수행한다. 단독 미션은 원본 모드 관문에서 자연 반환한다.
class RawNuggetReplenish {
public:
    RawNuggetReplenish(const RawPlayerGeyserBounds& bounds,NuggetReplenishState state,NuggetReplenishHooks hooks); //같은 월드의 지도·패턴과 현재 전역을 빌린다.
    void Run() const; //권한 재확인→모드→현재 지역0→원점 배치→확장→실제 배치 순서다.
private:
    const RawPlayerGeyserBounds& bounds_; //기존 간헐천과 같은 실제 ChunkMap/Canon/TYPE를 사용한다.
    NuggetReplenishState state_; //호출 중에도 바뀔 수 있는 현재 전역 참조다.
    NuggetReplenishHooks hooks_; //선택된 큰 하위만 요구하며 없는 배치를 성공 처리하지 않는다.
};
}
