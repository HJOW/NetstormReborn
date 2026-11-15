# Session GunHit·기하·기존 Gun 장애물 통지 (WB04-E582)

`225553df` RawCampaignSession.cpp1398..1426을 E574~E581의 같은 기반 owner에 연결했다.
GunHit→ProjectileGeometry→FenceTarget→TYPE 전체 최대 검색 범위→GunBlocker 순서다.
전체 Session은 계속 **partial**이다. 소유자색/파편·나머지 생성자·외곽 공개·Load/로그인/
프레임이 잔여이며 전체 교전 또는 미션 플레이 완료를 뜻하지 않는다.

## 연결 범위

`connectGunBodies()`는 실제 Hit Bindings 뒤 한 번 실행한다. GunHit는 같은 HP/피해
분배와 현재 TYPE별 Hit 행·실제 Building.NotifySurface를 사용한다. 미복원 가상 피해는
실제 공격/표적 TYPE를 기록해 원문 오류를 유지한다. ProjectileGeometry는 같은188행
보충 표·고정16/11 배율·현재 GunMode를, FenceTarget은 같은 풀/해시/TYPE/기하·현재
GunTarget 상태와 방향별 동맹 상태를 빌린다.

원문660의 GunBlockerState는 반경0/0·스톤 타워78로 구성한다. 실제 채움은 이 생성 구간의
전체 TYPE 번호 순회에서 수행한다. group1은 range+10, group0은 range+3을 float32로
저장한 뒤 이전 최대값과 비교한다. `std::max(a,b)`의 a<b 순서를 사용하므로 NaN 후보는
이전 최대값을 덮지 않는다. 다른 group은 범위 집계에 넣지 않는다.

기존 Gun 조회는 매번 실제 RawContainedLookup→현재 Form→PID/Host에서 찾는다.
Form/프로세스가 없는 반환과 실제 표적0을 구별한다. 다른 Kernel 타입이면 명시 오류다.
표적 지정은 현재 실제 RawCampaignAssetBindings.Gun의 SetTarget만 호출하며 새 Gun을
만들지 않는다. 실제 모드·포구는 같은 GunMode/FenceTarget을 사용한다. 자산 처리기는
후속 생성 구간이 제공할 현재 슬롯이며 아직 없으면 선택한 호출에서 오류를 낸다.

ApplyFlyerHit2374..2380도 같은 부모에 이식했다. 실제 Rain VT 외에는 false이며 기존
generic target0/free/dead 관문을 계속한다. Rain은 DamageResult를 먼저 호출한 뒤 현재
표적 dead·발사자 live·Server를 순서대로 읽고 현재 Flyer Form→Command1→방송으로
진행한다. CurrentFlyerProcess/BroadcastFlyerCommand의 실제 부모 연결은 후속 작업이다.
공통 인계에 따라 이번 새 검사에서 공중3종의 전용 생성/행동을 실행하지 않았다. 해당 Rain
경로는 정적 이식이며 실행 검증으로 세지 않는다.

부분 구성 실패는 앞서 만든 실제 GunHit/투사체/배리어와 채운 최대값을 유지하고 반복을
거부한다. 반납은 현재 World 프로세스 정리 뒤 단계 무효화·앞선 모듈 정리 순서다.

## 근거와 검사

Session·RawGunHit·RawProjectileGeometry·RawFenceTarget·RawGunBlocker의 소스/헤더는
기준선 이후 HEAD 변경0이며 cpppj 수정은 없다. 기존 Gun/기하/Blocker 원본 fixture와
경계6행·실제 GunProcess 공장/상태 기계 계약은 전체 회귀에서 유지한다. 새 원본 관측/
fixture는 없으며 manifest803항목을 유지한다.

신규11검사는 실제 보호 자산·World/Core·Host/Kernel에서 다음을 확인한다.

- 앞 단계/반복/반납·같은 실제 피해/기하 몸체·미공개 Runtime.
- 보호 TYPE 전체의 최대값, float32 여유 덧셈·음수/다른 group/NaN 비교 순서.
- 실제 일반 HP 피해와 generic target0/free 관문·미복원 TYPE 진단.
- 실제 시각 WORD/bit0·Building 통지와 다음 압축 시각 재조회.
- 실제 SHP의 signed 포구·현재 캐논 모드와 사정선 보정.
- 같은 현재 기하 행/TYPE hotspot·픽셀 배율·모드와 배리어 포구.
- 실제 Gun Form/PID 조회·표적0/현재 표적·현재 processType과 미설치 자산 처리기 오류.
- 같은 Form의 다른 실제 Kernel 타입 거부와 기존 객체 수 보존.
- 실제 자산 Gun SetTarget의 같은 Fields/단계2·감시 종속과 현재 표적 조회.
- 빈 실제 해시/현재 Boss의 조기 반환, 새 Gun 생성 없음.
- Blocker 구성 실패 앞의 실제 세 몸체/최대값·반복 거부·대여 반납.

Gun 조회 입력은 실제 RawGunProcess/GunProcessFields를 만들고 같은 Host에 무전송
flags50으로 부착한다. 후속 자산 PostPop 생성이 완료됐다고 세지 않는다. 표적 지정 검사는
실제 RawCampaignAssetBindings를 따로 구성해 기존 상태 기계를 선택한다. 전송은 외부
장치 호출 기록 경계이며 새 원격 MP 게임/패킷/통신 검사가 아니다. 비어 있지 않은 공간의
전체 타워 배치/통지 부모와 앱 교전은 후속 연결/기존 원본 단위 검증에 남아 있다.

최초 신규11검사는 모두 통과했다. 실제 감시 관계 확인을 더한 집중108검사도 Node22·Node26
모두 통과했다. 신규 모듈 줄92.77%·분기90%·함수87.5%다. Rain 전용 경로와 조회기가
선택하지 않은 삭제 콜백을 억지 입력으로 실행해 범위를 부풀리지 않았다. 로그는
`extracted/wb-e582-focus2-node22.log`·`extracted/wb-e582-focus2-node26.log`(Git 제외)다.

전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각5,539건 통과, 실패·취소·건너뜀0이다.
로그 `extracted/wb-e582-node22.log`·`extracted/wb-e582-node26.log`는 Git 제외다.
대응표 검사1,160행/오류0(`extracted/wb-e582-port-map-pre.log`)이다. port841·partial34·todo131·제외154,
session/oracle0·단계28/43은 유지한다. 원본/클론 게임·브라우저·오라클·디컴파일·공중3종
전용 새 생성/행동·새 원격 멀티플레이 실행은 하지 않았다.

## 다음 시작점

1427의 기본 소유자색 표→BridgeDraw/ShapeToBuffer 파편 캡처→CampaignParticles부터
원문 생성 순서로 이어간다. 이후 전체 모듈/등록·외곽 공개·같은 Template의 TypeNames→
Objects/Contents·Load·로그인·프레임/종료가 잔여다. L2/session-dump·실제 미션 화면/
자연 플레이는 미검증이다.
