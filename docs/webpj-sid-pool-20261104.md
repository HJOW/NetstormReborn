# webpj 실제 SID 풀 — WB04-A

2026-11-04, HJOW-Athlon. WB03-AB 저장 객체 로더가 요구하는 실제 월드 기반 중 **원본10.78의 SID 풀**을 옮겼다. 저장 객체 표시·M2·미션 시작과 WB04 전체는 아직 미완료다. 화면 변경은 없다.

## 1. 근거와 동기화 범위

AGENTS.md·공통/dotnetpj/webpj 인계와 Git 상태를 다시 확인하고 `git fetch`했다. 작업 시작 트리는 `4fa7e89cf7307291a4a764b74733761f6cc1e5c0`이다. 현재 PC는 마지막 전체 정밀 디컴파일 PC인 **HJOW-Athlon / 2026-11-03**과 일치한다. 디컴파일 산출물이나 dotnetpj 구현을 이식 근거로 사용하지 않았다.

근거는 cpppj `src/o/SidPool.cpp/.h`, `tests/SidTests.cpp`, [SID 복원 기록](exe/cpp-sid-reconstruction.md), 읽기 전용 원본 실행 관측 `tests/fixtures/sid-x86.tsv`다. 본체의 마지막 수정은 `6035f810`, 헤더는 `b7065914`다. WB03-AB 기준 `be8f80f9` 이후 헤더의 RawGraph friend 추가는 풀 알고리즘 변경이 아니다. 전체 cpppj HEAD를 이식 완료한 것으로 처리하지 않는다.

공간 해시 `SquidHash`와 실제 `RawSquidFinder`는 다음에 연결할 수 있음을 확인했다. `SquidFactory`의 Take/현재 TYPE/Unpop은 2026-11-04의 `98651032`·`11c86cdd`에서 바뀌었으므로 후속 생성자 이식 때 최신 본체를 다시 대조해야 한다.

| 원본10.78 | 웹 API | 범위 |
| --- | --- | --- |
| `004abb10` | `SidPool.reset` | 같은 풀 초기화 / 양수 크기 재확보, 삭제 기록 보존 |
| `004af1d0` | `allocate` | 서버·클라이언트·예측 번호 할당 |
| `004abf60` | `release` | void 반납·타입 보존·FIFO·삭제 기록 |
| `004abe00` | `rebuildServer` | 번호순 서버 free 목록 / 카운터 누적 |
| `004952d0`의 기록 정리 | `clearDeletionHistory` | other→client의80DWORD만 정리; 상위 월드 초기화 부모는 미이식 |

## 2. 저장소와 사용 계약

신규 파일은 `webpj/www/core/sid-pool.js`와 `webpj/tests/sid-pool.test.mjs`다. 브라우저/DOM/Canvas에 의존하지 않는 ES 모듈이다.

```javascript
// 실제 세션과 같은 서버 권한 소유자를 빌린다.
const serverOwner = { value: true };
// 원본50바이트 슬롯32768개를 확보한다.
const pool = new SidPool(32768, { serverOwner });
// 일반 서버 영역에서 번호 하나를 할당한다. 파생 생성자/Pop은 후속 연결이다.
const sid = pool.allocate();
```

- 판본은 읽기 전용 `edition === 'patch1078'`이다. 슬롯50바이트·일반 첫 번호5·서버 첫 번호15000·예측 머리23001을 같은 raw 바이트로 보유한다. 크기는23003~65535다. 원본에 없는 세대 비트나 별도 표시 객체 저장소를 만들지 않았다.
- `layout`·`capacity`·`freeCount`·`predictableCursor`·`isServer`는 getter다. `serverOwner={value:boolean}`를 지정하면 각 원본 분기 시점에 현재 값을 읽는다. 미지정 독립 풀만 생성자 `server` 입력을 사용한다.
- `slot(sid)`·`bytes()`·`deletions(client)`는 외부 쓰기로 풀을 바꾸지 못하도록 **시점 사본**을 반환한다. C++의 const span과 달리 후속 변경을 자동 반영하지 않으므로 효과/callback 뒤 현재 필드가 필요하면 반드시 다시 조회한다.
- `allocated(sid)`만 같은 현재 슬롯의 쓰기 view를 빌려준다. free/예약 슬롯은 거부한다. `reset(0/음수)`는 같은 버퍼를 초기화하고 양수 `reset(capacity)`는 새 버퍼를 확보한다. 양수 재확보 뒤 이전 view로 현재 풀을 수정할 수 없으며 다시 빌려야 한다. 실제 세션의 대여/Process/UI 정리는 이 모듈이 대신하지 않는다.
- `firstFree(client)`·`tail(client)`은 원본 목록 머리/예약 꼬리를 읽는다. `deletions(client)`는 해당 영역의 최신20개 `{sid,type}`를 반환한다.

WB03-AB `FortObjectLoad`가 요구하는 판본/`slot`/`allocated` 인터페이스와 맞지만 **이번에는 로더나 화면에 연결하지 않았다**. 생성·finder·Pop·내용물·소유자 마감의 필수 장치를 빈 성공 함수로 채우지 않는다.

## 3. 보존한 전이와 보호 경계

일반 할당은 마지막 free 슬롯을 예약하며 반납 번호는 FIFO 꼬리에 붙는다. flags의 bit0 예측이 bit1 클라이언트보다 우선한다. 예측 번호는 커서로 진행하며 서버일 때 다음 free 번호를 확인하고 예측 머리도 갱신한다. 반납해도 예측 커서는 돌아가지 않는다.

반납은 최근 삭제 기록→전체 슬롯 지움/이전 타입 보존→필요한 FIFO 연결→상태 순서다. 서버 예측 반납은 free+dead이지만 카운터를 더하지 않는다. 비서버에서 서버/예측 번호 반납은 free만 남기고 카운터를 더하지 않는다. 서버 목록 재구성도 원본처럼 기존 freeCount에 발견한 개수를 더한다. 카운터 연산은 DWORD 감김을 명시한다.

JS의 분수/문자열/NaN/폭 밖 입력은 첫 쓰기 전에 거부한다. 원본 assert 이후의 잘못된 접근, CRT malloc 실패, 원본 소진 UI·다리 파괴/재시도는 재현하지 않는다. 이 범위는 cpppj의 보호 경계와 같다. CD10.72, Take/파생 생성자, 가상 동작, SID 강건성 진단 부모, 상위 월드 reset/실패 좌표 초기화는 미이식이다.

## 4. 검증 결과

환경: Windows / HJOW-Athlon, Node v24.21.0. 원본/클론 EXE·analyzeManager·브라우저는 실행하지 않았다.

| 검사 | 결과 |
| --- | --- |
| 신규 SID 집중 검사 | **9/9 통과**, 실패·건너뜀0 (약4.49초) |
| 원본10.78 관측 재생 | **8세션·1584전이**, 반환 SID·카운터·머리/꼬리와 풀 전체/삭제 이력 Adler-32의9개 열 일치 |
| 명시 입력 payload | 원본 관측기에 제공한 Fill **768개** 재생; 독립 실행 전이 수에 합산하지 않음 |
| 경계 검사 | 일반 소진/예약 꼬리/FIFO·예측 우선/소진·재구성 누적·영역별 최신20개·payload 제거·타입 보존 |
| 공유 상태/수명 | 현재 서버 권한 전환,0/음수 버퍼 재사용·양수 재확보, 이력 보존/단독 초기화, 읽기 사본 격리, 오류 전 상태 보존 |
| 전체 `npm --prefix webpj test` | **279/279 통과**, 실패·건너뜀0 (31.32초) |
| 정적 확인 | `node --check webpj/www/core/sid-pool.js`, `git diff --check` 통과 |

fixture SHA-256은 `b45b5726f2860a762fd0fa53fa85949f941baf28e836570b43dfd7a2c9e40eb4`다. `sid-x86.tsv`는 두 판본16세션·3168전이를 담지만 이번에는 originals8세션·1584전이만 이식/검증했다. fixture가 없는 환경에서는 원본 대조1개만 건너뛰며 나머지8개 경계 검사는 계속 실행한다.

Adler-32 일치는 원본과 같은 전체 저장소를 대상으로 한 체크섬 대조이며 모든 바이트의 직접 비교나 암호학적 동일성 증명은 아니다. 별도 반납 검사에서는 타입·상태·next 외의50바이트 payload가 실제로 지워지는지 직접 검사한다. 기대값을 JS 결과로 재생성하거나 원본 fixture를 바꾸지 않았다. 이전 단위의 글꼴 관련 실패2개는 이번 전체 실행에서는 통과했으며 이 작업에서 자산·글꼴·매니페스트를 변경하지 않았다.

## 5. 다음 시작점

**다음은 같은 풀에 결합할 `SquidHash`→`RawSquidFinder`다.** 각각 기존 `hash-x86.tsv`(초기화4·주소807·ObjectLevel605)와 `rawfinder-x86.tsv`(10.78 Find584)로 원본 순서/현재 값 재조회를 검증한다. 이후 최신 생성자/Take·Pop·내용물·소유자 마감을 같은 저장소에 모은 뒤 WB03-AB 로더를 연결한다.

실제 객체 표시·M2·WB04 전체·WB05 미션 시작·L2~L5는 미완료다. `edgeFarm`, 가상 Draw/현재 프레임·소유자 색/신전 원소·그림자 표·배경/카메라·덱/미니맵·미션 선택 연결도 기존 대기로 유지한다.

AGENTS.md·보호 원본/CD·cpppj·dotnetpj·공통 인계·README·자산/매니페스트·사용자 Playwright 설정은 수정하지 않았다. 이번 모듈/검사·상세 기록·webpj 인계만 로컬 커밋하고 원격 push는 하지 않는다. 브라우저 검증은 UI 연결이 없는 이번 단위에서 실행하지 않았으며 기존 e2e 통과를 새 SID 세션 연결 검증으로 합산하지 않는다.
