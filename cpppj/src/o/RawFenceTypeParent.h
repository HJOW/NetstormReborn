// 원문 썬 바리케이드 두 dispatcher의 현재 타입 진단과 복귀 뒤 재판정을 보존한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 같은 raw 슬롯과 현재 타입 전역을 진단 앞뒤에 읽는다.
enum class FenceTypeOperation { Rebuild,Refresh }; // 457890→457460과457e20→4578f0의 서로 다른 부모다.
struct FenceTypeParentHooks { // 하위 전체 몸체와 진단 장치만 외부에 둔다.
    std::function<void(Sid)> body; // 타입 재판정에 성공했을 때만 같은 기존 하위 몸체를 실행한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 파일/행을 포함한 원문 진단이며 복귀할 수 있다.
};
// raw는 진단 동안 주소/수명이 유지되는 같은50BYTE 슬롯이며 currentType은 좁히지 않는 현재 전역 DWORD다.
void RunFenceTypeParent(std::span<const std::uint8_t> raw,Sid sid,const std::uint32_t& currentType,
    RawCheckingFlag checking,FenceTypeOperation operation,const FenceTypeParentHooks& hooks);
}
