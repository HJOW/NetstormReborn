// 420a40/41e100의 상태 주문 대상 목록·기한과 만료 처리를 같은 실제 Kernel에 연결한다.
#pragma once
#include "o/SquidProcess.h"
#include "o/RiftType.h"

namespace netstorm::o {
// 원본 +0c/+14 목록, +18 double 기한, +20 flags의 소유 자료다.
struct BombEffectTimerFields {
    std::vector<std::uint32_t> members; // 순서와 중복을 보존하는 원본 DWORD SID 목록이다.
    double endTime{}; // 생성 당시 현재 게임 시각 + signed 지연 시간이다.
    std::uint32_t flags{}; // 값1은 extra10(은신), 값2는 extra20(마비) 효과다.
    std::uint32_t capacity{100}; // 원본 +10의 고정 저장 한도이며 vector의 실제 확보량과 구별한다.
};
// 실제 현재 가상 호출만 공급하며 초기 갱신과 만료 뒤 갱신을 구별하지 않고 그대로 실행한다.
struct BombEffectTimerHooks {
    std::function<void(Sid,std::uint32_t)> unpop; // 현재 가상48(flags0)이다.
    std::function<void(Sid,std::uint32_t)> repop; // 현재 가상4c(flags40)이다.
    std::function<void(Sid)> refresh; // 현재 가상88이며 기본 몸체는4ad500이다.
    std::function<void(Sid)> fence; // 현재 TYPE136의457890→457460 재연결이다.
};
// 전체 Frame 몸체만 담당하며 부모/Form/삭제 권한은 기존 Host가 소유한다.
class RawBombEffectTimer {
public:
    RawBombEffectTimer(SidPool& pool,std::span<const RiftTypeRecord> types,const double& now,BombEffectTimerHooks hooks); // 같은 PATCH 풀/188TYPE/게임 시계를 빌린다.
    BombEffectTimerFields Construct(std::int32_t delay,std::span<const std::uint32_t> members,std::uint32_t flags) const; // 원본 초기100칸을 확보하고 목록·현재 기한을 복사한다.
    void Frame(BombEffectTimerFields& fields,const std::function<void()>& finish) const; // 갱신→현재 기한→효과 해제→현재 가상 갱신→자기 삭제 순서다.
private:
    Sid Target(std::uint32_t member) const; // 손상된 목록을 WORD로 축소하지 않고 물리 SID 범위를 확인한다.
    SidPool& pool_; // 상태 비트를 직접 바꾸는 같은50BYTE 풀이다.
    std::span<const RiftTypeRecord> types_; // 만료 콜백 뒤 현재 종류의 genus를 다시 읽는 표다.
    const double& now_; // 큰 효과 뒤에도 현재 값을 읽는 원본 게임 시각이다.
    BombEffectTimerHooks hooks_; // 재배치/표시/썬 바리케이드의 실제 소비자다.
};
// Kernel57의 실제 현재 필드와 기존 ProcessForm 수명을 함께 소유한다.
class BombEffectTimerProcess final : public SquidProcess {
public:
    BombEffectTimerProcess(SidPool& pool,std::span<const RiftTypeRecord> types,const double& now,std::int32_t delay,std::span<const std::uint32_t> members,std::uint32_t flags,BombEffectTimerHooks hooks); // 단일 목록/기한을 생성한다.
    void RunFrame() override; // 기존 Host의 실제 삭제에 원본 Frame을 연결한다.
    void Notify(std::uint32_t,std::uint32_t,std::uint32_t) override; // VT50330c+1c의480260은 ret12인 빈 통지다.
    const BombEffectTimerFields& Fields() const; // 선택 표시가 같은 현재 목록·기한·flags를 읽는다.
    void ReadPacket(std::span<const std::uint8_t> packet); // 원본41e230을 현재 유일한 목록에 누적 적용한다.
    std::uint32_t WritePacket(std::span<std::uint8_t> packet,std::uint16_t declaredLength) const; // 현재 Host Form/부모를 실제41e280에 전달한다.
private:
    RawBombEffectTimer logic_; // 같은 풀과 시계를 빌리는 전체 Frame 몸체다.
    BombEffectTimerFields fields_; // 이 프로세스의 유일한 실제 상태다.
};
}
