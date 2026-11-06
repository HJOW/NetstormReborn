// 메모리 소진/GUI 없이 원문 메모리 실패의 고정 문구·현재 관문·순서와 handler 복원을 검사한다.
#include "TestSupport.h"
#include "o/RawAllocationFailure.h"
#include "platform/ScopedNewHandler.h"
#include <array>
#include <functional>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace { // 기대 순서는 보호 PE435f50/4b3490의 직접 명령 순서에서 읽었다.
using namespace netstorm::o; // 실제 원문 부모를 직접 호출한다.
struct AllocationScene {
    std::uint32_t initialized{1},exiting{}; // 현재 초기화/재진입 DWORD이며 원문 비영 입력도 그대로 읽는다.
    bool fort{},battle{true},metamap{}; // 세 개의 현재 모드 주소다.
    std::uintptr_t event{}; // 정리 뒤 현재값을 조회해야 하는 선택적 이벤트다.
    std::vector<std::string> order; // 실제 선택된 효과의 순서이며 제품 본문은 이 목록을 만들지 않는다.
    std::function<void()> messageEffect,cleanupEffect; // 현재 전역 변경·예외·재진입을 명시 입력으로 제공한다.
    AllocationFailureState State() { return {initialized,fort,battle,metamap,exiting}; } // 사본 대신 같은 현재 주소를 전달한다.
    AllocationFailureHooks Hooks() { // 고정 함수 포인터가 현재 scene을 context에서 조회한다.
        AllocationFailureHooks hooks;hooks.context=this;
        hooks.message=[](void* context,const char* body,const char* title,std::uint32_t flags) { auto& scene=*static_cast<AllocationScene*>(context);CHECK(std::string_view(body)=="Out of memory!" && std::string_view(title)=="Unrecoverable Error!" && flags==0x10);scene.order.emplace_back("message");if (scene.messageEffect) scene.messageEffect(); };
        hooks.cleanup=[](void* context,std::uint32_t reason) { auto& scene=*static_cast<AllocationScene*>(context);CHECK(reason==0 && scene.exiting==1);scene.order.emplace_back("cleanup");if (scene.cleanupEffect) scene.cleanupEffect(); };
        hooks.currentEvent=[](void* context)->std::uintptr_t { auto& scene=*static_cast<AllocationScene*>(context);scene.order.emplace_back("event");return scene.event; };
        hooks.signal=[](void* context,std::uintptr_t event) { auto& scene=*static_cast<AllocationScene*>(context);CHECK(event==scene.event);scene.order.emplace_back("signal:"+std::to_string(event)); };
        hooks.stopMusic=[](void* context) { static_cast<AllocationScene*>(context)->order.emplace_back("music"); };
        hooks.terminate=[](void* context,std::int32_t code) { CHECK(code==0);static_cast<AllocationScene*>(context)->order.emplace_back("exit:0"); };
        return hooks;
    }
};
void AllocationPreviousHandler() {} // 등록 복원만 확인하는 이전 사용자 handler이며 실제 할당은 시도하지 않는다.
void AllocationOuterHandler() {} // 바깥 범위의 handler 정체성을 관찰한다.
void AllocationInnerHandler() {} // 중첩·예외 범위의 handler 정체성을 관찰한다.
}

TEST_CASE(AllocationFailure_MessageCurrentCleanupEventMusicAndExitOrder) { // 현재event를정리뒤읽고고정원문문구/종료코드를유지한다.
    AllocationScene scene;scene.event=3;scene.messageEffect=[&] { scene.initialized=2;scene.battle=false;scene.fort=true;scene.event=5; };scene.cleanupEffect=[&] { scene.event=7;CHECK(!CleanupRawGameForExit(scene.State(),scene.Hooks())); };
    HandleRawAllocationFailure(scene.State(),scene.Hooks());CHECK(scene.exiting==1 && scene.order==std::vector<std::string>({"message","cleanup","event","signal:7","music","exit:0"}));
}

TEST_CASE(AllocationFailure_CurrentInitializationLocalModesAndExitGuard) { // 초기화/편집·전투모드/임의비영guard의유한조합에서선택된정리만실행한다.
    for (const std::uint32_t initialized:{0U,1U,0x80000000U}) { // 초기화는1만이아닌현재비영값전체를허용한다.
        for (std::uint32_t modes=0;modes<4;++modes) { // 비멀티 편집/전투의 현재 조합을 읽고 메타맵 양성 검사는 제외한다.
            for (const std::uint32_t exiting:{0U,2U,0xffffffffU}) { // 원문은현재비영guard일때정리를생략한다.
                AllocationScene scene;scene.initialized=initialized;scene.exiting=exiting;scene.fort=(modes&1U)!=0;scene.battle=(modes&2U)!=0;
                const bool selected=initialized && modes && !exiting;HandleRawAllocationFailure(scene.State(),scene.Hooks());CHECK(scene.exiting==(selected ? 1U:exiting));
                CHECK(scene.order==(selected ? std::vector<std::string>({"message","cleanup","event","music","exit:0"}):std::vector<std::string>({"message","event","music","exit:0"})));
            }
        }
    }
}

TEST_CASE(AllocationFailure_MessageCanEnableCleanupAndNullEventNeedsNoSignal) { // 메시지정상복귀뒤현재모드를읽으며NULL이벤트에서는신호장치를요구하지않는다.
    AllocationScene scene;scene.initialized=0;scene.battle=false;scene.messageEffect=[&] { scene.initialized=1;scene.fort=true; };auto hooks=scene.Hooks();hooks.signal=nullptr;
    HandleRawAllocationFailure(scene.State(),hooks);CHECK(scene.exiting==1 && scene.order==std::vector<std::string>({"message","cleanup","event","music","exit:0"}));
}

TEST_CASE(AllocationFailure_CleanupFailurePreservesGuardAndEffectPrefix) { // 전체정리실패의앞선guard/메시지를되돌리거나후속신호를성공합성하지않는다.
    AllocationScene scene;scene.cleanupEffect=[] { throw std::runtime_error("명시 정리 실패"); };bool stopped{};try { HandleRawAllocationFailure(scene.State(),scene.Hooks()); }catch (const std::runtime_error&) { stopped=true; }
    CHECK(stopped && scene.exiting==1 && scene.order==std::vector<std::string>({"message","cleanup"}));
}

TEST_CASE(AllocationFailure_ScopedHandlerRestoresNormalNestedAndExceptionalRegistration) { // 실제실패를일으키지않고표준handler의등록/복원수명만확인한다.
    netstorm::platform::ScopedNewHandler baseline(AllocationPreviousHandler);CHECK(std::get_new_handler()==AllocationPreviousHandler);
    { netstorm::platform::ScopedNewHandler outer(AllocationOuterHandler);CHECK(std::get_new_handler()==AllocationOuterHandler);
      try { netstorm::platform::ScopedNewHandler inner(AllocationInnerHandler);CHECK(std::get_new_handler()==AllocationInnerHandler);throw std::runtime_error("명시 등록 범위 종료"); }catch (const std::runtime_error&) {}
      CHECK(std::get_new_handler()==AllocationOuterHandler); }
    CHECK(std::get_new_handler()==AllocationPreviousHandler);
}
