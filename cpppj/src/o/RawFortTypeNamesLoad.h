// 원본004bd800의TypeNames변환표읽기와별도004fb820/004bcc50 cleanup을복원한다.
#pragma once
#include "o/RawMemStream.h"
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
// Client/같은로더가직접공유하는두현재DWORD다.이전pointer를별도사본에보존하지않는다.
struct FortTypeNamesLoadGlobals {
    std::uint32_t& loadingDepth; //005c89b8이다.진입증가/정상감소/별도cleanup이현재값을직접쓴다.
    std::uint32_t& conversion; //005c89bc의현재255BYTE변환표토큰이다.각일치쓰기때다시읽는다.
};
// 실제Select/할당/TYPE/진단과물리메모리만분리하고zero/hash/검색/필드쓰기는본문이소유한다.
struct FortTypeNamesLoadHooks {
    std::function<std::uint32_t(std::int32_t)> manager; //깊이증가뒤고정this헤더의현재manager를읽는다.이본문에는manager0진단이없다.
    std::function<std::uint32_t(std::uint32_t,std::string_view,std::int32_t)> select; //0044ceb0(manager,"TypeNames",1)의실제선택stream토큰이다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> stream; //같은반환stream의실제16BYTE다.진단/할당callback뒤도주소를유지한다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> buffer; //선택한data토큰의실제물리범위다.끝을기본0으로확장하지않는다.
    std::function<std::uint32_t(std::uint32_t)> allocate; //004e4391(255)의실제할당이다.본문이반환주소를현재pointer에쓰고그블록만zero한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> conversionBuffer; //해당현재토큰의실제255BYTE다.0/없음은물리거부한다.
    std::function<std::int32_t()> typeCount; //00541348의현재signed DWORD다.최초/매타입끝에다시읽는다.
    std::function<std::span<const std::uint8_t>(std::int32_t)> typeName; //0049a840(index)+4의그시점실제NUL포함이름이다.20자에서자르지않는다.
    std::function<void(std::uint32_t)> assertion; //Template.cpp608의기존conversion진단이다.복귀하면그대로계속한다.
    std::function<void(std::uint32_t)> loadingDepthChanged; //실제깊이쓰기를같은월드소비자에동기화하는관측경계다.추가게임효과를넣지않는다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; //선택뒤같은fields/data를동기범위전체에서빌린다.지정하면stream/buffergetter보다우선하며뷰/장치를밖으로탈출시키지않는다.
};
// 원문본문만실행한다.명시callback/물리중단시앞선변경과증가한깊이를보존하며자동cleanup하지않는다.
void LoadFortTypeNames(std::int32_t slot,const FortTypeNamesLoadGlobals& globals,const FortTypeNamesLoadHooks& hooks);
// 원본예외funclet/helper의현재깊이한번감소만실행한다.전체OS SEH나이미정상감소한본문에중복적용하지않는다.
void UnwindFortTypeNamesLoad(const FortTypeNamesLoadGlobals& globals,const FortTypeNamesLoadHooks& hooks);
}
