// 실제 Client의 HTML 전역/원시 입력 큐/벽 시각을 공통 UI에 빌린다.
#pragma once
#include "client/ClientHtmlWindowGlobals.h"
#include "client/ScriptCommand.h"
#include "client/SoloMapCatalog.h"
#include <memory>
namespace netstorm::client { // HTML 부모의 실제 외부 경계 계약이다.
struct RawHtmlMetadataServices { // 같은 Client의 원시 명령/문자열 고리와 선택된 목록·입력 장치다.
    ScriptCommandHooks commands;ScriptStringRing* strings{}; // 조회/진단과 실제80칸 고리는 부모보다 오래 살아야 한다.
    SoloMapCatalog* maps{};SoloMapFileSpec* fileSpec{}; // 실제 파일 캐시와 현재9BYTE 키를 공유한다.
    std::function<std::string_view()> currentMap,fallbackMap; // 각 행 작성 시 현재 좁은 이름을 재조회한다.
    std::function<void(std::uint32_t)> tech;std::function<void(std::uint32_t,std::int32_t)> themeTech; // 실제 기술 목록의 선택된 하위 생성기다.
    std::function<void(std::string_view,std::uint32_t,std::string_view)> namedList,fileList; // @/# 목록의 원시 키/명령/인자 경계다.
    std::function<std::string(std::string_view,std::string_view,std::string_view)> inlineList; // % 목록이 반환하는 실제 좁은 문자열이다.
    std::function<void(std::int32_t,std::string_view,std::string_view)> input;std::function<void(std::string_view,std::string_view,std::string_view)> send; // 입력 창과 Zsend 패킷 장치이며 누락을 빈 성공으로 바꾸지 않는다.
};
struct RawHtmlWindowContext { // 같은 Client/공통 UI보다 오래 살아야 하는 실제 외부 장치다.
    std::shared_ptr<ClientHtmlWindowGlobals> globals; // 표시 부모가 닫혀도 유지되는 실제 Client 상태다.
    std::function<double()> wallSeconds; //460d70의 현재 벽 시각이며 게임 시계와 구별한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue;std::function<void(std::uint32_t)> link; //452f90의 원시 사건과4a00c0의 현재 문자열 처리다.
    std::function<std::uint32_t(std::uint32_t)> read; // 현재 Client의 뷰 폭/출력·전체 크기/지도 직사각형 DWORD를 읽는다. 미설치 PE0으로 대체하지 않는다.
    RawHtmlMetadataServices metadata; // 같은 실제 UI에 연결할 메타 장치이며 사용된 분기에서만 요구한다.
};
}
