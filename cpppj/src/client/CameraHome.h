// 원본 초기 카메라 선택/중심 이동을 동일 raw 풀의 현재 슬롯과 화면에 연결한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::client {
// 원본 resetCamera와 camera.setCenter가 읽는 현재 모드·화면·갱신 전역이다.
struct CameraHomeState {
    std::uint32_t localPlayer{1}; // PATCH540c70/CD50f6c8의 전체 DWORD이며 소유자 BYTE로 축소하지 않는다.
    bool fort{},battle{true},challenge{}; // 원본 요새/전투/Challenge 모드의 현재 값이다.
    std::uint32_t challengeType{}; // Challenge에서 찾는 현재 타입 번호다. 일반 캠페인은 읽지 않는다.
    std::int32_t width{640},height{480}; // 중심 이동 때 읽는 실제 화면 전체 크기다.
    std::int32_t worldWidth{4096},worldHeight{2816}; // Challenge 빈 목록의 카메라 월드 픽셀 크기다.
    std::uint32_t suppressionDepth{},requests{},revision{}; // 실제 억제 깊이와 독립 장치의 카운터 입력이다. Session의 요청/변환 번호는 같은 Client owner에서 직접 증가한다.
};
// 세션은005c89b8을 실제 object loadingDepth와 공유하고 Carrier preview를 같은 Kernel에 예약한다.
struct CameraHomeHooks {
    std::function<void(o::Sid)> contentPreview; //00426080/CD004e36f0의 실제 로딩/extra/타입 관문과 Regular491 생성이다.
    std::function<std::uint32_t()> suppressionDepth; // 현재 실제005c89b8을 읽는다. 미지정이면 state의 독립 저장소를 사용한다.
    std::function<void(std::uint32_t)> setSuppressionDepth; // 같은 실제 로딩 전역과 종속 모듈을 함께 갱신한다. getter와 쌍으로 제공한다.
};
// 원본 PATCH4340d0/CD488f50의 초기 대상과 같은 중심 이동 몸체를 사용한다.
class CameraHome {
public:
    // 풀/타입/상태는 호출 동안 살아 있어야 한다. center는 world float를 받는433aa0 전체 부모이며 픽셀 setter로 대체하지 않는다.
    CameraHome(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CameraHomeState& state,
        std::function<void(float,float)> center,CameraHomeHooks hooks={});
    // free만 건너뛰며 마지막 내 사제를 고른다. dead 후보의 CenterOn 자연 반환도 유지한다.
    void Reset() const;
    // 선택한 world float를433aa0에 그대로 전달한다. 미니맵의497f20 직접 요청과 달리 현재 카메라도 즉시 적용한다.
    void CenterPoint(float x,float y) const;
    //433ba0/CD488eb0: 실제 SID의 현재 좌표/TYPE footX/footY로 중심을 옮긴 뒤 현재 Carrier genus의 preview를 요청한다.
    void CenterOn(o::Sid sid) const;
private:
    // 원본 깊이는 매 조회마다 실제 소유자에서 읽고 state의 표시도 같은 값으로 맞춘다.
    std::uint32_t SuppressionDepth() const;
    // 실제 깊이와 독립 state를 같은 변경 지점에 갱신한다.
    void SetSuppressionDepth(std::uint32_t depth) const;
    const o::SidPool& pool_; // 원본 번호순 조회와 free/dead/좌표를 공유하는 실제 풀이다.
    std::span<const o::RiftTypeRecord> types_; // 같은 TYPE의 genus와 중심 이동용 발자국 필드다.
    CameraHomeState& state_; // 매 조회의 모드/소유자/화면 크기와 변경 카운터를 읽고 쓴다.
    std::function<void(float,float)> center_; // 공유 카운터/목표/실제497960을 수행하는 world float 부모다.
    CameraHomeHooks hooks_; // 현재 실제 로딩 전역과 같은 Carrier preview의 외부 경계다.
};
}
