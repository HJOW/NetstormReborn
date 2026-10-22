# 최신 TEST02와 원본 Test Battle의 영어 시나리오 누락 오류

2026-10-22, 작업 단위326. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-22**다. 기존 reverse-engineer-anything 분석 절차에 따라 보호 PE·정밀 소스·실제 실행을 대조했다. REA의 Windows H: 경로 접근 실패는 이전에 확인한 상태로 두고 기존 로컬 분석 도구를 사용했다. 원본 게임 실행은 AGENTS의 해당 호스트 예외에 해당한다.

## 최신 맵 확인

사용자가 갱신한 `originals/d/TEST02.fort`는 **2,150바이트**, SHA-256 `f785543d3b907fb3d09d568b740a21a8402a87e110f1469e873fc09f44858d6d`다. 과거 10월 4일 건설 분석의 TEST02 배치를 현재 맵으로 취급하지 않는다. `TEST02.english`는 1,001바이트, SHA-256 `03b41d7b2d9718d70fc7e29e14cec4e3779f77e95f7ea99722f83049c7052260`다.

Python Fort 판독기와 C++ `--inspect-mission originals TEST02`에서 35개 섹션·끝의 남는 바이트0·최상위 객체14개를 확인했다. 표의 이름과 소유자는 파일의 원시 타입/값이며, 내용물은 최상위 객체 수에 더하지 않는다.

| 영역 | 저장 배치 |
| --- | --- |
| Terr00 | 사제1(`qa=129`, owner1), `residence`1(owner1), `thunderVortex`1(owner1, `sunWalker` 내용물1), `thunderFactory`1(owner1), **Bulf1(owner1)**, 간헐천3(owner0), `sunFactory`1(owner1) |
| Terr01 | 사제1(`qa=2`, owner2), `residence`1(owner0). 신전·워크샾은 없다. |
| Terr02 | 사제1(`qa=3`, owner3), `residence`1(owner3), `rainVortex`1(owner3, `sunWalker` 내용물1) |

시나리오는 `missionType=Tutorial`, `myStartMoney=500000`, `myTech=all`, 사용자 자기 동맹1이다. AI2/3은 시작 SP0, 전체 기술, 각각 자기 동맹2/3이며 두 AI 모두 `!USE_PRIEST_TO_COLLECT`, AI2에는 추가로 `!BUILD_DAIS`가 있다. Fort의 Money **1187**과 시나리오의 시작 SP **500000**은 서로 다른 입력이다. 원본 Test Battle 브리핑/전투 화면에서는 500000을 확인했다.

포획·희생 회귀에는 이 배치를 다음 우선 자료로 사용한다. 기존 Bulf·워크샾과 상대 사제를 이용할 수 있지만 **시작 배치에 제단/다이스는 없다**. 실제 사용자 건설→상대 사제 포획→운반→제단 전달→희생 전체를 새 TEST02에서 완료했다고 주장하지 않는다. 현재 포획/희생 검사는 다른 맵과 명시 입력으로 수행한 범위를 유지한다. 영어 스크립트의 성공 조건 `[Ai4PriestDead]`도 그대로 보존했으며 AI2/3 포획 성공 조건으로 임의 변경하지 않았다.

## 원본 10.78 실제 A/B 재현

analyzeManager를 Release 빌드했다(경고0·오류0). 이 도구는 `originals` 전체를 독립 세션으로 복사하고 복사본 설정만 변경한다. 세션 **`20261022T054114444Z-efb6b1cb3b02`**, 게임 PID **11884**, 실행 파일 SHA는 보호 원본과 같다. 이번에는 기존 `ns_driver.py`의 MCP 입력/화면/자동 녹화를 사용했으며 새 UI 자동화 장치는 만들지 않았다.

처음에는 로컬 analyzeManager 실행 파일이 없어 빌드했으며, 첫 요청의 FPS10은 지원 값30/60 검사에서 게임 생성 전에 거부됐다. 이후 FPS30으로 위 한 세션을 실행했다. 두 준비 오류도 작업 폴더에 보관했다.

1. Edit → Create New Map 이름에 `TEST02` → 기존 최신 Fort를 편집기로 불러왔다.
2. `TEST02.english`가 존재하는 상태에서 Game → Test Battle → TEST02 브리핑 → Go! → 전투 진입이 정상이다. 화면 SP는 500000이다.
3. Game → Return to Editing으로 돌아왔다. **세션 복사본**의 `d/TEST02.english`만 `TEST02.english.review-backup`으로 이름을 바꾸었다. 보호 원본은 변경하지 않았다. 백업 원문 SHA가 보호 영어 파일과 같음을 확인했다.
4. 같은 편집 상태에서 Game → Test Battle을 다시 눌렀다. **2026-10-22 05:45:02 UTC**, Windows Application Error **1000 / Record 3668**이 발생했다. 예외 **`0xc0000005`**, 모듈 오프셋 **`0x639da`**, 실행 주소 **`0x4639da`**다. 별도로 열어 둔 프로세스 핸들도 자연 종료 코드 **`0xc0000005`**를 확인했다.
5. 입력 직후 1.8초의 첫 status는 Windows 오류 처리 중인 프로세스를 running으로 표시했으나, 후속 status는 running=false다. 첫 캡처의 편집 화면 잔상만 보고 정상 처리로 판단하지 않는다. 자동 녹화는 5,211프레임, 오류 없이 중단되었고 end_session과 드라이버도 종료했다.

이는 동일한 맵에서 영어 파일 유무만 바꾼 원본의 결함 재현이다. 빈 시나리오, 잘못된 missionType, 다른 언어 파일만 존재하는 경우, 10.72 CD판의 실제 실행은 별도로 검증하지 않았다. 새 맵 생성 직후의 별도 Save As 조합도 이번 A/B와 구별한다.

원자료는 `extracted/test02-review-20261022/`의 명령/결과·화면·`actions.jsonl`·`application-error.xml`·`exit-monitor.json`, 자동 영상은 해당 analyzeManager 세션의 `recording/`에 있다. 영상과 원본 작업본은 Git에 넣지 않는다. [기계 판독 보고서](../../cpppj/test02-scenario-20261022.json)에 주요 원문/증거의 SHA를 보존한다.

## 종료 원인의 기계어 대조

정밀 소스의 명령005429fc(**020000c2**, param0)는 `004bfb50`의 현재 Fort 갱신 뒤 `00463b70`으로 미션을 생성하고 바로 `004639d0`을 호출한다. `00463b70`은 미션 생성기 `00482fb0`의 반환값을 `Guide+0x800`에 보관한다.

`00482fb0`은 `missionSpec`의 언어 시나리오를 `00440380`으로 읽는다. 이 로더는 파일을 읽지 못하면 설정을 추가하지 않고 반환한다. 생성기는 Fort 존재와 `mission.missionType` 등록 일치를 요구하며, 미션을 만들지 못하면 MissingMission 안내를 시도하고 설정을 해제한 뒤 **0을 반환**한다. Test Battle의 뒤따르는 `004639d0`에는 이 실패값 검사가 없다.

```text
004639d0  8b8100080000  mov eax, dword ptr [ecx+0x800]
004639d6  8b4c2404      mov ecx, dword ptr [esp+4]
004639da  894864        mov dword ptr [eax+0x64], ecx
004639dd  c20400        ret 4
```

보호 PE의 이 **네 명령 전체**를 Unicorn에서 하위 대체 없이 실행했다. 정상 미션 포인터는 +64에 인자1을 저장하고 ret 4로 복귀한다. 미션 포인터0에서는 실제 **004639da에서 주소0x64에 DWORD1 쓰기**가 실패한다. 이 위치는 실제 Windows 오류 기록과 일치한다. 미션 생성기 전체를 에뮬레이션하거나 살아 있는 프로세스의 레지스터를 캡처한 결과는 아니며, NULL 원인은 정밀 호출 경로와 이 최소 재현을 함께 근거로 판단한다.

## 다시 확인하는 명령과 남은 연결

```powershell
python tools/analyze_test02_scenario.py
```

위 명령은 게임/창을 실행하지 않는다. 현재 TEST02와 보관한 실행 증거·기계어2경로를 확인하고 JSON 보고서를 다시 쓴다. 보호 PE 두 판본·TEST02 두 파일·TAFF·정밀 소스의 검사 전후 SHA도 대조한다. 실제 A/B 실행을 새로 수행하려면 analyzeManager로 새 독립 세션을 준비해야 한다.

이번 단위는 조사/재현이며 C++ 제품 소스는 변경하지 않았다. 기존 전체 CTest는 단위325의 **2,261개·실패0**을 유지하며 새 전체 통과로 합산하지 않는다. Edit 버튼은 여전히 미구현 안내다. 실제 Client/Editor 연결에서는 이 원본 결함을 기록한 상태로 **시나리오 로드 실패 후 비어 있는 미션을 사용하지 않는 경계**를 구현해야 한다. 파일을 자동 생성하거나 원래 플레이어 설정을 추측하는 방식으로 누락을 숨기지 않는다. TEST02 자연 포획·희생, 편집 저장/불러오기/Test Battle/복귀 및 전체 캠페인 목표는 계속 진행한다.
