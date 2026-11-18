# WB04-E816 Client State 효과 분배

기준선 `225553df`의 `CleanupClientStateWorld`가 사용하는 효과·자식 바인딩을 별도 함수로 이식했다. 같은 연결표·커서·시계·메타 구역·SP·Template·채팅·선택·Player/AI/Kernel·표면 저장소·Core를 원문 효과 위치에 연결한다. 원본의 단일 RET인 MapMode만 무효과로 두고 실제 UI·음향 장치는 도달할 때 요구한다. Pool 초기화 앞에는 실제 Runtime 반납을 호출한다. 기준선 이후 해당 cpppj 소스 변경은 없다.

Session 없는 실제 Client 장기 소유자에서 연결표·동기화·메타 표·선택·SP·Player·Kernel·Pool·다리 스캔·Zone 효과를 집중 검사했다. 음향·UI 및 시계는 관찰 대역이며 누락된 Gump 장치는 거부한다. 원본 `cleanupStateWorld` 부모와 전체 실제 Client·Session 수명 조립은 다음 단위다.

검증: 집중 검사 통과, 대응표 1,160행 오류 0, Node v26.13.0 전체 6,303건 통과(실패·취소·건너뜀 0). E811 Node v22 전체 검사 뒤 v26 회귀 5/8회다.
