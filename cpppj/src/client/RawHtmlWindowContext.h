// 실제 Client의 HTML 전역/원시 입력 큐/벽 시각을 공통 UI에 빌린다.
#pragma once
#include "client/ClientHtmlWindowGlobals.h"
#include <memory>
namespace netstorm::client { // HTML 부모의 실제 외부 경계 계약이다.
struct RawHtmlWindowContext { // 같은 Client/공통 UI보다 오래 살아야 하는 실제 외부 장치다.
    std::shared_ptr<ClientHtmlWindowGlobals> globals; // 표시 부모가 닫혀도 유지되는 실제 Client 상태다.
    std::function<double()> wallSeconds; //460d70의 현재 벽 시각이며 게임 시계와 구별한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue;std::function<void(std::uint32_t)> link; //452f90의 원시 사건과4a00c0의 현재 문자열 처리다.
    std::function<std::uint32_t(std::uint32_t)> read; // 현재 Client의 뷰 폭/출력·전체 크기/지도 직사각형 DWORD를 읽는다. 미설치 PE0으로 대체하지 않는다.
};
}
