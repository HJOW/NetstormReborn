// Client가 AI 진단의 실제 현재 DWORD를 메뉴/입력/Session/Renderer에 같은 주소로 제공한다.
#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace netstorm::client { // 원본 전역의 수명은 짧은 World/Session보다 길다.
class ClientAiDebugGlobals {
public:
    ClientAiDebugGlobals()=default; // 최초PE/BSS값만만들며장치/파일/AI행동은실행하지않는다.
    ClientAiDebugGlobals(const ClientAiDebugGlobals&)=delete; // 같은현재전역주소를별도사본으로복제하지않는다.
    ClientAiDebugGlobals& operator=(const ClientAiDebugGlobals&)=delete; // 살아있는로그참조의소유자를바꾸지않는다.
    std::int32_t& Checking() { return checking_; } // showSoftAsserts의 최초BSS0/누락유지와 현재전체비트다.
    const std::int32_t& Checking() const { return checking_; } // 메뉴와Renderer가 같은현재값을읽는다.
    std::uint32_t& Word(std::uint32_t address) { // 실제 원문 주소식별자만 현재저장소로 해석한다.
        for (std::size_t index=0;index<addresses_.size();++index) if (addresses_[index]==address) return words_[index]; // 원문7로그의순서/초기값을복제하지않는다.
        throw std::out_of_range("현재 AI 진단 DWORD 주소 미연결");
    }
    std::uint32_t Read(std::uint32_t address) const { // const조회도같은물리를읽고임의포인터는역참조하지않는다.
        for (std::size_t index=0;index<addresses_.size();++index) if (addresses_[index]==address) return words_[index]; // 원문주소의정확한일치만허용한다.
        throw std::out_of_range("현재 AI 진단 DWORD 주소 미연결");
    }
private:
    inline static constexpr std::array<std::uint32_t,7> addresses_{0x52f3cc,0x52f3d0,0x52f3d4,0x52f3d8,0x52f3dc,0x52f3e0,0x52f3e4}; // 보호PE의7로그전역이다.
    std::array<std::uint32_t,7> words_{99,99,29,99,99,99,29}; // 동일PE초기값이며미션대여/반납에서재설정하지않는다.
    std::int32_t checking_{}; // 005e4794의최초BSS0이며BOOL로좁히지않는다.
};
}
