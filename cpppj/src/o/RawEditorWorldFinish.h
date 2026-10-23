// EditorPlayers 마지막0046e980의noIsland정리·표면/spot·삭제·Graph순서를복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 현재Pool의물리바이트와원본주소산술/유효관문에쓰는기준값이다.각getter는효과없이현재값을읽는다.
struct EditorWorldFinishPool {
    std::span<const std::uint8_t> bytes; // 실제raw50BYTE배열이며캡처한form은해당반복이끝날때까지살아있어야한다.
    std::uint32_t address{}; // 동일Pool의32비트기준주소표현이다.재배치뒤에도이전form주소를보존한다.
    std::uint32_t capacity{}; // 005c847c현재DWORD이며free-skip상한은최초값을별도로캐시한다.
};
// 실제가상+48이받을캡처form이며현재Pool의같은SID를재조회한복제본이아니다.
struct EditorWorldFinishForm {
    std::span<const std::uint8_t> bytes; // 후보선택때고정한물리raw50BYTE를빌린다.
    std::uint32_t address{}; // 현재Pool이바뀌어도같은캡처객체의원본주소표현이다.
};
// 실제Finder의독립현재/다음수명이며island를찾은뒤에도끝까지Next한다.
struct EditorWorldFinishCursor {
    std::function<std::uint32_t()> current; // 실제Finder+34의현재후보DWORD를읽기만한다.
    std::function<void()> next; // 004b1810의다음후보효과이며실제같은Finder를진행한다.
};
// 실제하위와현재저장소만공급하고순회/타입/좌표/직접쓰기/순서는본체가책임진다.
struct EditorWorldFinishHooks {
    std::function<EditorWorldFinishPool()> pool; // 현재Pool기준주소/실제span/capacity를효과없이읽는다.
    std::function<std::uint32_t()> noIslandType,islandType; // 005412cc/005411d0전체DWORD다.신전/사제타입으로대체하지않는다.
    std::function<std::int32_t()> boardSize; // 00531928현재signedDWORD를필요할때한번float32로캐시한다.
    std::function<EditorWorldFinishCursor(const EditorWorldFinishForm&,std::int32_t,std::int32_t,std::int32_t,std::int32_t)> begin; // 캡처form은어댑터문맥이며실제004b16d0인자는(x,y,x,y)/flags0이다.
    std::function<std::span<std::uint8_t>()> surfaces,spots; // 현재005c84bc WORD표/005c7c44 BYTE표를쓰기시점마다다시빌린다.
    std::function<void(const EditorWorldFinishForm&,std::uint32_t)> unpop; // 캡처form의실제가상+48(0)이며Factory/Release를여기서중복호출하지않는다.
    std::function<void(std::int32_t)> release; // 가상효과뒤현재Pool기준으로재계산한signedSID의004abf60이다.
    std::function<void(std::uint32_t)> graph; // 모든순회가끝난마지막00463110(0)의실제Graph재구성이다.
};
// SID1→실제iterator의최초증가로SID2부터시작한다.미연결하위를빈성공으로만들지않는다.
void FinishEditorWorld(const EditorWorldFinishHooks& hooks);
}
