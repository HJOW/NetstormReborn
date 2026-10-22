// 실제 파일 시스템과 Template/State 수명을 원본 단독 맵 목록에 연결한다.
#pragma once
#include "o/RawSoloMapList.h"
#include <memory>
#include <span>

namespace netstorm::o { class BaseFileSystem;class ConfigInterface; }
namespace netstorm::client {
// 게임 규칙 대신 현재 호스트/언어/사용자 자료만 제공하는 경계다.
struct SoloMapCatalogEffects {
    std::function<std::int32_t()> typeCount; // 파일 서명 검사가 읽는 현재 타입 개수다.
    std::function<void(std::span<std::uint8_t>)> allocationBytes; // 빈 State가 보존하는 현재 미정 바이트 입력이다.
    std::function<std::string()> userName; // 원본 00540ca8의 현재 좁은 사용자 이름이다.
    std::function<std::string(std::string_view)> translate; // 손상 행 형식의 현재 좁은 번역이다.
};
// 파일마다 분류와 목록 메타 조회를 다른 임시 Template에서 수행한다.
class SoloMapCatalog {
public:
    SoloMapCatalog(const o::BaseFileSystem& files,o::ConfigInterface& config,SoloMapCatalogEffects effects); // 파일/설정/효과는 이 객체보다 오래 살아야 한다.
    ~SoloMapCatalog(); // 임시 manager와 문자열 캐시의 실제 소유 할당을 반납한다.
    SoloMapCatalog(const SoloMapCatalog&)=delete; // 고정 주소의 콜백/저장소 참조를 복사하지 않는다.
    SoloMapCatalog& operator=(const SoloMapCatalog&)=delete; // 같은 캐시와 임시 Template의 중복 소유를 막는다.
    std::uint8_t Kind(std::string_view name); // 00460c40처럼 새 Template에서 실제 State+17을 읽고 반납한다.
    std::int32_t Refresh(std::int32_t mode); // 실제 fortSpec 열거와 종류 필터를 원본 캐시 처리기에 인계한다.
    std::optional<o::SoloMapRow> Row(std::int32_t index,std::string_view current,std::string_view fallback) const; // 현재 선택 문자열로 원본 표시 행을 조회한다.
    void Invalidate(); // 다음 Refresh의 원본 dirty 정리를 예약한다. 즉시 캐시를 바꾸지 않는다.
    const o::SoloMapListState& Cached() const; // 실제 목록 상태를 읽기 전용으로 조회한다.
private:
    struct Impl; // 임시 실제 manager와 State getter의 고정 수명을 함께 소유한다.
    std::unique_ptr<Impl> impl_; // 복원한 목록과 그 하위 장치를 같은 주소에서 유지한다.
};
}
