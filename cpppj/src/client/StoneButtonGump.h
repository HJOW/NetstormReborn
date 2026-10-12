// 돌 버튼의 라벨/글자 문맥/자식 배경 생성과 가상 부착·기본 소멸자 위임을 복원한다.
#pragma once
#include "client/ButtonGump.h"
#include "client/GumpBackgroundCreate.h"
#include "client/GumpTree.h"

namespace netstorm::client {
// 라벨 +c0..+ff, 글자 문맥 +104/+124/+144, 측정값 +164/+168이다. background는 기본 버튼 +100을 공유한다.
struct StoneButtonGumpNode : ButtonGumpNode {
    std::array<char,64> label{};
    std::array<GumpTextContext,3> contexts{};
    std::int32_t textWidth{},textHeight{};
};
// 생성 시 읽는 색 BYTE·기본 글자 문맥·팔레트와 현재 배경 타입/프레임 전역이다.
// palette는 일시 번개 팔레트가 아닌 기존 ButtonColors 결과를 공급한다.
struct StoneButtonGumpState {
    std::int8_t color{};
    std::array<std::uint32_t,8> defaultContext{};
    ButtonPalette palette;
    std::uint32_t backgroundType{},backgroundFrame{};
};
// allocate(0xb8)는 배경 저장소 토큰이다. push/attach는 배경의 가상 +18/+20을 실제 GumpTree에 분배한다.
// measure는 원본 글자 측정 경계이며 입력 64바이트와 현재 기본 문맥, 출력 참조, 최대 길이 DWORD를 받는다.
// 측정 콜백은 같은 노드의 폭/높이/scale도 바꿀 수 있다. 원본 폰트/장치 연결은 호출자가 공급한다.
struct StoneButtonGumpHooks {
    std::function<StoneButtonGumpNode*(std::uint32_t)> node;
    std::function<std::uint32_t(std::size_t)> allocate;
    std::function<void(std::uint32_t)> push;
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    std::function<void(std::uint32_t,const std::array<char,64>&,const std::array<std::uint32_t,8>&,
        std::int32_t&,std::int32_t&,std::uint32_t)> measure;
};
// 같은 ButtonGump/GumpCreate/GumpTree/GumpLifetime을 사용한다. 자동 Client 연결은 별도 계층이 맡는다.
class StoneButtonGump {
public:
    // 참조한 처리기/전역/안정된 파생 저장소는 돌 버튼 분배기보다 오래 살아야 한다.
    StoneButtonGump(GumpCreateState& create,StoneButtonGumpState& state,const GumpCreate& factory,
        const ButtonGump& buttons,const GumpBackgroundCreate& backgrounds,const GumpTree& tree,
        const GumpLifetime& lifetime,StoneButtonGumpHooks hooks);
    // 004254b0/CD 00441310: 다섯 원본 인자 중 unused는 읽지 않는다. 기본 버튼 flags는 항상 0이다.
    // label=nullptr는 첫 바이트만 0으로 쓴다. context=nullptr이면 현재 기본 32바이트 문맥을 쓴다.
    // 배경 할당 실패는 원본 null 접근에 해당하는 호스트 예외로 거부하며 이미 쓴 필드는 유지한다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t width,std::int32_t height,const char* label,
        std::uint32_t unused=0,const std::array<std::uint32_t,8>* context=nullptr) const;
    // 원본 strncpy(63) 규칙으로 복사한다. 마지막 label[63]은 보존하며 짧은 문자열은 63칸까지 0으로 채운다.
    // CD 00441610의 라벨 변경 뒤 측정만 수행한다. 자동 크기/배경 부착/재그리기는 추가하지 않는다.
    void SetLabel(std::uint32_t token,const char* label) const;
    // 00424f90/CD 004415c0: 현재 문맥으로 측정 후 폭 +12/높이 +6을 기본/배율 크기에 저장한다.
    // 배경 크기는 Attach에서 맞추므로 여기서는 부모 크기만 변경한다.
    void FitToText(std::uint32_t token) const;
    // 00424f00/CD 00441520: 배경 Push→현재 부모 크기 -2→배경 Attach(1,1,this,0)→기본 부모 Attach 순서다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    // 돌 버튼 가상 소멸자는 추가 배경 free 없이 기본 scalar 소멸자에 위임한다.
    // 자식 파괴는 GumpLifetime::Destroy의 자식 우선 순회가 맡는다. 반환은 원래 토큰이다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
    // 실제 저장 문맥/측정값을 기존 버튼 Draw 계획에 전달한다. 미작성 문맥 필드도 보존한다.
    ButtonDrawPlan DrawPlan(std::uint32_t token) const;
private:
    // 기본 생성/표시 노드와 파생 저장소가 동일한 주소인지 호출 시 확인한다.
    StoneButtonGumpNode& Node(std::uint32_t token) const;
    GumpCreateNode& BaseNode(std::uint32_t token) const;
    // null 허용 여부는 공개 메서드에서 정하며 복사/측정은 실제 도달 시에만 수행한다.
    void CopyLabel(StoneButtonGumpNode& node,const char* label) const;
    void Measure(std::uint32_t token) const;
    GumpCreateState& create_;
    StoneButtonGumpState& state_;
    const GumpCreate& factory_;
    const ButtonGump& buttons_;
    const GumpBackgroundCreate& backgrounds_;
    const GumpTree& tree_;
    const GumpLifetime& lifetime_;
    StoneButtonGumpHooks hooks_;
};
// 돌 버튼만 파생 Attach/scalar 소멸자를 사용한다. 나머지는 기존 분배기를 보존한다.
// resolve는 호출 때 현재 처리기를 반환하며 트리/수명 처리기 생성 뒤 순환 의존을 연결할 수 있다.
void BindStoneButtonGump(GumpTreeHooks& tree,GumpLifetimeHooks& lifetime,
    std::function<const StoneButtonGump*(std::uint32_t)> resolve);
}
