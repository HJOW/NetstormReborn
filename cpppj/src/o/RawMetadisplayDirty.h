// Sky Overview의 고유 셀 등록·전체 갱신·프레임별 소비를 실제 raw 월드에 연결한다.
#pragma once
#include "o/SidPool.h"
#include <array>
#include <functional>
#include <vector>

namespace netstorm::o {
// 4×4 월드 칸을 묶은 미니맵 갱신 번호다. 세 번째 원본 DWORD는 셀 몸체에서 읽지 않는다.
struct MetadisplayDirtyCell {
    std::int32_t x{}; // 월드 X를 절삭한 뒤 signed /4한 셀 번호다.
    std::int32_t y{}; // 월드 Y를 절삭한 뒤 signed /4한 셀 번호다.
    std::uint32_t unused{}; // 원본의 미초기화 스택 잔여값 자리이며 이 구현은 0으로 정규화한다.
    // 전체 raw 큐의 의미 있는 좌표와 정규화된 잔여값을 검사할 때 사용한다.
    bool operator==(const MetadisplayDirtyCell&) const = default;
};
// 실제 현재 카메라 좌표와 화면 사각형이다. Float 비교의 판본 차이를 보존하도록 비트로 받는다.
struct MetadisplayDirtyView {
    std::array<std::uint32_t,2> camera{}; // 패치0059a900/CD00565c18의 float 좌표 비트다.
    std::array<std::int32_t,4> rectangle{}; // 패치005ca9cc/CD00583e20의 화면 사각형이다.
};
// 원본의 BSS 큐·비트맵·캐시를 다른 월드 모듈과 공유하는 자료다.
struct MetadisplayDirtyState {
    bool displayPresent{}; // 패치005949d0/CD0051c54c의 표시 객체가 존재하는지다.
    std::uint8_t displayFlags{}; // 표시 객체+4 bit1만 활성 여부를 결정한다.
    std::vector<std::uint8_t> bitmap; // 최초 Refresh에서 확보·0으로 채우는 128×128 BYTE 배열이다.
    std::int32_t bitmapWidth{}; // 원본 최초 확보 시 128로 설정하는 비트맵 행 폭이다.
    std::array<MetadisplayDirtyCell,100> cells{}; // 등록 순서와 count 밖의 옛 자료를 보존하는 원본 고정 큐다.
    std::int32_t count{}; // 큐의 유효 셀 수다. 유효 호스트 입력은 0..100이다.
    std::uint32_t fullRefresh{}; // 0이 아니면 64×64 셀을 행 우선으로 모두 다시 그린다.
    std::uint32_t initialized{}; // 카메라/사각형 static 초기화 비트1·2다.
    MetadisplayDirtyView cached; // 마지막 display rebuild 전에 저장한 현재 view다.
    std::uint32_t frames{}; // 활성 표시 Refresh마다 증가하는 원본 DWORD 프레임 번호다.
    std::uint32_t* sharedFullRefresh{}; // 실제 Client의005949d4를 빌리며 생략한 독립 입력만 기존 fullRefresh를 소유한다.
    std::uint32_t& FullRefresh() { return sharedFullRefresh ? *sharedFullRefresh : fullRefresh; } // HUD/섬/신전과 같은 현재 DWORD를 직접 변경하며 사본을 동기화하지 않는다.
    const std::uint32_t& FullRefresh() const { return sharedFullRefresh ? *sharedFullRefresh : fullRefresh; } // 외부 효과 뒤마다 같은 소유자의 현재 전체 갱신 요청을 다시 읽는다.
};
// bitmap의 셀 그림과 display 재생성 몸체는 별도 실제 구현을 반드시 연결한다.
struct MetadisplayDirtyHooks {
    std::function<void()> rebuild; // 00480270/CD004420d0의 표시/카메라 재생성 전체 효과다.
    std::function<void(std::int32_t,std::int32_t)> redrawCell; // 004808d0/CD00442680의 실제 2×2 미니맵 셀 그림이다.
};
// Pop/Unpop의 표면 통지를 같은 SID 풀과 원본 미니맵 상태로 전달하는 처리기다.
class RawMetadisplayDirty {
public:
    // 풀·상태·훅의 수명은 처리기보다 길어야 한다. 전투·로딩 상태를 임의 활성 조건으로 추가하지 않는다.
    RawMetadisplayDirty(const SidPool& pool,MetadisplayDirtyState& state,MetadisplayDirtyHooks hooks={});
    // 00481510/CD00442f30: 활성 표시에서 좌표를 고유 추가하고 100개 포화 때 전체 갱신을 요청한다.
    void Enqueue(float x,float y) const;
    // Pop/Unpop 호출 시점의 같은 raw 슬롯+14/+18 좌표를 사용한다. 표면 여부 판단은 원래 호출자가 맡는다.
    void Enqueue(Sid sid) const;
    // 00481320/CD00442d10: 최초 bitmap 확보→view/rebuild→프레임→전체/부분 그림→count/full 초기화다.
    void Refresh(const MetadisplayDirtyView& view) const;
    // 다른 raw 풀의 Pop 통지가 연결되는 것을 생성 단계에서 거부하기 위한 소유자다.
    const SidPool& Pool() const;
private:
    // 원본 signed32 절삭·나눗셈을 구현하며 비유한/32비트 밖 입력은 호스트 계약에서 거부한다.
    static std::int32_t Cell(float coordinate);
    // 큐 count와 확보된 bitmap의 폭/크기를 쓰기 전에 검사한다.
    void ValidateState() const;
    const SidPool& pool_; // 호출 SID와 판본별 float 비교를 결정하는 실제 월드 풀이다.
    MetadisplayDirtyState& state_; // 원본 큐·프레임·view·bitmap을 같은 인스턴스에서 읽고 쓴다.
    MetadisplayDirtyHooks hooks_; // 표시 재생성·셀 그림의 실제 효과 경계다.
};
// 같은 풀만 허용한 좌표 통지 훅을 반환한다. Pop/Unpop/Frame/Graph의 기존 호출 순서는 변경하지 않는다.
std::function<void(float,float)> MakeMetadisplayDirtyHook(const SidPool& pool,RawMetadisplayDirty& dirty);
}
