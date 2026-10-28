# 원본 F2/F3 표시 키 연결과 F8 실제 입력 검증

2026-10-28, 단위544, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 원래 산출물은 변경하지 않았다. 이번 독립 입력 관측은 같은 PC에서 보호10.78 바이너리를 읽어 격리 x86 메모리에서 실행했다. 원본 게임 프로세스는 실행하지 않았다.

## 동작과 연결

F2/F3가 키 입력에서 빠져 있던 부분을 원본 일반 입력 `4d62b0`의 확인된 분기로 연결했다. 메뉴와 Draw가 읽는 같은 Client 전역을 사용하며 새 숨김 상태를 만들지 않는다.

| 키 | 원본 동작과 조건 |
| --- | --- |
| F2 | 뗌을 제외하고 정지 여부를 조회한다. 비정지에서는 `59a8b0=1` 뒤 `59a8ac`의0/비0을 반전한다. Ctrl/Shift/Alt를 추가로 거부하지 않는다. |
| F3 | 비정지이며 Ctrl/Shift가 없으면 실제40칸 FIFO에 `02000088,0,2`를 넣는다. Alt는 허용한다. |
| Shift+F3 | Ctrl이 없을 때 초기0 지역값으로 `ReadInt("theammode")`를 호출하고 `SetIslands(0/1)`을 Tell한다. 이 호출은 정지 검사보다 앞이다. |
| Ctrl+F3·Ctrl+Shift+F3 | 테마 Tell과 하단 전환을 하지 않는다. |
| F8 | 기존 Guide→First 구현은 유지했다. QA 별칭을 추가해 실제 캠페인 안내 재표시와 모달 키 차단을 확인했다. 전체 Editor/도전/디버그 F8 구현 완료를 뜻하지 않는다. |

`ViewKeyInput`은 Guide/Gump가 먼저 소비한 사건과 현재 모달에는 호출하지 않는다. 뗌·다른 키에는 외부 장치를 요구하지 않으며, Shift+F3의 Tell 뒤 현재 정지 값을 다시 읽는다. 기본 캠페인 View에서 채팅 메뉴를 숨기는 원본 조건은 유지하지만, 원본의 F3 키 자체에는 그 표시 조건을 추가하지 않는다.

검사용 `--ui-script`에 F1~F9 별칭과 복수 Ctrl/Shift/Alt 접두 처리를 추가했다. 실제 Win32 가상 키/상위 사건 비트만 같은 입력 큐에 넣는다. F7 별칭은 검사 입력 표기이며 게임의 F7 효과를 구현한 것으로 세지 않는다. 읽기 전용 `raw_view_keys`는 같은 숨김/dirty DWORD와 현재 theammode를 보고한다.

## 원본과 집중 검사

[독립 관측 도구](../../tools/decomp_view_key_input.py)는 기존 Guide/Gump/정지/실제 큐 관측기를 빌려 `4d6e61..4da477`의 실제 현재 사건을 실행한다. ReadInt와 Tell만 명시 외부 장치로 제공한다. developer 전역55a480/55a484는 정상 시작0이다. 선행 소비·모달·뗌·모든 상위 플래그·정지·설정 부재/0/음수·FIFO 감김/포화를 포함했다.

**310사례행(307고유 입력, 관문 교차의 중복3행), 두 x87 정밀도의620실행, 차이0**이다. 출력 기대값을 Python에서 계산하지 않았다. 새 fixture의 실제 관측을 C++에서 replay하고, 공개 InputQueue API로 활성 FIFO를 비교했다. 비공개 물리 큐 전체가 C++과 같다는 주장은 하지 않는다. 원문 CPU의 물리 큐는 별도로 보존했다. 첫 정렬 전 정적 디스어셈블 출력도 남겼으며 주소 근거는 `native-disassembly-aligned.txt`와 실제 CPU 관측을 따른다.

새2개를 포함한 **42개 고유 검사/실패0, 등록3,373개**다. 기존608개 픽셀hit 관측과 실제 SHP7종·두 카메라11,774점을 포함했다. 공중3종 전용 검사/생성은 추가하지 않았다. 첫 제품/검사 Release target 빌드와 후속 검사 target 빌드는 모두 종료0이며 경고·오류0이다.

최초 집중 실행에서 기존 건물 숨김 검사의 두 선택 단언이 실패했다. 이 검사는524의 픽셀hit 복원 이전에 쓰던 고정 사각형 입력점(336,403)을 사용했고, 현재 복원된 픽셀 선택 경로에서는0이었다. 선택 결과로 입력을 탐색하지 않고, 별도로 그린 실제 SHP 공통 출력의 불투명 픽셀(403,434)을 입력으로 삼았다. 같은 사각형 가정을 사용하던 캠페인 월드 검사도 같은 방식으로 수정했다. 숨김 전/후/복원에서 정확한 SID15002와 표시 변화·getter 재조회를 계속 요구한다. 최초2실패와 최종 건물 숨김/두 캠페인 월드 성공을 따로 보존했다. 선택기·픽셀hit 제품 코드는 바꾸지 않았다.

## 실제 UI와 후속 범위

현재 제품의 기본 캠페인1-1을1024×768 창·1280×720 전체화면·1280×800 창에서 실행했다. 다음을 **3게임 모두 종료0**으로 확인했다.

- F2의 숨김/복원, key-up 무효와 Alt 허용.
- F3 하단 창 표시/숨김, key-up·Ctrl·Ctrl+Shift 무효와 Alt 허용.
- Shift+F9 정지 중 F2/F3 효과 차단과 Shift+F3의 테마0/1 두 전환, 정지 해제.
- 실제 F8의 원래 안내 재표시, 같은 HTML 모달에서 F2/F3 차단, Play Mission으로 복귀.
- 원래 View의 채팅/CHEAT 숨김, 같은 HUD·Session 유지, Leave→Main Menu의 HUD 반납.

4:3의 F2 전후 캡처를 육안으로 확인했고, 실제 변수/구분선/정지/HTML/수명은 세 화면비 모두 엄격히 대조했다. 첫4:3 실행은 같은 제품의 예비 실행을 수락했으며 불필요하게 반복하지 않았다. 확장 두 화면비도 같은 게임 SHA다.

F8의 잔여 관문도 다음 우선 작업이다. 현재 QueueMissionFirstKey가 Gump Route보다 앞에 있으므로 원본 Guide→Gump→뗌/console→F8 순서와 재대조해야 한다. Shift+F8의 Shift+F3 테마 분기와 Editor F8도 이번 실제 검증 범위 밖이다. 기본 비모달 안내 재표시의 성공을 이 경계의 완료로 확대하지 않는다. `f8-next-analysis.json`을 따른다.

F7도 이어서 작업한다. 원문 `43bc40`은 현재5319c0·표 준비557938·dirty를 공유하며 옵션0에서94/97 색표를 만들고 옵션1에서85/71/128/97/94 표를 해제한다. 현재 표시의 markerEnabled 고정과94/97의 무조건 색표, GameWorld::Remap의94 캐시가 연결 대상이다.85/71/128의 다른 생성/보존 호출자는 더 대조해야 한다. Shift+F7은 `4db140`의 원래 GIF 저장과 별도 Tell도 포함한다. 정적 후속 메모는 `f7-next-analysis.json`이며 F7 실제 키 실행·새 검사는 하지 않았다.

## 같은 최종 제품34경로 통합

표시/마나/UI 제품 변경 다섯 단위가 누적돼 지침의 통합 시점에 현재 게임 SHA로 다시 실행했다. Early6·Priest Training9·캠페인16의일반31경로와 실제Demo메뉴3경로가 **34/34, 종료0, 실패0**이다. 모든 경로에서 정확한 미션·일반/데모 구분·Start/Guide·Session·로딩 해제·비정지·전체화면을 확인했다. 플레이 관측 사이 실제gameTime은 모두 증가했고 최소6.390초다. 데모는 원래15초 안내 timeout을 지나 실제AI가 활성화된 장면까지 들어갔다.

일반은 `--frames 180 --fps 40 --fullscreen`, 데모는 같은검사기의 `--demos --fps 40 --fullscreen`(602프레임·원래 timeout)이며 기존532의보호된게임자료사본을사용했다. 사본의시나리오를이번에수정하지않았고각batch의자료/보호원본전후해시는같다. 실제키3게임과합쳐 **현재제품37게임모두종료0**이다. 완주/승패/전체동일성검증과구별한다.

```powershell
python -X utf8 tools/cpp_view_key_smoke.py --output extracted/cpp-view-keys-544/replay-keys
python -X utf8 tools/cpp_solo_entry_smoke.py --output extracted/cpp-view-keys-544/replay-normal --game-dir extracted/cpp-building-menus-532/final/data-1024x768 --fullscreen --frames 180 --fps 40
python -X utf8 tools/cpp_solo_entry_smoke.py --output extracted/cpp-view-keys-544/replay-demos --game-dir extracted/cpp-building-menus-532/final/data-1024x768 --fullscreen --demos --fps 40
```


[보고서](../../cpppj/view-key-runtime-20261028.json)와 `extracted/cpp-view-keys-544/`의 입력·상태·로그·캡처·초기 실패를 참고한다. 게임 SHA256은 `9fe4977df769a202a952902f8dad9fc733c5c540e0a65cfdc61c213bcd90e6ad`, 검사 SHA256은 `693611c5d7d8d4fc777a70728b85ad59bca4af6446c852dd48bf89154785ada0`이다. 원본/CD1,819파일·AGENTS와 기존 fixture는 불변이며, 다른 세션의 dotnetpj 파일은 수정하거나 stage하지 않았다. 최종 소스/검사/fixture2,934파일은 freeze-final.json으로 고정했다.

Windows10 Pro19045의 현재 제품 검증이다. Windows11 실기, 자연 완주·전체 게임 동일성, 사용자가 맡은 사제/마나 수정 후 실제 플레이는 남는다. 스펠 도움말·자연완주 등 사용자 보류, 공중3종 정적 제한과 멀티플레이 후순위·신규 검사/실행/통신 시험 금지는 유지한다.
