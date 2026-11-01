// 원문4ddc90의 8비트 BMP 헤더·색표·행을 같은 BaseFile 호출 순서로 기록한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // 픽셀/색표 소유권과 실제 파일 장치는 호출자가 빌려준다.
struct IndexedBitmapInput { // 원문8인자의 파일명 뒤7DWORD이며 효과 뒤의 현재 값을 읽는다.
    std::uint32_t pixels{},x{},y{},width{},height{},pitch{},palette{}; // 원문 포인터 토큰/자르기/크기/행 간격/현재 RGB0 색표다.
};
struct IndexedBitmapScratch { // 미작성 지역 바이트도 입력 물리로 보존한다. 기본0은 호스트 초기값이며 원문 미정 스택의 값이라는 주장이 아니다.
    std::array<std::uint8_t,14> file{}; // BITMAPFILEHEADER의 두번째reserved WORD는 원문이 쓰지 않는다.
    std::array<std::uint8_t,40> info{}; // BITMAPINFOHEADER의 X/Y 해상도 DWORD는 원문이 쓰지 않는다.
    std::array<std::uint8_t,1024> palette{}; // BGR 색표의 네번째BYTE는 각 원문 지역 값을 보존한다.
};
struct IndexedBitmapHooks { // 선택한 파일 경계만 필수이며 메모리 조회는 상태를 바꾸지 않는다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> construct; //41b4c0의 이름/mode1/create1 생성이며 실제 열기 실패 예외를 그대로 전달한다.
    std::function<std::uint32_t()> isOpen; //41aad0의 생성 뒤 현재 FILE 비영 판정이다.
    std::function<void(std::span<const std::uint8_t>,std::int32_t)> write; //41a910의 실제 BYTE/offset-1이며 반환값을 검사하지 않는다.
    std::function<void()> destroy; //41b410의 정상/예외 지역 BaseFile 반납이다.
    std::function<std::span<const std::uint8_t>(std::uint32_t,std::uint32_t,std::uint32_t)> memory; // 토큰/상대DWORD위치/길이에 해당하는 현재 실제 BYTE를 빌린다.
};
// 입력은 파일 생성/쓰기가 끝난 각 원문 소비 위치에서 읽는다. 행 패딩·예약 바이트 초기화를 추가하지 않는다.
void WriteRawIndexedBitmap(std::string_view name,const IndexedBitmapInput& input,IndexedBitmapScratch& scratch,const IndexedBitmapHooks& hooks);
}
