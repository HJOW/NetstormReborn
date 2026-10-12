// 원본의 현재 등록표 읽기와 콜백 전/후 링크 조회 차이를 보존한다.
#include "client/GumpFrame.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 통지/루트 필터의 visible·dead와 재그리기 요청의 자식 전파·생략 비트다.
constexpr std::uint32_t kVisible=1,kDead=0x10000,kNotify=0x100,kChildren=0x20,kSkipRedraw=0x42000;
// 한 형제 체인의 중복 토큰/한도를 검사한다. 정상 원본 링크 순서에는 영향을 주지 않는다.
void Enter(std::unordered_set<std::uint32_t>& seen,std::uint32_t token) {
    if (seen.size()>=kGumpRegistryCapacity || !seen.insert(token).second)
        throw std::runtime_error("Gump 프레임 형제 순환/한도 초과");
}
}
// 저장소 조회만 생성 시 요구하며 실제 도달한 가상 경계는 각 메서드에서 검사한다.
GumpFrame::GumpFrame(GumpCreateState& create,GumpLifetimeState& lifetime,GumpFrameHooks hooks)
    :create_(create),lifetime_(lifetime),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("Gump 프레임 저장소 필요");
}
// 토큰은 원본 포인터 대신 쓰는 안정된 저장소 식별자다. 0은 필드 조회 대상으로 허용하지 않는다.
GumpCreateNode& GumpFrame::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node) throw std::runtime_error("Gump 프레임 토큰/저장소 없음");
    return *node;
}
// 루트 칸을 제외하고 현재 count를 매 슬롯 진입 때 읽는다. 콜백 뒤 바뀐 count도 다음 슬롯에 적용된다.
void GumpFrame::Broadcast(std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    std::int32_t encountered=0;
    // 빈 칸은 개수를 늘리지 않으며 통지 조건에 실패한 비영 칸은 개수를 늘린다.
    for (std::size_t i=1;i<create_.entries.size();++i) {
        if (encountered>=std::bit_cast<std::int32_t>(create_.count)) return;
        const auto token=create_.entries[i];if (!token) continue;
        const auto flags=Node(token).flags;++encountered;
        if ((flags&(kVisible|kNotify))==(kVisible|kNotify) && !(flags&kDead)) {
            if (!hooks_.broadcast) throw std::invalid_argument("Gump 가상 통지 경계 필요");
            hooks_.broadcast(token,event,first,second);
        }
    }
}
// idle 표의 현재 슬롯을 그대로 호출한다. 콜백이 이미 방문한 칸을 채워도 이번 호출에서 돌아가지 않는다.
void GumpFrame::Idle() const {
    const std::size_t limit=create_.collection.edition==o::OriginalEdition::Patch1078 ? lifetime_.idle.size() : 80U;
    // 원본은 idle 표가 유효 노드만 가진다고 가정한다. visible/dead/void/idle 비트를 다시 검사하지 않는다.
    for (std::size_t i=0;i<limit;++i) {
        const auto token=lifetime_.idle[i];if (!token) continue;
        Node(token);
        if (!hooks_.idle) throw std::invalid_argument("Gump 가상 idle 경계 필요");
        hooks_.idle(token);
    }
}
// 각 공개 재귀 호출은 독립 경로를 만들어 잘못된 자기 참조를 거부한다.
void GumpFrame::RedrawNode(std::uint32_t token) const {
    std::vector<std::uint32_t> active;Redraw(token,active);
}
// 재그리기 요청 콜백 뒤 현재 자식 전파 비트를 읽고 각 재귀 뒤 현재 next를 사용한다.
void GumpFrame::Redraw(std::uint32_t token,std::vector<std::uint32_t>& active) const {
    auto& node=Node(token);
    if (active.size()>=kGumpRegistryCapacity || std::find(active.begin(),active.end(),token)!=active.end())
        throw std::runtime_error("Gump 프레임 자식 순환/한도 초과");
    active.push_back(token);
    if (!(node.flags&kSkipRedraw)) {
        if (!hooks_.redraw) throw std::invalid_argument("Gump 가상 재그리기 경계 필요");
        hooks_.redraw(token,false);
    }
    if (node.flags&kChildren) {
        std::unordered_set<std::uint32_t> seen;
        // 숨은 자식은 건너뛰며 dead/void 자식은 visible이면 실제 하위 몸체에 전달한다.
        for (auto child=node.child;child;child=Node(child).next) {
            Enter(seen,child);
            if (Node(child).flags&kVisible) Redraw(child,active);
        }
    }
    active.pop_back();
}
// 루트의 현재 자식 머리만 읽고 각 최상위 next는 redraw 콜백 전에 저장한다.
void GumpFrame::RedrawRoots() const {
    std::unordered_set<std::uint32_t> seen;
    // 최상위 visible/dead 필터만 적용한다. 하위의 서로 다른 필터는 실제 재귀 처리기에 둔다.
    for (auto token=Node(create_.entries[0]).child;token;) {
        Enter(seen,token);auto& node=Node(token);
        const auto flags=node.flags,next=node.next;
        if ((flags&kVisible) && !(flags&kDead)) RedrawNode(token);
        token=next;
    }
}
}
