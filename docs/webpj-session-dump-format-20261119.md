# WB04-E865 Session 상태 덤프 순수 서식

기준선 `225553df`의 `FormatSessionStateDump`, `SessionDumpHash`, `DecodeScrambledSp`를 웹 순수 함수로 옮겼다. 판번호 1의 헤더, Kernel/Regular, 풀 전체 해시·선택 슬롯·판본별 자산, Player·숨김·동맹·분산 SP·Graph·TotalMade 행을 지정된 순서와 비트 표현으로 만든다. 웹 게임 실행 자체는 10.78이지만 서식은 cpppj 시험과 같이 CD 10.72 필드 폭도 구분한다.

집중 검사는 FNV-1a 공개값, 실제 분산 SP 저장소의 회전 전후 복호, 두 판본의 원문 필드 폭·free 생략·정기/장부 행, 월드 없음과 잘못된 크기를 확인했다. cpppj `SessionStateDumpTests.cpp`의 독립 상수와 행 조건을 사용했으며 실제 cpppj 실행 결과를 새로 수집한 것은 아니다. Node v26.13.0 전체 회귀 6,401건은 실패·취소·건너뜀 없이 통과했다.

이 단위는 호출자가 전달한 관측값을 서식화한다. 실제 웹 Client/Session의 현재 Kernel·풀·Player·SP·난수 수집, 덤프 호출 시점, `cpp-oracle` 시나리오와 Tutorial1 L2 비교는 아직 연결하지 않았다. 원본 게임 창은 실행하지 않았다.
