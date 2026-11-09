// 원문0041fe90/0041feb0의 하드너 자격과 다리 강화 효과를 같은 물리 자료에 적용한다.
#pragma once
#include "o/SquidFrame.h"
#include "o/SpStore.h"
#include <functional>

namespace netstorm::o { // 주문의 공통 시전/패킷과 분리된 실제 대상 가상 함수 본문이다.
class RawBombHardener { // 현재 TYPE·프레임·게임 난수를 빌려 별도 상태 복제를 피한다.
public:
    RawBombHardener(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,GameRandom& random,const SquidFrame& setter,std::function<void(Sid,std::int32_t,std::uint32_t)> flare); // 동일 PATCH 풀/현재188TYPE/실제 프레임 지정기와 플레어를 연결한다.
    std::uint32_t MayTarget(Sid target) const; // 41fe90은 현재 genus의 다리 비트4를 그대로 반환한다.
    void Apply(Sid parent,Sid target) const; // QA 제외→현재 방향의 강화 프레임 검색/지정→플레어(1,2)를 실행한다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; // 원문 비정렬 필드를 지정 BYTE/WORD/DWORD 폭으로 읽는다.
    std::size_t Type(Sid sid) const; // 호출 위치의 현재 raw TYPE를 검사하여 같은 표를 조회한다.
    std::int32_t ChooseFrame(Sid target) const; //49aa90의 후보 공간을 프레임 지정/플레어 전에 해제한다.
    SidPool& pool_; // 대상/시전자 모두 같은50BYTE 물리 슬롯을 사용한다.
    std::span<const RiftTypeRecord> types_; // 대상 자격은 현재 genus를 직접 읽는다.
    std::span<const RiftTypeFrames> frames_; // 현재 방향/강화 후보의 같은 TYPE 코드 배열이다.
    GameRandom& random_; // 후보가 있을 때만 원문4558c0을 한 번 소비하는 단일 게임 난수다.
    const SquidFrame& setter_; // 기존4acee0의 표시/공간/프레임 쓰기를 그대로 공유한다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> flare_; // 검색 실패여도 선택되는 실제45a6c0의 기본 방향 플레어다.
};
}
