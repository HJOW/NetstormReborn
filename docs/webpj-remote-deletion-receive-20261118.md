# WB04-E729: 원격 객체 삭제 수신

cpppj `RawSquidMutationReceive.cpp/.h` 기준 `541624a9`의 53h 단일·62h 다중 원격 삭제 및 62h 송신 관찰기를 이식하고 실제 Session의 Carrier 등록과 같은 World 삭제에 연결했다. 단일은 SID 0·물리 범위 밖·free만 건너뛰고, 다중은 signed count와 각 WORD를 실제 소비 위치에서 읽으며 삭제 뒤 현재 count를 재조회한다. 송신 관찰기는 count -1 진단 복귀 뒤의 하위 WORD로 길이를 기록한다. 삭제 flags는 원본 그대로 0이다.

일반 독립 Pool에서 범위·free·부분 삭제·callback 뒤 바뀐 SID/count·송신 관찰기의 진단 복귀를 대조했다. 실제 Tutorial 1 Session에서는 빈 수신만 사용해 등록을 확인한다. 공중 3종 전용 생성·생산·우클릭·행동과 사제 포획·희생·자연 승리는 실행하지 않았고, 실제 게임이나 브라우저 창도 열지 않았다.

`RawSquidMutationReceive`의 내용물 2dh·일반 삭제 2eh·감시 65h는 아직 미이식이므로 대응표는 partial이다. 이후 검증·압축 및 나머지 Carrier, 서버 응답 왕복과 앱 미션 수명도 남았다.

실제 Session과 독립 수신 검사를 함께 실행해 20건이 통과했다. 예약 SID 1도 원본처럼 별도 차단하지 않는지 추가 확인했다.

대응표 오류 0, Node v26 전체 회귀 6,192건 통과다. Node v22는 E722에서 순차 전체 검사했고 현재 v26 주기는 7/8회다.
