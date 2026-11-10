# webpj `.type` 정의 해석 마감 — WB04-E267 (2026-11-10, 4-A TypeParser)

3.1절 4-A 의 **TypeParser** 를 마감했다. 웹 `parseTypeDefinition`(`www/assets/type-table.js`)은 WB03-T 에서 cpppj `TypeParser.cpp` 의 문법을 옮겨 실제 116개 자산에 쓰고 있었다. 기준선 판(`git show 1f00a25a:cpppj/src/o/TypeParser.cpp`)과 `AssetTests` 의 두 TypeParser 시험을 한 줄씩 대조해 남은 차이를 찾아 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 대조 결과와 옮긴 것

| cpppj | 웹 상태 |
| --- | --- |
| 토큰(단어·문자열·숫자·문장부호·끝), 이스케이프 없는 문자열, `//` 주석, NUL 절단, 십진 숫자(`1e3`·`.5`·`-.5`·`+2`·`1.`), 유한성 검사, 닫히지 않은 문자열 거부 | 이미 같음(탐침 확인) |
| `typename`·constructor 수식어·`typeflags … ;`·`{ 키 = 값 ; }`(문자열/숫자)·클러스터 이름 분해(첫 글자 대문자, 한 글자면 변형 'P', 십진 번호 전체, 하위 8비트)·알려진 7개 클러스터 비트·`baseframe`/`gumpframe`/`default`/`help` 순서 | 이미 같음(샘플 정의 전부 일치) |
| 클러스터 그림 참조 `images`(파일 이름 + 번호) | 웹은 개수(`imageCount`)만 세어 버렸다 → **`images: [{ file, number }]` 보관**(개수와 같음) |
| `RiftTypeDefinition::Property`/`Number`/`String` | 웹은 `definition.properties` 를 직접 훑었다 → **`typeProperty`/`typeNumber`/`typeString`**(마지막 지정값·ASCII 대소문자 무시·문자형/숫자형 비변환) |
| `Footprint()` | **`typeFootprint`**(foot_x/foot_y 를 0 쪽으로 버린 정수, 정상 정수 범위 밖 거부, foot_y 6 은 8) |
| `FrameTable()` | E266 의 `FrameCodeTable.fromDefinition` |

## 2. 검사 (`tests/type-parser.test.mjs` 6건)

cpppj `TypeParser_PreservesCodesAndSpecialFrameOrder` 의 샘플 정의 전부(이름·수식어·플래그·한글/`;`/`//`/역슬래시 문자열·`COST`/`cost` 중복·발자국 [7, 8]·클러스터 3개의 그림 참조와 코드·플래그 0x7e·특수 프레임 [2, 2, 1, 2]·`FindNumber('A','A',4) == 1`·기본 정의의 [1, 1] 과 [-1, -1]), `TypeParser_RejectsIncompleteOrInvalidDefinitions` 의 5가지 거부(닫히지 않은 문자열·`x=1}`·소문자 클러스터·세미콜론 없음·`1e999`)에 더해 typename 누락·빈 입력·`A0x1`·정수 범위 밖 프레임 번호, 숫자 토큰 다섯 종류·주석·NUL 절단, 속성 마지막 값 우선, 발자국 절삭, 실제 자산 클러스터 전부의 `images` 가 개수와 같음을 확인한다. 모두 첫 실행에 통과했다. 변형 11개(마지막→첫 지정값·대소문자·형 변환·발자국 8 보정/절삭/범위·그림 번호/개수·프레임 번호 범위)를 넣어 보니 전부 탐지했다.

## 3. 영향과 남은 경계

- 기존 소비자(`picture-resolver`·`terrain-tiles` 등)는 `imageCount` 를 그대로 쓰므로 영향이 없다(`picture.test`·`terrain-tiles.test` 통과).
- `Property` 조회 함수는 이후 단위(Config·번역·타입 표 사용처)가 필요할 때 쓴다. `type-flags.js` 의 `isProperty` 루프는 레코드 속성 적용 순서(원문 0049c3b0)를 그대로 보존하기 위해 그대로 둔다.
- 다음은 4-A **Config·ConfigInterface**(original-x86.tsv Config 270행)이다.
