// PATCH State.cpp004b2df0의1256BYTE 부모를 복원하며 실제 UI/Kernel/Pool 자식은 재사용 필수경계로 둔다.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 원문의 무인자/고정this 자식을 식별한다. enum값은Native주소이며host함수포인터로역참조하지않는다.
enum class StateWorldCleanupEffect : std::uint32_t {
    CloseAllConnections=0x4e29c0, //reason0의첫전송정리다.
    EndConnections=0x4e2a10, //reason0의두번째전송정리다.
    ResumeGame=0x460e30, //현재pauseDepth가비영일때게임정지한단계를푼다.
    ResumeAudio=0x4aa970, //같은반복의음향정지한단계를푼다.
    Clear446e70=0x446e70, //정지반복뒤원문첫자식을실행한다.
    Clear4c1f20=0x4c1f20, //명령/현재표의기존몸체를재사용한다.
    ResetClock=0x460e70, //원문시계정리몸체다.
    Clear42b1b0=0x42b1b0, //22DWORD직접초기화앞의기존자식이다.
    RefreshSp=0x43dad0, //실제SP작성뒤현재표시소비자다.
    ClearTemplates=0x4bfb00, //Player8순회뒤Template정리다.
    LeaveMetamap=0x4795a0, //그때현재metamap비영에서만선택한다.
    Clear445140=0x445140, //현재Template정리뒤의기존자식이다.
    ClearCursor=0x4d5c60, //선택/커서해제몸체를재사용한다.
    ClearPlayerLinks=0x48fcd0, //Pool보다먼저Player동적필드/기술링크를정리한다.
    ClearKernel=0x471af0, //실제Kernel표의현재슬롯을가상소멸한다.
    KillGumps=0x4666d0, //기존제외타입필터로현재Gump들을Kill한다.
    CleanupGumps=0x4662b0, //현재kill큐의실제정리를수행한다.
    ReplaceUber=0x4c77c0, //기존루트자식이동/새Uber루트교체몸체다.
    Clear43b540=0x43b540, //Core선택해제직전의기존자식이다.
    Reset422490=0x422490, //Pool초기화뒤원문상태자식이다.
    Clear4c2450=0x4c2450, //기존세표정리몸체다.
    MapMode=0x4437f0, //이름은기존API호환용이다.실제본문은C3빈RET이며지도/모드계산이없다.두호출위치를보존한다.
    SceneMusic=0x469fc0, //실제음악부모를그시점에호출한다.
    Clear472830=0x472830, //기존Graph/명령관련자식을실행한다.
    Clear4b2b50=0x4b2b50, //원문의현재State하위몸체다.
    Clear4328a0=0x4328a0, //기존해당정리몸체를필수로호출한다.
    Clear4ab940=0x4ab940, //기존해시/목록정리몸체를재사용한다.
    Reset568b60=0x46edf0, //원문고정this00568b60에실행한다.
    Reset54c8a0=0x432360, //원문고정this0054c8a0에실행한뒤그첫DWORD를1로쓴다.
    ResetGraph=0x462b20, //UI/Kernel/Pool정리뒤실제Graph표를초기화한다.
    Clear4a9e50=0x4a9e50, //실제Player재할당직전의자식이다.
    InitializePlayers=0x4910a0, //Core0선택뒤이전Player해제/새1620BYTE구성을실행한다.
    ResetZone=0x4bfd60, //원문고정this005c86c0의실제Zone몸체다.
    InterpretOptions=0x435220 //후반모드정리/조건부Core선택뒤현재설정을재해석한다.
};
// 실제Client/Session의같은전역과현재Player/TYPE를접근한다.사본동기화나임의전역0을공급하지않는다.
struct StateWorldCleanupGlobals {
    std::function<std::uint32_t(std::uint32_t)> read32; //지정Native주소의현재DWORD를매원문위치에서읽는다.
    std::function<void(std::uint32_t,std::uint32_t)> write32; //같은실제owner필드에DWORD효과를직접쓴다.
    std::function<void(std::uint32_t,std::uint16_t)> write16; //005c89b4.low16만0으로쓰고상위WORD를보존한다.
    std::function<void(std::uint32_t,std::uint64_t)> write64Bits; //원문의double store폭/비트를같은실제필드에쓴다.
    std::function<std::span<std::uint8_t>()> players; //각Player필드store직전에현재59534c의같은물리뷰를재조회한다.
    std::function<std::uint32_t(std::uint32_t)> templateManager; //현재지정slot의실제20BYTEheader첫DWORD를순수조회한다.
    std::function<float&(std::uint32_t)> typeDefault; //0049a840의선택된현재TYPE+c4실제float참조다.
};
// 선택한실제하위몸체만필수다.빈성공/가짜미션/새난수로큰계산을대신하지않는다.
struct StateWorldCleanupHooks {
    std::function<void(StateWorldCleanupEffect)> effect; //각enum의실제기존자식에분배하며미연결enum은명시오류여야한다.
    std::function<void(std::uint32_t)> disconnect; //004e2940의현재연결번호1..8/9/10/11이다.
    std::function<void(std::uint32_t)> assertion; //State.cpp1914의중복모드진단이며복귀하면원문을계속한다.
    std::function<std::uint32_t()> pauseDepth; //00460de0의현재깊이를resume뒤매번다시읽는다.
    std::function<void(std::uint32_t)> saveFort,saveBattle; //004bf9a0/004bf880은그때선택한실제Template slot을받는다.
    std::function<void(std::uint32_t)> resetAi,resetPriest; //00413ae0/00492240을각Player1..8에그순서로실행한다.
    std::function<void(std::uint32_t,std::uint32_t)> resetOtherPlayer; //00447810(owner,0)의실제자식이다.
    std::function<void(std::uint32_t,float)> setSp; //0040edb0의같은분산SP/난수소비자다.
    std::function<void(std::uint32_t,std::uint32_t)> selectCore; //0048f3b0의실제두owner/현재Corealias선택·정리몸체다.
    std::function<void(std::uint32_t)> resetPool; //004af170(0)의실제Pool재초기화다.
    std::function<double()> wallSeconds; //00460d70은마지막위치에서조회하며게임시각으로대체하지않는다.
};
//004b2df0:전송/정지→저장/Player→UI/Kernel→Core0→Pool→Player재구성→모드/기한/전역순서다.
//PATCH10.78몸체만구현한다. CD10.72대응0042f5c0은SourceMap문서이며동일몸체로가장하지않는다.
void CleanupStateWorld(std::uint32_t reason,const StateWorldCleanupGlobals& globals,const StateWorldCleanupHooks& hooks);
// PATCH004328a0의 PlayerNoMake 경고 첫255DWORD만0으로 만든다. 이웃 SP 추적값·추가 물리 칸은 보존한다.
void ResetStatePlayerNoMakeWarnings(std::span<std::uint32_t> warnings);
// PATCH004ab940의 같은256×256BYTE spot만0으로 만든다. 다른 Pool/Hash/Graph를 초기화하지 않는다.
void ResetStateSpotMap(std::span<std::uint8_t> spots);
}
