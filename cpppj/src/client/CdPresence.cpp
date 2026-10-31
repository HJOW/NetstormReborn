// 원문 strlen의 캡처 시점·현재 BYTE 재조회·부분 작성·중단 앞선 효과를 유지한다.
#include "client/CdPresence.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { // 잘못된 물리를 임의0/성공으로 바꾸지 않는 원문 몸체다.
namespace { // 선택된 장치와 현재 C문자열의 실제 물리만 확인한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("CD 확인의 선택된 실제 장치 미연결");return callback; } // 미선택 장치는 요구하지 않는다.
std::string_view Text(std::span<const char> bytes,std::size_t begin=0) { // 첫 NUL까지의 현재 source를 빌리며 뒤 BYTE는 바꾸지 않는다.
    if (begin>=bytes.size()) throw std::out_of_range("CD 현재 C문자열 시작 물리");const auto first=bytes.begin()+static_cast<std::ptrdiff_t>(begin),end=std::find(first,bytes.end(),'\0');if (end==bytes.end()) throw std::out_of_range("CD 현재 C문자열 NUL 물리");return {bytes.data()+begin,static_cast<std::size_t>(end-first)};
}
void Require(std::size_t at,std::size_t size,std::size_t capacity) { if (at>capacity || size>capacity-at) throw std::out_of_range("CD 원문 복사/쓰기 실제 물리 범위"); } // capacity 인자와 호스트의 실제 쓰기 물리를 혼동하지 않는다.
}
// CD 루트가 출력에 못 들어가도 필요한 길이를 누적하며 끝에서 첫BYTE를0으로 만든다.
std::uint32_t CollectCdDriveRoots(std::uint32_t capacity,std::span<char> output,std::span<char,128> scratch,const CdDriveRootsHooks& hooks) {
    if (!output.data()) Need(hooks.assertion)(0x529);const auto result=Need(hooks.logicalDrives)(scratch);if (!result || result>128U) return 0;
    std::uint32_t used{};std::size_t cursor{};
    if (!scratch[0]) return 0;
    for (;;) { // 각 루트의 길이는 GetDriveType 전에 캡처하고 복사 BYTE는 그 콜백 뒤 읽는다.
        const auto root=Text(scratch,cursor);const auto length=root.size()+1;auto next=used;
        if (Need(hooks.driveType)(root)==5U) { next=used+static_cast<std::uint32_t>(length);if (next<capacity) { Require(used,length,output.size());std::copy_n(scratch.data()+cursor,length,output.data()+used); } }
        cursor+=length;used=next;Require(cursor,1,scratch.size());if (!scratch[cursor]) break;
    }
    if (!used) return 0;const auto required=used+1U;
    if (capacity<required) { Require(0,1,output.size());output[0]=0;return required; }
    Require(used,1,output.size());output[used]=0;return required;
}
// 캐시와 stack은 같은 원문 수명이며 source 길이 캡처 뒤 콜백 변이도 실제 읽기 위치에서 반영한다.
std::uint32_t CheckCdPresence(std::span<char> cache,std::span<char,128> scratch,const CdPresenceHooks& hooks) {
    Require(0,1,cache.size());
    if (cache[0]) { const auto path=Need(hooks.path)(Text(cache),kCdPresenceMarker);if (Need(hooks.exists)(path)) return 1;cache[0]=0; }
    if (!Need(hooks.roots)(scratch)) return 0;std::size_t cursor{};
    while (scratch[cursor]) { // 실제440cf9는 다음 루트 첫BYTE를 AL에 읽는다. 디컴파일의 local_80[0] 대입과 달리 stack을 쓰지 않는다.
        const auto root=Text(scratch,cursor);const auto length=root.size()+1;const auto path=Need(hooks.path)(root,kCdPresenceMarker);
        if (Need(hooks.exists)(path)) {
            for (std::size_t index=0;;++index) { Require(cursor+index,1,scratch.size());Require(index,1,cache.size());const auto byte=scratch[cursor+index];cache[index]=byte;if (!byte) break; } // 존재 콜백 뒤 현재 source를 첫 NUL까지 전방 복사한다.
            return 1;
        }
        cursor+=length;Require(cursor,1,scratch.size());
    }
    return 0;
}
}
