// 현재Pool과표주소의재조회및Finder전캡처/삭제전캡처를구별한다.
#include "o/RawEditorWorldFinish.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kStride=50,kType=10,kState=11,kX=14,kY=18; // 10.78raw슬롯의실제폭/필드위치다.
constexpr double kInt64Boundary=9223372036854775808.0; // 실제__ftol의signed64불확정값경계다.
// 실제도달한외부효과만필수로요구하며선행부분쓰기를되돌리지않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Editor마지막처리의실제경계미연결");return function;
}
// 원본DWORD의감긴비트를signed로읽는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 실제필드폭만little endian으로읽고물리메모리밖은호스트에서명시거부한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::uint32_t at,std::uint32_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Editor마지막처리raw조회물리범위");std::uint32_t value{};
    for (std::uint32_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8); // 지정필드의실제BYTE만읽는다.
    return value;
}
// 원본표의지정WORD/BYTE만쓰며현재뷰를벗어나면앞선효과를유지한채거부한다.
void Put(std::span<std::uint8_t> bytes,std::uint32_t at,std::uint32_t width,std::uint32_t value) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Editor마지막처리표쓰기물리범위");
    for (std::uint32_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 이웃표항목은보존한다.
}
// 실제__ftol은float32입력을signed64절삭하고하위DWORD를반환한다.비정상/범위밖의EAX는0이다.
std::int32_t Truncate(float input) {
    const double value=input; // float32는double에정확히표현되어추가반올림이없다.
    if (!std::isfinite(value) || value>=kInt64Boundary || value< -kInt64Boundary) return 0;
    return Signed(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 실제raw주소에서동일물리50BYTE를고정한다.유효관문과이물리거부는별개다.
EditorWorldFinishForm Form(const EditorWorldFinishPool& pool,std::uint32_t offset) {
    if (offset>pool.bytes.size() || kStride>pool.bytes.size()-offset) throw std::out_of_range("Editor마지막처리Pool물리범위");
    return {pool.bytes.subspan(offset,kStride),pool.address+offset};
}
// 최초0040eb40과인라인증가는같은cached상한/기준Pool에서free bit0만건너뛴다.
void Advance(std::int32_t& index,std::int32_t cachedEnd,const EditorWorldFinishPool& pool) {
    index=Signed(static_cast<std::uint32_t>(index)+1U);
    while (index<cachedEnd) { // 루프도중다른capacity를읽지않으며최초상한밖free를추가생략하지않는다.
        const auto offset=static_cast<std::uint32_t>(index)*kStride; // 원본DWORD주소산술의감김이다.
        if (!(Read(pool.bytes,offset+kState,1)&1U)) break;
        index=Signed(static_cast<std::uint32_t>(index)+1U);
    }
}
}
// 같은원문캐시위치를유지하며타입이일치하지않는객체에Finder/삭제효과를추가하지않는다.
void FinishEditorWorld(const EditorWorldFinishHooks& hooks) {
    const auto initial=Need(hooks.pool)();const auto cachedEnd=Signed(initial.capacity);std::int32_t index=1; // 실제iterator생성의두DWORD다.
    Advance(index,cachedEnd,initial);
    for (;;) { // 삭제/보존뒤현재Pool을다시읽고최초cachedfree-skip상한만유지한다.
        const auto pool=Need(hooks.pool)();const auto offset=static_cast<std::uint32_t>(index)*kStride;const auto address=pool.address+offset; // 후보시작의현재Pool기준과고정원본주소다.
        const auto capacity=Need(hooks.pool)().capacity; // 원문유효관문의현재capacity이며초기값으로대체하지않는다.
        if (address<pool.address+kStride || address>=pool.address+capacity*kStride) { Need(hooks.graph)(0);return; }
        const auto form=Form(pool,static_cast<std::uint32_t>(Signed(offset)/static_cast<std::int32_t>(kStride))*kStride); // 원문signed나눗셈뒤고정한실제form주소다.
        Advance(index,cachedEnd,pool); // 현재타입을읽기전에다음iterator를진행한다.
        if (Read(form.bytes,kType,1)!=Need(hooks.noIslandType)()) continue;
        const auto roundedX=static_cast<float>(Truncate(std::bit_cast<float>(Read(form.bytes,kX)))),roundedY=static_cast<float>(Truncate(std::bit_cast<float>(Read(form.bytes,kY)))); // 실제0041d770의두float32쓰기다.
        const auto x=Truncate(roundedX),y=Truncate(roundedY);auto cursor=Need(hooks.begin)(form,x,y,x,y);bool island=false; // 실제Begin은정수점/기본필터이며후보전체를소진한다.
        for (;;) { // 첫island발견뒤에도Next/현재타입/현재Pool읽기를생략하지않는다.
            const auto candidate=Need(cursor.current)();if (!candidate) break;const auto currentPool=Need(hooks.pool)();
            if (Read(currentPool.bytes,candidate*kStride+kType,1)==Need(hooks.islandType)()) island=true;
            Need(cursor.next)();
        }
        bool keep=false; // Finder가끝난뒤현재캡처form좌표의엄격한열린범위를판정한다.
        if (island) {
            const auto currentX=std::bit_cast<float>(Read(form.bytes,kX)),currentY=std::bit_cast<float>(Read(form.bytes,kY)); // 원문은두좌표를먼저캡처한다.
            if (currentX>0.0F) {
                const auto bound=static_cast<float>(Need(hooks.boardSize)()); // lowerX성공뒤한번만float32로읽어두축에사용한다.
                keep=currentX<bound && currentY>0.0F && currentY<bound;
            }
        }
        if (keep) continue;
        const auto deleteX=Truncate(std::bit_cast<float>(Read(form.bytes,kX))),deleteY=Truncate(std::bit_cast<float>(Read(form.bytes,kY))); // 삭제직전현재좌표를다시읽는다.
        const auto cell=(static_cast<std::uint32_t>(deleteY)<<8)+static_cast<std::uint32_t>(deleteX);const auto wordOffset=cell<<1; // 원문표주소의DWORD감김과고정WORD오프셋이다.
        const auto temporarySid=Signed(form.address-Need(hooks.pool)().address)/static_cast<std::int32_t>(kStride); // 가상호출전에현재기준으로표에쓸SID를구한다.
        Put(Need(hooks.surfaces)(),wordOffset,2,static_cast<std::uint32_t>(temporarySid));
        Need(hooks.unpop)(form,0);
        Put(Need(hooks.spots)(),cell,1,0); // 가상효과뒤현재spot표를읽되셀은최초삭제좌표를유지한다.
        const auto releaseSid=Signed(form.address-Need(hooks.pool)().address)/static_cast<std::int32_t>(kStride); // 가상효과가바꾼현재Pool기준을다시쓴다.
        Need(hooks.release)(releaseSid);
        Put(Need(hooks.surfaces)(),wordOffset,2,0); // Release뒤현재surface표를다시읽고원래WORD오프셋에0을쓴다.
    }
}
}
