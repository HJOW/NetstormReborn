// 원본 편집 버퍼의 부분실패/현재 재조회/선택과 LF 줄offset을 유지한다.
#include "client/TextEntryBuffer.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kBufferVtable=0x00508da8,kInitialData=0x800,kOffsetBytes=0x200; // 원본의 가상표/기본본문/줄표할당 크기다.
// 실제signedDWORD의하위비트를유지한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// ADD/SUB의하위DWORD만남겨호스트signed넘침을피한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return Signed(std::uint32_t(a)+std::uint32_t(b)); }
std::int32_t Subtract(std::int32_t a,std::int32_t b) { return Signed(std::uint32_t(a)-std::uint32_t(b)); }
// 원시주소는부호확장하지않고32BIT에서합산한다.
std::uint32_t Address(std::uint32_t base,std::int32_t offset) { return base+std::uint32_t(offset); }
// 호스트범위를원본signed복사길이로만사용한다.
std::size_t Count(std::int32_t value) { if (value<0) throw std::out_of_range("편집버퍼 음수메모리길이");return static_cast<std::size_t>(value); }
// NUL이포함된입력만빌린다.빈span은원문의null문자열이다.
std::size_t CStringLength(std::span<const char> text) {
    if (text.empty()) return 0;const auto end=std::find(text.begin(),text.end(),'\0');if (end==text.end()) throw std::invalid_argument("편집버퍼 NUL입력범위 필요");return static_cast<std::size_t>(end-text.begin());
}
// 원본REP MOVSD→MOVSB처럼앞에서부터4BYTE씩읽고쓴다.유효한겹침도그순서를보존한다.
void ForwardCopy(std::span<std::uint8_t> target,std::span<const std::uint8_t> source,std::size_t count) {
    if (count>target.size() || count>source.size()) throw std::out_of_range("편집버퍼 복사범위");std::size_t offset{};
    for (;count-offset>=4;offset+=4) { const std::array<std::uint8_t,4> word{source[offset],source[offset+1],source[offset+2],source[offset+3]};std::copy(word.begin(),word.end(),target.begin()+offset); } // 원문DWORD복사순서다.
    for (;offset<count;++offset) target[offset]=source[offset]; // 마지막0~3BYTE도앞에서부터복사한다.
}
}
// 소유메모리와현재조회는필수이며clipboard는실제호출분기에서만필수다.
TextEntryBuffer::TextEntryBuffer(TextEntryBufferState& state,TextEntryBufferHooks hooks):state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.allocate || !hooks_.release || !hooks_.resize || !hooks_.bytes) throw std::invalid_argument("편집버퍼 실제메모리장치 필요");
}
// 토큰은원시주소이며필요한물리폭을소유저장소에서검사한다.
std::span<std::uint8_t> TextEntryBuffer::Memory(std::uint32_t address,std::size_t count) const {
    if (!count) return {};auto bytes=address ? hooks_.bytes(address) : std::span<std::uint8_t>{};if (count>bytes.size()) throw std::out_of_range("편집버퍼 현재메모리범위");return bytes.first(count);
}
// 줄표의정렬조건을호스트주소로가정하지않는다.
std::uint32_t TextEntryBuffer::Read(std::uint32_t address) const { const auto bytes=Memory(address,4);return bytes[0]|(std::uint32_t(bytes[1])<<8)|(std::uint32_t(bytes[2])<<16)|(std::uint32_t(bytes[3])<<24); }
// 원본little-endianDWORD쓰기다.
void TextEntryBuffer::Write(std::uint32_t address,std::uint32_t value) const {
    auto bytes=Memory(address,4);for (std::size_t i=0;i<4;++i) bytes[i]=static_cast<std::uint8_t>(value>>(8*i)); // 정렬되지않은주소도같은바이트순서다.
}
// NUL앞에서만LF를찾으며원문현재포인터를DWORD로보존한다.
std::uint32_t TextEntryBuffer::FindNewline(std::uint32_t address) const {
    const auto bytes=address ? hooks_.bytes(address) : std::span<std::uint8_t>{};
    for (std::size_t i=0;i<bytes.size();++i) { if (!bytes[i]) return 0;if (bytes[i]==10) return address+static_cast<std::uint32_t>(i); } // strstr/개행정규화없이원문strchr범위다.
    throw std::out_of_range("편집버퍼 줄검색NUL범위");
}
// 원문은요청크기와무관하게성공시capacity필드에2048을쓴다.본문첫BYTE는보존한다.
void TextEntryBuffer::Construct(std::uint32_t requested) const {
    state_.vtable=kBufferVtable;state_.selectionBegin=state_.selectionEnd=0;state_.capacity=0;
    const auto data=hooks_.allocate(requested ? requested : kInitialData);state_.data=data;if (data) state_.capacity=kInitialData;
    state_.length=0;state_.selectionBegin=state_.selectionEnd=0;const auto offsets=hooks_.allocate(kOffsetBytes);state_.lineCount=0;state_.offsets=offsets;state_.offsetCapacity=128;state_.dirty=1;
}
// data0이면줄표도반납하지않는원문조건을보존한다.
void TextEntryBuffer::Destroy() const {
    state_.vtable=kBufferVtable;if (!state_.data) return;hooks_.release(state_.data);state_.data=0;state_.capacity=0;hooks_.release(state_.offsets);state_.offsets=0;
}
// 선택방향은상태에저장하지않고정렬된양끝을현재length에맞춘다.
void TextEntryBuffer::Select(std::int32_t begin,std::int32_t end) const {
    if (end<begin) std::swap(begin,end);if (begin<0) begin=0;else if (state_.length<begin) begin=state_.length;
    if (end<begin) end=begin;else if (state_.length<end) end=state_.length;state_.selectionBegin=begin;state_.selectionEnd=end;
}
// 길이/원래선택폭은콜백전에고정하지만재할당후본문/현재선택끝/현재length를다시읽는다.
void TextEntryBuffer::Insert(std::span<const char> text) const {
    const auto size=CStringLength(text);if (size>std::numeric_limits<std::int32_t>::max()) throw std::length_error("편집버퍼 입력DWORD범위");
    const auto added=static_cast<std::int32_t>(size),removed=Subtract(state_.selectionEnd,state_.selectionBegin);
    if (removed<added) {
        const auto required=Add(Subtract(state_.length,removed),added);
        if (state_.capacity<required) {
            const auto capacity=required>0 ? (((std::uint32_t(state_.length)-std::uint32_t(removed)+std::uint32_t(added)-1U)>>10)+1U)<<10 : 0U;
            const auto data=hooks_.resize(state_.data,capacity);if (!data) return;state_.data=data;state_.capacity=Signed(capacity);
        }
    }
    if (removed!=added) {
        const auto source=Address(state_.data,state_.selectionEnd),destination=Address(source,Subtract(added,removed));const auto count=Count(Subtract(state_.length,state_.selectionEnd));
        if (count) std::memmove(Memory(destination,count).data(),Memory(source,count).data(),count);state_.length=Add(state_.length,Subtract(added,removed));
    }
    const auto destination=Address(state_.data,state_.selectionBegin);ForwardCopy(Memory(destination,size),{reinterpret_cast<const std::uint8_t*>(text.data()),size},size);
    const auto selection=Add(added,state_.selectionBegin);const auto end=Address(state_.data,state_.length);state_.selectionEnd=selection;state_.selectionBegin=selection;Memory(end,1)[0]=0;state_.dirty=1;
}
// 원본tailcall로Insert입력을같은순서로읽으므로자기본문Reset의첫NUL도보존된다.
void TextEntryBuffer::Reset(std::span<const char> text) const { Memory(state_.data,1)[0]=0;state_.length=0;state_.selectionBegin=state_.selectionEnd=0;state_.dirty=1;Insert(text); }
// 줄표확장중실패하면이미쓴첫offset/줄수/dirty를되돌리지않는다.
bool TextEntryBuffer::EnsureLines() const {
    if (!state_.dirty) return true;state_.lineCount=1;Write(state_.offsets,0);auto newline=FindNewline(state_.data);
    while (newline) { // 원문검색포인터는메모리장치콜백동안고정하고다음검색에도재사용한다.
        ++newline;Write(state_.offsets+std::uint32_t(state_.lineCount)*4,newline-state_.data);state_.lineCount=Add(state_.lineCount,1);
        if (state_.lineCount>=state_.offsetCapacity) {
            const auto offsets=hooks_.allocate(std::uint32_t(state_.offsetCapacity)*4+kOffsetBytes);if (!offsets) return false;
            const auto count=static_cast<std::size_t>(std::uint32_t(state_.offsetCapacity)*4);ForwardCopy(Memory(offsets,count),Memory(state_.offsets,count),count);
            hooks_.release(state_.offsets);const auto capacity=state_.offsetCapacity;state_.offsets=offsets;state_.offsetCapacity=Add(capacity,128);
        }
        newline=FindNewline(newline);
    }
    Write(state_.offsets+std::uint32_t(state_.lineCount)*4,std::uint32_t(state_.length));state_.dirty=0;return true;
}
// 부모는원문의EnsureLines실패뒤에도현재count를받는다.
std::int32_t TextEntryBuffer::LineCount() const { static_cast<void>(EnsureLines());return state_.lineCount; }
// 마지막offset은length이므로LF가있는행의길이에는LF가포함된다.
std::int32_t TextEntryBuffer::LineLength(std::int32_t line) const {
    static_cast<void>(EnsureLines());if (line<0 || line>=state_.lineCount) return -1;const auto address=state_.offsets+std::uint32_t(line)*4;return Signed(Read(address+4)-Read(address));
}
// 출력줄복사성공뒤에만선택쌍을작성한다.마지막length위치의빈선택도원문조건을유지한다.
std::uint32_t TextEntryBuffer::CopyLine(std::int32_t line,std::span<char> output,std::array<std::int32_t,2>& selection) const {
    static_cast<void>(EnsureLines());if (line<0 || line>=state_.lineCount) return 0xffffffffU;const auto address=state_.offsets+std::uint32_t(line)*4;
    const auto count=Read(address+4)-Read(address);if (output.size()>std::numeric_limits<std::int32_t>::max()) throw std::length_error("편집버퍼 출력DWORD범위");if (static_cast<std::int32_t>(output.size())<Add(Signed(count),1)) return 0xffffffffU;
    const auto size=Count(Signed(count));if (size>=output.size()) throw std::out_of_range("편집버퍼 줄복사물리범위");ForwardCopy({reinterpret_cast<std::uint8_t*>(output.data()),output.size()},Memory(state_.data+Read(address),size),size);output[size]=0;
    const auto begin=state_.selectionBegin;
    if (begin<state_.length && (Signed(Read(address+4))<=begin || state_.selectionEnd<Signed(Read(address)))) { selection={-1,-1};return count; }
    selection[0]=Signed(Read(address))<begin ? Subtract(begin,Signed(Read(address))) : 0;
    selection[1]=Signed(Read(address+4))<=state_.selectionEnd ? Signed(count) : Subtract(state_.selectionEnd,Signed(Read(address)));return count;
}
// capacity거부는출력에손대지않으며성공은실제NUL까지의문자열을복사한다.
std::uint32_t TextEntryBuffer::CopyAll(std::span<char> output) const {
    if (output.size()>std::numeric_limits<std::int32_t>::max()) throw std::length_error("편집버퍼 출력DWORD범위");if (static_cast<std::int32_t>(output.size())<Add(state_.length,1)) return 0xffffffffU;
    const auto data=Data();const auto end=std::find(data.begin(),data.end(),0);if (end==data.end()) throw std::out_of_range("편집버퍼 본문NUL범위");const auto size=static_cast<std::size_t>(end-data.begin())+1;
    if (size>output.size()) throw std::out_of_range("편집버퍼 전체복사물리범위");std::copy_n(data.begin(),size,output.begin());return std::uint32_t(state_.length);
}
// 빈선택은복사에서만전체범위로읽고저장선택은바꾸지않는다.
void TextEntryBuffer::Copy() const {
    auto begin=state_.selectionBegin,end=state_.selectionEnd;if (begin==end) { begin=0;end=state_.length; }const auto size=Subtract(end,begin);if (size<=0) return;
    if (!hooks_.clipboardWrite) throw std::runtime_error("편집버퍼 실제clipboard쓰기 없음");const auto bytes=Memory(Address(state_.data,begin),Count(size));hooks_.clipboardWrite({reinterpret_cast<const char*>(bytes.data()),bytes.size()});
}
// 원문Cut은빈선택을실제전체선택으로쓴뒤clipboard와같은Insert를실행한다.
void TextEntryBuffer::Cut() const {
    if (state_.selectionBegin==state_.selectionEnd) { state_.selectionBegin=0;state_.selectionEnd=state_.length; }Copy();Insert();state_.dirty=1;
}
// clipboard읽기뒤현재state에같은Insert를적용하고빈clipboard도dirty1을남긴다.
void TextEntryBuffer::Paste(std::span<const char> excluded) const {
    if (!hooks_.clipboardRead) throw std::runtime_error("편집버퍼 실제clipboard읽기 없음");std::array<char,256> text{};hooks_.clipboardRead(text);const auto length=CStringLength(text);
    if (!excluded.empty()) {
        const auto excludedLength=CStringLength(excluded);std::size_t index{};
        while (index<text.size() && text[index]) { // 원문은제외문자삭제시현재칸을다시검사한다.
            if (std::find(excluded.begin(),excluded.begin()+excludedLength,text[index])==excluded.begin()+excludedLength) ++index;
            else std::memmove(text.data()+index,text.data()+index+1,length-index);
        }
        text.back()=0;
    }
    if (text[0]) Insert(text);state_.dirty=1;
}
// 같은실제0x28필드참조를반환한다.
TextEntryBufferState& TextEntryBuffer::State() const { return state_; }
// 입력/출력모두현재data를조회하고이전재할당뷰를재사용하지않는다.
std::span<std::uint8_t> TextEntryBuffer::Data() const { return state_.data ? hooks_.bytes(state_.data) : std::span<std::uint8_t>{}; }
// 길이와물리소유범위를구별하며현재본문바이트를복제하지않는다.
std::string_view TextEntryBuffer::Text() const { const auto bytes=Memory(state_.data,Count(state_.length));return {reinterpret_cast<const char*>(bytes.data()),bytes.size()}; }
}
