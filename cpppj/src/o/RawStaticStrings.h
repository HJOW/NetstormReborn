// Staticstring.cpp의 실제 공유 고리 초기화/예약/서식 외곽을 같은 네 DWORD에 실행한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o { // Native 주소는 장치 토큰이며 host 포인터로 절단하지 않는다.
struct StaticStringGlobals { //005c8940/44/48/4c의 실제 단일 저장소를 빌린다.
    std::uint32_t& size;std::uint32_t& base;std::uint32_t& current;std::uint32_t& last; // 각 하위 복귀 뒤 원문 위치에서 현재값을 다시 읽는다.
};
struct StaticStringHooks { // 할당과 순수 물리 대여/진단/CRT만 외부 장치다.
    std::function<std::uint32_t(std::uint32_t)> allocate; // 실제 operator new이며 반환0도 원문 뒤 물리 접근에서 판정한다.
    std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> memory; // 한 원문 load/store의 같은 물리 폭이다. 조회 자체의 상태 변경은 허용하지 않는다.
    std::function<void(std::uint32_t)> assertion; // 이미 있는 base의 Staticstring.cpp17 진단이다.
    std::function<void(std::uint32_t,std::uint32_t)> format; //004de490의 같은 목적/서식 주소다. 필요한 값인자 수명은 실제 호출자가 소유한다.
};
void InitializeStaticStrings(std::uint32_t size,StaticStringGlobals globals,const StaticStringHooks& hooks); //004b92b0: 새 할당→DWORD/BYTE0→첫 D5→current+1이며 last는 보존한다.
std::uint32_t ReserveStaticString(std::uint32_t size,StaticStringGlobals globals,const StaticStringHooks& hooks); //004b9320: 최초8192·unsigned >= wrap·last/end D5/current 순서를 보존한다.
std::uint32_t FormatStaticString(std::uint32_t size,std::uint32_t pattern,StaticStringGlobals globals,const StaticStringHooks& hooks); //004b9390: 예약 결과와 pattern 주소가 같으면 실제 CRT 호출도 생략한다.
}
