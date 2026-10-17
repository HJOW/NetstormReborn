# 골렘의 귀환·간헐천 후보 선택 — 2026-10-18

기준 커밋은 `52e714f`다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 일치한다. AGENTS와 양 LEFT를 재독했다. 보호 원본 수정, 새 디컴파일, 원본 엔트리·원본 게임 초기화·원본 게임 창 실행, DirectDraw 수정은 없다. 새 선택기는10.78 전용이며10.72 본문은 실행하지 않았다. 골렘의 내부 이름은 `sunWalker`, TYPE133이다.

`RawCarrierChoice`는428ea0의 귀환 후보 수집,429420의 간헐천 후보 수집,4280a0의 공통 평가를 복원한다. 현재 Player9의+7c 귀환점을 먼저 검사하고, 별도55a70c/710/714 신전·아웃포스트 목록을 순회한다. 간헐천 선택은 귀환점 선택을 먼저 완료하고, 귀환점이 물리 범위 안이면 반환값을 쓰지 않는462bc0 부모 Graph 조회도 실행한다. 이후5950dc/e0/e4 목록을 그 귀환점에 대해 평가한다. 할당한 객체 전체를 검색해서 후보 목록을 대신 만들지 않는다.

두 수집기의 순회 조건은 다르다. 귀환 목록은 nonzero 방문 수와 현재 count를 비교하고 capacity에 도달하면 멈춘다. 간헐천 목록은 index와 현재 count를 비교하며 capacity 조기 종료가 없다. 검증 뒤 변경된 현재 count를 읽는다. 귀환점의 중복, 원래 목록 순서,500개 상한을 유지한다. 현재 검증은 기존 `RawCarrierArrival.ValidReturn/ValidGeyser`의 소유권·상태·타입·Graph 몸체를 재사용한다.

평가기는 다음 순서를 보존한다.

1. 유효 mover의 현재 TYPE에서 speed/turningSpeed를 한 번 보관하고, 현재 Player+b0 또는 비행 부류20000이면 경로 평가 생략 여부를 고정한다. owner0은 Player AI 포인터를 읽지 않는다. 무효 mover의 기본값은1.0/90.0이다.
2. 각 nonzero 후보의 현재 surface 플래그를 확인한다. surface면ace 진단 후 재조회하고, 여전히 surface면 첫 순회의 방문 수를 늘리지 않는다. 후보 좌표를 먼저 보관하고 anchor 발자국→일반 접근점, 후보 발자국→일반 접근점을 계산한다. **넓은 거리/speed**의 `_ftol` 하위 WORD를 후보 raw+8에 기록한다. free/dead 필터나 float 선행 축소를 추가하지 않는다.
3. 직전 단위의 `RawCarrierCandidateSort`에 원래 배열을 전달한다. 원본의 요소 주소 비교와 CRT 동률 순서를 그대로 사용한다.
4. WORD 점수가 현재 최소값보다 작고 경로 평가가 켜져 있으면 임시 Path의 count와5000개 시각을0으로 초기화한다. anchor와 후보 좌표는 **둘 다 첫 발자국 효과 전에 보관**한다. 후보 발자국에서 목적 접근점, anchor 발자국에서 출발 접근점을 계산한다. 따라서 간헐천은 부모의 현재 위치가 아닌 선택된 귀환점에서 평가한다.
5. 현재 mover 타입을 네 현재 cardinal 타입과 비교한 뒤 같은 BFS를 실행한다. 속도는 처음 보관한 값이지만, 이 타입 판정은 발자국 효과 뒤의 현재 값이다. 꼭짓점이 없으면 원래 거친 WORD 점수를 유지한다. 도달 불가능한 후보를 임의로 제외하지 않는다.
6. 경로가 있으면 기존 `RawPathRuntime.Prepare`로 현재 시각의 도착 시각을 계산한다. 선택 점수는 마지막 도착 시각에서 현재 시각을 뺀 값이다. speed를 곱한 값은540bfc의 임시 표시 값일 뿐 선택 점수가 아니다. 엄격한 `<`와 최소값의 float 저장을 보존한다. 최종에는540bf8의 DWORD를540bfc로 그대로 복사한다. signaling NaN 비트도 float 변환으로 바꾸지 않는다.

같은 Session이 실제 Player, 원래 자산 목록, `RawSquidNeighbors.Footprint`, 기존 유효성/Graph, 기존 `RawCampaignPathBindings`의 같은 BFS 작업 상태와 시계를 연결한다. 후보용 BFS는 새 Process/Form을 부착하거나 현재 이동을 삭제하지 않는다. 실제 GoHome/GoGeyser 명령에서만 기존 PathStart/Host40/직렬화로 새 이동을 시작한다. 외부 서비스가 공급하던 미복원 chooseReturn/chooseGeyser 경계를 실제 선택기로 교체했다.

상위 `RawCarrierArrival.Controlled`에도 현재 owner의 실제 Player AI 조회를 추가했다. 실제 Session은 같은 `RawPlayerParticipation.Brain`을 읽는다. 독립 원문 재생의 기존 bool 표는 현재 조회가 없는 경우의 명시 입력으로 유지한다. 원본 도착 자료406개를 현재 조회 경계로도 재생하여, 반대 값의 예전 bool 표가 분기를 결정하지 않음을 확인하는 검사를 추가했다.

독립 관측은 `tools/decomp_carrier_choice.py`다. 세 상위 본문, 실제 일반 기하·거리·CRT 정렬·TYPE/AI getter·cardinal getter·Prepare·방향 차이·`_ftol`·stack probe를 실행한다. Footprint/BFS 반환 배열/두 후보 검증/Graph/복귀하는 진단/2000바이트 확보·해제는 명시 경계다. 호스트의 임시 배열 꼬리 초기화와 OOM 동작을 검증했다고 주장하지 않는다. 정상 후보 prefix와 실제 결과를 대조하며, caller 내부 미사용 꼬리 바이트를 기대값으로 삼지 않는다.

첫 관측707입력·1414정상반환은 정상 완료했고 정밀도 차이는0이다. 추가 관측에서는 귀환점과 부모가 다른 간헐천 경로 및 주소 감김이 방금 기록한 후보 거리 WORD를 읽는 조합을 보강했다. 관측기의 인자 읽기는 실제 경계 진입에 한정하고, 기존 명령·읽기 허용 범위는 같은 주소 표로 가속했다. 원래707행 전체의 반환/효과/배열이 바이트까지 그대로임을 확인했다. 최종 관측은 **721입력·1,442정상 반환·30,384,330명령·정밀도 차이0**이고2000바이트 확보758회/해제758회다. GP/retN/SEH/canary/x87 제어어·TOP·tag/명령·읽기·쓰기 범위를 검사했다. 비교 대상은 선택/효과 순서/직접 평가 배열/현재 전역/전체 raw Adler이며, BFS 경계 호출 전5000개 시각의0 초기화도 확인했다.

첫 집중 검사에서 원문 재생의 raw Adler와 일부 선택 분기가 달랐다. 원본41cfc0의 입력과x87 반환을 추적한 결과, 거리 함수 뒤 **428210 `fdiv dword ptr [esp+18]`**가 speed로 나누고 있었다. 정밀 디컴파일 C에는 이 연산이 누락되어 있었다. 거친 점수도 이동 시간 단위로 계산하도록 구현을 수정했으며 원본 관측 자료는 변경하지 않았다. 첫 빌드의 검사 입력 채움에서 나온 C4244도 BYTE 값을 명시해 수정했다. 초기 Session 검사의 불필요한 새 간헐천 배치 탐색은 제거하고 원래 초기 목록을 사용했다. 추가 진단 중 빠진 표준 출력 헤더로 인한 빌드 오류도 로그에 보존했다.

집중 검사4개는 모두 통과했다. 원문721개 재생, 빈 목록/bit 복사 계약, 현재 AI 조회를 연결한 기존406개 도착 원문 재생, 실제 Session을 포함한다. Session은 초기1-1의 실제 home과 간헐천 목록을 사용한다. 후보 평가 중 기존40/Form/패킷 보존, GoHome의 실제 경로 시작을 확인했다. 같은 Graph여도 초기 위치에서 선택한 간헐천까지 BFS가 실패할 수 있으며, 이 단일 접근 실패는 원본처럼 기존40을 지우고 새 패킷을 만들지 않는다. 성공 조건에서는 원래 간헐천 섬 모서리에 운반체의 초기 위치만 준비한다. 실제 BFS가2개 이상 꼭짓점의40 경로를 생성했다. 섬 중앙의 부적절한 초기 위치를 성공 조건으로 강제하지 않았다. 실제 AI2를 생성·부착한 owner로 바꾸면 GoReturn이 현재40을 보존하고, 실제 AI 해제도 확인했다. 성공 경로/선택/시각은 외부에서 공급하지 않았다. 전체 Kernel이나 자연 채취 주기를 진행한 검사로 확대하지 않는다.

재현 명령:

```powershell
python tools/decomp_carrier_choice.py
python tools/decomp_carrier_choice.py --verify
cpppj/build/bin/Release/netstorm_tests.exe --filter CarrierChoice
ctest --test-dir cpppj/build -C Release --output-on-failure
```

최종 Release build8은 경고0·오류0이며 집중4개가 모두 통과했다. 전체 CTest는 **1,612개·실패0·종료0·204.28초**(CTest204.31초)다. 자료 감사는 **73종·62성공·기존11실패·새실패0**이다. 기존64보고서268현재핀을 갱신했고, 역치환하면 과거 JSON 전체가 일치한다. CMake 기존 접두·기존 fixture·원본 실행 파일2개/tarc·AGENTS·DN 인계를 보존했다. 새 파일은 BOM 없는 UTF-8/LF다. 클론 진입5회가 모두 성공했고 originals_unchanged=true다. 범위는1-1/1-2briefing→world와tutorial1~3briefing이며 자연 운반이나 완주를 증명하지 않는다. 제품 보고서는 `cpppj/campaign-carrier-choice.json`, 독립 보고서는 `cpppj/carrier-choice.json`이다. 모든 관측/빌드/검사/감사/클론 핸들은 종료했고 대기 작업은 없다. 인수인계 뒤 `20261018-03 CP 골렘 귀환과 간헐천 후보 평가 및 현재 AI 연결 복원`으로 로컬 커밋하며 push하지 않는다.

**다음 최우선은40의 대상 재확인이다.** 현재 `effects.path.retarget`은 미연결이다. 실제489ff0 Halt는 targetA/B가 있으면 이 분기를 방문하며, 대상이 살아 있으면 추가 효과 없이 지나가야 한다. 새 선택기를 붙인 것으로 이 프레임 하위를 완료한 것으로 보지 않는다. `RawPathProcessHooks.retarget`의 주소 주석489ff0은 별도 하위 함수 주소가 아니라 전체 Halt 주소다. 원본 본문 안에서 죽은 targetB+너겟이면 귀환을 다시 선택하고, 죽은 targetA+너겟 없음이면 간헐천을 다시 선택한다. 이번 선택기/이동/현재 내용물 조회를 재사용하되 새 이동이 기존 Runtime을 삭제할 수 있으므로 이후 접근을 피해야 한다. 실제 자연 운반을 위해 pickup/간헐천 mined packet/429010 회피와443800 시작·도착 겹침 정리도 이어서 연결해야 한다.

후속 범위는38/41/42 경로,429010 회피,5040c0의42ae80, 실제 Client 명령 소비자, 채취/경제·AI, Early Missions1~6·캠페인1-1/1-2·데모3개 스테이지의 자연 운반·승패·저장·다음 미션·완주다. 미등록187의 그래픽 경로를 지원한 것으로 바꾸지 않는다. 전체 goal은 active다. 로그 접두는 `extracted/goal-carrier-choice-20261018-`이다.
