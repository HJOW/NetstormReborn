# WB04-E866 Session 덤프의 Client 공통 관측

`captureSessionDumpBase`는 현재 Client의 판본, 고정된 프레임, 같은 게임/CRT 난수 소유자와 Kernel을 받아 판번호 1의 월드 없는 입력을 수집한다. PID 1부터 현재 슬롯을 읽고 SquidProcess의 Form·부모, Regular의 사건·횟수·payload·시각 비트를 기록한다. 관측은 시계 진행, 난수 소비와 Kernel 실행을 하지 않는다.

Regular의 payload 비트 읽기 경계를 추가해 패킷으로 수신한 signaling NaN도 숫자 재변환 없이 보존한다. 집중 검사는 실제 GameClock·ClientRandomGlobals·Kernel과 TheWarBegins Session의 실제 Regular, 공유 Regular 패킷의 원문 비트를 확인했다. Node v26.13.0 전체 회귀 6,404건은 실패·취소·건너뜀 없이 통과했다. 별도 cpppj 런타임 관측은 수집하지 않았다.

실제 브라우저 Client 장기 수명에서 호출하는 연결과 월드 풀·Player·SP·Graph·TotalMade 수집, 오라클 시나리오는 남아 있다. 원본 게임 창은 실행하지 않았다.
