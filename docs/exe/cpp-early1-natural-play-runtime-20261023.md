# Early1 실제 자연 완주 재검증 — 단위378

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 단위377 제품 `f3619bcd`의 실행 파일 SHA256은 `4e9d28dd308411fd40ec149203ea060fcb84636dd6d40455751e2ca34c9b3786`이다. 이 제품은 전체2,442검사와34진입회귀를 통과했다. 이번에는 [재검증 계획](cpp-early1-natural-play-plan-20261023.md)의 과거 입력을 같은 새 제품에서 다시 실행했다.

## 입력과 검사 계약

기존987행 입력 fixture와 과거 gui8/단위203 보고서는 그대로 보존했다. 화면에 표시된 안내 버튼과 합법 다리 배치·회전·사제 선택/목표 입력만 실행했다. 조건873개는 현재 Report를 읽고 입력을 선택하며 SP/HP/단계/객체를 직접 쓰지 않는다. 무작위 다리 조각 지정·강제 진행·직접 Kernel 명령을 사용하지 않았다. `placement-near/reach`의 합법 좌표 탐색 보조는 사용하므로 고정 화면좌표만 사용한 수동 플레이와는 구별한다.

helper를 현재 Guide4, 기본 `useProductTimers=1`, 실제 초기 완료값 보존에 맞췄다. 초기0 프로필은 명시 `--fresh-progress-profile`에서만 설정한다. 상태의 완료값 변화는 `progressObservedTransition`, 별도로 검사하지 않은 디스크 저장은 `persistedProgressVerified=false`로 기록한다. 같은15개 격리 계약 검사와 최종 진행값3사례, 두 prepare-only 계획은 GUI가 없는 도구 검사다.

실행 명령은 `python tools/cpp_tutorial_harvest_smoke.py --output extracted/cpp-early1-20261023-natural1 --timeout 600`이었다. 총3,924프레임의 기존 계획을 실행했고 실제 경과174.71초, 프로세스9060, 종료0/상한 미도달이다. 실행 파일 SHA와 명령/PID는 `running.json`, 종료·판정은 `execution.json`에 각각 보존했다. stderr의 Player UI/미션상태/설치 로그는 관측했으며 미처리 예외나 오류 종료로 판정하지 않았다.

## 실제 관측

- 실제 원본 Tutorial1/BridgeTheGap 설치에서 **A→B→C→D→E→F→G**를 진행했다. 현재 Guide는4이며 실제 Session/월드가 유지됐다.
- 실제 다리 건설과 간헐천 연결 뒤 사제SID15002를 한 번 명령했다. 같은 사제의 서로 다른22위치와40h 이동 요청1회를 관측했다.
- 실제 수확/귀환의 SP는 **0→200→400→600**이었다. 완료 G의 게임시각은99.217초이며 `Mission Accomplished!`와 Leave Tutorials/Next Tutorial 두 버튼을 실제 BMP에서 확인했다.
- 실제 Next Tutorial 입력 뒤 **Tutorial2, 현재 A 안내·Guide4, 새 Session/월드, 시작SP10000**을 보고서에서 확인했다. `next.bmp`에서도 새 월드를 확인했다. 이 BMP는 초기 안내가 합성되기 전 장면이므로 다음 A 안내 본문의 실제 화면 표시 증거로 사용하지 않는다.
- 초기 완료값은 **1→1**로 보존됐다. 따라서 이번 실행은 새0→1 완료 저장을 증명하지 않으며 `progressObservedTransition=false`, `persistedProgressVerified=false`다. 자연 완주 여부와 진행 저장의 증거를 합치지 않는다.
- 전체 원본 **1,394파일**의 실행 전후 manifest가 동일했다. 허용 설정은 영구백업 후 복원했으며 최종 git 비교에도 보호 원본/CD/AGENTS 변경이 없다.

## 원시 자료와 재검사

모든 새 실행 자료는 `extracted/cpp-early1-20261023-natural1` 아래에 있다. `states.tsv`, 실제 `steps.tsv`, `prepared.json`, `running.json`, `execution.json`, `results.json`, `goal.bmp`, `next.bmp`, `originals-before.json`, `originals-after.json`, `settings-backup`을 보존한다. 기존987행 fixture SHA는 `9217cefd46f3e245704c0d10904e060327ca67a3417359fa7f450205630ef325`다.

새 출처 파일은 `cpppj/tutorial-one-harvest-20261023.json`이다. `--verify` 기본 대상만 이 새 파일로 바꿨으며, 실제 GUI 실행 뒤 입력/실행/판정 코드는 수정하지 않았다. 실제 실행 당시 helper SHA는 `d25943b63e39f00403aa809c22735e96977852c1584e0893b6e334793be0a580`이고 새 보고서에는 검증 대상 상수만 바뀐 최종 helper의 현재 SHA도 별도로 기록한다. 과거 단위203의 `cpppj/tutorial-one-harvest.json`은 변경하지 않았다.

`python tools/cpp_tutorial_harvest_smoke.py --inspect extracted/cpp-early1-20261023-natural1`은 현재 원시 관측을 재판정한다. `--verify`는 새 보고서의 현재 소스/원시 SHA를 확인한다. 출처 해시는 소스가 달라지면 그 사실을 보고하며 과거 값을 자동 갱신하지 않는다.

이번 성공은 **Early1 자연 완주와 다음 미션 전환**이다. Early2~6/전체 캠페인/데모의 사용자 완주, 진행 디스크 저장, 모든 UI/그래픽/애니메이션/음향 동일성 및 Edit는 계속 미완료다. [Edit Snapshot 설계](cpp-editor-snapshot-next-20261023.md)와 [Client 설계](cpp-editor-client-next-20261023.md)는 후속 제안이며 이번 게임 실행이나 실제 Editor 성공의 근거가 아니다.
