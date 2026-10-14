// 원본 게임 난수 초기화와 SP용 임시 시각 재시드를 같은 CRT/게임 상태에 연결한다.
#pragma once
#include "o/SpStore.h"
namespace netstorm::o {
// 00455970/004559d0의 현재 randomMode·CRT·게임 난수를 함께 갱신하는 처리기다.
class RawGameRandomSeed {
public:
    // 모든 참조와 Unix 초 공급자는 이 객체보다 오래 살아야 한다. mode0은 고정 재현 시드다.
    RawGameRandomSeed(GameRandom& game,std::uint32_t& crt,std::uint32_t& mode,std::function<std::int32_t()> unixSeconds);
    // 실제 MSVC CRT rand 한 번의 상태 갱신과15비트 반환이다. 같은 CRT를 지형 소비자와 공유한다.
    static std::uint32_t NextCrt(std::uint32_t& state);
    // 00455970: 현재mode에 따라 seed→100회 버림→세rand 곱을 게임 난수에 기록한다.
    std::uint32_t Initialize() const;
    // 004559d0: mode를1로 임시 변경한 뒤 초기화하고 원래 DWORD를 복원한다.
    std::uint32_t Reseed() const;
private:
    GameRandom& game_; // SP/공격/생성 등이 사용하는 같은 원본 게임 난수다.
    std::uint32_t& crt_; // 원본 같은 스레드의 CRT rand 상태다.
    std::uint32_t& mode_; // 실제005c89a8 DWORD이며0만 고정 시드를 선택한다.
    std::function<std::int32_t()> unixSeconds_; // 실제 __time32(NULL)의 signed Unix 초 입력 경계다.
};
}
