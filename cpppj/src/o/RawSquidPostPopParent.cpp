// 실제원문의분기선택과callback뒤현재TYPE/전역재조회를구별한다.
#include "o/RawSquidPostPopParent.h"
#include <stdexcept>

namespace netstorm::o { // 함수포인터대신현재호스트장치를선택된위치에서만호출한다.
namespace { // 실행중자기연결이교체돼도현재callable의수명을보존한다.
template<class F> F PostNeed(const F& function) { if (!function) throw std::logic_error("공통후처리의선택된실제장치미연결");return function; } // 미도달장치는요구하지않는다.
constexpr std::uint32_t kPostRegion=0x50444208U,kPostNoGraph=0x2000000U; //원문건물군|8과그래프억제비트다.
}
void RunSquidPostPopParent(const SidPool& pool,Sid sid,std::uint32_t flags,std::uint32_t& depth,const SquidPostPopParentHooks& hooks) { //004b0d30의일반몸체이며깊이는호출자가이미증가시켰다.
    if (PostNeed(hooks.suppressed)()) { --depth;return; }
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("현재공통후처리는10.78전용");
    const auto raw=pool.Slot(sid);const auto firstGenus=PostNeed(hooks.type)(raw[10]).flags2;
    const bool skipGraph=(firstGenus&kPostRegion) && !PostNeed(hooks.graphsEnabled)(); //원문의LAB_004b0e4c직행을보존한다.
    if (!skipGraph) {
        if ((firstGenus&kPostRegion) && (flags&3U) && !(flags&kPostNoGraph)) PostNeed(hooks.region)(sid);
        if (PostNeed(hooks.graphsEnabled)()) {
            if (!(flags&0x203U) || (flags&kPostNoGraph)) {
                if ((PostNeed(hooks.type)(raw[10]).flags1&0x800U) && (flags&1U) && (flags&kPostNoGraph)) PostNeed(hooks.resetGraph)(sid);
            } else {
                if (flags&0x800U) PostNeed(hooks.assertion)("!(popFlags&pfDESTROY_GRAPH)","\\Ns\\O\\Squid.cpp",0x80b);
                if (PostNeed(hooks.type)(raw[10]).flags1&0x800U) PostNeed(hooks.add)(sid);
            }
        }
    }
    const auto savedGenus=PostNeed(hooks.type)(raw[10]).flags2;
    if ((flags&1U) && !(raw[40]&9U)) {
        PostNeed(hooks.cost)(sid);
        if (PostNeed(hooks.type)(raw[10]).flags1&0x10000000U) PostNeed(hooks.provider)(sid);
        if (savedGenus&0x4200U) { PostNeed(hooks.ownerFactory)(raw[34],sid);PostNeed(hooks.factory)(sid); }
        PostNeed(hooks.aiNotice)(raw[34],sid);
        if (raw[34]==PostNeed(hooks.localOwner)()) PostNeed(hooks.localCount)(raw[10]);
        PostNeed(hooks.globalCount)(raw[10]);PostNeed(hooks.recheckSelection)();
    }
    --depth;
}
}
