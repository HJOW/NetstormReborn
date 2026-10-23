// 경로/소스 캡처와 실제 파일 객체의 부분효과를 원문 위치에 유지한다.
#include "o/RawColorMapCache.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr double kHighlightThreshold=0.800000011920929; //00506710의 실제 double이며 같은 값/NaN은 교체하지 않는다.
// 원문이 선택한 필수 하위만 그 호출 시점에 검사한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("ColorMap cache의 실제 하위 장치 미연결");return function;
}
// Native REP MOVSD14의 각 DWORD 물리를 읽고 임시 기록에 복사한다. 실패 전에 쓴 출력은 되돌리지 않는다.
double CurrentLight(std::span<const std::uint8_t> primary,std::size_t offset) {
    std::array<std::uint8_t,56> record; // 원문56BYTE 지역 사본이며 입력 물리를 전체 읽는다.
    for (std::size_t word=0;word<14;++word) { //각4BYTE 복사 직전에만 현재 source의 물리 범위를 확인한다.
        const auto source=offset+word*4;
        if (source>primary.size() || 4>primary.size()-source) throw std::out_of_range("ColorMap cache raw ColorSort 기록 물리범위");
        std::copy_n(primary.begin()+static_cast<std::ptrdiff_t>(source),4,record.begin()+static_cast<std::ptrdiff_t>(word*4));
    }
    std::uint64_t bits{}; // 기록+8의 little endian double 원시 비트다.
    for (std::size_t byte=0;byte<8;++byte) bits|=std::uint64_t(record[8+byte])<<(byte*8); //호스트의 정렬/패딩을 원문이라고 읽지 않는다.
    return std::bit_cast<double>(bits);
}
}

// 원문은 source 포인터를 한 번 캡처하지만 각 행의 lowBYTE는 해당 행 시작에서 읽는다.
std::span<const std::uint8_t> PrepareColorMapCache(const ColorMapCacheGlobals& globals,const ColorMapCacheHooks& hooks) {
    const auto path=Need(hooks.path)();const auto primary=Need(globals.primary)(); //경로 장치가 바꾼 현재 primary를 그 뒤에 빌린다.
    for (std::size_t row=0;row<8;++row) { //원문8행의 순서이며 이후 owners callback에서 다시 생성하지 않는다.
        const auto selected=static_cast<std::uint8_t>(globals.ownerColors[row]); //현재 DWORD의 lowBYTE만 쓴다.
        for (std::size_t color=0;color<256;++color) { //원문은 solid→identity→56BYTE복사→명도조건 쓰기 순서다.
            const auto offset=row*256+color;globals.ownerSolid[offset]=selected;globals.ownerHighlight[offset]=static_cast<std::uint8_t>(color);
            if (CurrentLight(primary,color*56)>kHighlightThreshold) globals.ownerHighlight[offset]=selected;
        }
    }
    Need(hooks.owners)();return path;
}

// 실제 close는 같은 물리/장치가 소유한다. 정상 close 전에 부모는 constructed 관문을 먼저 내린다.
void CloseColorMapCacheFile(std::span<std::uint8_t,24> file,const ColorMapCacheHooks& hooks) { Need(hooks.close)(file); }

// writer는 reader와 다른 지역 객체이며 그 ctor 직전의 미정24BYTE를 부모가 초기화하지 않는다.
void RegenerateColorMapCache(std::span<const std::uint8_t> path,const ColorMapCacheGlobals& globals,const ColorMapCacheHooks& hooks,std::span<std::uint8_t,24> writer) {
    Need(hooks.buildMaps)();bool constructed=false; //maps 실패는 아직 File을 만들지 않은 catch 단계다.
    try {
        Need(hooks.openWrite)(writer,path,1,1);constructed=true;
        for (auto table:globals.maps) static_cast<void>(Need(hooks.write)(writer,table,0xffffffffU)); //아홉 반환 길이를 검사하거나 짧음으로 분기하지 않는다.
        constructed=false;CloseColorMapCacheFile(writer,hooks); //정상 close 실패에서 같은 객체를 두 번 close하지 않는다.
    }catch (...) {
        if (constructed) { constructed=false;CloseColorMapCacheFile(writer,hooks); } //실제 선택된 writer cleanup과 같은 고정 물리를 반납한다.
        throw;
    }
}

// 경로/TYPE표 부모는 try 앞이다. 그 단계의 marker를 reader cache miss로 잘못 잡지 않는다.
void InitializeColorMapCache(const ColorMapCacheGlobals& globals,const ColorMapCacheHooks& hooks,std::span<std::uint8_t,24> reader,std::span<std::uint8_t,24> writer) {
    const auto path=PrepareColorMapCache(globals,hooks);std::fill(reader.begin()+16,reader.end(),std::uint8_t{}); //reader 마지막 두DWORD만 원문 위치에서0쓴다.
    bool constructed=false; //ctor가 정상 복귀하기 전에는 outer destructor를 실행하지 않는다.
    try {
        try {
            Need(hooks.openRead)(reader,path,0,1);constructed=true;
            for (auto table:globals.maps) static_cast<void>(Need(hooks.read)(reader,table,0xffffffffU)); //같은 고정 아홉 목적 물리에 부분 읽기를 남긴다.
            constructed=false;CloseColorMapCacheFile(reader,hooks);
        }catch (...) {
            if (constructed) { constructed=false;CloseColorMapCacheFile(reader,hooks); } //원문 reader funclet은 writer와 같은 객체가 아니다.
            throw;
        }
    }catch (const ColorCacheUnableToOpen&) {
        RegenerateColorMapCache(path,globals,hooks,writer); //writer/catch 실패는 이 reader try로 재진입하지 않는다.
    }
}
}
