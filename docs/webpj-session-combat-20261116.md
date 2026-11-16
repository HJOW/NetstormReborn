# Session Archer/Missile·아이스 반사 결선 (WB04-E586)

`225553df`의 `RawCampaignSession.cpp` 1484..1536행을 같은 기반 owner에
연결했다. Session.cpp/.h·RawCampaignArcherBindings.cpp/.h·
RawIceRicochet.cpp/.h는 기준선 뒤 변경0이다. cpppj 수정·디컴파일·원본/클론
게임 실행은 하지 않았다.

## 구현·소유 관계

- Session 서비스의 Archer 효과와 지역 `combatEffects`는 중첩된
  animation/common·missile·beam까지 함수 값 사본을 보관한다. 실제 외부
  장치/owner 참조는 공유한다. 입력 묶음을 나중에 바꾸어 이미 선택된 함수를
  바꾸는 JS 참조 차이를 제거했다.
- 사용자 냉각·공통 소리/표적점·발사/충돌 소리·피해 함수가 있으면 보존한다.
  기본 냉각은 현재 SHP 핫스폿과 실제 입자21, 공통 소리는 현재 raw float 좌표와
  같은 SoundList/위치 재생, 표적점은 실제 FenceTarget다.
- 반더 빔 생성은 실제 Particles, 갱신/피해는 같은 GunHit와 Core의 현재
  시각으로 항상 연결한다. Missile의 피해/Impact/갱신은 기존 실제 Hit 몸체와
  MissileState 시각을 사용한다. 두 시계를 섞지 않는다.
- 포구·사정선·핫스폿·방향·치수·경계는 같은 현재 ProjectileGeometry다.
  비행 효과11은 실제 Particles에 붙이고 다른 번호는 원래 사용자 함수
  사본으로 넘긴다. 연결되지 않은 번호는 실패한다.
- 발사/충돌 토큰과 `iceCannonImpact1..4.wav` 같은 동적 파일명은 실제
  SoundList/현재 위치 재생에 연결한다. checking은 같은 Client DWORD의
  현재 unsigned 비트를 읽고, 진단 함수는 원문처럼 결선 시 값으로 캡처한다.
  입력 큐는 보호 명령5429c8과 두 인수의 signed DWORD 비트를 보존한다.
- 실제 IceRicochet는 같은 Pool/현재 TYPE·Missile 표/게임 난수/Finder/HP와
  각도 진단을 빌린다. 지역8BYTE 잔여 입력 함수도 값 사본이다. 후속 생성은
  현재 `combat.missile`과 같은 Host의 실제 CreateMissile을 사용한다.
  별도 근사 탄도·표적 목록·월드는 없다.
- World 프로세스 정리 뒤 생성 구간의 재사용을 차단한다. 초기화 전·반복·
  반납 후 결선과 반사 생성 뒤 Archer 생성자가 실패한 접두를 성공으로 바꾸지
  않는다. 장기 전역도 초기값으로 되돌리지 않는다.

## 검증

신규10검사는 실제 앞 생성 구간과 같은 World/Host/보호 자산으로 값 사본,
현재 음향/입력 큐, 현재 SHP 기하, checking/캡처한 진단 함수, 두 시각,
실제 불꽃/냉각/빔의 부착·종료, 실제 반사 Missile 표시/ProcessForm을 대조했다.
반사 생성기의 변위0 경계에는 원문이 읽는 지역8BYTE만 명시 입력했다.
같은 난수55·실제 Finder/HP를 거쳐 후속 반사탄까지 부착해 Kernel이2개 늘고,
부모 종료 뒤 정리되는 것을 확인했다. 프레임·충돌·적 교전은 실행하지 않았다.

첫 신규9검사는7통과/2실패였다. Client의 signed checking 상자에서
RawCheckingFlag가 읽는 unsigned 비트 변환을 빠뜨린 제품 코드를 보완했다.
다른 실패는 시험의 반사 플래그 위치를 추정한 오류였다(44→100으로 한 번
잘못 고친 뒤 실제 `RawMissile`의 +12로 교정). 기존 단언은 약화하지 않았다.
당시 하위 포함40검사는 모두 통과했다. 반사 생성기 통합을 보강할 때 시험에서
읽기 전용 `GameRandom.state`에 대입한 오류는 실제 `setState(55)`로 고쳤다.
실패 접두 검사를 더한 최종 신규10검사는 모두 통과했다.

원본 IceRicochet 관측과 실제 해시/HP는 기존 하위 검사에서도 계속 대조한다.
새 fixture는 없으며 manifest804항목을 유지했다. `icericochet`·
`icericochetscratch`와 이전 worktree 항목의 계산 메타데이터를 갱신했다.

최종 전체는 Node22(v22.23.3)·Node24(v24.22.3)·Node26(v26.13.0) 각각
**5,581건 통과, 실패·취소·건너뜀0**이다. 세 실행은 같은 전체 glob에
`--test-concurrency=4`를 사용했다. 대응표1,160행·오류0이며 상태는
port842·partial35·todo129·제외154로 그대로다. 웹 JS129,671줄·검사756파일이다.
로그는 `extracted/wb-e586-node22.log`·`wb-e586-node24.log`·
`wb-e586-node26.log`·`wb-e586-port-map.log`다(Git 제외).

## 남은 경계·다음 시작점

Session은 계속 partial이고 4-D29/44다. 생성 구간은 아직 Runtime/앱에
공개하지 않았다. 저장 미션의 전체 발사/회전·첫 비행·clock loop·L2/L3는
4-D D/E 설치 뒤 기존 E567 경계대로 재대조한다. 실제 브라우저 음향/출력,
자연 교전/완주·포획/희생·공중3종 전용 생성/생산/행동 검사는 하지 않았다.

다음은 생성자 **1537행 `InstallVortexDeparture()`**와 거주 구역 Dude/
자산 처리기 결선부터다. 필요한 Vortex 바인딩/규칙·재기준선 장기 저장소
부품을 먼저 옮기고 같은 실제 owner에 연결한다. 그 뒤 이동/건물·전체 모듈·
외곽 공개·같은 Template TypeNames/Objects/Contents·Load가 남았다.
