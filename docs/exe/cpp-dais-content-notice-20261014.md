# 다이스 포획 내용물 알림

- 현재/마지막 디컴파일 PC: **HJOW-Athlon, 2026-10-14**.
- 현재 읽기 전용 내보내기: `daiscontentnotice-functions.json`, 패치1/CD1, 종료0.
- 실제 Dais VT `00507950`/CD `00504090`의 `+24`: **00449660/CD004816c0**.
- 기존 단위12 생산/검사 파일은 동결하고 새 `RawDaisContentNotice.h/.cpp`와 전용 검사/도구를 추가했다.

## 원본 순서와 판본 차이

`reason & 1`이고 현재 내용물 `+18 WORD`가 현재 전역 사제 타입과 같을 때만 실행한다. 현재 다이스 기준점의 첫 typedAltar를 기존 `RawRunePostPop::AltarAt`에서 찾는다. 존재하면 알터의 가상 Unpop0 → 현재 알터 타입의 O 첫프레임을 plain 쓰기 → 포획 form의 원래 owner `&127`을 알터 `+12 WORD`에 쓰기 → 가상 Repop0의 순서다. 알터를 찾지 못해도 현재 다이스 owner가 로컬이면 현재 전역 cue를 정지한다.

원본 프레임 쓰기는 패치 `+36 DWORD`, CD `+34 BYTE`다. Frame.Set을 호출해 표시/공간 효과를 추가하지 않는다. 이전 owner는 패치 BYTE/CD WORD를 읽지만 `&127` 뒤 결과는 같다. 원래 WORD의 상위 바이트가 있는 입력도 직접 관찰한다. 공통 WORD helper는 같은 현재 다리 타입일 때만 표면 후속을 수행하며 정상 알터164는 그 분기를 실행하지 않는다.

이 알림 cue는 **00542a50/CD00512bd8**이며 희생 완료의 cue **00542a70/CD00512bf8**와 다르다. `reason2` 제거는 함수가 자연반환한다. 최초 Attach1까지 빈 부모 알림으로 대신하면 알터 O프레임/원래 owner 저장이 누락된다.

## 근거와 범위

- 도구: `tools/decomp_daiscontentnotice_oracle.py`.
- 자료: `daiscontentnotice-x86.tsv`, `daiscontentnotice.json`.
- C++ 전용 검사: `DaisContentNoticeTests.cpp`, macro `NETSTORM_DAISCONTENTNOTICE_FIXTURE`.
- 입력은 reason0/1/2/3, kind0/158/165, owner0/1/2, 로컬1/2, 알터 존재/부재, 원래 WORD0/2/0x82/0x1234/0xffff와 현재 O첫표1/3/7을 교차한다.
- 실제 새 알림·typed finder·타입 조회·WORD helper는 보호PE의 허용 함수 범위에서 실행한다. 가상 Unpop/Repop와 cue 장치는 명시 관찰 경계다. 기존 Altar 입력준비 호출은 새 호출수에서 제외하며 기존668/2866/1928/2964를 합산하지 않는다.
- 독립 원본 **1444정상/assert0**이며 각판본722입력/두x87반환1444가동일하다. 첫 C++ 공유 대조의 사제 후보 발자국 누락을 교정한 뒤 단위12 최종 Release는 경고/오류0, 전수1검사는 **1444개 실패0**으로 통과했다. 로그는 `extracted/run12-rawDaisContentNotice_final.log`다. 원본/복사본 게임·GUI 실행0이다.
- 최초1444과검사/도구/생산/fixture/report/spec를 `hist/daiscontentn-b4878`에 당시SHA로보존했다. Altar VT 선언을읽는검사 include1줄만추가했고동일1444를다시관찰했다. `analyze_daiscontentnotice_successor.py`/`daiscontentnotice-successor.json`은전체정확역변환·생산/도구/fixture/반환/효과/입력불변을감사하며추가고유0이다.

첫 공유 검사의 `Raw finder footprint` 예외는 알터가 없는 입력에서 사제158 후보를 검사할 때 발생했다. 원본 도구의 상위 `DaisOracle.setup`은 사제를 1×1로 준비했지만 C++의 `types[158].footX/Y`는0이었다. 현재 검사는 그 두 필드를1로 준비하는 한줄만 추가했다. 생산몸체·1444 fixture/report/tool·기존 successor는 당시 바이트 그대로 보존한다. `analyze_dais_followups_cpp_correction.py`와 `daiscontentnotice-cpp-correction.json`은 현재 Test의 정확 역변환과 현재SHA를 연결한다. 실패 로그/3차 빌드와 관련 전체126파일은 `hist/dais-followu-aeb72`에 보존했으며 새 원본 입력은0이다.

실제 `ContentFormHost`의 Attach1과 Unpop 제거2에 이 몸체를 공급하고, 같은 Pool의 알터 재등록 및 전체 희생 삭제/해방 소비자를 연결하는 작업은 후속이다. 이 부모 알림만으로 캠페인 의식 완주를 완료로 표시하지 않는다.
