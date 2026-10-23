# 공통 객체·Damageable·회수 메뉴 복원

**단위385 교정(2026-10-24):** 아래 초기 기록의 `global.salvageSid`가 루트 `salvageSid`에 저장된 결과는 당시 Registry에 무명 객체 둘만 있던 결함 때문이다. 실제 양판본 CRT에는 [장기 global·fort 객체](cpp-config-static-owners-20261024.md)가 있다. 두 객체를 복구한 뒤 실제 메뉴 검사는 `global.salvageSid=50`과 루트에 같은 키가 없는 것을 확인하도록 교정한다. 원문 Get의 this 버퍼 관문을 유지하도록 검사에 명시 루트 입력 한 항목도 둔다. 원문 메뉴/Config 본체·기존 Native fixture/report는 변경하지 않았다. 당시 실패와 잘못된 해석은 아래에 역사 자료로 남긴다.

이 교정 뒤 옛 `decomp_object_menu.py --verify`는 역사 제품 핀(메뉴 검사/CMake2파일)의 변경 때문에 종료1이다. 최초 report/도구/fixture는 그대로 보존한다. 현재 원문 입력6개와1434고유입력만 검증할 때는 `python tools/verify_native_manifest.py cpppj/object-menu.json`을 사용한다. 실제 제품 검사와 최종 전체 결과는 [단위385 기록](cpp-editor-checkpoint-runtime-20261024.md)에 있으며 옛 제품 핀을 현재 검증 결과로 고치지 않는다.

**단위115 교정:** 당시 외부 경계로 둔 `004844f0`/Player 메뉴의 문자열 API 해석은 잘못됐다. 실제 반환/전달 값은 subscriber DWORD다. [실제 조회와 형식 교정](cpp-player-identity-20261017.md)에서 두 판본의 실제 getter/검색 몸체와 메뉴48입력을 추가 대조했다. 아래1,434행은 당시 공급한 하위 경계와 상위 메뉴 몸체의 범위로 읽으며, 실제 이름 반환의 근거로 사용하지 않는다. 과거 fixture는 보존했다.

2026-10-17, `HJOW-Athlon`. 마지막 전체 정밀 디컴파일도 `HJOW-Athlon / 2026-10-16`이다. 새 Ghidra 내보내기나 원본/클론 게임 실행은 없다. 보호 PE를 Unicorn의 격리 메모리 사본에서 읽었다.

## 범위와 결과

사제 `RawPriestMenu`는 건설 항목 뒤에 항상 Carrier 기본 메뉴를 호출한다. 이 사슬의 마지막 세 몸체를 `RawObjectMenu`로 복원했다. Carrier 자체와 실제 Session 조립은 후속이다. 사제 Construct 항목은 여전히 비활성이다.

| 몸체 | 10.78 | CD10.72 | 고유 입력 |
|---|---|---|---:|
| 공통 객체 메뉴 | `004b1270` | `004ae780` | 478 |
| Damageable 메뉴 | `0044cbd0` | `00462a70` | 478 |
| 회수 메뉴 | `0044ca00` | `004628a0` | 478 |

합계 **1,434고유 입력**이며 각 입력을 x87 53/64비트 정밀도에서 실행했다. **2,868정상 반환·정밀도 차이0·실제 명령190,090회**다. C++는 기본 메뉴·Damageable·회수의 각478행, 전체 외부 효과 순서/문자열/명령/정수·문자열 인자를 대조한다. 가상 개수·HP·환불, 번역/CRT, controlled 메뉴/Player 하위 메뉴, 설정 쓰기는 명시 외부 경계다. 이 의존 몸체 자체나 GUI 픽셀까지 원문에서 실행했다는 의미는 아니다.

## 보존한 동작

- 편집 모드의 Destroy, 특수 모드/chargeBlocked의 치트 메뉴, 현재 도움말, SetOwner, Player 하위 메뉴 순서를 유지한다. 원본 도움말 문자열이 NULL이거나 도달한 하위가 없으면 빈 성공으로 처리하지 않는다.
- 로컬 Damageable만 가상 `+80`을 호출한다. 그 콜백이 타입을 바꿨을 때 **10.78은 호출 전 타입**, **CD는 호출 후 타입**을 회수 인자로 넘긴다. 양쪽 실제 기계어 관찰에 해당 입력을 포함했다.
- 현재 슬롯의 TYPE/소유자 조건과 명시 회수 TYPE/개수를 구분한다. 비용의 +0/-0/NaN, `INT_MIN` 환불의 DWORD 부호 반전, 현재 항목 수에 따른 구분선, 금지 안내, `global.salvageSid` 설정, Vortex/Inf/일반 명령의 순서를 보존한다.
- **10.78에만** 최종 현재 raw 타입이122(`00541240`의 초기값)일 때 일반 회수 항목을 생략하는 분기가 있다. 설정 쓰기 뒤 타입이 바뀐 입력도 검사한다. CD에는 이 생략을 추가하지 않는다.
- Player 메뉴의 `Player` 번역은 **10.78에서 하위 메뉴 생성 전**, **CD에서 생성 후**다. 이는 번역/생성이 현재 상태를 바꿀 수 있는 호출 순서 차이이므로 통일하지 않았다.
- 환불 조회, 번역, 항목 수 조회, 구분선, 설정 쓰기가 뒤의 상태를 바꾸는 입력을 포함한다. 이미 snapshot한 flags와 뒤에 다시 읽는 금지 상태/raw 타입을 구분한다.

## 실제 메뉴 연결 검사

`BindObjectMenuControls`는 기존 `MenuGumpControls`/`MenuGump`와 현재 `ConfigInterface`를 사용한다. 새 메뉴 트리/입력 큐를 만들지 않는다. 문자열 인자는 실제 명령 소비자와 같은 상수 표의 토큰으로 변환하도록 요구한다. 원본의 정적 문자열을80칸 스크립트 고리로 대체하면 열린 메뉴의 인자가 만료될 수 있으므로 그렇게 연결하지 않았다.

`RawMenuControls_ObjectMenuCommandsAndCurrentConfig`는 보호 TYPE/SHP/글꼴과 같은 공통 Gump 트리를 조립한다. 실제 입력으로 금지 메뉴의 `02000095`/문자열 토큰과 허용 메뉴의 `0200005d`/SID50을 확인하고, 현재 설정의 `salvageSid=50` 및 공통 삭제를 검사한다. OS 창 입력이나 게임 완주 검사는 아니다.

첫 보조 검사는 fixture 헤더를 숫자 입력으로 읽어 실패했다. 첫 실제 메뉴 검사도 타입150의 보호 flags를 Vortex라고 가정하고 `global.salvageSid`를 그대로 조회해 실패했다. 실제 flags는262160이며 일반 회수 명령이다. 기존 `Config.Set`의 이름 경로 처리는 기본 설정의 `salvageSid`에 쓴다. 두 검사 기대/입력을 수정했으며 제품 분기나 기존 fixture를 이 실패에 맞춰 바꾸지 않았다.

## 검증과 재현

원문/fixture/SHA 보고서는 `cpppj/object-menu.json`, `cpppj/tests/fixtures/object-menu-x86.tsv`다. 새 원문 관찰만 생성했으며 기존 사제/커서/TYPE 등의 기대값은 변경하지 않았다.

```powershell
python tools/decomp_object_menu.py --verify
cmake --build cpppj/build --config Release --parallel 3 -- /p:CL_MPcount=3
cpppj/build/bin/Release/netstorm_tests.exe --filter ObjectMenu_
cpppj/build/bin/Release/netstorm_tests.exe --filter RawMenuControls_ObjectMenu
ctest --test-dir cpppj/build -C Release --output-on-failure
```

최종 Release 빌드3은 **종료0·경고0·오류0**이다. 원문 대조3개/누락 의존성1개/실제 공통 입력1개가 통과했다. 전체 CTest는 **내부1,463개 모두 통과·실패0·226.66초·종료0**이며 전체 내부 출력을 `extracted/goal-object-menu-20261017-ctest-output.log`에 보존했다. 시스템 기본 화살표 검사도 이번 전체 실행에서 통과했다. 새 도구의 출력 인코딩/제품 SHA 감사만 정리한 뒤 원문을 재실행했으며 fixture 바이트 SHA가 전체 검사 전후 동일하다. 로그 접두사는 `extracted/goal-object-menu-20261017-`이다.

직전 단위112에서 시스템 IDC_ARROW 소프트웨어 커서 검사가 전체/단독으로 실패했다. 이번 소스 변경 전 단독 재실행에서는 통과했다(`extracted/goal-menu-session-20261017-cursor.log`). 간헐 원인은 미확정이며 수정 완료로 합산하지 않는다.

## 남은 실제 사용자 플레이 경계

Carrier `004288f0`, Player 메뉴 `004c9700`, 같은 실제 Session의 모드/현재 Player/SP/기술/가상 조회, 정적 문자열 명령 소비자, 객체 GumpForm 부착을 연결해야 한다. 사제 메뉴·선택 후 커서 생성/조회/준비 `445b30/445440/4458e0`와 일반 건설 확정/배치도 남아 있다. 기존 회수 계산/명령 실행과 이 메뉴 작성은 별개다.

Early Missions1~6·캠페인1-1/1-2·데모3개의400프레임 검사를 사용자 완주 증거로 사용하지 않는다. 생산/다리·경제·AI 전략·GUI Start/StartDemo·미션 단계·승패/저장/다음 미션·전체 그래픽/음향 동일성 검증은 계속 필요하다. 전체 목표는 active이며 전체화면 DirectDraw는 이번1차 범위에서 제외한다.
