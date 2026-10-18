// 원본43d330의 생산 내용물 정보 메뉴를 같은 현재 SID와 작성기에 연결한다.
#pragma once
#include "client/InventoryItemGump.h"
#include <optional>
namespace netstorm::client {
// 원본 종류/명령 전역이다. 내용물 클래스와 표시 종류를 구별한다.
struct InventoryAboutState {
    std::uint32_t suppressedClass{122},none{164},dais{165},fullValue{154}; // 회수 제외 클래스/개수 보정 종류/전액 종류다.
    std::uint32_t salvageCommand{0x0200005d},helpCommand{0x02000017}; // 실제 회수와 Quick Help 명령이다.
};
// 게임 조회와 메뉴 작성/가상 부착을 실제 현재 소비자에 연결한다.
struct InventoryAboutHooks {
    std::function<std::uint32_t()> begin,root,templateOwner; // 현재 fancy 시작/등록 루트/템플릿 소유자다.
    std::function<bool()> editor; // 현재 편집 DWORD의 참/거짓이다.
    std::function<std::int32_t(std::uint32_t)> cost; // 같은 현재 정수 비용의 복호화 값이다.
    std::function<bool(o::Sid)> affordable; // 기존4af010의 현재 비용/SP/편집 비교다.
    std::function<std::optional<std::string>()> helpWord; // NULL과 빈 문자열을 구별하는 현재 바이트다.
    std::function<void(std::uint32_t,bool)> idle; // 같은 실제 idle 등록표다.
    std::function<void(std::uint32_t,std::uint32_t)> reference; // 부착 전에 내용물 Form을 연결한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> describe; // 기존4ad780의 네 인자다.
    std::function<std::string(std::string_view)> translate; // 원본 조회 시점의 번역이다.
    std::function<std::string(std::string_view,std::int32_t)> print; // 번역 없이 정수 한 개만 서식화한다.
    std::function<void(std::string_view)> header,sound; // 실제 제목 머리와 원본 열기 음향이다.
    std::function<void(std::optional<std::string_view>,std::uint32_t,std::uint32_t,bool,bool,bool)> entry; // NULL까지 보존하는 실제477cb0 경계다.
    std::function<void(ScreenPoint,std::uint32_t)> attach; // 현재 controlled 메뉴를 가상 부착하고 꺼낸다.
};
// 문자열/메뉴/시계 사본 없이 원본의 현재 읽기와 작성 순서를 복원한다.
class InventoryAboutMenu {
public:
    InventoryAboutMenu(const o::SidPool& pool,InventoryAboutState& state,InventoryAboutHooks hooks); // 같은 슬롯과 현재 전역을 빌린다.
    std::uint32_t Open(const InventoryItemNode& item,ScreenPoint point) const; //43d330의 두 화면 좌표와 입력 노드를 사용한다.
private:
    std::uint32_t Word(o::Sid sid,std::size_t offset) const; // 같은 현재 비정렬 WORD를 읽는다.
    std::int32_t Refund(std::uint32_t kind) const; //43cb30(kind,0)→_ftol의 넓은 정밀도를 유지한다.
    const o::SidPool& pool_;InventoryAboutState& state_;InventoryAboutHooks hooks_; // 단일 현재 풀/전역/효과다.
};
}
