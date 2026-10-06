// raw 공통 표시 계산과 실제 Renderer를 연결하는 새 어댑터다.
#pragma once
#include "o/SquidDisplay.h"
#include "client/GameAssets.h"
#include "client/Renderer.h"

namespace netstorm::client {
class SquidRenderer final : public o::SquidDisplaySink {
public:
    // Renderer는 이 어댑터와 표시 계산기보다 오래 살아야 한다.
    explicit SquidRenderer(Renderer& renderer);
    // 첫 화면/전체 변경이면 원본처럼 표시 추가를 생략한다.
    bool Suppressed() const override;
    // main/shadow 변경을 실제 Draw/Present 표에 전달한다.
    void Invalidate(o::SquidDisplayRect rect,std::uint32_t flags) override;
    // 원본 로딩 순서의 .type 클러스터 수와 SHP 추가 헤더를 raw 타입 번호에 대응시킨다.
    static std::vector<o::SquidDisplayShape> Shapes(const GameAssets& assets,o::OriginalEdition edition);
private:
    Renderer& renderer_;
};
}
