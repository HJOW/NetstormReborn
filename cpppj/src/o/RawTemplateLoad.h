// Template manager 생성자와 기존 파일을 유지하는 바깥 Load의 원본 제어 순서다.
#pragma once
#include "o/RawTemplateLifetime.h"
#include <string_view>

namespace netstorm::o {
// 004bcc60: manager276의 지정 DWORD와 NUL까지의 이름만 쓴다. global은005c89c0의 같은4BYTE다.
// 파일명 입력은 출력 manager/global과 겹치지 않아야 하며256BYTE 밖 원본 쓰기는 예외로 거부한다.
void ConstructTemplateDataManager(std::span<std::uint8_t> manager,std::string_view fileName,std::span<std::uint8_t,4> global);
// 가상+0의004bcca0: 같은 manager+12의 NUL 종료 파일명을 빌린다. 이름이 바뀌면 뷰도 같은 바이트를 본다.
std::string_view TemplateDataManagerFileName(std::span<const std::uint8_t> manager);

// 원본 하위 호출의 효과를 같은 헤더/manager 소유자에 연결한다. 미선택 경계는 요구하지 않는다.
struct TemplateLoadHooks {
    std::function<std::string_view(std::uint32_t)> fileName; // 현재 manager 가상+0의 같은 파일명이다.
    std::function<bool(std::string_view,std::string_view)> sameName; // CRT stricmp==0 비교이며 파일명 문자열의 별도 사본을 만들지 않는다.
    std::function<void()> wrongName; // isCalled(_fileName) 진단 뒤 원본은 자연 반환한다.
    std::function<std::uint32_t(std::uint32_t)> allocate; // 실제 operator_new(276)의 DWORD 포인터/NULL이다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> construct; // 생성자 뒤 EAX로 돌려준 같은 manager 포인터다.
    std::function<bool(std::string_view)> readOnly; // 0041af80의 읽기 전용 판정이다. 파일 존재 판정과 다르다.
    std::function<bool(std::uint32_t)> readFile; // 현재 header.ptr에 대한0044d980이다.
    std::function<bool()> isCurrent; // this==Template9[currentIndex]의 부작용 없는 현재 주소 비교다.
    std::function<void()> refreshTitle; // 선택된 현재 Template의00460730 창 제목 갱신이다.
    TemplateLifetimeHooks lifetime; // 읽기 실패 시 base DataManager 소멸과 저장 포인터 free를 순서대로 호출한다.
};
// 004bf550: 기존 같은 파일은 그대로 두고, 빈 헤더에만 할당/읽기를 수행한다.
// 실패는 ptr만0으로 쓴다. C++ 예외 정리 정책은 실제 소유 어댑터가 담당하며 원본 정상 반환과 구분한다.
void LoadTemplateFile(std::span<std::uint8_t> header,std::string_view fileName,const TemplateLoadHooks& hooks);
}
