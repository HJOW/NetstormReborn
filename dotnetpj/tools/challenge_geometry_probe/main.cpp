// 실제 CPP 순수 생성자를 호출해 모든 저장된 float 비트를 외부 파일에 남긴다.
#include "o/RawChallengeState.h"
#include <bit>
#include <cstdint>
#include <iostream>
#include <vector>

// 값의 변환이나 반올림 없이 실제 저장된 float 비트를 출력한다.
void Float(const char* key,float value) { std::cout<<key<<'\t'<<std::bit_cast<std::uint32_t>(value)<<'\n'; }
// 실제 CPP의 생성/재사용 결과만 읽으며 창이나 원본 게임은 실행하지 않는다.
int main() {
    netstorm::o::ChallengeGeometry geometry;
    std::vector<std::uint8_t> bytes(netstorm::o::kChallengeBytes,0xcc);
    netstorm::o::ConstructChallengeState(bytes,geometry);
    std::cout<<"initialized\t"<<geometry.initialized<<"\nwidth\t"<<geometry.width<<"\nheight\t"<<geometry.height<<'\n';
    Float("firstRadius",geometry.firstRadius);Float("secondRadius",geometry.secondRadius);Float("originX",geometry.originX);Float("originY",geometry.originY);
    // 실제 저장된 ring/grid 순서로 두 성분을 모두 출력한다.
    for (const auto* list:{&geometry.ring[0],&geometry.grid[0]}) {
        const bool ring=list==&geometry.ring[0];const auto size=ring?geometry.ring.size():geometry.grid.size();
        // 입력값의 float 비트를 그대로 저장한다.
        for (std::size_t index=0;index<size;++index) {
            std::cout<<(ring?"ring":"grid")<<index<<"x\t"<<std::bit_cast<std::uint32_t>(list[index][0])<<'\n';
            std::cout<<(ring?"ring":"grid")<<index<<"y\t"<<std::bit_cast<std::uint32_t>(list[index][1])<<'\n';
        }
    }
    // 전체 기록이 초기화됐는지 원문 물리를 직접 관찰한다.
    for (const auto byte:bytes) if (byte) return 2;
    return 0;
}
