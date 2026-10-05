// 새로 쓰는 코드(원본에 없음): 복원한 Renderer에 검사 장면을 공급하는 임시 어댑터.
// 원본 방식의 화면 장치(8비트 DIB, 원본 팔레트, VFX 그리기)와 요새 읽기·영역 배치가 실제로 동작하는지 눈으로 확인하는 데 쓴다.
// 게임 월드의 재현은 아니다. Squid·지형·메뉴를 연결하면 해당 장면 생성 경로로 교체한다.
#pragma once
#include "client/ClientMain.h"
#include <string>
#include <vector>

namespace netstorm::app {
class InspectView {
public:
    // 보여 줄 내용: "types"는 타입 기본 프레임 표, 그 밖의 값은 미션 이름으로 보고 요새의 오브젝트를 놓는다.
    InspectView(netstorm::client::Client& client, std::string view, bool connect = true);
    // 메뉴의 미션 진입에서도 같은 정적 장면을 제출한다. 실제 월드 생성은 후속이다.
    void Ready(netstorm::client::Client& client);
    // 표시 크기를 바꾸거나 대화상자를 닫은 뒤 정적 장면을 다시 제출한다.
    void BuildScene(netstorm::client::Client& client);
private:
    // 월드에 놓은 오브젝트 하나: 월드 칸 좌표, 타입(로딩 순서), 그릴 프레임.
    struct Placed {
        int cellX{}, cellY{};
        std::size_t asset{};
        std::size_t frame{};
    };
    // 프레임마다: 입력 큐를 비우고 Esc면 참(종료)을 돌려준다. 화살표 키로 화면을 옮긴다.
    bool Input(netstorm::client::Client& client);

    std::string view_;
    std::vector<Placed> placed_;
    int scrollX_{}, scrollY_{};
};
}
