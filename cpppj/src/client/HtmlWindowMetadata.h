// 원문4c83b0의 실제 HTML 메타 우선순위/현재 부모/원시 버퍼·명령 수명을 연결한다.
#pragma once
#include "client/HtmlWindowGump.h"
#include "client/ScriptCommand.h"
namespace netstorm::client { // 실제 HTML 메타 처리의 상태/장치 계약이다.
struct HtmlWindowMetadataHooks { // 게임 계산 결과 대신 실제 공통 UI와 선택된 하위 장치만 공급한다.
    std::function<HtmlWindowGumpNode*(std::uint32_t)> node;std::function<void(std::uint32_t)> clearButtons; // 같은 현재 부모와4c7c10이다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t,std::uint32_t,std::uint32_t)> addButton; //4c7c60의 실제 label/command/parameter/Instant다.
    ScriptCommandHooks commands;std::function<std::uint32_t(std::string_view)> storeText; // 현재4d52c0 진단0과4c7910의 같은80칸 고리다.
    std::function<void(std::uint32_t)> beginMenu;std::function<void(std::string_view,std::uint32_t,std::uint32_t)> menuText; // 현재 세로 열/일반 항목이다.
    std::function<void(std::int32_t,std::string_view,std::uint32_t,std::uint32_t,bool)> checked; // 실제 체크값·라벨·해석 명령과 현재 enabled다.
    std::function<void(std::string_view,std::uint32_t&)> maps;std::function<void(std::uint32_t)> tech;std::function<void(std::uint32_t,std::int32_t)> themeTech; // 실제 맵/기술 목록이며 열 수는 같은 부모 필드를 빌린다.
    std::function<void(std::string_view,std::uint32_t,std::string_view)> namedList,fileList; // @/#의 현재 키·명령·원시 인자다.
    std::function<std::string(std::string_view,std::string_view,std::string_view)> inlineList; // %의 현재 키·원시 명령/인자로 반환된 실제 문자열이다.
    std::function<double()> wallSeconds;std::function<void(std::uint32_t,bool)> idle;std::function<std::uint32_t(std::uint32_t)> read; // 현재 시각/정상 idle 등록/실제 전역 DWORD다.
    std::function<void(std::int32_t,std::string_view,std::string_view)> input;std::function<void(std::string_view,std::string_view,std::string_view)> send; //4d3ed0과원문 Zsend 패킷의 선택 경계다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue; // Input 뒤 현재 MenuJustClosed raw 사건을 그대로 예약한다.
};
class HtmlWindowMetadata { //9종 고정 검색/현재 필드/원시 줄 제거와 반환을 실제 부모에 수행한다.
public:
    explicit HtmlWindowMetadata(HtmlWindowMetadataHooks hooks); // 같은 부모 조회만 미리 요구한다.
    std::uint32_t Process(std::uint32_t token,std::span<char> buffer) const; // 원문 mutable8192BYTE/첫NUL과 뒤 물리를 보존한다. 버튼 처리 수 또는 현재 열이 있으면1이다.
private:
    HtmlWindowGumpNode& Node(std::uint32_t token) const; // 같은 현재 실제 부모만 읽는다.
    HtmlWindowMetadataHooks hooks_; // 선택 장치/고리/공통 UI의 장기 수명을 빌린다.
};
}
