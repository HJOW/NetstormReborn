// R 생성자의 float 저장 위치와 실제 초기화 범위를 보존한다.
#include "root/ChallengeState.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::root { // 클라이언트의 다른 배열과 확장 정밀도 수명으로 대체하지 않는다.
namespace { // R PE 상수와 최초 계산만 공유한다.
constexpr double kAngle=std::bit_cast<double>(std::uint64_t{0x3fe921f9f01b866e}); //004bdac0의 실제 double 각도다.
constexpr float kStep=std::bit_cast<float>(std::uint32_t{0x3f32b8b8}); //0044cc5d의 실제 float 각도 증가량이다.
constexpr float kSpacing=8.0f,kXOffset=13.0f; // 원문 지역8과 PE004bdab8의13이다.
// 원문 R은 반경/중심/고리 반경/누적 각도를 각각 float32로 저장한다.
void InitializeGeometry(ChallengeGeometry& geometry) {
    if (geometry.initialized) return;
    const auto root=static_cast<float>(std::sqrt(20.0L));const auto side=static_cast<std::int32_t>(std::floor(root)); // sqrt 결과의 FST32→floor→FTOL 순서다.
    const auto radius=static_cast<float>((static_cast<long double>(side)*kSpacing)/(2.0L*std::cos(static_cast<long double>(kAngle))));
    const auto center=static_cast<float>(44.0L+radius),ringRadius=static_cast<float>(30.0L+radius); //0044cb46/5f의 별도 float 지역 저장이다.
    std::size_t gridIndex=1;
    while (gridIndex<geometry.grid.size()) { // 같은4×4 행/열을 반복해1..39를 채우며0번은 덮지 않는다.
        for (std::int32_t row=0;row<side && gridIndex<geometry.grid.size();++row) { // 다음 사각 순회에서는 행을0부터 다시 읽는다.
            for (std::int32_t column=0;column<side && gridIndex<geometry.grid.size();++column) { // 좌표의 홀짝은 현재 저장 번호를 사용한다.
                const auto half=static_cast<long double>(side)*kSpacing/2.0L;
                geometry.grid[gridIndex][0]=static_cast<float>(static_cast<long double>(column)*kSpacing+center-half+kXOffset);
                geometry.grid[gridIndex][1]=static_cast<float>(static_cast<long double>(row)*kSpacing+center-half-((gridIndex&1U) ? 0:2));++gridIndex;
            }
        }
    }
    float angle{};
    for (std::size_t index=1;index<geometry.ring.size();++index) { // 원문은 각 점 작성 뒤 다음 각도를 float32에 다시 저장한다.
        geometry.ring[index][0]=static_cast<float>(std::cos(static_cast<long double>(angle))*ringRadius+center+kXOffset);
        geometry.ring[index][1]=static_cast<float>(std::sin(static_cast<long double>(angle))*ringRadius+center);angle=static_cast<float>(static_cast<long double>(angle)+kStep);
    }
    for (std::size_t index=0;index<geometry.radii.size();++index) geometry.radii[index]=static_cast<float>(4.0L+static_cast<long double>(1-index)*8.0L+radius); //0044ccf9..21은 두 반경을 별도 float로 쓴다.
    geometry.origin={static_cast<float>(static_cast<long double>(center)+kXOffset),center};geometry.width=static_cast<std::int32_t>(static_cast<long double>(center)*2.0L+26.0L);geometry.height=static_cast<std::int32_t>(static_cast<long double>(center)*2.0L);geometry.initialized=1;
}
}
void ResetChallengeState(std::span<std::uint8_t> bytes) {
    if (bytes.size()!=kRootChallengeBytes) throw std::invalid_argument("R CCh 초기화의 실제2b80BYTE 필요");
    std::fill_n(bytes.begin(),8,std::uint8_t{});std::fill_n(bytes.begin()+kChallengeParticipantOffset,kChallengeParticipantCount*kChallengeParticipantBytes,std::uint8_t{});
    std::fill_n(bytes.begin()+kChallengeBattleOffset,kChallengeBattleCount*kChallengeBattleBytes,std::uint8_t{});std::fill_n(bytes.begin()+kChallengeConnectionOffset,kChallengeParticipantCount*4,std::uint8_t{});std::fill_n(bytes.begin()+kChallengeCounterOffset,4,std::uint8_t{});
}
void ConstructChallengeState(std::span<std::uint8_t> bytes,ChallengeGeometry& geometry) { ResetChallengeState(bytes);InitializeGeometry(geometry); } // 자식 header/고정값/10BYTE 초기화는 뒤의 Reset에 모두 덮이며 외부 효과가 없다.
static_assert(kChallengeBattleOffset+kChallengeBattleCount*kChallengeBattleBytes==kChallengeParticipantOffset && kChallengeParticipantOffset+kChallengeParticipantCount*kChallengeParticipantBytes==kChallengeConnectionOffset && kChallengeConnectionOffset+kChallengeParticipantCount*4==kChallengeCounterOffset && kChallengeCounterOffset+8==kRootChallengeBytes); // 원문 세 배열/카운터와 미작성 끝DWORD의 실제 접합이다.
}
