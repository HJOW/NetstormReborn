// 원본의 현재 메뉴 수/현재 캐시 개수 재조회와 SOLO 분기만의 정상 복구를 유지한다.
#include "client/TellMapMenus.h"
#include "o/OriginalText.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 선택된 외부 경계만 확인하며 누락 장치를 빈 성공으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Tell 맵 목록의 현재 장치 미연결");return function;
}
}
// Begin의 콜백이 현재 메뉴 수를 바꿔도 원본처럼 복귀 뒤 다시 읽어 증가시킨다.
bool AppendTellMapMenus(std::string_view label,std::uint32_t& menuCount,const TellMapMenuHooks& hooks) {
    const auto begin=[&] { Need(hooks.beginVertical)(kTellMenuFlags);++menuCount; }; // 같은 Tell의+21c8에 쓰는 시점을 유지한다.
    if (!menuCount) begin();
    const auto lower=o::AsciiLower(label);const bool solo=lower=="*sololist*";
    if (!solo && lower!="*fortlist*") return false;
    Need(hooks.invalidate)();Need(hooks.selectFileSpec)(solo);Need(hooks.refresh)(solo ? 2 : 0);
    std::int32_t index=0;
    // Refresh의 과거 개수 대신 각 성공 행 추가 뒤 현재 개수로 분할 지점을 검사한다.
    while (Need(hooks.appendRow)(index)) {
        ++index;const auto count=Need(hooks.count)();
        if (count>15 && index==count/2) begin();
    }
    if (solo) { Need(hooks.invalidate)();Need(hooks.selectFileSpec)(false); } // FORT는 행 효과가 바꾼 dirty/키를 보존한다. 예외 시 복구는 원문에 없다.
    return true;
}
// 파일/이름/번역/선택 표시는 이미 복원된 목록 몸체에 맡기고 메뉴 부모만 조립한다.
TellMapMenuHooks MakeTellMapMenuHooks(SoloMapCatalog& catalog,SoloMapFileSpec& fileSpec,
    std::function<std::string_view()> current,std::function<std::string_view()> fallback,
    std::function<void(std::uint32_t)> begin,std::function<void(const o::SoloMapRow&,std::uint32_t)> add) {
    TellMapMenuHooks hooks;hooks.beginVertical=std::move(begin);
    hooks.invalidate=[&catalog] { catalog.Invalidate(); };
    hooks.selectFileSpec=[&fileSpec](bool solo) { if (solo) fileSpec.SelectSolo();else fileSpec.SelectFort(); };
    hooks.refresh=[&catalog](auto mode) { static_cast<void>(catalog.Refresh(mode)); };
    hooks.count=[&catalog] { return catalog.Cached().count; };
    hooks.appendRow=[&catalog,current=std::move(current),fallback=std::move(fallback),add=std::move(add)](auto index) {
        const auto row=catalog.Row(index,Need(current)(),Need(fallback)());if (!row) return false;
        Need(add)(*row,kReadFortCommand);return true;
    };return hooks;
}
}
