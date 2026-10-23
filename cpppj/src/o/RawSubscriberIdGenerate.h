// 원본004bc950의현재초기화플래그·CRT소비·재시도/실패반환순서를복원한다.
#pragma once
#include "o/CrtRandom.h"
#include <functional>
#include <string_view>

namespace netstorm::o {
// Client/같은스레드소유자가유지하는현재초기화DWORD만빌린다.
struct SubscriberIdGenerateGlobals {
    std::uint32_t& initialized; //005c89a4이며0일때만시간시드를만들고srand복귀뒤현재값에1을더한다.
};
// OS시각과기존CRT·로그의실제선택장치다.생성기자체에별도난수상태는없다.
struct SubscriberIdGenerateHooks {
    std::function<std::int32_t()> time32; //최초선택시에만time32(0)의signed32비트를공급한다.
    std::function<std::uint32_t()> clock; //time32복귀뒤CRT clock의현재32비트값을공급한다.
    std::function<void(std::uint32_t)> seed; //같은현재CRT의srand다.time32 XOR clock을그대로쓴다.
    std::function<std::int32_t()> next; //같은현재CRT rand의signed DWORD반환이다.부모에서15비트clamp를추가하지않는다.
    std::function<void(std::int32_t,std::string_view)> expired; //소진/ffffffff거부의원문로그9다.복귀하면실패DWORD를그대로반환한다.
};
//004bc950:최초시드→현재플래그+1→항상100버림→32bit×최대16회→27bit/>64/ffffffff판정이다.
//실패반환ffffffff와장치예외를구별하고앞선CRT/플래그를복구하지않는다.
std::uint32_t GenerateSubscriberId(const SubscriberIdGenerateGlobals& globals,const SubscriberIdGenerateHooks& hooks);
//기존실제CrtRandom객체를빌려seed/next만연결한다.time32/clock/로그장치는보존하며새상태를만들지않는다.
//crt는반환hooks보다오래살아야한다.임시Borrow객체가아닌장기실제객체/명시지역빌림을전달한다.
SubscriberIdGenerateHooks BindSubscriberIdCrtRandom(CrtRandom& crt,SubscriberIdGenerateHooks hooks);
}
