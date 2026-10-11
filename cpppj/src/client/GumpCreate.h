// 원본 Gump의 0xa0바이트 기본 필드 초기화·등록과 화면 크기의 루트 생성을 복원한다.
#pragma once
#include "client/GumpCollection.h"

namespace netstorm::client {
// 원본 등록 표 크기와 기본 객체의 할당 크기다. 파생 객체는 별도 필드를 가질 수 있다.
inline constexpr std::size_t kGumpRegistryCapacity=1000,kGumpBaseSize=0xa0;
// 같은 객체의 표시 필드를 상속하므로 수집/Draw의 캐시 쓰기가 생성 저장소에 바로 반영된다.
struct GumpCreateNode: GumpCollectionNode {
    std::uint32_t vtable{},alignFlags{},previous{},id{},command{},form{},serial{},secondaryDepth{}; // +0x28 form은 기본 생성자가 쓰지 않는다.
    float scale{},baseWidth{},baseHeight{};
    std::array<std::uint32_t,6> positioning{}; // +0x54~0x68의 좌표/속도 입력 비트다.
    std::uint32_t untouched{},textPointer{},alternateTextPointer{}; // +0x74는 기본 생성자가 쓰지 않는다.
    std::array<std::uint32_t,8> textContext{}; // +0x80~0x9c: 기본 글자 문맥의 마지막 두 DWORD는 1이다.
};
// 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다.
struct GumpCreateState {
    std::array<std::uint32_t,kGumpRegistryCapacity> entries{};
    std::uint32_t count{},serial{};
    int screenWidth{},screenHeight{};
    GumpCollectionState collection;
};
// node는 안정된 기본/파생 저장소, allocate는 요청 크기에 대응하는 새 저장소 토큰을 제공한다.
// allocate는 현재 전역을 바꿀 수 있다. 0은 원본 new 실패이며 생성자/카운터 갱신을 생략한다.
struct GumpCreateHooks {
    std::function<GumpCreateNode*(std::uint32_t)> node;
    std::function<std::uint32_t(std::size_t)> allocate;
};
// 004642c0/CD 00492da0과 004663d0/CD 00492b30이다. 부모/자식 부착·파생 생성/삭제는 후속이다.
class GumpCreate {
public:
    // 상태/저장소는 생성기와 그 수집 연결보다 오래 살아야 한다. node는 필수다.
    GumpCreate(GumpCreateState& state,GumpCreateHooks hooks);
    // caller가 준비한 저장소를 초기화한다. command/크기/플래그는 원본 네 인자다.
    // 미등록 bit 0x100000은 표/개수를 건드리지 않으며 serial은 모든 생성에서 DWORD로 늘린다.
    // 정상 반환값은 원본 this 포인터에 대응하는 입력 토큰이다.
    std::uint32_t Construct(std::uint32_t token,std::uint32_t command,float width,float height,std::uint32_t flags) const;
    // 표만 비운 뒤 new(0xa0)→현재 화면 크기의 기본 생성→표 +0/루트 갱신을 수행한다.
    // count/serial/overrideHead와 이전 노드 저장소는 그대로 남는다. 수명 정리는 호출자 책임이다.
    std::uint32_t InitializeRoot() const;
    // 현재 기본/파생 노드를 같은 표시 저장소로 공급한다. 투영은 별도 현재 게임 경계다.
    GumpCollectionHooks CollectionHooks(std::function<ScreenPoint(std::uint32_t)> project={}) const;
private:
    GumpCreateState& state_;
    GumpCreateHooks hooks_;
};
}
