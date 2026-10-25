// 삭제 효과가 바꾼 부모/소유자/글로벌 비용을 실제 호출 순서 사이에서 다시 조회한다.
#include "o/ContentFormLifetime.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 타입6과 공통 raw 위치다. 패치 선지급 억제 BYTE는 목록 순서 BYTE와 다른 +22다.
constexpr std::size_t kType=10,kState=11,kParent=16,kKind=18,kPaid=22,kOwner=34,kX=14,kY=18;
constexpr std::uint8_t kContent=6,kFree=1,kDead=2;
// CRT의 유효한 signed64 변환 범위 안에서 기존 SP 모듈과 같은 하위 DWORD 정책을 쓴다.
constexpr double kIntegerLimit=9.2e18;
// raw float 좌표의 정렬되지 않은 DWORD를 little endian으로 조립한다.
float Float(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 네 바이트를 호스트 정렬/엔디언과 무관하게 읽는다.
    for (std::size_t index=0;index<4;++index) bits|=std::uint32_t(raw[offset+index])<<(index*8);
    return std::bit_cast<float>(bits);
}
// 너겟의 선지급 AI 금액은 SP float 금액과 구별한다.
std::int32_t Wallet(float cost) {
    if (!std::isfinite(cost) || std::fabs(static_cast<double>(cost))>=kIntegerLimit) throw std::out_of_range("Content 너겟 지갑 금액 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(cost)));
}
}
// 실제 풀/장부가 다른 지갑 처리기를 연결하는 것은 생성 시 거부한다.
ContentFormLifetime::ContentFormLifetime(SidPool& pool,std::span<const RiftTypeRecord> types,ContentFormHost& forms,RawSquidDestroy& destroy,
    const ContentLifetimeState& state,ContentLifetimeHooks hooks,SquidDestroyHooks fallback,SquidReward* reward)
    :pool_(pool),types_(types),forms_(forms),destroy_(destroy),state_(state),hooks_(std::move(hooks)),fallback_(std::move(fallback)),reward_(reward) {
    if (&forms.Pool()!=&pool || &destroy.Pool()!=&pool || (reward && &reward->Pool()!=&pool)) throw std::invalid_argument("Content 수명 SID 풀 불일치");
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("Content 수명 타입 판본/크기 오류");
}
// dead 타입6 재진입은 허용하며 실제 필드 접근 시 free를 따로 확인한다.
bool ContentFormLifetime::IsContentForm(Sid form) const {
    return form.value>=5 && form.value<pool_.Capacity() && form.value!=pool_.Layout().predictableFirst && pool_.Slot(form)[kType]==kContent;
}
// 원본 kind/parent의 WORD 전체를 읽는다.
std::uint16_t ContentFormLifetime::Word(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);return static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8));
}
// 패치 너겟 효과에서만 쓰는 현재 부모 owner BYTE다.
std::uint32_t ContentFormLifetime::Owner(Sid parent) const { return pool_.Slot(parent)[kOwner]; }
// 효과마다 현재 전역 타입 번호로 현재 cost를 다시 읽는다.
float ContentFormLifetime::Cost() const {
    if (state_.nuggetType>=types_.size()) throw std::out_of_range("Content 너겟 타입 번호 오류");
    const auto cost=types_[state_.nuggetType].cost;if (!std::isfinite(cost)) throw std::invalid_argument("Content 너겟 비용 유한값 필요");return cost;
}
// 실제 004adea0는 footZ를 포함하지 않은 발자국을 쓰고 각 결과를 float에 저장한다.
std::array<float,2> ContentFormLifetime::Center(Sid parent) const {
    const auto raw=pool_.Slot(parent);if (raw[kType]>=types_.size()) throw std::out_of_range("Content 중심 부모 타입 오류");
    const auto& type=types_[raw[kType]];
    return {static_cast<float>(static_cast<double>(Float(raw,kX))-static_cast<double>(type.footX)*0.5),
        static_cast<float>(static_cast<double>(Float(raw,kY))-static_cast<double>(type.footY)*0.5)};
}
// 커서 취소는 내용물 kind 검사보다 먼저이며 콜백 뒤 현재 슬롯에서 다시 읽는다.
void ContentFormLifetime::PreDestroy(Sid form) const {
    if (!IsContentForm(form) || (pool_.Slot(form)[kState]&kFree)) throw std::logic_error("Content preDestroy 슬롯 오류");
    if ((state_.currentCursor ? state_.currentCursor() : state_.cursor)==form || Word(form,kParent)==form.value) {
        if (!hooks_.cancelCursor) throw std::invalid_argument("Content 커서 취소 경계 필요");hooks_.cancelCursor();
    }
    // CD는 공통 깊이 반납만 하며 패치판에만 너겟 알림/선지급 효과가 있다.
    if (pool_.Edition()==OriginalEdition::Patch1078 && Word(form,kKind)==state_.nuggetType) {
        const Sid parent{Word(form,kParent)};
        if (!parent.value || parent.value>=pool_.Capacity()) throw std::logic_error("Content 너겟 부모 번호 오류");
        if (!(pool_.Slot(parent)[kState]&kDead)) {
            if (Owner(parent)==state_.localOwner) {
                if (!hooks_.localNotice) throw std::invalid_argument("Content 로컬 수입 알림 경계 필요");hooks_.localNotice(state_.notice);
            } else if (state_.Boss()) {
                if (!hooks_.remoteNotice) throw std::invalid_argument("Content 원격 수입 알림 경계 필요");hooks_.remoteNotice(Owner(parent));
            }
            if (!pool_.Slot(form)[kPaid]) {
                const auto recipient=Owner(parent);const auto amount=Wallet(Cost());
                if (reward_) reward_->CreditWallet(recipient,amount);
                else { if (!hooks_.wallet) throw std::invalid_argument("Content 지갑 선가산 경계 필요");hooks_.wallet(recipient,amount); }
                // 지갑 콜백 뒤 current owner/cost/localOwner를 다시 읽어 정확한 SP 가산 경로를 고른다.
                const auto owner=Owner(parent);const auto cost=Cost();const bool local=owner==state_.localOwner;
                if (reward_) reward_->AddSp(owner,cost);
                else { if (!hooks_.addSp) throw std::invalid_argument("Content SP 가산 경계 필요");hooks_.addSp(owner,cost,local); }
                if (!hooks_.sound) throw std::invalid_argument("Content 너겟 위치 소리 경계 필요");
                const auto point=Center(parent);hooks_.sound(point[0],point[1]);
            }
        }
    }
    destroy_.CompletePreDestroy();
}
// 중첩 삭제는 현재 dead 상태를 읽는 같은 공통 삭제기로 전달한다.
void ContentFormLifetime::Destroy(Sid form,std::uint32_t flags) const {
    if (!IsContentForm(form)) throw std::logic_error("Content 삭제 타입 오류");destroy_.Destroy(form,flags,Hooks());
}
// 다른 파생 Form/자산 분배는 fallback으로 유지하며 부모 release 통지는 빈 실제 가상 함수다.
SquidDestroyHooks ContentFormLifetime::Hooks() const {
    SquidDestroyHooks hooks;hooks.selected=fallback_.selected ? fallback_.selected : std::function<Sid()>([] { return Sid{}; });
    // 실제 제거 알림·부모 dead 링크 보존은 앞서 검증한 ContentFormHost로 연결한다.
    hooks.unpopForm=[this](Sid form) {
        if (IsContentForm(form)) forms_.Unpop(form);
        else { if (!fallback_.unpopForm) throw std::invalid_argument("Content 외 Unpop 경계 필요");fallback_.unpopForm(form); }
    };
    // 깊이 반납과 종속 삭제도 같은 원본 순서를 사용한다.
    hooks.emit=[this](const SquidDestroyEvent& event) {
        if (IsContentForm(event.sid)) {
            switch (event.effect) {
            case SquidDestroyEffect::PreDestroy:PreDestroy(event.sid);return;
            case SquidDestroyEffect::PostDestroy:destroy_.CompletePostDestroy();return;
            case SquidDestroyEffect::ReleaseDependent:return;
            case SquidDestroyEffect::DestroyDependent:Destroy(event.sid,event.flags);return;
            default:break;
            }
        }
        if (!fallback_.emit) throw std::invalid_argument("Content 외 삭제 효과 경계 필요");fallback_.emit(event);
    };return hooks;
}
// 전체 월드 연결기의 동일 풀 검증에 사용한다.
const SidPool& ContentFormLifetime::Pool() const { return pool_; }
}
