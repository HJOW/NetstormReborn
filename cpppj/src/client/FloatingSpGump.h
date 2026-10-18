// 원본 비용 표시 Gump의 생성·부착·프레임 이동을 복원한다. 원본 클래스 이름은 미확정이다.
#pragma once
#include "client/TextGump.h"

namespace netstorm::client {
// 기본 한 줄 TextGump와 같은 저장소에 원본 추가 필드를 둔다.
struct FloatingSpGumpNode: TextGumpNode {
    float velocity{},acceleration{}; // +a4/+a8의 프레임 이동량과 초당 이동량 변화다.
    std::uint32_t padding{}; // +ac는 생성자가 쓰지 않는 패딩이다.
    double stamp{}; // +b0의 직전 게임 시각이며 프레임마다 먼저 현재 값으로 갱신한다.
};
// 같은 실제 TextGump·등록표·시계·수명에 연결하는 외부 효과다. 모든 토큰은 같은 기본 노드를 가리킨다.
struct FloatingSpGumpHooks {
    std::function<FloatingSpGumpNode*(std::uint32_t)> node; // 안정된 파생 노드다.
    std::function<double()> now; // 현재0055b4d0 게임 시각이다.
    std::function<std::uint32_t(std::size_t)> allocate; // 실제0xb8 파생 저장소를 확보한다.
    std::function<void(std::uint32_t)> constructText,clearText,fitText,kill; // 실제 한 줄 생성/해제/측정·지연 삭제다.
    std::function<void(std::uint32_t,std::string_view)> styledText; // 현재 기본 문맥으로 원본 서식을 해석한다.
    std::function<void(std::uint32_t,bool)> idle,redraw; // 같은 idle 등록표와 가상+8이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 가상+20의 네 인자다.
    std::function<void(std::uint32_t,std::uint32_t)> reference; // 00464540 객체 부착 경계다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> deleteText; // 기존 TextGump scalar 소멸/반납이다.
};
// 별도 타이머/문자 출력/등록표를 만들지 않고 기존 원본 모듈을 조립한다.
class FloatingSpGump {
public:
    // create와 안정된 노드/훅 저장소는 처리기보다 오래 살아야 한다.
    FloatingSpGump(GumpCreateState& create,FloatingSpGumpHooks hooks);
    // 0043db70: 한 줄 생성→속도/가속도→원본 서식/측정→idle→현재 시각/깊이다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t cost,float acceleration) const;
    // 0043dd20: new0xb8/가속도-2/선택 객체 참조/실제 루트 Attach(mode0)다.
    std::uint32_t Show(std::int32_t cost,std::int32_t x,std::int32_t y,std::uint32_t reference=0) const;
    // 0043dc50: double 시간 차이를 보관하고 이동 뒤 범위 검사/속도 갱신 또는 idle 해제/Kill한다.
    void Idle(std::uint32_t token) const;
    // 0043e620→004d3080→004bc790: 기존 TextGump 소멸과 하위 BYTE bit1 반납이다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
private:
    FloatingSpGumpNode& Node(std::uint32_t token) const; // 실제 사용 시 현재 노드를 조회한다.
    GumpCreateState& create_; // 같은 현재 화면 크기와 루트다.
    FloatingSpGumpHooks hooks_; // 같은 실제 처리기/가상 효과다.
};
}
