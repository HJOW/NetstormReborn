# WB04-E812 State Pool·손실 좌표 전역

기준선 `225553df`의 State 전역 중 예측 SID 할당 커서 `005424b0`, 손실 좌표 `005c848c/90`, NS 구역 `005467b8`, 월드 재그리기 `0059a8b0`를 같은 장기 소유자에 연결했다. Pool 커서의 직접 쓰기는 다른 할당 목록·개수를 변경하지 않는다. GameAux 손실 좌표를 같은 float32 배열로 보관해 일반 좌표 접근과 State DWORD 비트 접근이 공유하며 NaN 페이로드·부호 비트를 보존한다. 기준선 이후 관련 cpppj 코드 변경은 없다.

실제 GameAux·Pool·NS 정보·Kencloud 소유자에서 직접 접근을 확인했고 원본 PE 초기값과 기존 SID 풀 fixture 회귀를 유지했다. Audio·Template·TYPE·메뉴 팝업, WORD/double 접근과 실제 State 부모 결선은 후속이다.

검증: State 전역 집중 검사와 GameAux·SID 풀 검사 21건 통과, 대응표 1,160행 오류 0, 관련 fixture 추적 항목을 선택 갱신했다. Node v26.13.0 전체 6,301건 통과(실패·취소·건너뜀 0). E811 Node v22 전체 검사 뒤 v26 회귀 1/8회다. 전체 fixture 검사는 오래 걸려 중단하고 관련 항목 갱신으로 대체했다.
