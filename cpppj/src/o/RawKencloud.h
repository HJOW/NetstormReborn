// 원본 Kencloud의 파일 부모00470bb0/생성00471040/행 이동00470b20을 같은 현재 물리·GameRandom에 복원한다.
#pragma once
#include "o/SpStore.h"
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
// 원문 전역을 현재 소유자의 실제 DWORD 참조/물리 표로 빌린다. 객체 자체가 난수나 캐시를 새로 소유하지 않는다.
struct KencloudGlobals {
    std::uint32_t& cache; //00569ffc 현재512×512BYTE 할당의 주소 토큰이다.
    std::uint32_t& end; //00569ff0이며 cache변경시임의자동동기화하지않는다.
    std::uint32_t& nextRow; //00569ff8은생성당시cache+512다.
    std::uint32_t& lastRow; //00569ff4는생성당시end-512다.
    std::uint32_t& phase; //0056b804의현재위상이며low4비트로진행한다.
    std::uint32_t& allDirty; //0056b7fc의전체표시요청이다.
    std::uint32_t& partialDirty; //0056b800의부분표시요청이다.
    std::uint32_t& spacer; //00540a88은같은GameRandom(256)+5를저장한다.
    const std::uint32_t& lowColor; //00540a34의현재lowBYTE로fallback할당을채운다.
    const std::uint32_t& speed; //00540a80 cloudSpeed이며각warm-up반복의재귀복귀뒤캡처한다.
    const std::uint32_t& fury; //00540a84의signed값으로10000을나눈다.0나눗셈을성공으로바꾸지않는다.
    const std::uint32_t& seedCount; //00540a90은첫관문과각seed뒤signed값을다시읽는다.
    std::span<const std::uint32_t,16> phaseRows; //00540a98의현재16개행번호표다.
    std::span<std::uint32_t> dirtyRows; //0056a000의실제512DWORD행표다.물리길이를넘는쓰기는예외다.
    std::span<std::uint32_t> seeds; //0056a800의실제주소표다.설정의미상상한을새로만들지않는다.
};
// 무정의 초기값을 임의0으로 만들지 않고 caller가 명시하는 파일 부모의 현재264BYTE stack입력이다.
struct KencloudFileScratch {
    std::int32_t height; //원문local+0의현재DWORD다.디코더는이실제참조를쓴다.
    std::int32_t width; //원문local+4의현재DWORD며반환뒤width부터비교한다.
    std::array<std::uint8_t,256> path; //원문local+8의실제NUL포함이름복사버퍼다.
};
// 파일 시스템/원본 GIF 디코더는 기존 구현에 합성하는 필수 경계다.
struct KencloudFileHooks {
    std::function<std::span<const std::uint8_t>(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t)> pathSpec; //004410f0("CloudFileSpec",0,0,0)의NUL물리를빌린다.
    std::function<std::uint32_t(std::span<std::uint8_t>)> exists; //0041ab10은같은복사버퍼를받으며비영일때만decode한다.
    std::function<std::uint32_t(std::span<std::uint8_t>,std::int32_t&,std::int32_t&)> decode; //004dae60(path,&width,&height)의현재필드와반환주소다.
    std::function<void(std::uint32_t)> release; //치수가512×512가아니면반환주소0도004e4354에그대로전달한다.
};
// 메모리 소유권·파일 장치를 명시하며 실제 fill/memmove/난수 본체는 모듈이 실행한다.
struct KencloudHooks {
    std::function<std::uint32_t()> loadFile; //LoadKencloudFile의실제필수합성경계다.도달하지않으면미연결을허용한다.
    std::function<std::uint32_t(std::uint32_t)> allocate; //operator_new(0x40000)의현재반환주소다.0을가짜성공으로바꾸지않는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> memory; //주소부터남은현재물리를반환한다.순수주소조회이며미등록주소는out_of_range다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> afterMove; //실제memmove뒤의선택관측경계다.없어도이동본체는실행한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> afterRandom; //같은GameRandom.Next뒤limit/직전상태/반환값관측이다.새난수나상태동기화가아니다.
};
// 00470bb0의현재256BYTE복사→exists→decode→현재치수검사/free를실행한다.모든scratchBYTE는caller입력이다.
std::uint32_t LoadKencloudFile(KencloudFileScratch& scratch,const KencloudFileHooks& hooks);
// 00470b20은cache첫BYTE읽기뒤phase쓰기/end[-1]쓰기와현재base/end재조회를보존한다.난수를소비하지않는다.
void StepKencloud(const KencloudGlobals& globals,const KencloudHooks& hooks);
// 00471040은현재캐시/파일/할당/fill/seed/재귀warm-up을실행하며받은실제GameRandom참조만소비한다.
void BuildKencloud(const KencloudGlobals& globals,GameRandom& random,const KencloudHooks& hooks);
}
