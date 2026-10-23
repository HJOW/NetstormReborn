# Early Mission1 자연 완주 재검증 계획

**루트 단위378 갱신:** 새 제품의 실제 자연 완주/다음Tutorial2전환을 완료했다. [실행 기록](cpp-early1-natural-play-runtime-20261023.md)과 새 날짜 보고서를 따른다. 아래 ‘미실행’은 최초 조사·helper 작성자 범위의 기록이다. 실제입력/판정은 작성자 동결판 그대로 재생했고, 그 뒤 `--verify`의 기본 출처 경로만 새 보고서로 바꿨다. 과거 단위203 보고서는 보존했다.

단위377 저장 작업과 별도로 수행한 **읽기 전용 후속 조사와 Python 재검사 helper 교정**이다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon /2026-10-23이다. 후속 교정은 `tools/cpp_tutorial_harvest_smoke.py`와 이 문서에만 적용했으며 GUI·게임·공유 빌드·C++·LEFT·git 작업은 수행하지 않았다.

## 이미 있는 재사용 경로

[cpp_tutorial_harvest_smoke.py](../../tools/cpp_tutorial_harvest_smoke.py)와 [987행 입력 계획](../../cpppj/tests/fixtures/tutorial-one-harvest-ui-input.tsv)이 실제 A~G·사제 수확·다음 Tutorial2 진입 경로다. 계획은4~3920프레임, 실제 실행은3924프레임이며 조작 종류는 click/key/keyup/when뿐이다. when873개는 관측 조건 뒤 실제 마우스 입력만 수행한다. `surface-delete`, `kernel-tell`, `--test-bridge-patterns`, 단계/HP/SP/객체 직접 쓰기를 사용하지 않는다. [모달 생산 패널 검사](../../tools/cpp_modal_production_smoke.py)는 같은 자연 완주에 패널 수명/금액 표시 검사를 더한 경로다.

기존 `extracted/goal-early-harvest-20261019-gui8`의 원시 TSV와 결과가 현재 PC에 남아 있다. 실제 당시 진행은 A~G, 사제17개 위치·40h 이동 요청1회, SP0→200→400→600, 메모리 진행값0→1, Next Tutorial 클릭→Tutorial2다. 첫 G 관측은2730프레임/게임117.383초이고3900프레임까지 G를 유지했다. 현재 입력 fixture SHA는 당시와 같은 `9217cefd46f3e245704c0d10904e060327ca67a3417359fa7f450205630ef325`다. **당시 실행은 현재 제품 완주 증명이 아니므로 새 실행은 별도 새 경로/SHA로 남겨야 한다.**

## 원본 조건과 필요한 최소 행동

보호 `tutorial1.english`는 BridgeTheGap Fort, 시작SP0/myTech빈값/AI off, tutorialNumber1, salvage/ascend금지다. 현재 [RawTutorialOne](../../cpppj/src/o/RawTutorialOne.cpp)은 다음 원문 조건을 실제 게이트와 공유한다.

| 단계/관측 | 실제 사용자 행동 | 다음 단계의 조건 |
| --- | --- | --- |
| A./A1. | MORE→OK→정지 해제 후 F4(`key home`/`keyup home`) | 홈 명령 또는 누적 다리칸>0 |
| B. | OK 후 생산창의 실제 다리를 취득·배치 | 누적 다리칸>7 |
| C./C1. | MORE→OK, 필요하면 우클릭 회전하며 추가 배치 | 누적 다리칸>18; 실제 간헐천 생성 |
| D./D1. | MORE→OK, 생성된 간헐천까지 다리 연결 | 현재 신전 Graph와 최근 생성 간헐천 Graph가 같음 |
| E. | OK→실제 사제 선택→연결한 간헐천 목표 클릭1회 | 실제 수확/귀환 후 SP≥200 |
| F. | 기본 OK 버튼을 닫고 같은 사제의 자동 왕복을 기다림 | 실제 수확/귀환 후 SP≥600 |
| G. | Mission Accomplished/현재 기술·돈·진행을 관측하고 Next Tutorial 클릭 | Tutorial2의 새 월드/첫 안내/Guide4·SP10000 |

다리 누적 조건은 조각 개수가 아니라 실제 `BuiltForGenus(4)`의 칸 집계다. 무작위 조각/붕괴 때문에 고정 조각 수를 가정하지 않는다. 기존 계획은 B/C에서 `production:82`→`placement-near:70,50`, D에서 회전0~3회→`placement-reach:53,65`를 사용하고, E에서 `world:priest`→`cell:53,65`로 목표를 준다. 취득/배치는 실제 커서·서버 명령을 거친다. placement-near/reach는 **검사 전용 좌표 탐색 보조자**로 현재 Canon/합법 위치를 조회하고 커서 미리보기를 옮긴다. 결과 객체·길·진행을 생성해 주지는 않지만 고정 화면 좌표만 사용한 수동 재현과는 구별한다.

## 현재 제품 재검사 계약 — helper에 적용 완료

- 기존 `inspect`의 다음 미션 `mission_start == [1,1,0]`는 옛 Start 순서 기대다. 현재 helper는 Early1 완료와 Tutorial2 진입 모두 **[1,4,0]**을 요구한다. 이번 전체34진입에서도 확인한 값이며 옛 검사에 맞추려고 제품을 되돌리지 않는다. 안내의 host phase를 강제하지 않고 현재 G/A. 페이지·실제 컨트롤·정지 상태로 판단한다.
- 기존의 기본 `DoneTutorial1=0`을 제거하고 실제 초기값을 기록한다. 시작값이1이면 G 도달·자연 수확·다음 전환을 검증하되 `progressObservedTransition=false`로 구별한다. 실제 초기0→G의1은 **메모리 진행값 변화**이며, 이 도구는 복원 전 options.cfg를 독립 해독/재읽기하지 않으므로 `persistedProgressVerified=false`를 명시한다. `--fresh-progress-profile`을 명시할 때만0으로 시작하며 `initialProgressInjected=true`/`noStateInjection=false`로 기록하고 실제 초기0 적용 여부도 확인한다. 일반 재검사는 이 옵션을 사용하지 않는다. 재실행 뒤 진행값 유지 등 디스크 영속성 검증은 별도다.
- 기존 `useProductTimers=0`은 현재 `stockEnabled`에 연결되어 flags8 생산 재고 동작/표시를 바꾼다. **다리 생성 시계 자체를 끄는 설정은 아니다.** 현재 helper는 기본값1을 명시하고 `raw_production_maintenance`에서도1을 요구한다. 실제 `test_bridge_patterns` 진단 행이 있으면 자연 재검사를 거부한다. 무작위 조각 지정이나 기타 검사 상태 주입은 추가하지 않는다.
- `when` 하나마다 전체 Report를 만들기 때문에873회 조건 비용을 제품 FPS로 해석하지 않는다. 첫 재검증은 기존 입출력 계획을 재사용하고, 통과 뒤 그 실행의 실제 단계/버튼 시점에 맞춰 줄일 수 있다. 무작위 조각 때문에 과거 G2730에 맞춰 제한을 즉시 줄이지 않는다. 현재 계획의3901 Next Tutorial은 G가 없으면 실패로 보존한다.
- 기존360초 상한은 기본600초/선택 `--timeout`으로 바꿨다. 기존 공용 execute로 PID/제품 SHA·상한·원시 stdout/stderr를 보존하고, 실행 전후 전체 원본 SHA 목록·허용 설정 영구 백업/복구·판정 실패/예외도 기록한다. `--prepare-only`는 새 경로에987행 계획/명령/프로필만 만들고 GUI를 열지 않는다. 기존 과거 JSON/fixture는 변경하지 않는다. `--verify`도 과거 SHA를 새 도구에 맞춰 자동 갱신하지 않는다.

기본 재검사 명령은 `python tools/cpp_tutorial_harvest_smoke.py --output extracted/새경로`다. **최초 helper 작성자 작업에서는 실행하지 않았다.** 이후 root 실제 실행은 위 실행 기록을 따른다. 정적 준비 두 경로는 `extracted/cpp-early1-harvest-helper-20261023-prepare`와 `...-profile-prepare`이며, 기본 명령에 완료값 설정이 없고 명시 초기 프로필에만 Done0이 포함됨을 확인했다.

`extracted/cpp-early1-harvest-helper-20261023-isolated/contract-checks.json`에 첫 교정판의 합성 관측/가짜 프로세스 경계만 사용한 **15개 Python 계약 검사 통과**를 기록했다. 초기1 유지/초기0→1 구별·Dialog1/Briefing3·Guide1 거부·재고0 거부·SP599/중복 사제 명령/월드 없는 다음 안내/강제 조각/누락 단계 거부·명시 초기 프로필 적용·프로세스 시작 오류/상한 실패 보존을 확인했다. 실제 원본 설정 경로는 이 격리 검사에서 mock 처리했다. 이후 진행값 필드명을 `progressObservedTransition`으로 교정하고 디스크 영속성 미검증을 명시한 최종판에서 관련 **3사례를 재검사**했다(`progress-contract-checks-final.json`). 보존 초기1·보존 초기0·명시 초기0 모두 `persistedProgressVerified=false`이며 옛 저장 의미의 필드는 반환하지 않는다. 과거 원시 TSV는 파일을 바꾸지 않고 메모리에서만 합성 계약 입력으로 가공했다. 이는 **현재 제품의 자연 완주 관측이 아니며**, 실제 GUI 재검사 결과를 대체하지 않는다. 최종 도구 SHA는 `d25943b63e39f00403aa809c22735e96977852c1584e0893b6e334793be0a580`다.

## 새 실행의 필수 증거

실제 `mission_gate`의 A~G, `mission_bridges`의>7/>18, 최초SP0과 자연200/400/600, E의 연결 안내, 같은 사제 SID의 여러 위치/귀환 및 단회40h 요청, G의 본문/두 버튼/BMP·현재 완료값, 실제 Next Tutorial 클릭 뒤 Tutorial2/Guide4/새 월드/SP10000을 함께 요구한다. 현재 Report의 `raw_world`는 Graph 총계만 내므로 개별 신전/간헐천 Graph 번호를 직접 대조했다고 주장하지 않는다. 실제 사제 왕복과 자연 수입도 함께 관측하여 G 표시만으로 성공 처리하지 않는다.

헤드리스 `CampaignTutorialOne_ActualSessionFrameAndInputPauseOwnership` 등의 명시 시계/대화창/이벤트 조건 검사는 원문 수명 계약 증거다. 실제 사제·다리·수확·사용자 완주 증거와 별도로 유지한다. Early1 재검증이 통과해도 Early2~6·전체 캠페인/데모·Edit·전체 UI/음향 동일성은 계속 남는다.
