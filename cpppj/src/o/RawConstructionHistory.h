// 원문 최근 로컬 건설 위치40BYTE와 Tab 순번을 실제 Runtime보다 오래 공유한다.
#pragma once
#include <array>
#include <cstdint>

namespace netstorm::o { // Native 주소를 호스트 포인터로 재해석하지 않는 값형 장부다.
struct ConstructionNoticePoint { // 건설 통지와 카메라가 같은 단정도 좌표 비트를 사용한다.
    float x{},y{}; // x/y는 각각 원문 DWORD이며 복사할 때 NaN/무효 좌표 비트도 보존한다.
};
struct ConstructionHistoryState { // 패치00558ea4/00558e78의 서로 다른 실제 전역을 한 소유자가 보관한다.
    std::array<ConstructionNoticePoint,5> points{}; // 가장 최근 배치부터 다섯 좌표며 최초 명시 초기화 전에는BSS0이다.
    std::uint32_t index{}; // Tab의 현재 순번이며 배치 통지는0으로 쓰고 카메라 복귀 뒤 원문 signed 나머지를 쓴다.
};
}
