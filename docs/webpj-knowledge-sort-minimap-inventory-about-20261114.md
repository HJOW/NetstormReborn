# webpj 지식 정렬·미니맵 기하·Inventory 정보 메뉴 KnowledgeSort/MinimapGeometry/InventoryAboutMenu (WB04-E448, 2026-11-14)

RawMenuControls 선행 **KnowledgeSort 105줄·MinimapGeometry 98줄·InventoryAboutMenu 72줄(기준선 `1f00a25a`의 .cpp/.h 줄 수, 합계 275줄, todo→port 3단위)**을 새 모듈 [`www/ui/knowledge-sort.js`](../webpj/www/ui/knowledge-sort.js)·[`www/ui/minimap-geometry.js`](../webpj/www/ui/minimap-geometry.js)·[`www/ui/inventory-about-menu.js`](../webpj/www/ui/inventory-about-menu.js)로 이식했다. 기준선 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약·손 계산 값·독립 기준 구현·실제 `SidPool`로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

세 단위 모두 기준선 뒤 변경이 없다.

## 구현

- **`KnowledgeSort`(00453f70/004e6010)**: 행(`type`/`available`/`inProduction`/`mana`/`description`)의 비교는 두 TYPE 의 flags2 를 우선 순서 7비트(`0x800, 0x8000000, 0x10000, 0x4000000, 0x20000000, 0x40000, 0x20000`)로 보며 한쪽에만 켜진 첫 비트에서 결정(앞쪽만 -1·뒤쪽만 1)하고 둘 다 켜졌으면 분류를 끝내 뒤 비트를 보지 않은 채 설명을 첫 NUL 까지의 부호 없는 ASCII 차이(대문자 A..Z 만 소문자로)로 비교한다. TYPE 이 표 밖이면 RangeError 다. 정렬은 JS `Array.sort` 가 아니라 원본 CRT qsort 를 그대로 따른다(8항목 이하 선택 정렬은 동률을 뒤집고, 큰 구간은 중앙 피벗 3점 정렬·피벗 자기 비교 생략·동률 구간 제거·30쌍 스택). 동률 항목의 상대 순서가 원본과 같아야 하기 때문이다.
- **`MinimapGeometry`(00480270/004803f0/00480440/004805e0/00480660)**: `rebuildMinimap`은 눌림 중이면 아무것도 바꾸지 않고 false, 아니면 카메라 float 비트의 0 방향 절삭 + 지도 영역 원점의 정수 나눗셈을 더해 2 로 나눈 중심, 영역 폭/높이를 칸 배율의 두 배로 나눈 보이는 칸 수, (크기 - 보이는 칸 수)×0.5 의 ftol 을 뺀 후보를 float32 로 좁혀 0..(128-크기)로 제한한 crop 을 쓴다. `minimapTarget`은 (crop + 지역 좌표)×2 를 마지막에 float32 로, `dragMinimap`은 끝점 포함 경계(좌/상 ≤0, 우 ≥폭, 하 ≥높이-1)에서 crop 을 4 씩 옮기고, `clampMinimapAttachment`는 crop+크기가 128 을 넘는 축만 보정(음수 crop 은 그대로), `planMinimapDraw`는 블릿 원점·크기(min(128,크기)의 ftol)·카메라 outline 을 계산한다. x87 절삭(CRT `_ftol`: signed 64비트 절삭의 하위 DWORD, 무효 값은 0)·signed 32비트 감김·`std::min/max`의 NaN 처리 순서를 보존한다.
- **`InventoryAboutMenu`(43d330)**: 내용물 슬롯(하위 16비트, 범위 밖이면 어떤 효과도 하기 전에 RangeError)을 확인하고 새 메뉴 → idle → Form 참조 → 설명(종류 WORD, 0, 소유자, 0xffffffff) → 비용 머리 → 구매 불가 머리 → (편집 중이 아니고 +20 WORD 의 0x20 비트가 있고 +10 종류 바이트가 편집 관문 뒤의 현재 회수 제외 클래스가 아니면) 회수 항목 → Help 항목(NULL 과 빈 문자열 구별) → 루트 부착 → `opengump.wav` 순서다. 회수 비용은 정수 비용을 float32 로 저장한 뒤 제단/다이스는 합 0, 너겟은 전액, 나머지는 1/4 을 0 방향 절삭한다(제단·다이스·너겟 번호는 같은 Client 장기 owner 의 상자 값을 읽는다). 생성자는 10.78 풀과 16개 필수 훅이 없으면 TypeError 다.
- 웹 적응 표기: C++ `invalid_argument`→`TypeError`, `out_of_range`→`RangeError`, `logic_error`→`Error`, `std::optional<std::string>`→`null`/문자열, `const std::uint32_t*` 공유 owner→`{ value }` 상자.

## 검사와 발견

새 시험 파일 3개, **24건 통과**(`knowledge-sort.test.mjs` 8건·`minimap-geometry.test.mjs` 9건·`inventory-about-menu.test.mjs` 7건). KnowledgeSort 는 동률 순서를 손으로 모의 실행했고(동률 2개는 뒤집힘, 3개는 [x2,x3,x1], 8개는 [x2..x8,x1], 9개 동률은 분할 뒤 그대로) 동률이 많은 임의 입력 600회와 정렬/역순/동일 입력에서 **MS CRT qsort 를 인덱스 재귀로 다시 쓴 독립 기준 구현**과 항목마다 같음을 확인했다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류):** 동률이 있는 정렬은 멱등이 아니라(같은 입력에서만 같은 결과) 결정성으로 바꿨고, 폭 0·높이 1 인 미니맵의 드래그는 좌/상 -4 뒤 우/하 +4 가 차례로 적용되어 원래 값이 되며, `128 - 128.5` 의 절삭은 `-0` 이 아닌 0 이고, 기본 인자 구조분해가 `undefined` 를 기본값으로 바꿔 Help 단어 시험을 훅 교체로 고쳤다.

**변형 확인:** 연산자/상수 변형 187개(KnowledgeSort 89·MinimapGeometry 63·InventoryAboutMenu 35) 중 169개를 잡았다. 처음 생존한 항목은 홀수 위치의 설명 차이, 칸 배율 1 의 유효성, 드래그 좌/상 경계(0.5/1 은 경계 안), 플래그 0x01 이 회수 비트가 아님을 시험해 잡았다. 남은 18개는 동등 변형이다: 도달하지 않는 스택 한도(30↔31)·범위 끝 `undefined` 읽기·`std::min/max` 동률·한 원소 구간의 분할 경계 비교(`>`↔`>=`, 한 원소 구간은 정렬 대상이 아님)와 push/continue 조건·`ArrayBuffer` 크기·crop+크기가 정확히 128 일 때의 재대입(값이 같음).

전체 회귀: Node v26.13.0·v22.23.3 각각 **4,647건 통과, 실패 0**(2026-11-14). `node webpj/tools/port-map.mjs`는 984행 오류 0이다. Node v24는 이 PC에 설치되어 있지 않아 검증하지 못했다.

## 남은 경계

- Factory/전체 지식 창 소비자·실제 미니맵 Gump 출력/입력·`InventoryItemGump` 와의 연결은 별도 단위다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
