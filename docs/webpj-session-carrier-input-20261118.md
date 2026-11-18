# WB04-E794 실제 Session 운반 입력

cpppj `RawCampaignCarrierInput.inc`의 `MakeCarrierInput`을 캠페인 Session에 부분 이식했다. 기존 `RawCarrierInput`이 같은 SID 풀·TYPE·Player·Graph·spot·내용물·배치·경로 요청 및 SoundProcess 프레임 장부를 사용한다. TYPE 이동/투하 음향을 실제 자산에서 읽고, Session의 실제 이동·줍기·투하 요청으로 연결한다. 외곽 UI 콜백은 같은 Session 반납 뒤 사용할 수 없도록 수명을 확인한다. MetaMap의 미설치 입력 경로와 전체 World 입력·UI·앱 조립은 아직 남는다.

TheWarBegins의 실제 로컬 로그인과 선택된 사제에서 현재 지형을 이용한 커서 평가, 강제 커서, 일반 목적지 클릭, 선택 해제와 실제 내부 40h 이동 패킷 송신을 확인했다. 반납 뒤 오래된 입력 콜백이 거부되는 것도 검사했다. 포획·희생 관련 경계는 정적으로 연결만 했으며 이 검사에서 실행하지 않았다. 세 비행 유닛의 생성·생산·전용 행동은 검사하지 않았다. 브라우저 실제 미션 플레이는 아직 불가하다.

검증: Node v26.13.0 전체 6,293건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. 마지막 Node v22 전체 검사는 E787이며 이번 단위까지 이후 v26 전체 검사 7/8회다.
