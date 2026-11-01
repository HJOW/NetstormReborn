// 원문 파일 형식과 미작성 지역 BYTE를 현대 검사용 RGBA BMP와 섞지 않는다.
#include "client/RawIndexedBitmap.h"
#include <stdexcept>

namespace netstorm::client { // 원문의 정수 폭/현재 인자/부분 쓰기를 유지하는 본체다.
namespace { // 장치와 명시 물리만 확인하며 원문 미정 바이트를 새로 채우지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("8비트 BMP의 선택된 실제 장치 미연결");return hook; } // 선택되지 않은 표면/색표 장치를 요구하지 않는다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t length=4) { // 정렬과 호스트 구조체 padding에 의존하지 않는다.
    for (std::size_t index=0;index<length;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); // 지정된 낮은 바이트만 쓴다.
}
std::span<const std::uint8_t> Memory(const IndexedBitmapHooks& hooks,std::uint32_t token,std::uint32_t offset,std::uint32_t length) { // Native가 실제로 읽는 지역 밖의 BYTE를 요구하지 않는다.
    const auto bytes=Need(hooks.memory)(token,offset,length);if (bytes.size()<length) throw std::out_of_range("8비트 BMP의 실제 픽셀/색표 물리 범위");return bytes.first(length);
}
void Body(const IndexedBitmapInput& input,IndexedBitmapScratch& scratch,const IndexedBitmapHooks& hooks) { // FILE이 열린 뒤에만 원문 헤더/픽셀을 소비한다.
    const auto width=input.width,height=input.height;
    Put(scratch.file,0,0x4d42,2);Put(scratch.file,2,width*height+0x436U);Put(scratch.file,6,0,2);Put(scratch.file,10,0x436);
    Need(hooks.write)(scratch.file,-1);
    Put(scratch.info,0,40);Put(scratch.info,4,width);Put(scratch.info,8,height);Put(scratch.info,12,1,2);Put(scratch.info,14,8,2);
    Put(scratch.info,16,0);Put(scratch.info,20,0);Put(scratch.info,32,0);Put(scratch.info,36,0);Need(hooks.write)(scratch.info,-1);
    const auto palette=Memory(hooks,input.palette,0,1023); // 마지막RGB의세번째BYTE까지만실제로읽으며네번째BYTE를요구하지않는다.
    for (std::size_t index=0;index<256;++index) { // 원문은 RGB0를 BGR로 바꾸고 출력 네번째BYTE는 보존한다.
        scratch.palette[index*4+2]=palette[index*4];scratch.palette[index*4+1]=palette[index*4+1];scratch.palette[index*4]=palette[index*4+2];
    }
    Need(hooks.write)(scratch.palette,-1);
    const auto pixels=input.pixels;auto offset=(input.y+height-1U)*input.pitch+input.x;
    for (auto remaining=height;remaining!=0;--remaining) { // 높이/폭은 앞서 캡처하고 매 행 뒤에는 현재 stride를 다시 읽는다.
        Need(hooks.write)(Memory(hooks,pixels,offset,width),-1);offset-=input.pitch;
    }
}
}
void WriteRawIndexedBitmap(std::string_view name,const IndexedBitmapInput& input,IndexedBitmapScratch& scratch,const IndexedBitmapHooks& hooks) {
    Need(hooks.construct)(name.substr(0,name.find('\0')),1,1);
    try { if (Need(hooks.isOpen)()) Body(input,scratch,hooks); }catch (...) { Need(hooks.destroy)();throw; } // 원문 지역 BaseFile의 예외 반납 개념을 유지한다.32비트SEH ABI 자체를 합성하지 않는다.
    Need(hooks.destroy)();
}
}
