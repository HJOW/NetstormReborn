// 원본004b5890의저장상태파일/8Player복원과SEH파일해제순서를보존한다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
inline constexpr std::size_t kSavedStateReadScratchBytes=0x188; //steadySP+10부터의명시392BYTE이며미정stack도호출자가초기입력으로공급한다.
// 현재Client/Session의실제출력전역을선택위치에서빌린다.이모듈은같은이름의새전역을소유하지않는다.
struct SavedStateReadGlobals {
    std::function<std::uint32_t&()> owner; //00542528은pending복귀뒤ffffffff를먼저쓰고실제4BYTEread를받는다.매레코드비교때현재값을읽는다.
    std::function<std::uint32_t&()> subscriber; //0054252c는현재복구owner와같은레코드에서만파일ID를받는다.
    std::function<std::span<std::uint8_t>()> previousHandle; //005c84c0의같은NUL출력물리다.새Player이름을쓰기전에기존이름을복사한다.
    std::function<std::span<std::uint8_t>()> players; //0059534c의현재Player배열을매레코드7read뒤조회한다.해당레코드쓰기끝까지그물리가살아있어야한다.
    std::array<std::function<std::uint32_t&()>,3> tail; //00595090/0055a420/0055b4ac의직접read출력이다.읽기짧음은현재상위BYTE를유지한다.
};
// 실제파일객체/이름/Core초기화/9×16메타데이터의하위장치를선택시필수로빌린다.
struct SavedStateReadHooks {
    std::function<std::span<const std::uint8_t>()> path; //004b5470의NUL포함같은살아있는경로할당이다.부모는한번받은주소를존재/open양쪽에서쓴다.
    std::function<std::int32_t(std::string_view)> exists; //004dd560의GetFileAttributesA!=ffffffff관문이다.디렉터리도비영이며0이면다른장치를요구하지않는다.
    std::function<std::uint32_t(std::string_view,std::span<std::uint8_t>,std::uint32_t,std::uint32_t)> open; //0041b4c0(path,0,0)이다.같은24BYTE파일object와반환opaque토큰의수명은close까지유지한다.
    std::function<std::uint32_t(std::uint32_t,std::span<std::uint8_t>,std::uint32_t)> read; //0041a7a0의대상물리/요청폭/ffffffff다.부분복사만하고나머지를초기화하지않는다.반환길이는부모가검사하지않는다.
    std::function<void(std::uint32_t,std::span<std::uint8_t>)> close; //0041b410의같은파일object해제다.정상꼬리또는실제SEH해당선택중단에서한번호출한다.
    std::function<void(std::string_view)> pending; //00460bb0의실제기존pending이름helper다.current이름변경으로대체하지않는다.
    std::function<void(std::span<std::uint8_t>)> initializeCore; //0042ce70의같은지역Core10BYTE초기화다.짧은파일read보다먼저매레코드실행한다.
    std::function<void(std::uint32_t,std::uint32_t,std::array<std::uint8_t,10>)> metadata; //0048f350의owner/ID/값으로복제한Core10BYTE다.현재Core캐시10BYTE와다른9×16실제장부가필요하다.
};
// 004b5890:없음/잘못된서명/점이름은0,종료DWORD0이고참가자2명이상인경우만signed저장시각이다.
// scratch는호출전명시초기화한392BYTE다.파일짧음의잔여와부분Player/이름/전역효과를복구하지않는다.
// owner가실제Player물리밖이거나NUL없는문자열이물리끝을넘으면host계약오류로거부한다.
std::int32_t ReadSavedState(std::span<std::uint8_t> scratch,const SavedStateReadGlobals& globals,const SavedStateReadHooks& hooks);
}
