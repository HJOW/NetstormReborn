# 녹화 원자료 최종 판독과 클론 반영 (2026-10-02)

> 작업 환경: Windows `HJOW-Athlon`. 원본 게임은 실행하지 않았다. 클론만 실행해 화면을 확인했다.
> 요청: `playingVideos/`의 분석기 녹화(`record-play`) 세 개를 클론에 모두 반영하고 원자료를 지운다. 참고용 mp4 영상은 지우지 않는다.

원자료를 지우면 다시 측정할 수 없으므로, 지우기 전에 원자료(영상 프레임·녹음·입력)로만 얻을 수 있는 값을 다시 재고, 앞으로 참고할 축소 자료를 보관했다.
그다음 아직 클론에 들어가지 않은 항목을 구현했다.

| 세션 | 내용 | 기존 관찰 노트 |
|---|---|---|
| `20260930T154921831Z-8bdcc06b6539` | 캠페인 1-1 전체 플레이, 13분 53초, 1280×960 | [the-war-begins-record-play-20260930.md](the-war-begins-record-play-20260930.md) |
| `20261001T114152818Z-778e248ca3aa` | 캠페인 1-2 재현 플레이, 19분 44초, 1024×768 | [master-of-whirligigs-record-play-20261001.md](master-of-whirligigs-record-play-20261001.md) |
| `20261001T145330210Z-425636a2e82c` | UI·조작키 시연, 4분 16초, 1024×768 | [ui-controls-record-play-20261001.md](ui-controls-record-play-20261001.md) |

시각은 각 녹화 첫 프레임 기준 영상 경과 시각(mm:ss.s)이다. 녹음 판독은 [`tools/audiomatch.py`](../../tools/audiomatch.py)의 FFT 정규화 상관(문턱 0.3~0.55), 프레임 판독은 [`tools/recordplay_frames.py`](../../tools/recordplay_frames.py)를 썼다.

## 보관한 자료 (Git 제외)

| 위치 | 내용 |
|---|---|
| `extracted/record-play-archive-20261002/<세션>/thumbs/` | 1초 간격 축소 프레임(가로 384px JPEG). 1-1 834장·1-2 1,181장·UI 257장. `thumbs.csv`에 영상 경과 초·전역 프레임 번호 |
| `extracted/record-play-archive-20261002/<세션>/` | `video-*.frames.csv`(프레임 시각), `recording-index.json`, `audio-*.start.txt`, `input-events.jsonl`(마우스 이동을 뺀 버튼·키·경계 사건, 영상 경과 초 추가) |
| `extracted/record-play-final-20261002/` | 이번 측정 CSV(`r1-sfx-ritual.csv`·`r2-sfx-low.csv`·`r2-sfx-end.csv`)와 원본 크기 잘라내기(시작 팁·View 메뉴·우클릭 창·도움말·SP 숫자·낙하) |
| `extracted/screens/record-final-20261002/` | 클론 확인 화면(메뉴·미션·SP 숫자·GUI 검사 4종) |

이전 판독 산출물(`extracted/record-play-20261001/`, `extracted/record-play-20261001-2354/`, `extracted/audio/`)은 그대로 둔다.

## 룬 마크와 의식 시간

룬 음성 뒤 제단에 마크가 생기면 원본은 그 룬이 탈 때까지 `jimbuild.wav`(1.14초)를 이어서 반복한다. 첫 `jimbuild` 시각을 마크 시각으로 썼다.

| 룬 | 1-1 음성 → 마크 | 1-2 제단 2 | 1-2 제단 3 | 음성 파일 길이 | 마크 → 소멸 |
|---|---:|---:|---:|---:|---:|
| Wind | 1.97 | 1.98 | 1.98 | 1.91 | 10.03 / 10.04 / 10.03 |
| Sun | 2.05 | 2.07 | 2.07 | 1.91 | 10.06 / 10.05 / 10.03 |
| Rain | 1.87 | 1.87 | 1.87 | 1.83 | 10.03 / 10.04 / 10.05 |
| Thunder | 2.16 | 2.19 | 2.18 | 2.02 | 10.02 / 10.04 / 10.04 |
| Storm | 2.41 | 2.43 | 2.42 | 2.35 | 10.02 (완료) / 판매로 중단 / 10.03 (완료) |

- 마크는 **음성이 끝나는 직후**(파일 길이 + 0.04~0.17초)에 나타난다. 이전의 "1.6~2.6초 범위, 임시 2.0초"를 룬별 값으로 바꿨다.
- 마크에서 소멸까지는 14건 모두 10.02~10.06초다. 소멸 → 다음 음성은 사제가 계속 있던 6건에서 2.39~3.15초(평균 2.77초)였다.
- 1-2 제단 3의 첫 Thunder(18:34.6)는 마크 전에 사제가 떠나 `jimbuild`가 없었고, 복귀 뒤 18:42.2에 다시 음성이 나왔다(기존 판독과 같음).
- 의식 완료(`itIsDone2`) → `priestSacrifice2`는 4.02·4.01초, → 제단 폭발(`explodeSlot`)은 9.47·9.44초다.

클론: `SacrificeRuneMarkSeconds`(룬별)·`SacrificeRuneWardSeconds`(10.04)·`SacrificeRuneGapSeconds`(2.77)·`AltarConsumeDelaySeconds`(9.45), 새 사건 `SacrificeRuneMarked`. [희생 의식 계약](../gameplay/sacrifice.md#적용한-시간과-임시값).

## 희생 음악 재시작

1-2의 18:28.8 `sacrifice.mus` 재시작은 `thu22`(225초) 곡 끝이 아니었다. 18:28.2의 왼쪽 클릭(사제를 제단으로 보내는 명령) 0.6초 뒤이고, 프레임에서 사제는 18:32 무렵 제단에 닿았다.
1-1(12:05.5 → 12:06.4)과 1-2 제단 2(15:16.5 → 15:17.2)도 명령 직후 음악이 바뀌었다. 17:37.1에 희생 음악이 끝났을 때는 새 의식의 룬이 하나도 타지 않아 `thu22`로 넘어갔다.
클론은 포로가 묶인 제단으로 사제를 보내는 명령·의식 시작·재개에 희생 음악을 요청하고, 곡 끝 재선택은 룬이 하나 이상 탔을 때만 한다. [음악 문서 5절](../exe/music.md#5-의식이-끝난-뒤-음악-사용자-설명-2).

## 생산 창 Storm Power 숫자

- exe `FUN_0043e530`(“Storm Power Available” 창 갱신): 표시 숫자가 실제 값과 300 넘게 차이 나면 100, 30 넘게 차이 나면 10, 그 밖은 1씩 따라간다. 표시 색은 표시 숫자로 고른다.
- 네 번째 녹화 01:03.0~01:03.9 매 프레임: 2200 → 2100 → 2030 → 1970 → 1900 → 1830 → 1824 → 1819 → 1813 → 1806 → 1800. 한 단계가 초당 약 60~67번이다.
- 1-2 녹화 19:31.0 성공 창(게임 정지)과 같은 프레임에 +5,000이 한 번에 올랐다. exe의 즉시 맞춤 조건(`DAT_00594fb8`)을 게임 정지로 보고 클론도 정지 중에는 곧바로 맞춘다.
- 생산 창 유닛을 눌렀는데 비용이 SP보다 크면 exe `0x43ecc4 → FUN_0043db10`이 숫자를 0.15초 간격으로 10번 깜빡이고 집지 않는다. 재충전 중이면 아무 일도 하지 않는다.

클론: `StormPower.StepDisplay`(테스트 `StormPowerDisplay_FollowsRecordedSteps`), `FortMapViewer.Effects.cs`.

## 떨어지는 유닛과 다리 조각

- 1-2 13:21.6 운반 골렘 낙하를 프레임 차분으로 추적했다. 처음 약 30px/초에서 가속도 약 109px/초²로 내려가 1.6초에 약 190px를 떨어지고 13:23.7 무렵 사라졌다(1024×768 기준).
- 14:02 무너진 다리 조각은 약 1초 동안 초당 80~90px로 떨어지다 사라졌다.
- 클론은 Core가 낙하 기록(`DrainFallen`)을 남기고, 화면이 유닛은 1.8초·다리 조각은 1.0초 동안 같은 등가속 궤적으로 떨어뜨리며 마지막 0.3초에 흐리게 한다. 규칙상 제거 시점은 그대로다.

## 효과음과 사건

| 녹음 | 시각 예 | 클론 연결 |
|---|---|---|
| `jimbuild.wav` 1.14초 반복 | 1-2 00:16~00:23 건설, 룬마다 9번 | 화면 안 건설 중 오브젝트·룬 마크 동안 반복 |
| `priestForceField.wav` 이어서 반복 | 1-2 08:27 이후 기절 동안 | 화면 안 기절 사제가 있는 동안 1.238초마다 |
| `priestFall.wav` | 13:21.9, 14:02.0 | `PriestSuspended` |
| `priestFree.wav` | 11:06.7 Drop, 11:34.2 골렘 판매, 14:41.2·16:49.3 제단 판매 | `PriestReleased` |
| `collapse.wav` | 11:34.2, 14:41.2, 16:49.3 (모두 판매) | `Salvaged` |
| `upgradeComplete.wav` | 1-1 01:38.1, 1-2 01:44.4·08:39.2 | 새 사건 `WorkshopUpgraded` |
| `priestStruggleFade02-800.wav` | 15:16.3·17:36.5 묶기 | `PriestBound` |
| `unitLost.wav` | 1-2 09:30.5·13:17.5·13:27.5 (1-1에는 없음) | exe `0x4b0b89`: 내 오브젝트 제거 때 재생하고 위치 저장 → 클론은 전투로 파괴된 내 오브젝트(비행 공격체 제외), **U 키**로 그 위치 이동 |
| `golemMove1·2·4·5`, `priestmove1`·`priestMove3` | 명령 클릭 0.06~0.10초 뒤 33번 | 사제·골렘 명령 응답음(들린 변형만) |
| `openGump.wav` | 우클릭 0.07~0.08초 뒤, 00:08.5 Construct 클릭 | 우클릭 정보 창과 하위 창 열기 |

## 화면

| 항목 | 원본 녹화 | 클론 반영 |
|---|---|---|
| 시작 팁 | UI 녹화 00:00~00:01.6 "Did You Know?" tip15, Prior Tip / Next Tip / No More Tips / OK | `tell.english [TipList]` 40개, 한국어 번역, Options "Tell Tips at Startup" 연동, 팁 번호 저장 |
| 미션 화면 | 상단 막대·하단 상태줄 없음, Esc로 메뉴 막대 | 지도를 화면 전체에 그림, 알림은 지도 아래쪽 그림자 글자, 타이머는 T로 켬(기본 꺼짐) |
| View 메뉴 | 1-1 04:50~05:08: Hide buildings - F2 … View Player List - F9, 키는 노란색, 켜진 항목 파란 마크 | 같은 8항목, Shift+F3 섬 테마 숨기기(모든 섬 초록 지면) 구현, F9는 비활성 |
| Game 메뉴 | UI 01:31.5: 목표 다시 보기 / 재시작·떠나기 / 종료, 구분선 | 같은 순서·구분선 |
| Help 버튼 | UI 01:41: General Help - F1 / Technical Help / Version | 같은 목록, Technical Help(외부 Windows 도움말)는 비활성, Version은 원본 [About] 문구 창 |
| 우클릭 창 | 사제·워크샵·생산 항목·운반 골렘: 금색 모서리 창, 큰 제목, 노란 값, 밝은 안쪽 판·구분선, `Upgrade costs 800`·`Salvage gains 200` 뒤 SP 아이콘, `Carrying: High Priest`·`Drop High Priest`, `Player >` | 같은 구성. 하위 창 제목 `Construct Building`·`Knowledge Available`(+`Production Slots Available:`)·`Current Production`(+About). `Player >`는 비활성 |
| 도움말 | 본문 바탕 약 (52,49,51), 링크 연한 청록 약 (176,214,214), 위·아래 화살표 스크롤바, 능력치 값 노란색 | 같은 색·화살표 단추·값 색. `mana.8` 같은 그림 번호는 원본 그림 프레임 번호로 해석(이전에는 클론이 종료됨) |
| Options | Auto-Demo·Tell Tips·Pass Server Diagnostic도 켜고 끔 | 세 항목을 켜고 끄며 저장. 자동 데모 재생·서버 진단 기능 자체는 없다 |
| 생산 창 경고 | SP 부족 시 빨간 `Need more Storm Power to Build!` | 빨간 글자 |

## 원자료를 지운 뒤 더 확인할 수 없는 것

- `unitLost.wav`의 정확한 제외 종류(exe의 genus 10·타입 `DAT_005412a4`)와 1-2에서 무엇을 잃었는지(화면 밖).
- 다리 조각 낙하의 정확한 궤적(구역 안에서만 추적), 템플·대형 건물 낙하 모습(녹화에 없음).
- 생산 창 새 항목 깜빡임(`FUN_0043d690`) 조건, 다리 조각의 금 간 그림, 원본 Arial 계열 글꼴(클론은 AGENTS.md에 따라 D2Coding).
- 자동 데모 재생, Technical Help, 멀티플레이 Player 하위 메뉴·플레이어 목록(F9).

## 검증

- 단위 테스트: Release 빌드 오류 0, Assets 202·Core 251 통과(이번 신규 7개: 룬 시간표·희생 음악 단계·SP 숫자 따라가기·옵션 저장·낙하 기록·시작 팁 2개).
- 클론 GUI(1024×768, 클론 자체 입력): `tools/clone_help_options_smoke.ps1`의 Menu(시작 팁·Help 목록·버전 창 포함)·Mission·Workshop과 `tools/clone_ui_smoke.ps1`을 Windows PowerShell로 실행해 모두 통과했다. 별도로 영어 미션 화면·View 메뉴·섬 테마 숨기기·우클릭 창·도움말·SP 숫자 변화(3000 → 2680 → 2612 → 2600)를 캡처해 원본 프레임과 대조했다.
- GUI로 직접 보지 못한 것: 낙하 연출(Core 기록 테스트만), SP 깜빡임, 반복 효과음·명령 응답음의 실제 청취.
