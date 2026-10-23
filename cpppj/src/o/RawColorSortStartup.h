// 원문43b430/43b300 생성 부모의 현재 슬롯·호출순서만 복원하며 하위 실제 계산은 필수 경계로 분리한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 같은 Client 수명의 실제 pointer/count/loaded DWORD를 직접 빌린다.
struct ColorSortStartupGlobals {
    std::uint32_t& primary; //00555d88의 현재0x3800 ColorSort 주소다.
    std::uint32_t& search; //00555dd8의 현재0x5008 검색객체 주소다.
    std::uint32_t& gradient; //00555d78의 현재0x5020 기본색띠 주소다.
    std::uint32_t& count; //0053199c는첫부모가10을쓰며색띠할당뒤현재값을캡처한다.
    const std::uint32_t& paletteLoaded; //00555e08이0이면진단뒤계속한다.
};
// 토큰은 실제 host 포인터가 아니며 생성·정렬·색띠는 원문 하위와 같은 필수 구현을 연결해야 한다.
struct ColorSortStartupHooks {
    std::function<void(std::uint32_t)> assertion; //Colorsort.cpp451의paletteLoaded진단이다.
    std::function<void(std::uint32_t)> release; //현재비영슬롯을free하며0쓰기는부모에남긴다.
    std::function<std::uint32_t(std::uint32_t)> allocate; //0x3800/0x5008/0x5020의실제할당결과다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> construct; //439de0(this,같은logical포인터)의실제반환주소다.입력은이시점현재내용이다.
    std::function<void(std::uint32_t)> constructSearch; //43a8e0(this)는자체진입에서현재primary를읽는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> memory; //직접vtable store를위한같은실제할당의안정된뷰다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> sort; //qsort(search+8,256,80,0043a1e0)의실제정렬이다.
    std::function<std::uint32_t(std::uint32_t,std::array<double,3>,std::array<double,3>,std::uint32_t,double)> constructGradient; //43ad20(this,from,to,현재count,2.4)의실제계산·반환주소다.
};
//43b300:현재gradient조건부free/0→new→현재count/고정RGB→실제색띠생성→반환주소대입이다.
void InitializeColorSortGradient(ColorSortStartupGlobals globals,const ColorSortStartupHooks& hooks);
//43b430:loaded진단→두현재슬롯free/0→primary생성→검색생성/vtable/qsort→search대입→count10→색띠부모다.
void InitializeColorSort(std::uint32_t logical,ColorSortStartupGlobals globals,const ColorSortStartupHooks& hooks);
}
