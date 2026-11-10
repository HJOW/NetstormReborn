// 원본의 현재 메뉴 수/현재 캐시 개수 재조회와 SOLO 분기만의 정상 복구를 유지한다.
#include "client/TellMapMenus.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>
#include <string>
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
    const bool keep=hooks.takeKeepCache && hooks.takeKeepCache(); // 5차: 스크롤 재표시이면 목록을 다시 열거하지 않고 같은 캐시를 쓴다. 일회성 표시는 여기서 소비한다.
    if (!keep) { Need(hooks.invalidate)();if (hooks.setWindowFirst) hooks.setWindowFirst(0); } // 새로 연 목록은 첫 창에서 시작한다.
    else if (hooks.reuseCache) hooks.reuseCache(); // 스크롤 재표시는 직전 구성이 끝에 건 재열거 요청을 지우고 같은 캐시를 쓴다(수백 개 목록을 매번 다시 읽지 않는다).
    Need(hooks.selectFileSpec)(solo);Need(hooks.refresh)(solo ? 2 : 0);
    if (hooks.windowFirst && hooks.setWindowFirst && hooks.appendScroll && Need(hooks.count)()>kMapListPageRows) { // 5차: 긴 목록은 한 창(50행)씩 스크롤한다. 50행 이하는 아래 원본 흐름을 그대로 쓴다.
        const auto total=Need(hooks.count)();
        auto first=std::clamp<std::int32_t>(hooks.windowFirst(),0,total-kMapListPageRows);hooks.setWindowFirst(first); // 마지막 창은 항상 꽉 채운다.
        const auto last=first+kMapListPageRows;
        const auto word=[&hooks](std::string_view key) { return hooks.translate ? hooks.translate(key) : std::string(key); }; // 번역 장치가 없거나 키가 없으면 영어 원문 그대로다.
        if (first>0) hooks.appendScroll("~o< "+word("Previous")+" "+std::to_string(kMapListPageRows)+"  ("+std::to_string(std::max(1,first-kMapListPageRows+1))+"-"+std::to_string(first)+" "+word("of")+" "+std::to_string(total)+")",-kMapListPageRows);
        std::int32_t shown=0; // 이번 창에서 추가한 행 수다.
        for (std::int32_t item=first;item<last;++item) { // 창의 모든 행을 순서대로 추가하고 절반에서 두 번째 열을 만든다.
            if (!Need(hooks.appendRow)(item)) break;
            if (++shown==kMapListPageRows/2) begin();
        }
        if (last<total) hooks.appendScroll("~o"+word("Next")+" "+std::to_string(kMapListPageRows)+" >  ("+std::to_string(last+1)+"-"+std::to_string(std::min(total,last+kMapListPageRows))+" "+word("of")+" "+std::to_string(total)+")",kMapListPageRows);
    } else {
        std::int32_t index=0;
        // Refresh의 과거 개수 대신 각 성공 행 추가 뒤 현재 개수로 분할 지점을 검사한다.
        while (Need(hooks.appendRow)(index)) {
            ++index;const auto count=Need(hooks.count)();
            if (count>15 && index==count/2) begin();
        }
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
    hooks.windowFirst=[&catalog] { return catalog.Window().windowFirst; }; // 5차: 스크롤 표시 창은 같은 목록 상태에 둔다.
    hooks.setWindowFirst=[&catalog](auto first) { catalog.Window().windowFirst=first; };
    hooks.reuseCache=[&catalog] { catalog.Window().dirty=0; };
    hooks.takeKeepCache=[&catalog] { auto& keep=catalog.Window().keepCache;const bool value=keep;keep=false;return value; };
    hooks.appendScroll=[add](std::string_view label,std::int32_t delta) { add(o::SoloMapRow{std::string(label),std::to_string(delta),false,true},kScrollMapListCommand); }; // 이동 행의 인자는 행 수의 10진 문자열이다.
    hooks.appendRow=[&catalog,current=std::move(current),fallback=std::move(fallback),add=std::move(add)](auto index) {
        const auto row=catalog.Row(index,Need(current)(),Need(fallback)());if (!row) return false;
        Need(add)(*row,kReadFortCommand);return true;
    };return hooks;
}
}
