// 같은 Client의 실제 임시 고리와 이전 할당 수명을 메뉴/Editor에서 공유한다.
#pragma once
#include "o/RawStaticStrings.h"
#include "o/RawFileName.h" // 범용 임시 할당과 별개인 원본10칸 경로 고리를 같은 Client가 소유한다.
#include <map>
#include <memory>
#include <string_view> //연속 저장의 두 파일명을 같은 원시 문자열 폭으로 빌린다.

namespace netstorm::client { // Native 알고리즘은 RawStaticStrings 본체만 실행한다.
class ClientStaticStrings {
public:
    ClientStaticStrings()=default; // 최초 네 BSS0만 만들고 실제8192 할당은 원문 첫 예약 위치에서 실행한다.
    ClientStaticStrings(const ClientStaticStrings&)=delete;ClientStaticStrings& operator=(const ClientStaticStrings&)=delete; // 고정 슬롯/할당 주소와 callback을 복제하지 않는다.
    o::StaticStringGlobals Globals(); // 같은 네 DWORD를 실제 본체와 관측에 빌린다.
    o::RawFileName& FileNames(); // HUD 교체/미션 전환에서 초기화하지 않는 별도10칸 경로 작성기를 빌린다.
    void Initialize(std::uint32_t size); // 원문 공통 시작은10240이며 lazy 예약의8192와 구별한다. 재호출도 원문 진단/재할당 순서다.
    std::span<std::uint8_t> Reserve(std::uint32_t size); // 예약 폭 뒤도 같은 실제 할당 안에서는 쓸 수 있다. 임의 절단하지 않는다.
    std::span<const std::uint8_t> FormatInteger(std::uint32_t size,std::span<const std::uint8_t> pattern,std::int32_t value); // 실제 CRT의 한 정수 서식 장치와 원문 예약/alias 생략을 사용한다.
    std::span<const std::uint8_t> FormatCaptureRange(std::string_view first,std::string_view last); //원문 완료 안내의80예약/고정 Wrote 서식·두 파일명을 공유 고리에 쓴다.80으로 절단하지 않는다.
private:
    struct Allocation { std::unique_ptr<std::uint8_t[]> bytes;std::uint32_t size; }; // 실제 할당과 요청 폭을 함께 소유하며 재초기화로 이전 할당을 임의 해제하지 않는다.
    o::StaticStringHooks Hooks(); // 같은 고정 owner에 실제 할당/순수 물리/진단 장치를 연결한다.
    std::uint32_t Allocate(std::uint32_t size); // host 포인터를 절단하지 않는 실제 할당과 비영 원문 주소 토큰이다.
    std::span<std::uint8_t> Memory(std::uint32_t address,std::uint32_t size); // 해당 원문 접근의 전체 물리 폭만 확인한다.
    std::span<std::uint8_t> Tail(std::uint32_t address); // 현재 할당 끝까지 같은 물리를 빌리며 끝 이후를 사본으로 늘리지 않는다.
    std::map<std::uint32_t,Allocation> allocations_; // 오래 빌린 문자열도 원래 할당의 같은 주소를 유지한다.
    o::RawFileName fileNames_; // 원문0055a760/0055b160이며 RawStaticStrings의 가변 할당과 섞지 않는다.
    std::uint32_t size_{},base_{},current_{},last_{},nextBase_{0x20000000U}; // 원문 네 DWORD와 host의 실제 할당 토큰 발급기다.
};
}
