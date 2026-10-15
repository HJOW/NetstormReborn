# 썬 아비어리 실제 프로세스와 미션 Load 연결

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 일치한다. 보호 원본10.78과 기존 정밀 산출물을 읽었다. 새 전체 디컴파일·원본/클론 GUI 실행·보호 원본/AGENTS/dotnetpj 수정·원격 push는 없다.

직전 단위74 `446713e`에서는 buried의580입력 대조를 통과했고, Tutorial5 Load의 다음 **SID15013·타입72 sunAviary·VT00509930**에서 멈췄다. 전체1,316개 중 유일한 실패였다. 이번 단위75는 해당 원본 후처리와 실제 프로세스를 연결한다.

## 원본 몸체

- **0045ec50 / CD0049b420:** server00540bc0와 최초 flags1이면 `004b1a90(parent,type32,0,0)`으로 첫 nondead ProcessForm을 찾는다. 중복 진단이 반환하면 계속 진행한다. new0x30→0045d690→inner+20→0041bd20(type32,parent,0,8) 뒤 같은 Damageable0044bf80을 호출한다. free/void를 추가 필터로 넣지 않는다.
- **0045d690 / CD004998c0:** 전체48BYTE 중 원본이 쓰는 위치만 변경한다. dirty+8=ff, time+10=0, inner+20의 Form/부모0, counter+2c=0이다. outer VT00509dd8과 inner VT00509db8을 기록한다. 서버만 현재 gameSeconds를 time에 쓰고 stage+18=1로 만든다. 비서버의 stage와 padding은 caller 입력을 유지한다.
- **0045d8c0 / CD00499b10:** Kernel의 가상+18은 **inner+20**을 기준으로 호출한다. 차단 extra20과 editor는 먼저 반환한다. 최초 프레임27/시작음, 양 끝에서 방향을 바꾸는 흔들림, 열기11~26, 플라이어 생성, 대기3초와 닫기, 대기 상태를 원본 순서로 처리한다. 프레임 지연의 원본 double은0.20000000298023224이다.
- **0045d6e0→0045b070:** 기본 패킷은 protocol1/message38, 길이9와 Form DWORD/dirty BYTE다. 부모 포함 시 DWORD4개 바이트, bit2는 time8바이트, bit4는 stage DWORD4개 바이트를 덧붙인다. 미사용 dirty 비트는 헤더에 남고, writer 마지막에 dirty를0으로 쓴다. counter는 이 패킷에 포함되지 않는다.

플라이어 생성 시에는 **아비어리 QA에 플라이어 SID**, **플라이어 QA에 현재 아비어리 SID**를 WORD로 기록한다. 첫 부모 포인터의 현재 좌표/소유자와 효과 이후 다시 읽는 inner 부모 DWORD는 원문에서 서로 다르다. Source Position은 x+0.44999998807907104f, y-3.0f이다. Owner/Pop/Frame/음향이 현재 부모·시각·상태·counter를 바꾸는 입력도 각각 구분했다.

## 제품과 실제 Session

`RawSunAviaryProcess`는48BYTE 이미지, 현재 전역 참조, 실제 효과 경계를 사용한다. `SunAviaryProcess`는 같은 C++ Kernel/ProcessForm 수명을 소유하며 원본이 빈 notify를 빈 함수로 유지한다. Host가 기록한 Form/부모는 writer가 OnAttached보다 먼저 호출될 때도 같은 원문 이미지에 반영한다.

Session은 현재 CRT 할당 바이트를 생성자 입력으로 보존하고, 같은 Host에 **타입32·flags8**로 부착한다. RawAssetPostPop은 실제00509930만 이 전체 몸체에 분배한다. 초기 생성과 Load 뒤의 Path 전송 조립 모두에서 같은 아비어리 writer와 실제 공통 송신기를 연결한다. 빈 생성 callback이나 로컬 flags50로 서버 전송을 우회하지 않는다.

개발자 frameCheck의 정상 범위는 실제 타입 코드표로 확인하고 잘못된 자료의 진단은 명시 거부한다. 개발자 GUI/손상 메모리 진단 전체를 복원한 것으로 표시하지 않는다. new 실패의 NULL inner 부착은 원본0041bd20에서도 주소0 쓰기 경로이므로 실제 C++ Session은 bad_alloc으로 거부한다. 정상 부착 성공으로 세지 않는다.

## 독립 기계어 대조

`tools/decomp_sun_aviary.py`의 고유 입력은 **1,734개**다. 생성자18·최초 후처리98·프레임594·writer1,024를 분리한다. 두 x87 정밀도027f/037f로 같은1,734입력을 재생했고 출력이 같았다. 두 재생을3,468개로 중복 합산하지 않는다.

원본 타이머0040eab0, QA004ac220, 부모0041bfb0, 첫 Process 조회004b1a90도 실제 명령이다. 힙·부착·Damageable·프레임 지정·음향·플라이어 생성/Owner/Pop·BaseFlyer 연결·재전송은 명시 외부 입력/효과 경계다. 개발자 frameCheck는 꺼진 입력이다. 실제 전체 게임을 실행한 관찰이나 자연 완주 횟수로 세지 않는다.

전체48BYTE·다섯 객체250BYTE·현재 전역20BYTE·출력64BYTE/길이·호출 인자와 순서를 기록한다. 정상 반환 ESP와 비휘발 레지스터 및 허용 주소 밖 쓰기 guard를 확인했다. C++ 신규5개는 네 범위의 전수 비교와 필수 경계/진단/짧은 버퍼의 부분 쓰기를 확인한다. 실제8미션 검사에는 저장 sunAviary의 동일 Form/Kernel 객체/초기stage1/전송 뒤 dirty0 검사를 추가했다.

## 중단 시점의 실행 결과와 다음 검사

Release 빌드 exec40417은 종료0이다. `extracted/goal-sun-aviary-build.log`와 `extracted/goal-sun-aviary-focused.log`를 보존했다. 중단 전 `SunAviary_` **5개·실패0**에서 고유1,734개 native 입력을 전수 대조했다. `CampaignMissionStart_`는 **3개 중2개 통과·1개 실패**다. 보호 미션 순회는 Tutorial1~5를 통과했고, Tutorial5의 buried QA114/두 Regular 및 같은 실제 아비어리 Form/Kernel/stage/dirty 검사를 실행했다. 다음 **Tutorial6 Load: SID15006·TYPE136·VT00509470(10진5280880)**의 미복원 Pop 후처리에서 실패한다. 8미션 전체 성공이나 자연 플레이 완주가 아니다.

사용자의 추가 요청 **“검사도 다음으로 넘겨”**에 따라 시작했던 전체 회귀를 강제 종료했다. 이번 단위의 전체 회귀 결과는 없으며 `extracted/goal-sun-aviary-ctest.log`는 취소된 로그다. 과거 LastTest.log나 직전 단위74의1,316개 결과를 이번 결과로 사용하지 않는다. 마감 native 재검증·변경 파일 감사·전체 CTest는 실행하지 않고 다음 재개로 넘긴다. [중단 인수인계](cpp-pause-handoff-20261016-sun-aviary.md)에 명령과 확인 순서를 기록했다.

## 남은 실제 플레이 연결

stage6의 실제 sunFlyer73 생성/소유자/Pop는 같은 월드를 사용하지만, 플라이어 자체의 파생 후처리/이동 및0045c1b0의 BaseFlyerProcess 연결은 후속 복원이 필요하다. 선택되면 기존 명시 실패를 유지한다. 실제 GUI의 로그인→Guide→Start, 데모3개 진입과 자연 플레이·승패·다음 미션·그래픽/음향 동일성 목표는 미완료다. 기존 전체화면 DirectDraw 오류 수정은 제외한다.
