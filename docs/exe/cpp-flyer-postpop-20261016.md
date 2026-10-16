# 맨 오 워 최초 후처리와 공통 비행 프로세스 생성

분석/마지막 전체 정련 PC는 **VM-W11-Codex / 2026-10-16**, 대상은 보호된10.78 `originals/Netstorm.exe`다. 이번에는 기존 정련 함수와 실제 PE 명령을 재사용했으며 디컴파일 산출물을 바꾸거나 게임/GUI를 실행하지 않았다. 10.72의 비행 프로세스 복원과 동등성은 이번 범위가 아니다.

사용자 표시 명칭은 **Man o’War/맨 오 워**, 내부 타입은 `rainFlyer`144다. 생산 건물 `rainAviary`143은 Man o’War Pool/맨 오 워 풀이며 공통 비행 프로세스는 `baseFlyerProcessType`31이다. [명칭 대조표](../unit-source-name-map.md)를 함께 참고한다.

| 원본 몸체 | 이번에 복원한 범위 |
|---|---|
| 0045fac0 / VT0050a388+20 | flags1·현재server일 때 new70/공통 생성·부착/맨 오 워 확장, 이후 공통 비행 PostPop |
| 0045c340 | outer0/inner+30 기본 필드와 실제 타입31/부모/flags8 부착 |
| 0045c310→0045aed0 | protocol1/message39, 변경 비트별 WORD/DWORD/double 전송 |
| 0045c1b0 | 비행 genus 검사, 첫 nondead 타입31 Form, dirty2와 홈 WORD 갱신 |
| 0045e6b0 | 그림자 생성/현재 프레임 갱신/현재 시각 기록/Unpop/offset/Pop/Damageable |
| 0045faa0 | 맨 오 워 그림자 offset 두float(0.5,0.5) |

## 생성·부착·전용 필드 순서

기본 생성자는112BYTE 중 선택된 필드만 쓴다. outer+20/+28/+40/+48의 double은0, dirty+8은ff, 홈+a는 전달된WORD, 표적+c는0, speed+10과 상수+14는 전달된DWORD다. 현재 게임 시각을+20에 기록하고+18을0으로 쓴 뒤 **기본 outer00509cc8/inner00509ca8 상태에서 실제 Host에 부착**한다. 부착은 기본 생성자 안에 있다.

그 뒤 맨 오 워 wrapper가 outer00509f60/inner00509f40으로 바꾸고+60/+68 double을0, +54 DWORD를0으로 쓴다. 부착/최초 상태 전송을 이 추가 쓰기 뒤로 미루지 않는다. 부착 경계가+54를 변경하는 입력에서도 wrapper가 다시0으로 쓰는 원문을 대조했다. 생성자가 쓰지 않는 padding과 비트는 caller 이미지에서 보존한다. new70 실패는 프로세스를 생성하지 않고 공통 비행 PostPop으로 진행한다. 할당 이후 현재 홈/TYPE speed를 읽으며, 할당 경계가server/processType/부모TYPE/QA를 바꾸는 입력도 별도 대조했다.

상태 패킷은9BYTE 헤더, 선택적 부모DWORD, dirty2 홈WORD, dirty4 표적WORD, dirty8 speedDWORD, dirty10 상수DWORD, dirty20 상태DWORD, dirty40/80 double 순서다. force는ff로 쓰고 마지막에는dirty0이다. 맨 오 워의 추가+60/+68 필드는 이 기본 패킷에 들어가지 않는다. 동일 raw 필드와 현재 Host의 Form/부모를 사용한다.

## 그림자와 압축 시각

공통 비행 PostPop은 TYPE을 한 번 조회한다. 최초 Pop이고 signed현재프레임이 TYPE+114의 전체 프레임 수보다 작으며 flyershadow(400000)가 켜져 있으면 **같은 TYPE/flags2** 객체를 만든다. 현재 부모 프레임+전체 프레임 수를 새 객체에 쓰고 extra8을 켠다. 부모QB(+8WORD)에 새SID를 기록한 뒤 부모의 현재 좌표로 flags0 Pop한다. 별도 그림자 TYPE을 만들어 대체하지 않는다.

최초 Pop의004d4e10은 **현재 시각을1/8초로 압축해 QA의bit0을 보존하고 표면 통지하는 몸체**다. 디컴파일에는 인자 없는 호출처럼 보이지만 실제004e49c0은 x87 `__ftol`이다. 난수 소비로 해석하지 않으며 기존 [총 시각/피해 복원](cpp-gun-hit-20261013.md)의 `RawGunHit::SetTime`을 재사용한다.

그 뒤 현재QB가 있으면 해당 그림자의 프레임을 다시 현재 부모 프레임+TYPE 프레임 수로 갱신하고 Unpop한다. Unpop 후 부모의 현재x/y를 snapshot하고 현재 가상+c8의 offset을 더해 flags0 Pop한다. 마지막은 같은 실제 Damageable/공통 장부다. 그림자 Pop, 압축 시각 처리와 Unpop가 부모의 프레임/QB/좌표를 바꾸는 독립 입력으로 재조회 순서를 대조했다. 프레임 덧셈의 low DWORD 감김을 보존한다.

## 독립 검증과 실제 연결 범위

`tools/decomp_flyer_postpop.py`는 생성38·전체후처리329·홈연결30·writer1024의 **1421고유 입력**을 원본 명령으로 관찰한다. 각 입력을 x87 027f/037f 두 정밀도로 재생해 **2842정상 반환**을 확인한다. 모든112BYTE 필드/250BYTE 객체/현재16BYTE전역/부착직전112BYTE/64BYTE패킷/모든 효과 순서와 스택·SEH·비휘발·쓰기guard를 확인한다. 두 정밀도 재생을 고유 입력으로 중복 합산하지 않는다.

최초 C++ 비교에서는 writer 입력의 두WORD 뒤 DWORD를 관찰기가+e부터 연속 배치하여, 실제+10의 speed보다2BYTE 앞서는 입력 오류가 드러났다. +e/+f는 padding이다. 생성38/후처리329/홈30 비교는 이미 통과한 상태였으며, 관찰기 DWORD 시작을+10으로 정정하고 원본 명령을 다시 재생했다. C/P/B의 모든 행이 이전과 그대로임을 확인했다. 최초 자료 `extracted/goal-flyer-postpop-20261016-first-writer-input.{json,tsv}`와 실패 로그 `...-focused.log`는 보존하며 새1421개에 합산하지 않는다. C++ 출력으로 기대값을 만들거나 writer 제품 몸체를 바꾸지 않았다.

독립 비교5개와 아비어리 회귀10개는 실패0이다. 실제 데모 자원을 Load한 같은 월드의 맨 오 워 풀 위치에서 새TYPE144를 직접 생성/소유자설정/QA홈/Pop한 별도 통합1개도 통과했다. 실제 Host의 타입31 Form·부모·홈·최초 writer의dirty0·전용 가상 표·그림자의 같은TYPE/프레임/좌표/extra8·그림자에 프로세스가 없음·공통 깊이0을 확인한다. 이 검사는 첫 생성의 연결을 검증하며 비행 Frame을 호출하지 않는다.

최초 통합 실행은 접근 위반(-1073741819)으로 종료됐다. 빌드 중 인터페이스 수정으로 오브젝트가 혼재했을 가능성을 제거하기 위해 `RawFlyerProcess.cpp`/`SquidPostPop.cpp`/`RawCampaignSession.cpp`/`FlyerPostPopTests.cpp`/`CampaignMissionStartTests.cpp` 다섯 소스를 고정된 최종 헤더로 일괄 재컴파일했다. 소스 본문 변경 없이 실제 통합1개가 통과했다. 최종 검증은 이 `...-coherent-build.log`의 실행 파일을 사용하며 최초/부분 빌드와 빈 최초 통합 로그도 보존한다.

관찰의 힙/Host 부착·PID 조회/생성/Pop/Unpop/압축 시각/Damageable는 명시 외부 경계다. 실제 Session에는 동일 Pool·Host·Kernel, 실제 factory/공간 Pop/Unpop/SetTime/표면 통지/Damageable와 공통 장부를 연결한다. `SquidPostPop`은 실제0050a388 전체 몸체와 같은 풀을 등록한 경우만 지원하며 공통 깊이를 한 번 줄인다. 원본의 비행 Frame0045d630→0045c9d0, notify0045b760의 표적 해제, 부모 사망0045b7a0, 파생 PreDestroy0045e7e0은 이번에 복원한 것으로 간주하지 않는다. 선택 시 명시 오류를 유지한다.

다음 프레임의 정적 분석에서는0045d630이 상태+54가0이면 outer가상+4의0045bf00을 호출하는 것을 확인했다. 이 초기화는+68=현재시각+60초, +48=현재시각+20초, +50/+58=0, +54=1이다. 두 double 상수는 실제 PE00506580/005012d8에서 읽었다. 이후 boss일 때 부모extra20 또는 수명 만료를 검사하고0045c9d0으로 진행한다. 이는 다음 복원 대상을 식별한 정적 근거이며 해당 Frame의 C++ 복원/실행 대조가 완료됐다는 뜻은 아니다.

최종 Release 고정 빌드는 경고0·오류0, 전체 CTest는 **1372개 중1370통과·2실패**, 검사155.92초/전체156.00초·CTest종료8이다. 새6개는 모두 통과했다. 실제11미션 중9개가400 raw Kernel 프레임을 통과하는 상태는 유지된다. **DemoStormRages는103프레임/2.575초에서 생성 후처리를 통과해 미복원0045d630/0045c9d0 Frame에 도달했다.** 같은 프레임에서 멈추므로 생존 프레임 수가 늘었다고 주장하지 않는다. DissolvedAlliance는3프레임/0.075초의 다른 총 animation 실패가 유지된다. 예외를 성공으로 취급하거나 미션을 건너뛰지 않았다. 자연 완주/전체GUI/AI 전략/사용자 명령/승패·저장·다음 미션/그래픽·음향 동일성은 미달성이다. DirectDraw 오류 수정은 cpppj2차 목표 필수이며 현재1차 미션 복원 범위에서만 제외한다.

새1421개와 기존 요구에너지2352개의 SHA/고유 입력 감사, Python구문, UTF-8/새 주석/문서 링크/공백 검사를 통과했다. CMake 이전/현재 SHA와 이번 전체 검사 결과는 요구에너지 보고서의 단위85 이력에 보존하고 기존2352개 제품/입력을 바꾸지 않는다. 과거 fence-create의 Session/CMake2/실제미션검사4파일, fence-regular/buried-regular의 Session/CMake2파일 SHA 차이는 새 보고서에 구별해 보존하며 과거 보고서를 현재 해시로 덮어쓰지 않는다.

독립 자료/SHA는 `cpppj/flyer-postpop.json`, 로그는 `extracted/goal-flyer-postpop-20261016-*`다.

10.72 참고: 정련 자료의 자동 대응 후보는 홈 연결0045c1b0→0049a6f0(callseq0.465), 맨 오 워 Frame0045d630→004997d0(neighbor0.912), 공통 Frame0045c9d0→004987a0(string0.67)이다. 이는 자동 매칭 메타데이터이며 이번에 CD 명령을 재생하거나 동등성을 확인한 결과가 아니다. 기본 생성0045c340·최초 wrapper0045fac0·공통 PostPop0045e6b0·writer0045aed0은 이 정련 내보내기에서 CD 대응 없음으로 표시된다. 이후 CD 복원 시 실제 가상 표/호출 규약/필드를 별도로 확인해야 한다.
