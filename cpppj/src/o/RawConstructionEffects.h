// 건설 요청의 유닛 밀어내기와 섬 실패 정리를 복원한다. 공간 이동/삭제의 가상 효과는 호출자가 연결한다.
#pragma once
#include "o/RawConstructionRequest.h"
#include <string_view>

namespace netstorm::o {
// 원본 관계 표와 섬 타입/모드 전역이다. 요청 모듈과 같은 풀·전역 상태를 공급한다.
struct ConstructionEffectsState {
    std::array<std::uint32_t,81> alliances{}; // 00595200: [요청 플레이어*9+후보 소유자], 0 이외면 동맹이다.
    std::uint32_t islandType{94},noIslandType{157},stalagType{95},stumpType{96}; // 두 판본의 섬/받침/종유석/그루터기 번호다.
    bool blocked{}; // 0054db84 / CD 0052e92c: 섬 정리 전체를 생략한다.
    bool server{}; // 00540bc0 / CD 00540a28: 섬 정리 메시지를 다른 플레이어에 전파한다.
    const bool* sharedServer{}; // 실제 Client server이며 boss와 독립인 전파 권한이다.
    const bool& Server() const { return sharedServer ? *sharedServer : server; } // 섬 정리 방송 직전에 현재 서버 역할을 읽는다.
};
// 각 가상 호출의 인자를 보존한다. 경계는 같은 풀/타입/지도/전역을 바꿀 수 있다.
struct ConstructionEffectsHooks {
    // 00488fe0 / CD 0047d020: 현재 유닛 타입에 대응하는 경로 contained 타입을 조회한다.
    std::function<std::uint32_t(std::uint32_t)> pathType;
    // 004b1a90 / CD 004eb310(sid,pathType,0,1): 첫 살아 있는 경로 contained의 삭제를 요청한다.
    std::function<void(Sid,std::uint32_t)> clearPath;
    // vtable +0x48/+0x90: Unpop(0) 후 찾은 정수 위치에 Pop(x,y,0)한다.
    std::function<void(Sid,std::uint32_t)> unpop;
    // pop: 대상을 지정 위치의 공간에 등록하는 Pop 콜백.
    std::function<void(Sid,float,float,std::uint32_t)> pop;
    // 00460600 / CD 00454810: 섬 조각을 떨어뜨리는 표시 효과다.
    std::function<void(Sid)> fall;
    // 004a9d70 / CD 00438c00: 현재 조각 좌표에서 islandFall.wav를 loop=0으로 재생한다.
    std::function<void(float,float)> sound;
    // vtable +0x10: 원본 플래그로 삭제한다. 이후 현재 타입의 genus를 다시 읽는다.
    std::function<void(Sid,std::uint32_t)> destroy;
    // vtable +0xc8: genus 0x10000 후보의 후속 가상 알림이다.
    std::function<void(Sid)> notify;
    // 메시지 초기화/다른 플레이어 전파 경계다. x/y는 __ftol(좌표*65536)의 16.16 signed DWORD 비트를 유지한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> broadcast;
    // 원본 보고 함수가 반환하면 다음 검사를 계속한다. 비우면 보고만 생략한다.
    std::function<void(std::string_view,int)> assertion;
    // 대체 없는 탐색기 Begin 인자의 진단 경계다. 비워 둘 수 있다.
    std::function<void(const SquidSearchArea&)> scan;
};
// 00443800 / CD 004d2450와 0046e6c0 / CD 0048f960의 공통 입력과 실제 탐색기를 묶는다.
class RawConstructionEffects {
public:
    // 지도는 최소 256×256이며 모든 효과 경계가 필요하다. 참조 자료와 경계의 수명을 호출자가 보장한다.
    RawConstructionEffects(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const std::uint8_t> spots,const ConstructionEffectsState& state,ConstructionEffectsHooks hooks);
    // 발자국 안의 유닛을 아래/오른쪽으로 옮긴다. 10.78은 적대 후보에서 false, CD는 항상 true다.
    // 실패 전에 처리한 후보의 효과를 되돌리지 않는다. 이동 자리가 없으면 그 후보를 건너뛴다.
    bool Displace(float x,float y,std::uint32_t type,std::uint32_t player) const;
    // 섬 발자국의 소유자 일치 후보를 정리하고 9/1/1 조각 수 조건을 보고한다. 실패 정리 flags는 0x1000이다.
    void DropIsland(float x,float y,std::uint32_t flags,std::uint32_t player) const;
    // 요청 어댑터가 같은 풀을 사용하는지 확인한다.
    const SidPool& Pool() const;
private:
    // 후보의 현재 raw 타입에서 genus를 조회한다. 경계 뒤의 재조회를 위해 값을 캐시하지 않는다.
    std::uint32_t Genus(Sid sid) const;
    // spot bit 0x10이 없고 bit 2/4 중 하나가 있는 칸이면 이동 가능하다. 지도 밖은 C++ 계약으로 거부한다.
    bool Open(std::int64_t x,std::int64_t y) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // hash_: 좌표별 객체 후보를 찾는 공간 해시.
    const SquidHash& hash_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // state_: 원본 관계 표와 섬 타입/모드 전역이다. 요청 모듈과 같은 풀·전역 상태를 공급한다 (ConstructionEffectsState).
    const ConstructionEffectsState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ConstructionEffectsHooks).
    ConstructionEffectsHooks hooks_;
};
// 건설 요청의 세 효과를 실제 밀어내기/섬 정리 몸체에 연결한다. MayPlace/확정/진단은 받은 값을 보존한다.
ConstructionRequestHooks MakeConstructionEffectsRequestHooks(const SidPool& pool,const RawConstructionEffects& effects,
    ConstructionRequestHooks hooks);
}
