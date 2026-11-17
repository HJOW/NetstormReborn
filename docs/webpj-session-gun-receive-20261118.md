# WB04-E725: 총 Process 데이터 수신

cpppj `RawCampaignSession.cpp` 기준 `225553df`의 `ReceiveGunPacket`(5190~5202)과 Carrier 3ah 등록을 실제 Session에 연결했다. 신규 수신은 같은 Gun 본체를 현재 게임 시각에 준비하고 필드를 초기화·복원한 뒤 명시 Form SID에 실제 Process Host로 부착한다. 기존 수신은 현재 Form의 PID에서 같은 `GunProcess`를 선택해 필드만 갱신한다. 원문 진단·부모 DWORD 범위와 실패 시 수명 정리도 유지한다.

실제 Tutorial 1 Session에서 일반 Form 부모에만 신규 Process를 부착하고 같은 PID의 그룹 데이터를 갱신했다. 새 통합 검사는 발사·유닛 행동을 실행하지 않는다. 공중 3종 전용 생성·행동이나 사제 포획·희생·자연 승리는 실행하지 않았고 실제 게임 또는 브라우저 창도 열지 않았다.

다음은 원본 등록 순서의 비행 수신·명령 정적 이식이다. 압축·변경 수신과 서버 응답 왕복, Native 및 앱 미션 수명은 아직 남았다.

같은 Process의 기존 단위 검사와 실제 Session 검사를 함께 실행해 44건이 통과했다. 신규 검사는 짧은 수신 실패 뒤 빈 Form과 Kernel 크기도 확인한다.

대응표 오류 0, Node v26 전체 회귀 6,187건 통과다. Node v22는 E722에서 순차 전체 검사했고 현재 v26 주기는 3/8회다.
