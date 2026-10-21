// 같은 DataManager의 임시 stream 생성·이름 교체·최소 길이 확장을 원본 순서로 처리한다.
#pragma once
#include "o/RawDataManagerSelection.h"
#include "o/RawDataManagerList.h"
#include "o/RawMemStream.h"
#include <optional>
namespace netstorm::o {
// 모든 조회는 같은 노드/버퍼를 빌려야 하며 각 경계 뒤 현재 원문을 다시 읽는다.
struct DataManagerResizeHooks {
    DataManagerSelectionHooks selection; // 같은 head/current와60BYTE 노드의 이름 검색이다.
    DataManagerListHooks list; // 같은 노드의 추가/압축/제거/실제 반납이다.
    MemStreamHooks memory; // 같은 실제 버퍼의 할당/쓰기/조회다.
    std::function<std::uint32_t(std::uint32_t)> allocateStream; // 원본 new60의 실제 할당과 반환 핸들이다.
    std::function<void()> missingSource; // 0044d290의 원문 섹션 누락 진단이다. 잘못된0포인터를 정상값으로 바꾸지 않는다.
};
// 0044d070: 첫16BYTE 초기화→최소1 할당→현재 이름/NUL 복사→next0, 같은 this 핸들을 반환한다.
std::uint32_t ConstructDataManagerStream(std::uint32_t stream,std::optional<std::string_view> name,std::int32_t size,const DataManagerResizeHooks& hooks);
// 0044d160: 대상 이름이 있으면 제거한 뒤 원래 이름을 찾고 대상 문자열/NUL만 쓴다.
std::uint32_t RenameDataManagerStream(std::span<std::uint8_t> manager,std::string_view name,std::string_view previousName,const DataManagerResizeHooks& hooks);
// 0044d290: signed end가 최소보다 작을 때만 임시 생성/현재 원문 복사/0 채움/추가/이름 교체를 수행한다.
std::uint32_t EnsureDataManagerStreamSize(std::span<std::uint8_t> manager,std::string_view name,std::int32_t minimum,const DataManagerResizeHooks& hooks);
}
