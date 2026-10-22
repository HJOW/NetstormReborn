// 데빌메이커 전용80BYTE 본체의 생성자와 변경 필드 패킷을 원본10.78 순서로 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::o {
// 원본 outer0/inner+38의80BYTE다. 생성자가 쓰지 않는 필드와 padding도 caller 입력으로 보존한다.
struct WindAviaryProcessFields {
    std::array<std::uint8_t,0x50> bytes{}; // 후속 생산/비행 상태 기계와 전송이 공유할 유일한 원문 이미지다.
};
// 실제0045d730/0045d7a0→0045b150/0045b210의 저장·전송 규칙이다. 실행 프레임은 별도 후속 몸체다.
class RawWindAviaryState {
public:
    // 서버일 때만 상태1/현재 시각을 쓴다. 비서버의 미정 상태와 두 예약 double 주변은 그대로 둔다.
    static void Initialize(WindAviaryProcessFields& fields,bool server,double now);
    // inner Form/부모와 실제 변경 비트를 사용하고 끝에서 dirty만0으로 바꾼다. 반환은 실제 길이 WORD다.
    static std::uint16_t WriteProcess(WindAviaryProcessFields& fields,std::span<std::uint8_t> packet,
        bool includeParent,bool force=false,std::uint8_t protocol=1,std::uint8_t message=0x74);
    // 현재 packet의 선택 필드만 덮고 부모 출력 DWORD를 돌려준다. dirty와 inner 소유권은 바꾸지 않는다.
    static std::uint32_t ReadProcess(WindAviaryProcessFields& fields,std::span<const std::uint8_t> packet);
};
}
