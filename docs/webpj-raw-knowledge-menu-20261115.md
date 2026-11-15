# 전체 지식 창 이식 (WB04-E537)

6단계의 "공장·지식 메뉴와 기술" 중 `client/RawKnowledgeMenu`(원문 ShowTechnology 00492b60 Storm 카드·00493210 World 카드·004938c0 선택 메뉴)를
`webpj/www/core/raw-knowledge-menu.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
`GameMenuCommands`가 이 단위의 이름표(`knowledgeScriptCode`)를 공통 조회에 묶으므로 그 선행 부품이다.

## 옮긴 범위

- `showKnowledgeMenu`(Storm theme 0..3, 머리 WIND/RAIN/THUN./SUN)·`showWorldKnowledgeMenu`(World theme 4..7, 머리 EARTH/FIRE/WATER./MOON, 아이콘 12..15):
  메타맵 차단이면 효과 없이 빈 목록 → 커서 취소 → 현재 기술 부모 1회 → 타입 순서 순회(group 10·flags2 0x200000·신전 상품 제외, 보유/마나 조회 뒤 테마 범위 판정,
  문자열은 첫 NUL 앞만) → 행 수(마나 9·설명 81·머리 포함 100개) 범위 → 공유 CRT 정렬(`ui/knowledge-sort.js`) → 네 머리 뒤 카드마다
  번호 행·설명 두 줄(마지막 공백/하이픈에서 한 번만 나눔, 길이 8 이하·위치 1은 나누지 않음, Excluded/언어 3은 빈 줄) → 네 줄 배치(signed 정수 x/y,
  단정도 폭/높이, targetWidth 절반 보정, 줄별 누적 오프셋) → 위치 조회 순회 → 모두 만든 뒤에만 팝업 비트 → 표식(명령, 80BYTE 0).
- `showKnowledgeSideMenu`(004938c0): 기술 보유·번역을 조회하지 않고 Storm/World 두 300 폭 메뉴를 만들며 표식은 `ShowTecSideMenu`이다.
- `knowledgeScriptCode`: `ShowTechnology`(0x020000a0)·`CheatAllTechnology`(0x02000028)·`ShowTecSideMenu`(0x02000132)·`ShowTecMenu2`(0x02000133)·`GetTechnology`(0x0200009f),
  ASCII 대소문자 무시, 미등록은 0xffffffff. `createKnowledgeMenuState`는 화면/아이콘/언어/상품/도움말 명령 기본값을 만든다.
- 선택한 분기에서 도달한 효과만 필수이며 미연결은 `Error`(`연결 누락`)로 거부한다. 단정도는 `Math.fround`, 정수 배치는 int32로 계산한다.

## 시험

`webpj/tests/raw-knowledge-menu.test.mjs` 4건:

1. 원본 관측 `knowledge-menu-x86.tsv` **25관측**(Storm 카드): 입력 10필드(차단/타입 수/보유/제외/언어/화면/아이콘/번역)에서 노드 아홉 필드의 단정도 비트·표식·80BYTE 인자·호출 순서 전체.
   **첫 실행에서 25관측 전부 일치**했다.
2. `knowledge-world-menu-x86.tsv` **50관측**(World 카드 25·선택 메뉴 25). 첫 실행 전부 일치.
3. 계약: 이름표·차단 시 무효과·미연결 거부·문자열/행 수 범위·NUL 뒤 문자열 제외.
4. 계약: 설명 줄 나눔 경계(길이 8/9·위치 1·여러 공백)와 팝업 비트의 중간 상태(만드는 동안 꺼짐, 표식 직전 모두 켜짐). 원본 관측이 가리지 못한 경계라 변형 확인 뒤 보강했다.

**변형 확인:** 27가지(분할 길이/위치·제외·언어 3·오프셋 상수·targetWidth 보정·x/y 상수·World 범위·행/마나/설명 상한·팝업 비트·표식 명령·선택 간격·보유 판정·정렬 제거·
신전 상품/group 10/flags2 제외·줄 계산·머리 문자열·아이콘 폭·차단 검사·선택 표식·World 아이콘·NUL 절단)을 넣어 시험이 모두 실패함을 확인했다.
처음에 살아남은 3개(길이 8 경계·위치 1 경계·팝업 비트 해제)를 4번 시험으로 잡았다.

`webpj/tests/fixtures/manifest.json`에 `knowledge-menu`(25관측)·`knowledge-world-menu`(50관측) 항목(원본 10.78)을 추가했다.

## 남은 경계

- cpppj `KnowledgeWorldMenu_Native21NamesAndQueue`의 `knowledge-world-names-x86.tsv`(21행: 원문 이름→명령/토큰/큐)는 `GameMenuCommands`·`QueueScriptCommand`가 있어야 같은 경로로 대조할 수 있어
  `GameMenuCommands` 단위에서 한다. `KnowledgeMenu_ActualScriptCommandQueue`도 같다.
- 실제 Gump 메뉴 저장소·번역·기술 보유 조회와 `ThemeTechnologyCommand` 연결은 아직 없다(PORT_MAP `port`).
