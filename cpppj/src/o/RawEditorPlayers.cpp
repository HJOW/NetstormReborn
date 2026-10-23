// callback뒤현재표를읽고한번선택한owner레코드와기술부모는해당반복동안유지한다.
#include "o/RawEditorPlayers.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 실제도달한경계만요구하여타입이없을때Contained경계를강제로호출하지않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Editor Player의실제경계미연결");return function;
}
// 원본주소부터남은동일물리범위만빌리고복제레코드를만들지않는다.
std::span<std::uint8_t> At(std::span<std::uint8_t> bytes,std::size_t offset) {
    if (offset>bytes.size()) throw std::out_of_range("Editor Player현재주소물리범위");return bytes.subspan(offset);
}
// 원본문자복사처럼한BYTE읽기/쓰기를한뒤NUL을판정하며뒤바이트는보존한다.
void Copy(std::span<std::uint8_t> target,std::span<const std::uint8_t> source) {
    // source/target의실제끝은무제한C문자열스캔대신명시물리거부한다.
    for (std::size_t index=0;;++index) {
        if (index>=source.size() || index>=target.size()) throw std::out_of_range("Editor Player이름NUL/쓰기물리범위");
        const auto value=source[index];target[index]=value;if (!value) return; //먼저복사한바이트는이후실패해도되돌리지않는다.
    }
}
// 같은캡처레코드의지정DWORD만little endian으로조회한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("Editor Player DWORD조회물리범위");std::uint32_t value{};
    // 현재네BYTE를낮은비트부터합친다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value;
}
// 실제레코드의필드밖바이트를건드리지않고네BYTE만쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("Editor Player DWORD쓰기물리범위");
    // 원본의하위BYTE부터지정폭을기록한다.
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
}
// 타입개수는고정하지않고각기술부모는00490250이반환한SID를반복동안캐시한다.
void InitializeEditorPlayers(const EditorPlayersHooks& hooks) {
    const auto human=Need(hooks.translate)("Human Player");Copy(At(Need(hooks.players)(),0xc0),human);
    const auto player=Need(hooks.translate)("Player");Copy(Need(hooks.handle)(),player);
    // owner1은Human이름만쓰며새Player/Computer/직접기술생성은owner2..8이다.
    for (std::uint32_t owner=2;owner<=8;++owner) {
        const auto parent=Need(hooks.technology)(owner); //생성callback뒤현재Player표를다시조회한다.
        auto record=At(Need(hooks.players)(),owner*180);Put(record,0x2c,owner); //번역/format동안이레코드주소를유지한다.
        const auto computer=Need(hooks.translate)("Computer #%d");
        const auto name=Need(hooks.format)(30,computer,static_cast<std::int32_t>(owner-1));Copy(At(record,0xc),name);
        Put(record,0x50,(Read(record,0x50)&0xfffffeffU)|3U); //이름callback이바꾼같은레코드flags의다른비트는보존한다.
        // group10/theme5만제외하며0번부터현재signedcount를매번다시읽는다.
        for (std::int32_t type=0;type<Need(hooks.typeCount)();++type) {
            const auto& kind=Need(hooks.typeAt)(type);if (kind.group==10 || kind.theme==5) continue;
            static_cast<void>(Need(hooks.createContained)(parent,static_cast<std::uint32_t>(type),0,4,0x10,0)); //존재검사없이직접추가한다.
        }
    }
    Need(hooks.finish)();
}
}
