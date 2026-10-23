// 현재주소재조회/고정this주소/원문BYTE복사와진단뒤계속을보존한다.
#include "o/RawFortCurrentName.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 실제도달한경계만요구하고미연결을임의성공으로대체하지않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Fort현재이름의실제경계미연결");return function;
}
// 같은실제헤더의little endian DWORD만읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("Fort현재이름헤더조회범위");std::uint32_t result{};
    // 비정렬주소에서도원문네BYTE만조립한다.
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8);return result;
}
// 원본헤더+8을선택한동일주소에쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("Fort현재이름헤더쓰기범위");
    // 다른필드와주변바이트는유지한다.
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 현재NUL버퍼의첫BYTE만읽고없는물리주소를빈문자열로가장하지않는다.
bool Empty(std::span<const std::uint8_t> bytes) {
    if (bytes.empty()) throw std::out_of_range("Fort현재이름첫BYTE범위");return bytes[0]==0;
}
// 원문strchr처럼NUL또는첫점을만날때까지읽는다.검사뒤에도원본source주소를유지한다.
bool HasDot(std::span<const std::uint8_t> bytes) {
    // NUL뒤점은검사하지않고미종료물리입력은명시거부한다.
    for (const auto byte:bytes) { if (byte=='.') return true;if (!byte) return false; }throw std::out_of_range("Fort새이름NUL범위");
}
// 전방한BYTE읽기/쓰기뒤NUL을판정하여자기참조/겹침과이후실패전부분효과를보존한다.
void Copy(std::span<std::uint8_t> target,std::span<const std::uint8_t> source) {
    // 무제한C주소접근은실제공급된물리범위에서만명시적으로중단한다.
    for (std::size_t index=0;;++index) {
        if (index>=source.size() || index>=target.size()) throw std::out_of_range("Fort이름NUL복사물리범위");
        const auto byte=source[index];target[index]=byte;if (!byte) return;
    }
}
// 실제현재owner를그시점에한번읽고고정Template주소를선택한다.
std::span<std::uint8_t> CurrentHeader(const FortCurrentNameGlobals& globals,const FortCurrentNameHooks& hooks) {
    return Need(hooks.header)(Need(globals.owner)());
}
// 원본은PathSpec호출마다전역포인터를선택하되FileSpec내용자체는변경하지않는다.
std::span<const std::uint8_t> Path(std::span<const std::uint8_t> name,const FortCurrentNameHooks& hooks) {
    Need(hooks.selectFileSpec)();return Need(hooks.pathSpec)(name);
}
}
// 진단이현재owner/manager를바꾸더라도this헤더는고정하고manager값만다시읽는다.
void SetFortManagerFileName(std::uint32_t slot,std::span<const std::uint8_t> path,const FortCurrentNameHooks& hooks) {
    auto header=Need(hooks.header)(slot);
    if (!Read(header,0)) Need(hooks.assertion)(429);
    if (Read(header,4)) Need(hooks.assertion)(430);
    Copy(Need(hooks.managerName)(Read(header,0)),path);Need(hooks.notify)(slot);
}
// callback뒤현재owner/name을재조회하고원본명령이캡처한readonly만유지한다.
void SetCurrentFortName(std::span<const std::uint8_t> name,std::span<std::uint8_t,256> oldPathScratch,const FortCurrentNameGlobals& globals,const FortCurrentNameHooks& hooks) {
    if (Read(CurrentHeader(globals,hooks),0)) {
        if (Empty(Need(hooks.currentName)())) Need(hooks.assertion)(39);
        Copy(oldPathScratch,Path(Need(hooks.currentName)(),hooks));
    }
    if (HasDot(name)) Need(hooks.assertion)(88);
    const auto beforeCopy=CurrentHeader(globals,hooks);
    if (Read(beforeCopy,0) && !Read(beforeCopy,8)) Need(hooks.snapshot)();
    Copy(Need(hooks.currentName)(),name);
    auto afterCopy=CurrentHeader(globals,hooks);
    if (Read(afterCopy,0)) {
        const auto savedReadonly=Read(afterCopy,8);Put(afterCopy,8,0); //이후callback이바꿔도원본ESI에캡처한값만마지막관문에사용한다.
        if (Empty(Need(hooks.currentName)())) Need(hooks.assertion)(39);
        const auto path=Path(Need(hooks.currentName)(),hooks); //PathSpeccallback이현재owner를바꿀수있으므로그뒤다시읽는다.
        SetFortManagerFileName(Need(globals.owner)(),path,hooks);Need(hooks.snapshot)();
        if (!savedReadonly) {
            const auto newPath=Path(name,hooks);
            if (!Need(hooks.samePath)(oldPathScratch,newPath)) Need(hooks.deleteRequest)(oldPathScratch);
        }
    }
    Need(hooks.refreshTitle)();const auto dirty=globals.dirty;const auto fort=Need(globals.fort)();globals.dirty=dirty+1U;
    if (fort) {
        if (Need(globals.editor)()) { Need(hooks.config)("lastEditFort",Need(hooks.currentName)());return; }
        if (Need(globals.mission)()) return;
        const auto slot=Need(globals.owner)();if (!Read(Need(hooks.header)(slot),0)) return;
        const auto state=Need(hooks.state)(slot);if (state.size()<=0x11) throw std::out_of_range("Fort현재State관문BYTE범위");
        if (!state[0x11]) Need(hooks.config)("lastMultiplayerFort",Need(hooks.currentName)());
    }
    if (Need(globals.editor)()) Need(hooks.config)("lastEditFort",Need(hooks.currentName)());
}
}
