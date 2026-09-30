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
* 원본 창의 그림·배치는 확인하지 못했다(캡처 없음).

## 3. `GetTechnology` (참고)

`GetTechnology,N` 은 로컬 플레이어에게 타입 N 의 지식 플래그(`0x4afa70(…, 4, N, …)`)를 켜고,
`global.newTechName`(타입 이름)·`global.newTechPic`(그림)을 설정한 뒤 `tell.english` 의 `[NewTech]`("Technology Gained! Your Altar is consumed!") 를 Tell 한다.

## 4. 클론 구현

* `TutorialDialogScript.Choose` 는 `ShowTechnology` 를 `ShowKnowledge` 로 돌려주고 브리핑은 닫지 않는다.
* `FortMapViewer.Knowledge.cs` — 브리핑 위에 겹치는 지식 창(원소별 이름 목록)과 미션 화면의 F6. 원본처럼 스크립트가 아닌 직접 연 창이라 시계를 멈추지 않는다.
* 미확인: 창의 실제 모양, `0x594fc8` 플래그의 뜻, `+0xec & 0x200000`·`+0x9c == 10` 의 게임상 의미(클론은 `ProducibleUnit` 기준으로 대신함).
