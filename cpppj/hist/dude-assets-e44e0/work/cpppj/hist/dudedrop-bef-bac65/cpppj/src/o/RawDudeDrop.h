// 피해 후처리의 실제 random/진입점 Dude 드롭을 같은 생성/task 모듈에 연결한다.
#pragma once
#include "o/RawDudeSpawn.h"
#include "o/RawDudeTask.h"
#include "o/RawDudeNotice.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 00450b30/00450c20와 CD004e8d20/004e8e00의 실제 부모 호출이다.
class RawDudeDrop {
public:
    // 같은 풀/생성/task/현재 진입점과 게임난수를 사용하며 모두 이 객체보다 오래 살아야 한다.
    RawDudeDrop(const SidPool& pool,std::span<const RiftTypeRecord> types,RawDudeSpawn& spawn,
        const RawDudeTask& task,const RawDudeNotice& notice,GameRandom& random);
    // random=true는 현재발자국 안의 두 난수좌표→생성→5103/index*0.1f다.
    // false는 실제 진입점→생성→5104/index*0.7f다. SourceSID0 진단은 판본별로 다르다.
    void Drop(Sid source,std::int32_t index,bool random) const;
    // 실제 Damageable PostDestroy의 같은 월드 연결을 확인한다.
    const SidPool& Pool() const;
private:
    // 원본float 두끝점의 절대차/상위16비트 난수로53비트중간 결과를 구한다.
    double RandomFloat(float minimum,float maximum) const;
    const SidPool& pool_; // 현재 source/새 Dude의 raw 상태와 소유자 저장소다.
    std::span<const RiftTypeRecord> types_; // random좌표에쓰는 현재 source의 실제 발자국이다.
    RawDudeSpawn& spawn_; // 같은 실제 factory/owner/Pop/최초후처리 생성기다.
    const RawDudeTask& task_; // 실제 Regular 생성/재예약과 task WORD쓰기다.
    const RawDudeNotice& notice_; // 별도 본체로 복원한 실제 source 발자국 진입점이다.
    GameRandom& random_; // SP/다른 게임기능과 공유하는 현재32비트 난수다.
};
}
