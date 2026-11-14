# webpj 메뉴 Gump 원본 관측 연결 (WB04-E492, 2026-11-15)

3.8절 이월 목록의 `menugump`를 원본 10.78 독립 x86 관측 **154행**(originals; fixture 308행 중 CD 10.72 154행은 웹 범위 밖이라 제외)에 대조했다. 154행 모두 첫 실행에서 일치했고 이식 결함은 나오지 않았다. `PORT_MAP`과 모듈·시험 머리의 "전용 PE fixture 없음/cpppj에도 직접 시험이 없다"는 오기였다.

## 비교한 것

`tests/menu-gump-native-fixtures.test.mjs`가 cpppj `MenuGumpTests`(`OriginalMachineObservations`)의 입력 구성을 옮긴다. 입력 열만 공급하고 관측 열은 비교에만 쓴다.

- **입력**: 세 노드(메뉴·장식·제목)의 원본 offset 55DWORD(vtable/flags/scale/기본·표시 크기/확장 중/항목 폭/목표 크기/항목 offset 40칸/메뉴 flags/항목 수/장식/제목), controlled 메뉴 표(20칸)·index·앞 네 idle 칸·등록 루트, 콜백 응답(콜백 직전 상태를 남긴 뒤 입력이 지정한 필드(크기·목표 크기·장식/제목 토큰 또는 현재 index)만 쓰는 재조회 입력), 실행할 action 열(Width·Height·Redraw·Expand·Idle·Item·Take·Current·Count·Attach·Object).
- **비교**: 콜백 호출 기록(`push`/`pop`/`redraw`/`base_redraw`/`attach`/`object`과 인자 비트, 각 호출 당시 세 노드 55DWORD·표 상태), 최종 노드 55DWORD×3, 최종 표 상태, 각 action 의 반환값(`Item`은 float 비트 두 값), 4번째 이후 idle 칸이 0 인지(뒤쪽 idle 보호). float32 필드는 NaN 비트 보존을 위해 Uint32 보기와 `floatBits` 로 오가며 비교한다.

## 변형 확인

자동 변형 75개 중 34개가 잡히고 41개가 생존했다. 기존 `menu-gump*` 시험 여섯 파일과 합치면 4개가 남고 모두 동등 변형이다: `ftol` 의 ±2^63 경계(`>=`→`>`·`<`→`<=`·상수의 더블 표현) — 경계 값에서 정수 절삭의 하위 DWORD 가 두 경우 모두 0 이다. 나머지 37개는 상수의 낮은 비트·클래스 필드 기본값·생성자 검증으로 이 시험의 입력(모든 필드를 원문 raw 로 설치·훅 항상 연결)에 가려지고 기존 시험이 잡는다.

## 남은 것

3.8절의 `hud-nsinfo-create`(170행)·`minimap-blit`(122행)이 HUD 쪽 남은 항목이다.
