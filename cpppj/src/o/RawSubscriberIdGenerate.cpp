// 게임GameRandom/표준rand대신호출자가공급한같은현재CRT만소비한다.
#include "o/RawSubscriberIdGenerate.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kSubscriberWarmup=100,kSubscriberAttempts=16,kSubscriberBits=32,kSubscriberMask=0x07ffffffU,kSubscriberMinimum=64; //원본버림/재시도/생성폭/최종27비트/허용최솟값관문이다.
constexpr std::string_view kSubscriberExpired="Expired retry count in generateSubId()\n"; //004bc9db의정확한로그본문이다.
// 실제선택한장치만요구하며중단전플래그/CRT를되돌리지않는다.
template<class Function> const Function& NeedSubscriberId(const Function& function) { if (!function) throw std::logic_error("Subscriber ID 실제 장치 미연결");return function; }
}
//초기검사후시간장치가플래그를바꾸어도이미선택한srand까지계속하고그뒤현재값을증가한다.
std::uint32_t GenerateSubscriberId(const SubscriberIdGenerateGlobals& globals,const SubscriberIdGenerateHooks& hooks) {
    if (!globals.initialized) {
        const auto seconds=std::bit_cast<std::uint32_t>(NeedSubscriberId(hooks.time32)()),ticks=NeedSubscriberId(hooks.clock)(); //첫time32값은clock호출동안보존한다.
        NeedSubscriberId(hooks.seed)(seconds^ticks);globals.initialized=globals.initialized+1U; //srand복귀뒤의현재DWORD를재조회하여감김도유지한다.
    }
    for (std::uint32_t discarded=0;discarded<kSubscriberWarmup;++discarded) static_cast<void>(NeedSubscriberId(hooks.next)()); //호출때마다100회소비하며재시도마다반복하지않는다.
    std::uint32_t result=0xffffffffU; //16회소진시원문EBP의실패값이며0으로대체하지않는다.
    for (std::uint32_t attempt=0;attempt<kSubscriberAttempts;++attempt) { //각시도는상위31번부터하위0번까지정확히32회소비한다.
        std::uint32_t candidate{}; //현재시도만의32비트후보이며이전시도비트와합치지않는다.
        for (std::uint32_t remaining=kSubscriberBits;remaining;--remaining) { //원본signed idiv100뒤나머지<50을각비트에넣는다.
            const auto value=NeedSubscriberId(hooks.next)();if (value%100<50) candidate|=std::uint32_t{1}<<(remaining-1U);
        }
        candidate&=kSubscriberMask;if (candidate>kSubscriberMinimum) { result=candidate;if (result!=0xffffffffU) return result;break; } //27비트mask뒤에도있는원문ffffffff거부를보존한다.
    }
    NeedSubscriberId(hooks.expired)(9,kSubscriberExpired);return result; //로그장치가복귀하면실패DWORD로정상반환한다.
}
//기존현재CRT객체에대한두참조lambda만추가하며임의재시드/ThreadCrtRandom생성을하지않는다.
SubscriberIdGenerateHooks BindSubscriberIdCrtRandom(CrtRandom& crt,SubscriberIdGenerateHooks hooks) {
    hooks.seed=[&crt](auto value) { crt.SetState(value); };hooks.next=[&crt] { return static_cast<std::int32_t>(crt.Next()); };return hooks;
}
}
