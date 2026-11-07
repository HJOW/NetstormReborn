// 원문 생성자의 덮어쓴 자식 초기화와 같은 최종 바이트·float 저장 위치를 보존한다.
#include "o/RawChallengeState.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o { // CS의 객체 기록과 Client 수명 좌표 초기화를 구별한다.
namespace { // 보호 PE 상수와 원문 순수 수학 계산만 지역화한다.
constexpr double kAngle=std::bit_cast<double>(std::uint64_t{0x3fe921f9f01b866e}); //00504820의 실제double0.7853975다.
constexpr float kStep=std::bit_cast<float>(std::uint32_t{0x3f32b8b8}); //00504810의 실제float 각도 증가량이다.
constexpr float kSpacing=8.0f,kCenterOffset=44.0f,kRingOffset=30.0f,kXOffset=13.0f; //00504828/481c/4818/4814의 float 상수다.
void InitializeGeometry(ChallengeGeometry& geometry) { // 원문은 initialized가 비영이면 좌표0번을 포함해 아무 것도 다시 쓰지 않는다.
    if (geometry.initialized) return;
    const auto side=static_cast<std::int32_t>(std::floor(std::sqrt(20.0))); // FSQRT20→double 저장→004e62a0 floor→FTOL의 실제 상수 입력이다.
    const auto square=static_cast<float>(static_cast<long double>(side)*kSpacing); //42cf70의 FST32를 보존한다.
    const auto radius=(static_cast<long double>(side)*kSpacing)/(2.0L*std::cos(static_cast<long double>(kAngle)));
    const auto center=static_cast<float>(static_cast<long double>(kCenterOffset)+radius); //42cf88의 저장된center를 뒤의 모든 위치 계산이 다시 읽는다.
    std::size_t index=1;
    while (index<geometry.grid.size()) { // 원문은 같은4×4 순회를 반복하면서39번까지 채운다.
        for (std::int32_t row=0;row<side;++row) { // 다음 반복에서도 행/열을0부터 다시 시작한다.
            if (index>=geometry.grid.size()) break;
            for (std::int32_t column=0;column<side && index<geometry.grid.size();++column) { // 각 현재번호의 홀짝을 저장 전에 읽는다.
                const auto half=static_cast<long double>(square)*0.5L;
                geometry.grid[index][0]=static_cast<float>(static_cast<long double>(column)*kSpacing+center-half+kXOffset);
                geometry.grid[index][1]=static_cast<float>(static_cast<long double>(row)*kSpacing+center-half-((index&1U) ? 0 : 2));++index;
            }
        }
    }
    auto angle=0.0L;const auto ringRadius=static_cast<long double>(kRingOffset)+radius;
    for (auto& point:geometry.ring) { // 원문은9번 FST32 뒤에 각도를 계속 증가시키고 마지막 각도는 버린다.
        point[0]=static_cast<float>(std::cos(angle)*ringRadius+center+kXOffset);
        point[1]=static_cast<float>(std::sin(angle)*ringRadius+center);angle+=static_cast<long double>(kStep);
    }
    geometry.originY=center;geometry.firstRadius=static_cast<float>(12.0L+radius);geometry.secondRadius=static_cast<float>(radius+4.0L);
    geometry.originX=static_cast<float>(static_cast<long double>(center)+kXOffset);
    geometry.width=static_cast<std::int32_t>(static_cast<long double>(center)*2.0L+26.0L);geometry.height=static_cast<std::int32_t>(static_cast<long double>(center)*2.0L);
    geometry.initialized=1;
}
}
void ResetChallengeState(std::span<std::uint8_t> bytes) { // logical 유효성 대신 실제 요청21328BYTE 물리만 확인한다.
    if (bytes.size()!=kChallengeBytes) throw std::invalid_argument("CS 초기화의 실제21328BYTE 필요");
    std::fill_n(bytes.begin(),8,std::uint8_t{});
    std::fill_n(bytes.begin()+kChallengeParticipantOffset,kChallengeParticipantCount*kChallengeParticipantBytes,std::uint8_t{});
    std::fill_n(bytes.begin()+kChallengeBattleOffset,kChallengeBattleCount*kChallengeBattleBytes,std::uint8_t{});
}
void ConstructChallengeState(std::span<std::uint8_t> bytes,ChallengeGeometry& geometry) { //42b9c0/42ce80의 자식 초기화는 뒤의 전체DWORD 지우기에 모두 덮이며 외부 효과가 없다.
    ResetChallengeState(bytes);InitializeGeometry(geometry);
}
static_assert(kChallengeBattleOffset+kChallengeBattleCount*kChallengeBattleBytes==kChallengeParticipantOffset && kChallengeParticipantOffset+kChallengeParticipantCount*kChallengeParticipantBytes==kChallengeBytes); // 두 원문 배열 사이/끝에 임의 padding을 추가하지 않는다.
}
