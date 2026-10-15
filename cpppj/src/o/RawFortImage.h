// 현재Template의35개원문섹션을PATCH DataManager SaveImage형식으로직렬화한다.
#pragma once
#include "o/Template.h"
#include <functional>

namespace netstorm::o {
// 원본SaveImage의진단후계속조건이며물리범위초과는호스트에서별도로거부한다.
enum class FortImageAssertion {
    TypeCount, //004bcd80의numTypes<256진단이다.
    StreamCapacity, //memstream의pos+length<=alloc진단이다.
    SectionCapacity //0044d640의destStream.pos+s->end<alloc엄격진단이다.
};
// 원본0x8000메모리stream을소유하며논리끝과할당의0tail을구분한다.
struct FortImageData {
    std::vector<std::uint8_t> storage; //실제32768바이트0초기화후현재섹션을쓰는단일배열이다.
    std::size_t end{}; //원본stream+8의현재논리끝이며반환cursor는0이다.
    //실제이미지끝까지만빌린다.배열밖으로변조된end는거부한다.
    std::span<const std::uint8_t> Bytes() const;
};
// 현재타입개수와원본선택진단만외부경계다.파일읽기나게임월드를대신하지않는다.
struct FortImageHooks {
    std::function<std::uint32_t()> typeCount; //004bcd80이읽는현재00541348이다.
    std::function<void(FortImageAssertion)> assertion; //해당원문진단이선택된위치에서만필수다.
};
// 섹션현재원문을복사하며예전파일bytes/해석필드로내용을대체하지않는다.
class RawFortImage {
public:
    static constexpr std::size_t kCapacity=0x8000; //0044d640이실제memstream에확보하는고정물리크기다.
    //fort/현재조회대상은Save가끝날때까지살아있어야한다.섹션순서는FortTemplate::SectionNames와같다.
    RawFortImage(OriginalEdition edition,const FortTemplate& fort,FortImageHooks hooks);
    //004bf520→0044d640/004bcd80의F/flag+35개길이WORD/현재payload/마지막길이수정을수행한다.
    FortImageData Save() const;
private:
    const FortTemplate& fort_; //현재35개원문섹션과reader가읽은flag이며별도파일이미지캐시는없다.
    FortImageHooks hooks_; //현재타입개수/실제선택진단만빌린다.
};
}
