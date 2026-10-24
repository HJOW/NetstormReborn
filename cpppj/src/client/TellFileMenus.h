// 원본4ce1b0(@)/4ce580(#)의 파일 설정 목록과 실제 행 생성 수명을 공유한다.
#pragma once
#include "client/ScriptCommand.h"
#include <memory>

namespace netstorm::client { // Tell 파일 목록의 장기 Config와 현재 메뉴 행 계약이다.
struct TellFileMenuItem { // 라벨과 인자는 원본 글꼴/고리의 좁은 바이트다.
    std::string label;std::uint32_t command{},parameter{};bool separator{}; // 구분선에는 명령/인자 고리 효과가 없다.
};
class TellFileMenus { // 한 Client에서 적재한 이름별 Config를 페이지보다 오래 보유한다.
public:
    TellFileMenus(o::ConfigInterface& config,std::function<std::vector<std::string>(std::string_view)> enumerate); // 실제 파일 열거는 UTF-8 경로를 원문 OS 순서로 공급한다.
    ~TellFileMenus(); // 소유 Config를 같은 실제 Registry에서 반납한다.
    TellFileMenus(const TellFileMenus&)=delete;TellFileMenus& operator=(const TellFileMenus&)=delete; // 등록된 고정 Config 주소를 복제하지 않는다.
    void Append(std::string_view pattern,std::uint32_t command,std::string_view argument,bool emptyTitleUsesName,
        ScriptStringRing& strings,const std::function<void(const TellFileMenuItem&)>& emit); // @는 빈 제목을 이름으로 대체하고 #는 빈 제목을 보존한다. 각 행의 cur.file은 emit이 반환할 때까지 유지한다.
    std::string Inline(std::string_view pattern,std::string_view command,std::string_view argument); // %의80개 prefix/두 번 치환·행별 ERROR 복구를 유지한다. 물리 범위 거부와 수집/제목 오류는 호출자에게 전달한다.
private:
    std::vector<o::Config*> Collect(std::string_view pattern,std::size_t capacity); // 고유 prefix 수집/등록·원문 정렬을 공유하되 @/#40개와 %80개 지역 배열을 구별한다.
    o::ConfigInterface& config_; // 현재 실제 등록부/치환/파일 읽기를 빌린다.
    std::function<std::vector<std::string>(std::string_view)> enumerate_; // 원문 순서의 현재 디스크 파일 장치다.
    std::vector<std::unique_ptr<o::Config>> owned_; // 새 설정의 prefix 주소와 메타 path/fileName/index는 목록 교체 뒤에도 유지한다.
};
}
