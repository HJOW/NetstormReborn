// 생산 실패 안내의 원본 TextGump 파생과 두 번역 wrapper를 복원한다.
#pragma once
#include "client/TextGump.h"
namespace netstorm::client {
// +a4는 미작성이고 +a8/+b0/+b4는 벽시각 만료/소유 문자열/기준 노드다.
struct ProductionHintNode: TextGumpNode {
    std::uint32_t reserved{}; // 생성자가 덮어쓰지 않는 +a4다.
    double until{}; // 게임 속도와 독립인 벽시계 만료 시각이다.
    std::uint32_t owned{},reference{}; // 복제 문자열의 소유권과 기준 Gump 토큰이다.
};
// 원본 현재 팔레트/종류 BYTE/마지막 생성 일련번호를 공유한다.
struct ProductionHintState {
    std::int8_t kind{25}; // 540434의 초기 종류 BYTE다.
    std::uint32_t white{7},black{},lastSerial{}; // 5a4058/5bec2c/592988의 현재 값이다.
};
// 기존 TextGump/StyleText/수집/수명/시계에만 위임하는 실제 경계다.
struct ProductionHintHooks {
    std::function<ProductionHintNode*(std::uint32_t)> node; // 같은 기본·파생 저장소를 조회한다.
    std::function<double()> now; // 현재 벽시각을 읽는다.
    std::function<std::uint32_t(std::size_t)> allocate; // 0xb8 파생 저장소를 확보한다.
    std::function<void(std::uint32_t)> constructText,kill; // 기존 Text 생성과 지연 삭제다.
    std::function<std::uint32_t(std::string_view)> duplicate; // NUL 문자열을 별도 소유 토큰으로 복제한다.
    std::function<std::string_view(std::uint32_t)> string; // 현재 소유 바이트를 읽는다.
    std::function<void(std::uint32_t)> free; // 문자열 소유권만 반납한다.
    std::function<std::string(std::string_view)> translate; // 원본 wrapper 위치에서 번역한다.
    std::function<void(std::uint32_t,std::string_view)> style; // 같은 현재 기본 문맥으로 파싱한다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> fit; // 같은 TextGump 측정과 여백이다.
    std::function<void(std::uint32_t,bool)> idle; // 공통 idle 등록표를 바꾼다.
    std::function<ScreenRect(std::uint32_t)> bounds; // 공통 현재 화면 경계를 읽는다.
    std::function<ScreenPoint(std::uint32_t)> position; // 공통 현재 화면 위치를 읽는다.
    std::function<void(std::uint32_t,ScreenPoint)> draw; // 같은 StyleText head를 출력한다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 공통 가상 부착이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> deleteText; // 기존 Text 소멸과 선택 객체 반납이다.
};
// 10.78의 4728d0/4729d0/472a00/472a50/472be0/472d20 처리기다.
class ProductionHintGump {
public:
    ProductionHintGump(GumpCreateState& create,ProductionHintState& state,ProductionHintHooks hooks); // 같은 등록/전역을 빌린다.
    std::uint32_t Construct(std::uint32_t token,std::string_view text) const; // 복제·파싱·측정 뒤 4초 만료를 설정한다.
    std::uint32_t Show(bool empty,std::int32_t x,std::int32_t y,std::uint32_t reference) const; // 두 원본 실패 안내 wrapper다.
    void Idle(std::uint32_t token) const; // 만료 시 같은 지연 삭제를 예약한다.
    void Draw(std::uint32_t token) const; // 경계/위치를 읽고 (3,2) 여백으로 출력한다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const; // 문자열 다음 TextGump/객체 소유권 순서다.
private:
    ProductionHintNode& Node(std::uint32_t token) const; // 도달한 현재 노드를 검사한다.
    GumpCreateState& create_;ProductionHintState& state_;ProductionHintHooks hooks_; // 단일 공통 등록표/전역/효과다.
};
}
