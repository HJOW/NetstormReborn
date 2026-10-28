// 원본 vtable +0x10의 다리 삭제 재정의를 공통 destroy 앞에 연결한다.
#pragma once
#include "o/Bridge.h"
#include "o/RawSquidDestroy.h"
#include "o/RawSquidNeighbors.h"

namespace netstorm::o {
struct RawAuthorityState; //현재 서버/권한의 장기 소유자는 이 가상 분배기보다 오래 살아야 한다.
// 원본 vtable +0x10의 다리 삭제 재정의를 공통 destroy 앞에 연결한다.
class RawSquidDestroyDispatch {
public:
    // 풀·공간·프레임·모드를 공유한다. Graph가 없으면 비활성 표의 표면 수 0으로 처리한다.
    // 참조하는 객체들은 이 분배기보다 오래 살아야 하며 모드 변경은 다음 호출부터 반영된다.
    RawSquidDestroyDispatch(RawSquidDestroy& base,const RawSquidNeighbors& neighbors,
        const BridgeDecayMode& mode,const RawGraph* graph=nullptr);
    // false는 다리 재정의의 삭제 거부다. true는 공통 몸체로 전달했음을 뜻하며 이미 dead이면 몸체가 반환한다.
    // 원본이 공통 destroy를 직접 부르는 지점은 RawSquidDestroy::Destroy를 사용한다.
    bool Destroy(Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) const;
    void BindCurrentModes(const RawAuthorityState& authority,const std::uint32_t* editor=nullptr); //Client의 현재 boss/editor를 가상 삭제 진입 시 직접 읽는다. NULL editor는 기존 mode를 유지한다.
private:
    // base_: 파생 처리에서 공통 동작을 위임하는 기본 객체.
    RawSquidDestroy& base_;
    // neighbors_: 발자국에 인접한 표면·객체를 조회하는 검색기.
    const RawSquidNeighbors& neighbors_;
    // mode_: 원본 실행 상태 전역. 기본값은 싱글 플레이(로컬이 서버) 전투 화면이다 (BridgeDecayMode).
    const BridgeDecayMode& mode_;
    // graph_: 서로 연결된 표면의 Graph 소속을 관리하는 처리기.
    const RawGraph* graph_{};
    const RawAuthorityState* authority_{}; //호출마다 현재 boss를 읽으며 프레임별 복사 지연을 만들지 않는다.
    const std::uint32_t* editor_{}; //같은 Client/Session의005c85a4 주소이며 bool 사본과 구별한다.
};
}
