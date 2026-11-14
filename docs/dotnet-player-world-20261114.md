# dotnetpj 0.504 — 실제 Player 화신과 기술부모·정기 프로세스 결선

2026-11-14, HJOW-Athlon. R4의 AI 생성 선행 조건인 Player 화신 공간 등록을 실제 호스트에 연결했다. AI 생성 호출과 게임 프레임 실행은 아직 연결하지 않았다.

## 구현과 근거

기준은 `b43574aea7f7f56a6d761b0fd50846017c84cd5a`의 `client/RawCampaignSession.cpp` Player 갈고리 결선, `o/RawPlayerWorld.cpp`, `o/SquidPostPop.h`의 sharedTotalMade/sharedTotalCost, `client/SessionStateDump.cpp`의 실제 프로세스 관측이다.

- `HostSession.PlayerWorld.cs`에서 같은 공장·소유자·해시·점유 지도·실제 SHP 헤더를 연결한다. 빈 Pop 갈고리를 제거했다. `RawPop → RawPostPop → RawPlayerWorld.PostPop`이 Player+0x9c에 실제 화신 SID를 기록한다. 월드 적재와 AI용 공개 조회는 동일한 Player 몸체·9개 기록·현재 상태를 사용한다.
- 원본 Regular 할당 관문에서 실제 관리 객체를 확보하고, 동일한 객체를 ProcessHost에 전달한다. 이벤트 1·payload 5·Form·부모 체인·커널 등록은 기존 원본 부품이 처리한다. 초기 시각은 세션 공유 `DoubleBox`를 읽는다. 프레임 시계 갱신과 Regular 실행은 R6 잔여다.
- TotalMade 세 배열은 PostPop·Owner·덤프가 같은 저장소를 빌린다. 비용·깊이·생산 알림도 한 상태로 통합했다. 독립 fixture용 기본 생성자는 계속 별도 저장소를 소유한다. 전체 월드 장부 집계 완료를 뜻하지 않는다.
- 세션 덤프는 커널의 실제 Squid/Regular 객체에서 Form·부모·이벤트·횟수·float/double 비트를 읽는다. 호스트 종료는 이 세션이 만든 화신 Regular의 커널 소유권을 반납한다.

실제 SHP는 공간 크기 조회에 사용한다. R5의 화면 무효화·그리기 장치와 R6의 다른 파생 PostPop은 여전히 미연결이다. 외부 갈고리를 직접 주는 독립 월드 적재 하네스는 기존 독립 몸체를 유지한다.

## 검증

전체 비증분 Release 빌드는 경고 0·오류 0이다. 실제 자산 화신·다른 소유자·공유 장부·덤프를 포함한 Client 집중 23건과 Player/PostPop/Owner/TotalMade/ProcessHost/fixture 관련 Core 134건을 통과했다. 공간 좌표 x+1 변이에서는 조회 기대 SID 5가 실제 0으로 달라지는 실패를 검출했고 원복했다. 원복 후 Client23건과 PORT_MAP/manifest6건을 재통과했다. TRX와 실행 산출물은 `extracted/dotnet-player-0504/`에 있다.

같은 시드 1760000000·고정 25ms로 thewarbegins와 savetheisland의 cpp/dn을 각각 120프레임 실행해 f090 덤프를 수집했다. 공격·포획·희생·승리 조작은 하지 않았다. 두 미션 모두 종료 0이고, originals 1,394파일의 실행 전후 해시는 불변이다.

| 관측 | cpp | dn 현재 | dn 0.499 |
| --- | --- | --- | --- |
| 로컬 Player+0x9c | 5 | 5 | 0 |
| 첫 프로세스 PID/Form/부모 | 1/6/5 | 1/6/5 | 없음 |
| 첫 Regular 이벤트/payload 비트 | 1/1084227584 | 1/1084227584 | 없음 |
| thewarbegins 슬롯 수 | 1941 | 1854 | 1853 |
| savetheisland 슬롯 수 | 2926 | 2619 | 2618 |

f090의 cpp Regular는 이미 한 번 실행됐으므로 횟수·기한이 다르다. dn은 초기 예약만 연결했고 실행은 하지 않는다. 두 경로 모두 dn_only SID는 0이며, 난수·객체·AI·장부의 남은 차이로 전체 L2는 불일치다. M1은 계속 0/34다.

PORT_MAP은 3행 추가해 701행(L1 346·port 355), fixture는 241개로 유지한다. 신규 NB 수정·외부 접속·기존 경로 은퇴는 없다. AGENTS·README 개요·원본/CD·cpppj·LEFT_JOBS.md·병행 webpj 작업은 보존했다.

## 다음

실제 AI 메모리·생성자·초기 갱신·기술 Form 부착을 이 Player 몸체에 결선한다. 사제 조회는 실제 PostPop 공유 목록을 사용하고 풀 스캔으로 대신하지 않는다. 초기 AI 조회에 필요한 지역·색 변경도 원본 부품으로 연결한다. 이후 다른 파생 PostPop·농지·장부·요새 이미지·메뉴 미션 시작과 M1 대조를 이어 간다.
