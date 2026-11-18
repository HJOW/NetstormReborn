# WB04-E817 Client State 정리 부모

기준선 `225553df`의 `CleanupClientStateWorld` 앞단을 이식했다. 실제 표면을 확인하고 활성 Session 콜백 중에는 첫 효과 전에 재진입을 거부한다. 같은 장기 전역 접근, Runtime 반납 및 효과 분배를 조립해 기존 `cleanupStateWorld`의 원문 읽기·쓰기·효과 순서를 실행한다. 기준선 이후 해당 cpppj 소스 변경은 없다.

Session 없는 첫 메뉴에서 실제 Gameplay·Local·연결표·Player·Template·SP·Kernel·표면·팝업 소유자를 조립해 전체 부모를 실행했다. 프레임 표, Zone, 팝업 명령, 시드 WORD, Pool/spot과 기한 및 소유자 반납을 확인했다. UI·음향·시계는 관찰 대역이다. 별도 실제 Session의 Runtime 고정 중에는 부모가 첫 효과 전에 거부되는 것도 확인했다. 적재된 실제 Session의 전체 정리와 실제 Client·UI·음향 수명 결선은 남았다.

검증: 집중 검사 2건 통과, 대응표 1,160행 오류 0, Node v26.13.0 전체 6,304건 통과(실패·취소·건너뜀 0). E811 Node v22 전체 검사 뒤 v26 회귀 6/8회다.
