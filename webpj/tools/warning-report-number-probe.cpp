// 현재 cpppj Report와 같은 표준 스트림의 17자리 출력만 관찰한다. 게임을 실행하지 않는다. (2026-11-14 WB04-E454)
#include <array>
#include <bit>
#include <cstdint>
#include <iomanip>
#include <iostream>

// 명시 비트 입력의 결과를 고정 TSV로 출력한다. 임시 정적 검증 도구다.
int main() {
    // 부호0·반올림 동률·지수 경계·최소/최대·특수값의 독립 입력이다.
    constexpr std::array<std::uint64_t,18> values{0,0x8000000000000000ULL,0x3fb999999999999aULL,0x3ff0000800000000ULL,0x3ff0001800000000ULL,0x3f1a36e2eb1c432dULL,0x3ee4f8b588e368f1ULL,0x4341c37937e08000ULL,0x4376345785d8a000ULL,1,0x0010000000000000ULL,0x7fefffffffffffffULL,0x7ff0000000000000ULL,0xfff0000000000000ULL,0x7ff8000000000000ULL,0xfff8000000000000ULL,0x7ff0000000000001ULL,0xfff0000000000001ULL};
    // 같은 실제 C++ defaultfloat/setprecision(17)을 사용하며 기대 문자열은 JS로 계산하지 않는다.
    const auto write=[](std::uint64_t bits) { std::cout<<std::hex<<std::setfill('0')<<std::setw(16)<<bits<<'\t'<<std::setprecision(17)<<std::bit_cast<double>(bits)<<'\n'; };
    // 먼저 명시 경계 입력 18개를 출력한다.
    for (const auto bits:values) write(bits);
    // 임의 비트 분포를 재현하는 uint64 LCG의 고정 시작값이다. 게임 난수와 무관하다.
    std::uint64_t state=0x123456789abcdef0ULL;
    // 고정 1,000입력을 같은 네이티브 표준 스트림에 통과시킨다.
    for (int index=0;index<1000;++index) { state=state*6364136223846793005ULL+1442695040888963407ULL;write(state); }
}
