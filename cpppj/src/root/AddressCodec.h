// R의20BYTE 주소 문법/5칸 서식 고리와 Windows의 실제 로컬 IPv4 장치를 연결한다.
#pragma once
#include "root/ChallengeRuntime.h"

namespace netstorm::root { // 구형 주소의 바이트/문법을 유지하며 명시 이름의 첫IPv4도 LAN 범위로 제한한다.
class AddressCodec final { // 같은 서식 고리와 callback owner가 R 로그인/현재 CCh보다 오래 살아야 한다.
public:
    AddressCodec()=default; // 생성은 OS 조회·WSA 시작/소켓 열기를 수행하지 않는다.
    AddressCodec(const AddressCodec&)=delete;AddressCodec& operator=(const AddressCodec&)=delete; // 고리 뷰와 callback this를 복제하지 않는다.
    std::string_view Format(std::span<const std::uint8_t,20> address,bool port,bool protocol=false,bool flags=false); //004864b0:5칸30BYTE 고리의 다음 문자열을 빌린다. 같은 칸의 다음 사용 때 무효화된다.
    std::array<std::uint8_t,20> Parse(const std::optional<std::string>& text,std::int32_t port=0); //00486870의80BYTE 지역/문법·현재 flags와 network endian을 복원한다. NULL은 원문 불법 읽기로 거부한다.
    bool Local(std::span<std::uint8_t,20> address); //004870d0의 로컬 IPv4 공급 경계다. 실제 Up 어댑터의 첫 LAN 주소를 사용하며 실패는 원문처럼 family만0으로 쓴다.
    void Bind(LoginRequestHooks& login,ChallengeRuntimeHooks& challenge); // 실제 localAddress/주소 서식·해석 hook을 같은 owner에 연결한다.
private:
    std::array<std::array<char,30>,5> buffers_{};std::size_t next_{}; //004d8ad8의5*30BYTE와004d8d2c의 최초BSS0이다.
    std::optional<std::uint32_t> ResolveLocalName(std::string_view name); //00486870의 OS 이름 조회가 반환한 첫IPv4를 복사한다. 조회 실패/공개주소는 빈 값이며 Parse의원문IP0효과를 유지한다.
};
}
