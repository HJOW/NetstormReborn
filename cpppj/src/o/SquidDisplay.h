// 공통 update88/update8c의 raw 프레임·SHP 경계 갱신. 클라이언트 장치에는 의존하지 않는다.
#pragma once
#include "o/RiftType.h"
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::o {
struct SquidDisplayRect { int left{},top{},right{},bottom{}; }; // right/bottom 제외.
struct SquidDisplayFrame {
    std::int16_t width{},height{},hotspotX{},hotspotY{}; // SHP 프레임 앞 12바이트의 signed short.
};
struct SquidDisplayShape {
    int frameCount{}; // 타입 +0x114의 클러스터 수. 그림자/abstract 레이어를 포함하지 않는다.
    bool loaded{}; // 원본 타입 +0xdc의 null 여부를 빈 배열과 구별한다.
    std::vector<SquidDisplayFrame> frames;
};
struct SquidDisplayView {
    int cameraX{},cameraY{},zoom{65536}; // 원본 카메라 픽셀과 Q16 배율.
    SquidDisplayRect viewport;
};
class SquidDisplaySink {
public:
    // Renderer 또는 독립 변경 표의 수명을 호출자가 관리한다.
    virtual ~SquidDisplaySink()=default;
    // 원본 전체 갱신/표시 억제 전역에 대응한다.
    virtual bool Suppressed() const=0;
    // 원본 순서대로 main·shadow 영역과 플래그를 전달한다.
    virtual void Invalidate(SquidDisplayRect rect,std::uint32_t flags)=0;
};
class SquidDisplay {
public:
    // 타입 번호와 같은 인덱스의 SHP 목록을 받는다. 두 목록과 sink는 이 객체보다 오래 살아야 한다.
    SquidDisplay(OriginalEdition edition,std::span<const RiftTypeRecord> types,
        std::span<const SquidDisplayShape> shapes,SquidDisplaySink& sink,SquidDisplayView view);
    // 카메라/viewport 변경은 호출자의 전체 갱신과 함께 사용한다.
    void SetView(SquidDisplayView view);
    // 공간 쓰기 전에 미지원 vtable·물리 프레임 범위·판본 불일치를 확인한다.
    void Validate(OriginalEdition edition,std::span<const std::uint8_t> bytes,std::uint32_t flags) const;
    // 공통 가상 메서드의 프레임/extra 비트를 읽어 변경 표에 등록한다.
    void Update(std::span<const std::uint8_t> bytes,std::uint32_t flags=0);
    // Renderer의 원본 경계 계산 함수. 표시 억제/선택 확장은 여기서 적용하지 않는다.
    SquidDisplayRect Bounds(std::size_t type,int frame,float x,float y) const;
private:
    // 패치의 frameCheck(false) 경로에서 invalid frame/null/해제 흔적이면 갱신하지 않는다.
    bool PatchFrameValid(std::size_t type,int frame) const;
    OriginalEdition edition_;
    std::span<const RiftTypeRecord> types_;
    std::span<const SquidDisplayShape> shapes_;
    SquidDisplaySink& sink_;
    SquidDisplayView view_;
};
}
