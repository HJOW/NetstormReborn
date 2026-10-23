// Kencloud의실제메모리변화와기존GameRandom을합성하며ColorSort/CRT/새RNG로대체하지않는다.
#include "o/RawKencloud.h"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kBytes=0x40000,kRowBytes=512,kStride=0x3dd,kStrideBatch=0x3dd0; //원문의512×512할당과행별이동offset이다.
// 외부소유자가빌리는주소의물리길이를확인하고원문의해당접근시점에만실패한다.
std::span<std::uint8_t> Bytes(const KencloudHooks& hooks,std::uint32_t address,std::size_t length,std::string_view cause) {
    std::span<std::uint8_t> bytes; //현재같은할당의주소부터빌리는뷰다.
    try { bytes=hooks.memory(address); }
    catch (const std::out_of_range&) { throw std::out_of_range("Kencloud "+std::string(cause)); }
    if (bytes.size()<length) throw std::out_of_range("Kencloud "+std::string(cause));
    return bytes.first(length);
}
// fury의signed해석과매회IDIV를보존한다.음수는그대로음수몫이며0만명시예외다.
std::int32_t WarmCount(const KencloudGlobals& globals) {
    const auto divisor=std::bit_cast<std::int32_t>(globals.fury); //현재같은DWORD를해석한다.
    if (divisor==0) throw std::domain_error("Kencloud divide-zero");
    return 10000/divisor;
}
// 기존실제난수를정확히한번소비하고선택관측뒤반환값은그대로유지한다.
std::uint32_t Next(GameRandom& random,std::uint32_t limit,const KencloudHooks& hooks) {
    const auto before=random.State(); //관측용숫자일뿐독립RNG상태소유/동기화가아니다.
    const auto result=random.Next(limit); //동일Session/Impl의실제객체를직접소비한다.
    if (hooks.afterRandom) hooks.afterRandom(limit,before,result);
    return result;
}
}
std::uint32_t LoadKencloudFile(KencloudFileScratch& scratch,const KencloudFileHooks& hooks) {
    const auto source=hooks.pathSpec("CloudFileSpec",0,0,0); //호출뒤반환된같은NUL물리를캡처한다.
    // 전체길이선계산/문자열사본없이원문처럼NUL포함한BYTE씩복사한다.
    for (std::size_t index=0;;++index) {
        if (index>=source.size()) throw std::out_of_range("Kencloud file-path-read");
        const auto value=source[index]; //출력물리검사보다앞선원문read다.
        if (index>=scratch.path.size()) throw std::out_of_range("Kencloud file-path-write");
        scratch.path[index]=value;
        if (value==0) break;
    }
    if (hooks.exists(scratch.path)!=0) {
        const auto image=hooks.decode(scratch.path,scratch.width,scratch.height); //중단해도현재치수와이름의부분쓰기는남는다.
        if (scratch.width==512&&scratch.height==512) return image;
        hooks.release(image);
    }
    return 0;
}
void StepKencloud(const KencloudGlobals& globals,const KencloudHooks& hooks) {
    const auto previousPhase=globals.phase; //원문첫load이며물리read실패면phase는아직변하지않는다.
    const auto first=Bytes(hooks,globals.cache,1,"step-read")[0]; //cache첫BYTE가phase쓰기보다앞이다.
    const auto end=globals.end; //end도phase쓰기앞에현재값을캡처한다.
    const auto phase=(previousPhase+1U)&15U; //원문의32비트inc/and다.
    globals.phase=phase;
    Bytes(hooks,end-1U,1,"step-tail")[0]=first;
    const auto start=std::bit_cast<std::int32_t>(globals.phaseRows[phase]); //위상쓰기뒤현재행표를읽는다.
    if (start>=512) return;
    auto row=std::bit_cast<std::uint32_t>(start); //원문의고정dirty포인터를행offset으로표현한다.
    auto offset=row*kStride; //imul의low32를사용하여signed곱셈UB를피한다.
    // 각CRT호출복귀뒤다음행의현재cache/end를다시읽는다.
    do {
        auto destination=globals.cache+offset; //현재base를매행조회한다.
        const auto currentEnd=globals.end; //현재end와unsigned주소비교한다.
        if (destination>=currentEnd) destination=currentEnd-1U;
        auto last=destination+kStride; //DWORD주소덧셈은원문처럼감긴다.
        if (last>=currentEnd) last=currentEnd-1U;
        const auto length=last-destination; //CRT에전달한원문DWORD길이다.
        const auto target=Bytes(hooks,destination,length,"move-physical"); //길이0도물리one-past를허용하는소유자에조회한다.
        const auto source=Bytes(hooks,destination+1U,length,"move-physical"); //같은버퍼의겹침은실제memmove로보존한다.
        if (length!=0) std::memmove(target.data(),source.data(),length);
        if (hooks.afterMove) hooks.afterMove(destination,destination+1U,length);
        if (row>=globals.dirtyRows.size()) throw std::out_of_range("Kencloud dirty-physical");
        globals.dirtyRows[row]=1;
        row+=16U;offset+=kStrideBatch;
    } while (std::bit_cast<std::int32_t>(row)<512);
}
void BuildKencloud(const KencloudGlobals& globals,GameRandom& random,const KencloudHooks& hooks) {
    if (globals.cache!=0) { globals.partialDirty=0;globals.allDirty=1;return; }
    const auto image=hooks.loadFile(); //파일부모의반환주소를캡처하고같은전역에쓴다.
    globals.cache=image;
    bool generated=false; //파일실패fallback에서만warm-up을실행하는원문EBX다.
    if (image==0) {
        const auto allocated=hooks.allocate(kBytes); //할당callback뒤현재lowColor를읽는다.
        const auto color=static_cast<std::uint8_t>(globals.lowColor); //원문color읽기는cache대입보다앞이다.
        globals.cache=allocated;
        generated=true;
        const auto bytes=Bytes(hooks,allocated,kBytes,"fill-physical"); //실패전cache대입은이미발생했다.
        std::fill(bytes.begin(),bytes.end(),color);
    }
    const auto base=globals.cache; //fallbackrep종료뒤의현재base다.
    const auto end=base+kBytes; //같은base에서상·하부행뷰를구성한다.
    globals.nextRow=base+kRowBytes;
    const auto initialCount=std::bit_cast<std::int32_t>(globals.seedCount); //첫signed관문에사용한다.
    globals.end=end;globals.lastRow=end-kRowBytes;
    if (initialCount>0) {
        std::uint32_t index=0; //원문EDI는0부터시작하며각callback뒤현재count와비교한다.
        // 첫양수관문뒤에는현재count가줄어도현재한seed의두난수와쓰기를먼저완료한다.
        do {
            const auto y=Next(random,510,hooks)+1U; //첫호출의반환값을다음호출동안캡처한다.
            const auto x=Next(random,512,hooks); //같은GameRandom상태를두번째소비한다.
            if (index>=globals.seeds.size()) throw std::out_of_range("Kencloud seed-physical");
            globals.seeds[index]=globals.cache+y*kRowBytes+x;
            const auto count=std::bit_cast<std::int32_t>(globals.seedCount); //원문은저장뒤다시읽고EDI를증가시킨다.
            ++index;
            if (std::bit_cast<std::int32_t>(index)>=count) break;
        } while (true);
    }
    globals.spacer=Next(random,256,hooks)+5U;
    if (generated&&WarmCount(globals)>0) {
        std::int32_t iteration=0; //현재signed몫과비교할원문EDI다.
        // fury는각batch복귀뒤재조회하며재귀생성뒤현재speed를캡처한다.
        do {
            if (globals.cache==0) BuildKencloud(globals,random,hooks);
            auto remaining=globals.speed; //재귀복귀뒤의현재DWORD한번만캡처한다.
            // stepcallback이speed를바꾸어도현재batch의남은횟수는변하지않는다.
            while (remaining!=0) { StepKencloud(globals,hooks);--remaining; }
            const auto limit=WarmCount(globals); //현재fury0이면reset에앞서중단한다.
            ++iteration;
            if (iteration>=limit) break;
        } while (true);
    }
    globals.partialDirty=0;globals.allDirty=1;
}
}
