// 공통 Form Pop의 진단 후 상태/현재 권한과 공유 postPop 깊이를 연결한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 실제 Form +70과 공통 부착의 직접 Pop 호출이 공유하는 부모다.
struct FormPopHooks { // 선택된 자식만 요구하고 원문 저장값과 현재값을 구별한다.
    std::function<bool()> server; // 진단 복귀 뒤 현재 서버 권한을 읽는다.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> transmit; // 고정 -1/-1/0과 정규화 flags를 현재 전송 자식에 전달한다.
    std::function<std::uint32_t()> depth; // 현재 공통 basePostPop DWORD를 읽는다.
    std::function<void(std::uint32_t)> setDepth; // 원문 DWORD 증가/감소를 같은 장기 저장소에 쓴다.
    std::function<void(Sid,std::uint32_t,std::uint32_t)> postPop; // 증가 후 현재 VT+20과 flags를 선택한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 식/파일/행의 정상 복귀를 허용한다.
};
void PopCommonForm(SidPool& pool,Sid form,std::uint32_t flags,const FormPopHooks& hooks); // PATCH004ac560/CD004abfb0 부모만 실행하며 원문에 없는 선행 guard를 추가하지 않는다.
}
