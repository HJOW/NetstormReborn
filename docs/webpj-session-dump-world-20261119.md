# WB04-E867 실제 Session 월드 덤프 수집

`captureSessionStateDump`는 Client의 공통 관측 뒤 같은 Kernel과 난수 소유자를 쓰는 실제 Session의 풀 전체, 자유 목록, Player·숨김·동맹, 분산 SP 원문, Graph와 생산 장부를 사본으로 수집한다. Session에 같은 SP 저장소와 난수 소유자의 읽기 경계를 공개했으며 다른 Kernel·난수를 섞으면 거부한다. SP는 미초기화면 분산 배열을 읽지 않고 초기화하지 않는다.

실제 TheWarBegins Session에서 Load 전후 풀·난수·Kernel이 관측으로 변하지 않고, 이후 월드 변경도 이미 수집한 풀 사본에 반영되지 않는 것을 확인했다. 그래프는 관련 문서의 251칸 설명과 달리 cpppj `RawGraph::Records()`의 실제 255칸 전체(251~253의 틈과 254 sentinel 포함)를 수집한다. 최초 집중 검사에서는 독립 Session 생성자가 SP를 이미 초기화한다는 사실을 놓친 잘못된 시험 전제를 고쳤다. Node v26.13.0과 v22.23.3 전체 회귀는 각각 6,405건 모두 통과했다. 실제 cpppj 덤프 수집·비교는 하지 않았다.

브라우저 Client의 덤프 호출 시점과 오라클 시나리오·Tutorial1 L2 비교는 남아 있다. 원본 게임 창은 실행하지 않았다.
