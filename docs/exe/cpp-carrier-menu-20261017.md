# Carrier의 운반물·주문 메뉴와 기본 객체 사슬

2026-10-17, `HJOW-Athlon`. 마지막 전체 정밀 디컴파일도 **HJOW-Athlon / 2026-10-16**이다. 기존 정밀 소스를 읽었으며 새 Ghidra 내보내기·원본/클론 게임·GUI 창 실행은 없다. 두 보호 PE는 격리 메모리 사본으로만 실행했다.

## 복원 범위

`RawCarrierMenu`는 PATCH10.78 `004288f0`와 CD10.72 `004e5180`의 전체 메뉴 부모 몸체다. 같은 Pool/현재 TYPE/기존 `RawContainedFinder`로 운반물을 순회하며 Carrying 머리·Drop·Cast·About을 작성한다. 마지막에는 기존 `RawObjectMenu::OpenDamageable`을 호출한다.

`BindPriestMenuCarrier`와 `BindCarrierMenuBase`는 **사제 기본 호출→Carrier→Damageable/회수→공통 객체 메뉴** 사슬을 연결한다. `BindCarrierMenuControls`는 기존 controlled 메뉴를 사용한다. 별도 UI 트리/명령 큐를 만들지 않는다. 현재 로컬 번호·편집 모드·지도 크기·CD SP·도움말 명령은 다른 처리기의 같은 상태를 참조한다.

`BindCarrierMenuWorld`는 같은 Pool의 기존 사제 지면 불가·실제 Host 사건·종속 조회·현재 비용에 연결하고 다른 Pool을 거부한다. 실제 이동 종류/활성 프로세스, 전체 내려놓기 위치, PATCH 분산 SP, 번역/설명, 기본 객체 메뉴는 실제 호출자가 공급해야 한다. 이번 단위로 클라이언트 Session에서 이 모든 의존이 활성화된 것은 아니다.

## 원본 관찰

| 항목 | 결과 |
|---|---:|
| 10.78 고유 입력 | 314 |
| CD 고유 입력 | 314 |
| 합계 | **628** |
| x87 53/64비트 정상 반환 | **1,256** |
| 두 정밀도 차이 | **0** |
| 실행한 실제 명령 | **340,232** |

`decomp_carrier_menu.py`는 전체 Carrier 몸체 외에 실제 CRT `__ftol`, PATCH25d 사건 래퍼, CD의 extra/지면·이동·493 사건 래퍼, CD 타입 비용 getter와 지불 가능 계산을 실행한다. TYPE/설명과 contained finder의 결과, 큰 지면/이동/공간 몸체, PATCH SP, 번역/문자열 CRT, 메뉴 작성/기본 Damageable 호출은 명시 외부 경계다. 게임/OS 호출은0이다.

내용물0/1/2/3개·혼합 flags, 건너뛸 genus, 강제/지면 불가, 이동 프로세스 유무/활성, 음수/0/큰 비용과 SP, NaN/경계 좌표, 편집 모드, 493/25d 사건을 포함한다. 설명/이동/위치/SP/사건/항목 콜백이 소유자·타입·현재 로컬/편집 상태·현재/다음 내용물 flags를 바꾸는 입력도 포함한다. 다음SID는 기존 finder가 보존하는 원본 순서를 사용한다.

## 유지한 원본 차이와 교정

- PATCH Drop=`03000076`, Cast=`0300011a`이며 둘 다 carrier SID를 인자로 쓴다. **CD는 Cast에도 Drop 명령을 사용하고 `(contentSID<<16)|carrierSID`를 인자로 쓴다.** About은 양쪽 모두 도움말 명령과 내용물 TYPE다.
- PATCH는 현재493 사건을 물은 뒤 현재 raw 타입이 사제일 때25d도 묻는다. CD는493만 묻는다. 사건 조회가 타입을 바꾼 경우 다음25d 조회 여부도 현재 타입으로 정한다.
- PATCH 비용은 현재 인코딩 정수 표를 디코딩하고 실제 fild/ftol을 지난 값이다. SP 비교에서는 int 비용을 float로 축소하지 않는다. CD는 TYPE.cost의 실제 float→signed64 ftol 하위DWORD→float를 사용한다. CD SP가 NaN일 때의 지불 가능 결과도 PATCH와 통일하지 않는다.
- 가격 판단은 일반 위치 가능 여부를 덮어쓴다. 반면 이동/마비 중에는 비용/SP를 묻지 않고 비활성 안내를 작성한다. 이미 보존한 내용물 타입/flags와 콜백 뒤 현재 조회하는 owner/raw 타입/전역을 구분한다.
- 비활성 문구와 가격의 원본 접두사는 **`~r~E`**다. 첫 C++ 대조에서114개 차이가 나왔고, 이 접두사와 NaN 좌표 판정을 실제 기계어에 맞춰 교정했다. 기존 기대값을 제품에 맞춰 재생성하지 않았다.
- CD 디컴파일의 좌표 `(int)`는 수치 절삭이 아니라 **float 원문DWORD의 부호 검사**였다. 소수 좌표가 잘린다는 초기 추정을 기계어로 부정했다. 두 판본 모두 이번 NaN 좌표를 거부한다. 비교의 중간 정밀도/저장 순서는 판본별로 유지했다.

## 검사와 현재 한계

`CarrierMenu_Native628`은 두 PE의 전체 효과 순서·문자열·명령·인자·활성화를 모두 대조한다. 누락 의존성/건너뛸 genus/사제 기본 연결 검사1개도 통과했다. 첫 Release 빌드는 성공했고, 최종 수정 빌드2도 종료0이다.

`RawMenuControls_CarrierAndObjectMenuChain`은 보호10.78 TYPE/SHP/글꼴과 실제 공통 Gump 트리/등록/입력/삭제에 세 처리기를 조립한다. Cast `0300011a`/SID50과 About `02000017`/TYPE123을 실제 입력으로 확인했다. 명령 선택은 메뉴를 닫으므로 두 번째 선택 전에 실제 새 메뉴 수명을 연다. Player 전체 몸체와 지면/공간·SP 등의 효과는 검사에 명시한 외부 경계이며 게임 실행/자연 완주 증거가 아니다.

최종 Release 빌드2는 **경고0·오류0·종료0**이다. 집중2개와 실제 공통 입력1개가 통과했다. 전체 CTest는 **내부1,466개 모두 통과·실패0·213.96초·종료0**이며 전체 내부 출력을 `extracted/goal-carrier-menu-20261017-ctest-output.log`에 보존했다. 시스템 기본 커서 검사도 이번 전체 실행에서 통과했다. 이전 간헐 실패 원인의 수정 증거로 합산하지 않는다. 로그 접두사는 `extracted/goal-carrier-menu-20261017-`이다.

```powershell
python tools/decomp_carrier_menu.py --verify
python tools/decomp_object_menu.py --verify
cmake --build cpppj/build --config Release --parallel 3 -- /p:CL_MPcount=3
cpppj/build/bin/Release/netstorm_tests.exe --filter CarrierMenu_
cpppj/build/bin/Release/netstorm_tests.exe --filter RawMenuControls_Carrier
ctest --test-dir cpppj/build -C Release --output-on-failure
```

공유 CMake2개와 기존 공통 메뉴 검사 소스의 실제 변경 SHA만 이전/현재 이력과 함께 갱신했다. 기존 객체 메뉴1,434 및 TYPE2,352 원문/기대값은 보존한다. 실제 Carrier 결과는 새 `cpppj/carrier-menu.json`과 `carrier-menu-x86.tsv`에 독립 보존한다.

## 다음 실제 플레이 연결

Player 메뉴 `004c9700`, 내려놓기 위치 `00426a70`, 같은 실제 Session/현재 모드·Player/SP/기술·제외목록, 정적 문자열 명령 소비자, 객체 GumpForm 부착이 남아 있다. 커서 `445b30/445440/4458e0`와 일반 건설 확정/배치도 남아 **사제 Construct는 여전히 비활성**이다. 이번에 검사에서 조립한 사슬을 실제 Client가 사용하도록 후속 연결해야 한다.

Early Missions1~6·캠페인1-1/1-2·데모3개의400프레임 통과를 사용자 완주로 합산하지 않는다. 생산/다리·경제·AI 전략·GUI 미션 시작/단계·승패/저장/다음 미션과 전체 그래픽/음향 동일성은 미완료다. 전체 목표는 active이며 전체화면 DirectDraw는 이번1차 범위에서 제외한다.
