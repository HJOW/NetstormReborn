// Client가 읽는 OS 시각(밀리초 timeGetTime·유닉스 초 time32)의 단일 공급원이다.
// 기본은 OS 시계를 그대로 읽어 기존 동작과 같다. 검사 전용 고정 시각 모드(--fixed-frame-ms)를 켜면
// 프레임마다 정해진 밀리초만큼만 전진하는 가상 시계를 돌려주어, 같은 입력 단계표의 두 실행이 같은 게임 시각·난수 재시드를 쓰게 한다.
// 사용: Client 생성 전에 EnableFixedClientTime을 한 번 부르고, Client 메인 루프가 프레임 시작마다 AdvanceFixedClientTime을 부른다.
// 2026-11-09: webpj/dotnetpj의 L2·L3(cpppj와 같은 단계표로 진행한 상태) 대조에 필요한 결정론 실행을 위해 추가했다. 원본 게임에는 없는 검사 기능이다.
#pragma once
#include <cstdint>

namespace netstorm::client {
// 고정 시각 모드의 설정이다. frameMilliseconds가 0이면 꺼진 상태다. (2026-11-09 추가)
struct FixedClientTime {
    std::uint32_t frameMilliseconds{}; // 프레임마다 전진하는 가상 밀리초(1~60000)다.
    std::uint32_t originMilliseconds{1000000}; // 첫 Client 시계 원점으로 쓰는 가상 timeGetTime 값이다.
    std::int32_t unixSeconds{946684800}; // 가상 시계 원점에 대응하는 유닉스 초(기본 2000-01-01 00:00:00 UTC)다.
};

// 고정 시각 모드를 켠다. Client 생성 전 프로세스에서 한 번만 부르며, 범위 밖 값이나 두 번째 호출은 거부한다. (2026-11-09 추가)
void EnableFixedClientTime(FixedClientTime setting);
// 고정 시각 모드를 끄고 가상 경과를 0으로 되돌린다. 단위 시험의 정리에만 쓴다. (2026-11-09 추가)
void DisableFixedClientTimeForTests();
// 고정 시각 모드가 켜져 있는지 반환한다. 메인 루프의 maxFPS 바쁜 대기 생략에 쓴다. (2026-11-09 추가)
bool FixedClientTimeEnabled();
// 현재 Client 밀리초다. 기본은 timeGetTime(), 고정 모드는 원점+가상 경과(unsigned 32비트 감김)다. (2026-11-09 추가)
std::uint32_t ClientMilliseconds();
// 현재 유닉스 초다. 기본은 std::time, 고정 모드는 설정 초+가상 경과 초(내림)다. time32 범위 밖은 거부한다. (2026-11-09 추가)
std::int32_t ClientUnixSeconds();
// 고정 모드에서 가상 시계를 한 프레임만큼 전진한다. 꺼져 있으면 아무것도 하지 않는다. (2026-11-09 추가)
void AdvanceFixedClientTime();
}
