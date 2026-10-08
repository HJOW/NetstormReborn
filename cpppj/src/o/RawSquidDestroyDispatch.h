// 원본 vtable +0x10의 다리 삭제 재정의를 공통 destroy 앞에 연결한다.
#pragma once
#include "o/Bridge.h"
#include "o/RawSquidDestroy.h"
#include "o/RawSquidNeighbors.h"

namespace netstorm::o {
class RawSquidDestroyDispatch {
public:
    // 풀·공간·프레임·모드를 공유한다. Graph가 없으면 비활성 표의 표면 수 0으로 처리한다.
    // 참조하는 객체들은 이 분배기보다 오래 살아야 하며 모드 변경은 다음 호출부터 반영된다.
    RawSquidDestroyDispatch(RawSquidDestroy& base,const RawSquidNeighbors& neighbors,
        const BridgeDecayMode& mode,const RawGraph* graph=nullptr);
    // false는 다리 재정의의 삭제 거부다. true는 공통 몸체로 전달했음을 뜻하며 이미 dead이면 몸체가 반환한다.
    // 원본이 공통 destroy를 직접 부르는 지점은 RawSquidDestroy::Destroy를 사용한다.
    bool Destroy(Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) const;
private:
    RawSquidDestroy& base_;
    const RawSquidNeighbors& neighbors_;
    const BridgeDecayMode& mode_;
    const RawGraph* graph_{};
};
}
