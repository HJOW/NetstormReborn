# WB04-E784 Session State Player 링크·Deck 반납

cpppj `RawCampaignSession.cpp`의 `Production`, `ReleasePlayerLinks`, `ClearPlayerLinksForState`, `ReturnPlayerLinksForState`를 같은 Session의 현재 Player 기록과 생산 Deck 소유자에 연결했다. 원본 순서로 기술 링크와 개수를 지우고 실제 Deck을 반환한 뒤 포인터를 지우며, 정상 복귀 증빙은 발급한 Deck이 한 번만 소비한다. 이미 State에 반납한 링크의 일반 반복 정리와 Deck 사용은 거부한다.

독립된 두 실제 Session에서 생산 조회로 Deck을 확보하고 링크 정리 뒤 실제 할당 블록·Player 세 필드가 비워지며 인접 필드는 보존됨을 확인했다. 다른 Session의 증빙과 중복 반납도 거부했다. 잘못된 Deck 토큰은 앞선 두 쓰기를 보존한 채 실패하고 다음 Player에 도달하지 않는다. 전체 State 순서와 Process 반납, 실제 게임 종료 UI는 남는다.

Node v26.13.0 전체 회귀 6,287건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E779의 Node v22 전체 회귀 뒤 다섯 번째 v26 회귀다.
