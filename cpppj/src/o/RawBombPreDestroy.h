// Point Blast 계열 Bomb의 가상 전처리41e960을 공통 Squid 삭제와 구분한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
namespace netstorm::o {
// 폭발·현재좌표 소리·공통 전처리의 실제 하위 경계다.
struct BombPreDestroyHooks {
    std::function<void(Sid)> explosion; //4605d0의 실제 같은 월드 효과다.
    std::function<void(float,float)> sound; //현재좌표의explosion.wav/loop0이다.
    std::function<void(Sid,std::uint32_t)> base; //4b0950의 비가상 공통 전처리다.
};
// 다른 Bomb 파생 중 공통 전처리를 가진 VT는 여기로 분배하지 않는다.
class RawBombPreDestroy {
public:
    RawBombPreDestroy(const SidPool& pool,BombPreDestroyHooks hooks); //같은PATCH 풀의 장기참조를 빌린다.
    bool Handles(Sid sid) const; //실제502370의 +14=41e960만 처리한다.
    void PreDestroy(Sid sid,std::uint32_t flags) const; //최초extra/flags를 검사하고 효과 뒤 현재좌표를 다시 읽는다.
private:
    const SidPool& pool_; //원본과 같은50바이트 슬롯이다.
    BombPreDestroyHooks hooks_; //선택한 실제 효과 누락은 거부한다.
};
}
