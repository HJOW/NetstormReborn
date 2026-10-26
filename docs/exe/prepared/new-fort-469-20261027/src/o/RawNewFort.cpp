// 원문 NewFort의 부분효과·재조회·명시된 하위 장치만 보존하며 제품 통합은 별도다.
#include "o/RawNewFort.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace netstorm::o { // 같은 Client의 저장/난수/Player를 빌리는 세 원문 부모다.
namespace { // 현재 물리 접근과 선택된 callback만 확인한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("NewFort의 선택된 실제 하위 미연결");return function; } // 미선택 경계를 빈성공 callback으로 요구하지 않는다.
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t at) { // header의 정확한DWORD만 little endian으로 읽는다.
    std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);return value; // 주변 stale 필드를 해석하지 않는다.
}
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } // 원문DWORD 범위만 쓴다.
std::string_view CString(std::span<const std::uint8_t> bytes) { // 실제 handle의 첫NUL까지 빌리며 내용을 복사/trim하지 않는다.
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("NewFort 현재 handle의 NUL 물리 범위");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
}
void CreateNewFortManager(std::span<std::uint8_t> header,std::string_view name,std::uint32_t argument,const NewFortManagerHooks& hooks) { // 진단/exists 뒤 같은 입력 이름을 사용한다.
    if (header.size()!=20) throw std::invalid_argument("NewFort의 실제20BYTE header 누락");if (Word(header,0)) Need(hooks.assertion)(0x172);
    Put(header,4,0);Put(header,16,argument);
    if (Need(hooks.exists)(name)) {
        const auto allocation=Need(hooks.allocate)(0x114);const auto temporary=allocation ? Need(hooks.construct)(allocation,name) : 0U;
        if (!temporary) throw std::out_of_range("원문 새 manager 임시NULL의 가상 이름 접근"); // 실제004bfbff의NULL접근이며삭제/해제를성공으로진행하지않는다.
        const auto oldName=Need(hooks.fileName)(temporary);Need(hooks.erase)(oldName);Need(hooks.destroy)(temporary);Need(hooks.release)(temporary);
    }
    const auto allocation=Need(hooks.allocate)(0x114);const auto current=allocation ? Need(hooks.construct)(allocation,name) : 0U;
    Put(header,0,current);Need(hooks.initialize)(current); //공장뒤현재ptr를공개한뒤실제초기화한다.할당0도이원문호출로이어진다.
}
void CreateNewFortData(NewFortDataGlobals globals,const NewFortDataHooks& hooks) { // 로컬 Territory와 현재 Client 전역의 수명은 구별한다.
    globals.subscriber=0xffffffffU;const auto unnamed=Need(hooks.translate)("Unnamed");const auto length=unnamed.find('\0');const auto text=unnamed.substr(0,length);auto handle=Need(globals.handle)();
    if (handle.size()<=text.size()) throw std::out_of_range("NewFort Unnamed의 실제 handle 출력 폭");
    for (std::size_t index=0;index<text.size();++index) handle[index]=static_cast<std::uint8_t>(text[index]);handle[text.size()]=0; // 원문의 앞으로복사와마지막NUL을기록한다.
    Need(hooks.configure)("fort.mySubHandle",CString(Need(globals.handle)()));Need(hooks.title)();Need(hooks.registerSubscriber)();
    std::array<std::uint8_t,120> records{}; //004b3975는20개레코드의6BYTE를각각0으로쓴다.
    if (!Need(globals.editor)()) { records[0]=static_cast<std::uint8_t>((records[0]&0xc7U)|7U);Need(hooks.prepareRandom)();records[3]=static_cast<std::uint8_t>(Need(hooks.nextRandom)(254)+1U);records[1]|=1;records[2]=0x11; }
    const auto money=Need(globals.playerKnown)() ? Need(hooks.sp)(Need(globals.localOwner)()) : 0.0F;Need(hooks.addSp)(-money);
    globals.coreActive=1;auto core=Need(hooks.coreData)();if (core.size()<10) throw std::out_of_range("NewFort 현재 CoreData의10BYTE 물리 범위");std::fill_n(core.begin(),10,std::uint8_t{}); //4/4/2로0을쓰는0042b200의본문이다.
    Need(hooks.coreWrite)(Need(globals.templateOwner)());Need(hooks.prepareRandom)();
    do { const auto next=Need(hooks.nextRandom)(65536);globals.coreSeed=(globals.coreSeed&0xffff0000U)|(next&0xffffU); } while ((globals.coreSeed&0xffffU)==0); // 매반환뒤현재상위WORD를보존하고0도실제로기록한뒤재시도한다.
    const auto currentHandle=Need(globals.handle)();const auto subscriberOwner=Need(globals.templateOwner)();const auto id=globals.subscriber;Need(hooks.subscriber)(subscriberOwner,id,currentHandle); //004b3a48은문자열내용/길이대신현재포인터만캡처한다.실제writer의등록후읽기를보존한다.
    Need(hooks.configure)("fort.mySubHandle",CString(Need(globals.handle)()));Need(hooks.title)();Need(hooks.territory)(Need(globals.templateOwner)(),records);Need(hooks.money)(Need(globals.templateOwner)());
    const auto editor=Need(globals.editor)();const auto stateOwner=Need(globals.templateOwner)();const auto kind=editor ? 2U : 0U;auto state=Need(hooks.state)(stateOwner);if (state.size()<18) throw std::out_of_range("NewFort 현재 State+11의 물리 범위");state[17]=static_cast<std::uint8_t>(kind);Need(hooks.notify)(Need(globals.templateOwner)());
}
bool InstallMissingNewFort(std::uint32_t& alternate,const NewFortMissingFileHooks& hooks) { // 기존파일 분기를 이 신규부모의 성공으로 가장하지 않는다.
    if (Need(hooks.exists)(Need(hooks.path)())) return false;
    const auto name=Need(hooks.path)();Need(hooks.manager)(Need(hooks.owner)(),name,0);
    auto state=Need(hooks.state)(Need(hooks.owner)());if (state.empty()) throw std::out_of_range("NewFort State 첫BYTE 물리 범위");state[0]|=1U;Need(hooks.notify)(Need(hooks.owner)());Need(hooks.data)();alternate=1;
    Need(hooks.subscriber)(Need(hooks.owner)());Need(hooks.configure)("fort.mySubHandle",Need(hooks.handle)());Need(hooks.title)();return true;
}
}
