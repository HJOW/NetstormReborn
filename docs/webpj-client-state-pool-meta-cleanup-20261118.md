# WB04-E807 State Pool·메타 구역 정리

기준선 `225553df`의 `ClientStateStorageCleanup`에서 Pool reset과 메타 구역 정리를 E806 저장소 경계에 이었다. Pool은 같은 Client/표면 확인과 World 대여 반납 뒤 기존 `resetSquidWorldPool`을 실행한다. 메타 구역은 초기 빈 표 정리에는 Session이 필요 없고 비영 마커 삭제에 도달할 때만 현재 Session의 Runtime을 고정한다. 같은 표면과 SID 범위를 검사하고 실제 World 가상 삭제 후 기록을 정리하며 예외에서도 고정을 반납한다. 기준선 이후 해당 cpppj 파일 변경은 없다.

실제 장기 표면에서 대여 중 Pool reset 거부, 삭제 기록·실패점 정리, Session 없는 빈 메타 표 정리를 확인했다. TheWarBegins Session에서는 실제 객체 마커의 가상 삭제와 Runtime 고정 반납, 범위 밖 SID 실패 시 원래 마커와 고정 반납을 확인했다. 첫 집중 검사에서 삭제 이력 API의 레코드를 숫자 배열로 잘못 비교해 실패했고 SID·TYPE 필드로 바로잡아 통과했다. 전체 Client/State 부모 결선은 남았다.

검증: 집중 검사 2건 통과, 대응표 1,160행 오류 0, Node v26.13.0 전체 6,299건 통과(실패·취소·건너뜀 0). E803 Node v22 전체 검사 뒤 v26 회귀 4/8회다.
