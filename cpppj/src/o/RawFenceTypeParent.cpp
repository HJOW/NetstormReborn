// 진단 후 조건을 캐시하지 않고 동일 슬롯의 BYTE와 현재 전역 DWORD 전체를 다시 비교한다.
#include "o/RawFenceTypeParent.h"
#include <stdexcept>

namespace netstorm::o { // 원문 두 부모의 동일한 분기/효과 순서를 공유한다.
void RunFenceTypeParent(std::span<const std::uint8_t> raw,Sid sid,const std::uint32_t& currentType,
    RawCheckingFlag checking,FenceTypeOperation operation,const FenceTypeParentHooks& hooks) {
    if (raw.size()!=50 || (operation!=FenceTypeOperation::Rebuild && operation!=FenceTypeOperation::Refresh)) throw std::invalid_argument("펜스 타입 부모의 물리 슬롯/작업 오류");
    if (checking && raw[10]!=currentType) {
        if (!hooks.assertion) throw std::logic_error("펜스 타입 원문 진단 장치 미연결");
        hooks.assertion("fence.getType() == sunFenceType","\\Ns\\O\\Fence.cpp",operation==FenceTypeOperation::Rebuild ? 0x60fU : 0x602U);
    }
    if (raw[10]==currentType) {
        if (!hooks.body) throw std::logic_error("펜스 타입 부모의 실제 하위 미연결");hooks.body(sid);
    }
}
}
