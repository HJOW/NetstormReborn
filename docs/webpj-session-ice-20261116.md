# Session의 성장형·완성형 아이스 타워 부모 (WB04-E605)

`225553df` 생성자1654/ConnectIceTower2145..2173의 독립 typed
분기를 같은 owner에 연결했다. Session.cpp·RawIceTower.cpp/.h의
기준선 뒤 변경은0이다. Native 장기 몸체/역할은 실제 Client 결선에서
남기고 기존 RawIceTower의 원본 대조 몸체를 그대로 재사용했다.

## 같은 현재 장치

- 같은 Pool/188프레임·지역 역할147/150/157·현재 Server/boss/로딩
  깊이를 사용한다. Source의 Server540bc0을 같은 권한 owner에서
  읽으며 전투 모드 값으로 대신하지 않는다. 구성은 자산/예약/난수를
  소비하지 않는다.
- 실제 미부착 Regular를 먼저 확보하고 같은 객체를 재초기화/Host에
  한 번 부착한다. 부착 효과 뒤 현재 시각으로 ResetTask하여 event/
  count0을 맞춘다. 예약 누락/부착 실패는 부분 효과 뒤 명시 오류다.
  Source unique_ptr 대입처럼 미부착 예약은 교체할 수 있고 반납 때
  비운다. JS VM의 복구 불가능한 OOM은 모사하지 않는다.
- grid는 float32 인자·signed64 절삭·하위 signedDWORD를 보존한다.
  비유한/64비트 범위 밖은0이며 캐스트 뒤 보드 범위를 검사하고 같은
  실제 Hash0을 조회한다. 거의 올림이나 별도 지형을 추가하지 않는다.
- WORD는 같은 할당 슬롯의 두BYTE만 쓴 뒤 현재 Bridge TYPE이면
  표면을 통지한다. 소유자 BYTE와 가상 Create/Unpop/Pop/Destroy·
  Frame은 현재 실제 부모를 사용한다. 받침 정리 직전 현재 게임 시각을,
  Damageable 후처리 직전 현재 전투/population을 공급한다.

## 검증·경계

신규9건은 첫 실행부터 통과했다. 실제 예약 객체/Host 부착·효과 후
시각·실패/반납, 합성 Hash 입력의 NaN/무한대/부호0·32/64비트 경계·
float32 반올림, 같은 WORD/인자와 실제 RawIceTower의 Server/loading/
boss 분기를 대조했다. Hash 검사는 실제 지형 생성/미션 시작이 아니다.
전용 원본 관측을 새로 수집하지 않았고 직접 manifest 항목0개/총806
항목을 유지한다. Source 계약을 원본 fixture로 등재하지 않았다.

cpppj `CampaignIceTower_ActualKernelRegrowthAndRepeat`와
`CampaignIceTower_ActualDeletionGates`는 session.Load() 뒤 실제
배치/Kernel/재성장 반복을 확인하므로 후속 전체 World/Load 조립 뒤
다시 대조한다. 이번 검사를 이 두 전체 Session 검사의 통과로 세지 않는다.

신규9건과 기존 원본1426행을 소비하는 관련11건 통과. Node
v22.23.3·v26.13.0 전체는 각각5821건 통과, 실패/취소/건너뜀0·
종료0이다. Node24는 사용자 지시로 생략하고 호환을 가정한다.
대응표1160행/오류0, port846/partial39/todo122·session0,
잔여161단위/34340줄, JS131552줄/검사775파일과 변경6파일의
UTF-8을 확인했다. 마지막 편집은 훅의 한국어 설명 주석이며 별도
node --check도 통과했다. 다음은 생성자1655
ConnectCarrierSteal,1656 BombPostPop·AcidZap/AcidFence·운반/이동/
건물과 나머지 모듈이다. World.BindRuntime1825·외곽 공개·Load/Login·
프레임·L2/L3는 남고 실제 미션 플레이는 아직 불가다. 보호 파일/cpppj
수정·디컴파일·게임/브라우저 실행·실제 포획/희생/특정 적 공격·승리·
공중3종 전용 관측은 하지 않았다.

로그: `extracted/wb-e605-initial.log`.

최종 로그: `extracted/wb-e605-focused.log`, `wb-e605-node22.log`,
`wb-e605-node26.log`, `wb-e605-port-map.log`.
