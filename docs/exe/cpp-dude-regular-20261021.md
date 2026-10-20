# Dude 정기·네 하위 처리와30칸 초기화

2026-10-21 단위246. 시작 커밋 `f58fec2e`. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. 기존 정밀 자료와 보호PE를 읽었고 새 디컴파일·원본/CD·AGENTS 변경은 없다. 다른 세션의 dotnetpj/README 변경과 커밋은 보존하고 포함하지 않는다. 직전 목표 턴은 단위245의 발자국0 수정·기록·커밋을 완료한 진행 턴이다.

## 원본과 독립 관측

`RawDudeRegular`는10.78의450fa0 정기 분배와451190 보행,451210 복귀,451270 드롭 후 보행,451340 낙하,4508b0 목표 이동 wrapper를 복원한다. poll 사건005326c0의 초기값은1a2b다. count는 원래 signed 비교를 쓰며 기존24 경로가 있고 count!=0일 때5107을 제외하고 payload로 기다린다. WORD를 쓴 뒤 task별 처리하고5101/잘못된 사건의 진단 후 fallthrough도 보존한다.

451190의 공급자 목록00592f84/88/8c는 기존 에너지 공급자 목록이며0055a4fc의 순환 값과 같은 게임 RNG를 쓴다. count!=0의 확률 관문,1..2 전진,목표 진입점/quick 인자와 실패 시 task5103을 유지한다.451210은5회부터 삭제하고 그 전에는 현재 소유자의 거주 구역을 찾는다.451270은0회에 원래 위치를 보존한 뒤 거주 구역 진입점으로 옮기고 원래 위치로 걸어 나온다.

451340은 dead 우선/100회 초과 삭제/10회 BYTE35=20, I단계의 A첫 프레임 지정, `float((count×0.5+1)×0.0625+현재y)` 저장과254 상한, Unpop 후 현재 단계 재조회·실제 RNG 프레임·Pop40·반환0.05 순서다. 디컴파일의 미정의 `uStack_8`을 쓰지 않고 보호PE 실제 명령의 y 계산을 읽었다. 부모·프레임·공간 효과 뒤의 현재 raw를 다시 조회한다.

population은 CRT 실행 전 원본 데이터에서0이지만 객체 시작55a4ec의static initializer4fc9b0이 **push1e(30)→44fb20**을 호출한다. 실제 생성자는new120·ptr/capacity30/count0·VT508c04·registry472880과atexit4ff190 등록을 수행한다. C++은 Session 소유권으로 같은 active 목록의 수명을 유지하고 첫 객체 Pop 전에30칸/count0을 준비한다. 원본 포인터 주소나 미사용 heap 바이트를 그대로 호스트에 복제한다는 뜻은 아니다.

**제품 작성 전** `tools/decomp_dude_regular.py`로 **1600입력·두 정밀도3200정상 반환·181584명령**을 관측했다. 정기/하위1597입력과 정적 초기화3입력이다. 정기·하위·목표 wrapper·실제 RNG/TYPE/FrameSide/WORD/공통496e70·초기 생성자는 원본 명령이다. CheckSpace/현재 경로 lookup/task·거주 구역/진입점/실제 Move·가상 공간/삭제/프레임·new/registry/atexit는 명시 인자 경계다. 실제 caller float32·GP/retN/canary/SEH/x87·코드/읽기/쓰기 범위를 검사했고 게임/OS/창/C++제품 검사는 실행하지 않았다.

최초 도구/보고서/fixture는 `extracted/dude-regular-20261021-first-capture/`에 보존했다. 수집 뒤 보고서 메타데이터의 한국어 표기 문자열만 수정했다. 함수 몸체와 **fixture 전체 바이트는 최초와 동일**하며 `observationToolSha256`과 표기만 역변환한 도구 SHA를 함께 감사한다. 새 원문은 `cpppj/dude-regular-native.json`·`dude-regular-x86.tsv`다. 기존 기대값을 덮어쓰지 않는다. CD10.72의 정기/하위 대응은 현재 정밀 표에서 확정하지 못했으므로 제품은10.78에 한정한다.

## 실제 Session 연결

첫 객체 전에 `InitializeDudePopulation`을 호출하고 단위243의 `RawDudeBehavior`, 같은 `RawDudeTask`/contained lookup/거주 구역·공급자 active 목록/RNG/프레임/Pop·Destroy와 단위244의24 경로를 연결했다. Root의 Regular 분배는 실제70/VT508cc8을 현재 `dudeRegular->Handle`로 보낸다. `Population()`은 같은 목록을 읽기 전용으로 빌리며 사본을 만들지 않는다.

capacity0에 가로막혔던 기존 피해 후 Dude 생성이 원래30칸 한도에서 동작할 수 있게 된다. 이는 자연 피해/대피/전체 게임 실행을 검증했다는 뜻이 아니다. 상태와 실제 경로를 공유하므로 별도 보행/낙하 모형을 만들지 않는다.

## 빌드와 남은 검증

새 검사2개를 작성·컴파일했다. `DudeRegular_Native1600`은1597행의 float 반환·효과 순서·전체50BYTE·RNG·순환 값과3행의 capacity/count 대응을 비교하도록 작성했다. registry/atexit 토큰과 미사용 heap을 제품 바이트 대조로 세지 않는다. `CampaignDude_ActualRegularFallAndPopulation30`은 실제 미션 로드/명시 Dude에서30칸 등록→poll만 종료→실제 task5105 설정→같은 Regular의 낙하 프레임/공간/102회 뒤 부모와 Form 삭제·population 복귀를 검사하도록 작성했다. 전체 Kernel/자연 생성·대피 검사는 아니다.

최종 전체 Release는 **경고0·오류0·종료0**이다. 첫 빌드도 종료0이며 효과 뒤 현재 타입의 범위 보호를 보완한 최종 로그는 `extracted/cpp-dude-regular-20261021-build2.log`다. **사용자 지시대로 C++검사/CTest/게임/전체 회귀는 실행하지 않았다.** 빌드·원본 관측을 제품 검사 통과/지정 미션 완주로 표시하지 않는다.

현재 소스 핀만 후속 이력으로 갱신하고 전체 JSON 역변환을 시작 커밋과 대조한다. 원본·fixture·과거 files/실행/바이너리·기존 이력은 보존한다. `cpppj/dude-regular-session.json`, `tools/decomp_dude_regular.py --verify`, `tools/cpp_dude_regular_audit.py --verify`를 따른다.

남는 별도 경계는 거주 구역 event2의450a90이다.00594fb8은 **요새 모드**이므로 지정 미션의 전투 모드와 구별해야 하며, 현재 Residence의 추가 예약 조건/상태 연결도 함께 확인해야 한다. 지형 Devastation·포획/Drop·제단/희생·지정11미션·F8 Guide/키·전략AI·자연 대피/완주·전체 동일성의 통합 검증은 계속 남는다. 전체 goal은 **active/미완료**다. 인수인계를 갱신하고 날짜/CP 규칙으로 로컬 커밋하며 push하지 않는다.
