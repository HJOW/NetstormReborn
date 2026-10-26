// 원문0044d870은 진단 뒤 받은 이름표 포인터와 고정manager를 끝까지 보존한다.
#pragma once
#include "o/RawSnart.h"
namespace netstorm::o { // 초기화 반복의 제어와 현재 실제 이름표만 담당한다.
enum class DataManagerInitializeAssertion : std::uint32_t { RootNotEmpty=0xc9,NameSeparator=0x46 }; // 실제 Datamanager.cpp/Snart.cpp 진단이다.
struct DataManagerInitializeHooks { // 큰 실제stream 생성은 같은 고정manager의 어댑터에 인계한다.
    std::function<void(DataManagerInitializeAssertion)> assertion; // 진단에서 반환하면 그 뒤 현재 표를 얻는다.
    std::function<std::span<std::uint8_t>()> names; // 가상+4의 현재 실제 표이며 한 번만 호출한다.
    std::function<void(std::span<const std::uint8_t>)> create; // 같은 이름 시작 포인터/물리범위다. 실제0044d1b0에 고정manager를 연결한다.
};
void InitializeDataManager(std::span<std::uint8_t> manager,const DataManagerInitializeHooks& hooks); // root검사→이름표1회조회→현재이름/실제Create→마지막빈이름 반환이다.
}
