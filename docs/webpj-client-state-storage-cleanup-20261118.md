# WB04-E806 Client State 저장소 정리 앞단

기준선 `225553df`의 `ClientStateStorageCleanup`에서 시간 동기화·Zone과 State 저장소의 소유자 색, 등록 SID 개수, TotalMade, 해시, 경고, spot, 섬, 청크, 그래프, 다리 스캔 정리를 같은 Client 장기 소유자에 연결했다. 표면·권한·GameAux 정체성을 검사하고 소유자 색 정리만 대여 중 허용하며 다른 표면 정리는 World 반납을 요구한다. 시계는 다리 스캔 효과에 도달할 때 조회한다. 기준선 이후 해당 cpppj 파일 변경은 없다.

실제 `ClientLocalStateGlobals`·`ClientGameplayGlobals`·`RawSurfaceStorage`로 각 효과와 Zone 인접 WORD 보존, 대여 관문, 다른 owner 거부를 확인했다. 최초 시험에서 섬 저장소를 객체의 `records`로 잘못 접근했으나 웹 소유자의 실제 바이트 배열로 수정하고 재검사했다. 실제 Client 수명·State 부모 연결, Pool reset 및 메타 Zone 객체 삭제는 남았다.

검증: 집중 검사 통과, 대응표 1,160행 오류 0, Node v26.13.0 전체 6,298건 통과(실패·취소·건너뜀 0). E803 Node v22 전체 검사 뒤 v26 회귀 3/8회다.
