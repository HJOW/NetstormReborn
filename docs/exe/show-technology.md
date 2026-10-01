# 미션 스크립트 `ShowTechnology` / `GetTechnology` 버튼 (exe 정적 분석)

2026-10-01, 원본 게임 실행 없음. `tools/exe_callscan.py --dis`(capstone)로 `originals/Netstorm.exe`(10.78) 바이트를 읽었다.
디컴파일 결과(`Netstorm.c`)에는 이 함수들이 없다(Ghidra가 함수로 만들지 못한 구간).

## 1. 명령 이름 표

`.data` 의 명령 표는 8바이트 항목 `{이름 포인터, 번호 변수 주소}` 이다.

| 명령 | 이름 문자열 | 번호 변수 | 디스패처(`0x4d7cc4`~) 호출 |
|---|---|---|---|
| `GetTechnology` | `0x518728` | `0x542970` | `0x494ea0`(→ 점프 `0x492a30`), 인자 1개 |
| `ShowTechnology` | `0x518718` | `0x542974` | `0x492b60`, **인자 없음** |

`ShowTechnology` 는 `0x594fc8 == 0`(무엇인지 미확인 플래그)이고 `0x595344 != 0`(전투 중 표시)일 때만 실행된다.

## 2. `ShowTechnology` = 지식 창 (F6 "View Netstorm Knowledge")

* 스크립트의 `$Button=Review Knowledge,ShowTechnology,55` 에서 **인자 55는 디스패처가 넘기지 않으므로 쓰이지 않는다.**
  모든 원본 스크립트(24개)가 같은 55 를 쓰는 것도 이 때문으로 보인다.
* `0x492b60` 은 타입 전체(`0x541348`개)를 훑어, 로컬 플레이어가 아는 타입(`0x4acac0(플레이어, 4, 타입)`)만 목록으로 만들어 창(gump, `0x477bb0`)을 만든다.
  원소 분류(타입 `+0x98` 값 0~3)에 속하는 항목만 담고, 플래그 `+0xec & 0x200000` 인 타입과 종류 `+0x9c == 10` 인 타입, 자기 템플 타입(`0x54126c`)은 뺀다.
  항목 이름은 타입 `+0xa0` 의 문자열 길이를 쓴다.
* **2026-09-30 사용자 플레이 녹화에서 화면 확인:** 원본 창은 SUN·WIND·RAIN·THUN.의 네 행과 그림·이름이 있는 카드 격자다. 카드 하나를 열면 그림, 원소·분류·체력·사거리·피해·비용·필요 에너지, 스크롤 본문, `Back`·`OK`가 있는 상세창이 뜬다. 브리핑의 `Review Knowledge`로도 열리는 장면이 기록되었다. 근거와 시각은 [녹화 관찰 노트](../videos/the-war-begins-record-play-20260930.md)를 참조한다.
* **2026-10-01 녹화 재판독으로 확정:**
  * **어두운 카드 = 마우스가 올라간 카드**(00:14 Whirlibase → 00:16 Sun Cannon, 그 사이 마우스가 같은 행에서 카드 4칸만큼 왼쪽으로 이동).
  * **카드 목록 = 맵 `.fort` Technology 의 목록 플래그 4 타입**(The War Begins! 26개)에서 `sunWalker`(골렘)를 뺀 25장. exe 의 `0x4acac0(플레이어, 4, 타입)` 과 같은 플래그 값이다. 생산 등록 목록(워크샵 "Knowledge Available")은 이와 별개로 미션 `myTech`(Rain Generator·Sun Cannon·Whirlibase)였다.
  * **행 안 순서 = `.type` group 순서** battery → cannon → archer → walker → blocker → fence → aviary → balloon. 카드 이름은 `description`, 카드 그림은 유닛 기본 모습, 상세창 그림은 도움말 삽화(help 프레임), 상세창 본문은 `help.english` 의 `<a name="<타입>Type">` 절이다.
  * 1024×768 기준 격자 왼쪽 위 (227, 155), 머리 칸 폭 약 70, 카드 약 70×103, 간격 73·106px. 머리 칸은 원소 이름(위)과 원소 기호(아래, `mana` 타입의 켜진 그림).
  * **창이 열려 있어도 게임 시간은 흐른다**: 09:25.0~09:38.2 에 열린 채 SP 3,450 → 3,650.

## 3. `GetTechnology` (참고)

`GetTechnology,N` 은 로컬 플레이어에게 타입 N 의 지식 플래그(`0x4afa70(…, 4, N, …)`)를 켜고,
`global.newTechName`(타입 이름)·`global.newTechPic`(그림)을 설정한 뒤 `tell.english` 의 `[NewTech]`("Technology Gained! Your Altar is consumed!") 를 Tell 한다.

## 4. 클론 구현

* `TutorialDialogScript.Choose` 는 `ShowTechnology` 를 `ShowKnowledge` 로 돌려주고 브리핑은 닫지 않는다.
* `Netstorm.Core.Rules.KnowledgeCatalog` — 카드 행 규칙(행 순서·group 순서·골렘 제외)과 `.fort` Technology 지식 추출. 테스트 `KnowledgeCatalogTests`가 녹화의 25장을 행·순서·이름까지 대조한다.
* `FortMapViewer.Knowledge.cs` — 2026-10-01 원본 화면대로 카드 격자(호버 어둡게)와 상세창(도움말 삽화·수치·`help.english` 본문 스크롤·Back·OK)으로 다시 만들었다. 원본처럼 스크립트가 아닌 직접 연 창이라 시계를 멈추지 않는다. 검증: `--mission thewarbegins --knowledge [타입]`.
* 원본과 다른 점: 원본 돌 질감 창 그림 대신 단색 상자, Damage 값(원본 계산 미확인 — Shooter 는 `?`), 격자 위치(클론은 화면 가운데), 상세창 OK 가 격자까지 닫는지(클론은 모두 닫음)는 미확인.
* 미확인: `0x594fc8` 플래그의 뜻, `+0xec & 0x200000`·`+0x9c == 10` 의 게임상 의미(클론은 `ProducibleUnit` + 골렘 제외로 대신함), 카드가 없는 원소 행을 그리는지(클론은 네 행을 항상 그림).
