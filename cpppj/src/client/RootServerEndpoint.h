// 원본 서버 행/endpoint의 두 고정 패턴과 현재 포트 선택을 복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { //00458e70/004b3ed0의 문자열 경계이며 네트워크를 열지 않는다.
struct RootServerEndpoint { //원문 지역80BYTE에서 선택한 host와 별도 DWORD 포트다.
    std::string host; //현재 첫 일치의 ':' 앞 그룹 또는 매치 없는 원래 C문자열이다.
    std::uint32_t port{}; //명시 인자가0일 때만 행의 십진 포트/현재 기본 포트를 사용한다.
};
void ReadRootServerRecord(std::string_view record,std::span<char> endpoint,std::span<char> name,std::int32_t* subscriber); //00458e70의 선택 출력0→첫';' 패턴→최대크기-1 복사/선택 subscriber atol이다. 빈 span/null은 원문 NULL 출력이다.
RootServerEndpoint ResolveRootServerEndpoint(std::string_view text,std::uint32_t port,const std::function<std::int32_t()>& configuredPort); //004b3ed0의80BYTE 복사/첫([^:]*):([0-9]+) 선택→기본 포트 조회→명시 포트 우선이다.
}
