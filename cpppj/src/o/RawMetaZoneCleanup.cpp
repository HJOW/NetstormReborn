// 고정 표의 DWORD 순서·첫 관문과 선택 자식의 부분효과를 원문대로 유지한다.
#include "o/RawMetaZoneCleanup.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 실제 rep stosd의 한 DWORD 폭을 검사하고 앞서 완료한 쓰기는 남긴다.
void ZeroMetaRecords(std::span<std::uint8_t> records) {
    for (std::size_t offset=0;offset<kMetaZoneCleanupBytes;offset+=4) { // 원문0x72c DWORD만 낮은 주소부터 쓴다.
        if (offset>records.size() || 4>records.size()-offset) throw std::out_of_range("메타구역 정리의 실제 DWORD 물리 부족");
        for (std::size_t byte=0;byte<4;++byte) records[offset+byte]=0; // 현재 DWORD 전체 폭 이외에는 쓰지 않는다.
    }
}
}
// 인자 바인딩은 초기화 효과가 없으며 선택한 하위만 실제 호출 위치에서 요구한다.
void ClearMetaZoneRecords(const MetaZoneCleanupGlobals& globals,const MetaZoneCleanupHooks& hooks) {
    if (globals.firstReset!=0) {
        ZeroMetaRecords(globals.records);
        globals.firstReset=0;
        return;
    }
    for (std::uint32_t index=0;index<kMetaZoneCleanupCount;++index) { // callback의 flag 변경에도 진입 때 선택한51회 분기를 유지한다.
        if (!hooks.clearMarker) throw std::logic_error("메타구역 정리의 실제 marker 자식 미연결");
        hooks.clearMarker(index);
    }
    ZeroMetaRecords(globals.records);
}
}
