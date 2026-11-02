# webpj 저장 오브젝트 적용 로더 — WB03-AB

2026-11-02, HJOW-Athlon. M2 4단계(b-2)의 선행 작업으로 **읽은 Fort 레코드를 현재 월드에 적용하는 로더 모듈**을 옮겼다. 이번에는 화면 변화가 없으며 미션 시작/저장 오브젝트 표시가 완성된 것은 아니다.

## 1. 근거와 범위

AGENTS.md·공통/dotnetpj/webpj 인계와 Git 변경을 확인하고 fetch했다. 현재 PC는 마지막 전체 정밀 디컴파일 호스트(HJOW-Athlon/2026-10-23)와 같다. 디컴파일 산출물이나 dotnetpj 구현을 근거로 사용하지 않았다.

검토한 트리는 `be8f80f9`다. 새 웹 파일은 `www/core/fort-object-load.js`와 `tests/fort-object-load.test.mjs`다. 근거는 cpppj `o/RawFortObjectLoad.cpp/.h`, `tests/FortObjectLoadTests.cpp`, 읽기 전용 원본 관측 `tests/fixtures/fortobjectload-x86.tsv`다. HP 장치의 검사 입력을 확인하려고 `SquidReward.cpp/.h`도 읽었다. cpppj의 실제 월드 연결(`RawFortWorldLoadBinding.cpp`, `RawCampaignFactoryDisplay.cpp`)을 확인했으며, 가상 Draw/색표 소스도 살펴봤지만 이번 단위에 이식하지 않았다.

| 원본10.78 | 옮긴 API | 동작 |
| --- | --- | --- |
| `004bdc60` | `FortObjectLoad.object` | 저장 객체 하나의 초기 필드→소유자/HP/프레임→Pop/내용물→Fort 후처리 |
| `004bd130` | `contents` | 실제 현재 부모/자식 타입으로 내용물 부착과 재귀 |
| `004be160` | `chunk` | 청크 원점×16와 파일 객체 순서 |
| `004be610` | `load`·`section` | Terr 번호순→허용 시 Chaff→소유자 마감·공유 로딩 깊이 |

10.78 **값형 Fort 경로**만 옮겼다. CD판, 원시 스트림을 읽으면서 즉시 실행하는 `ObjectStream/LoadStream`, 실제 생성자/Pop/finder/소유자/내용물 장치의 몸체와 전체 미션 부모는 이번 범위 밖이다. 기존 `parseFort`는 수정하지 않았다.

AGENTS.md·보호 원본/CD·cpppj·dotnetpj·공통 인계·README·자산/매니페스트·사용자 추가 Playwright 설정/검사는 바꾸지 않았다. 원본/클론 EXE·analyzeManager는 실행하지 않았다. 다른 세션의 dotnetpj 변경은 보존했다.

## 2. 공유 저장소와 필수 장치

`createFortObjectLoadState(versionOwner)`는 `{ value: DWORD }`의 같은 판본 저장소를 getter/setter로 빌린다. 로딩 깊이·scope·타입 전역·마스크·미션/편집/Fort/Chaff 관문은 원본 기본값을 사용한다. `readFullDeck`·`readFortKeepGenus`가 있으면 호출할 때마다 현재 공유 값을 읽고, 없을 때만 독립 입력의 fallback 필드를 사용한다.

`new FortObjectLoad({ pool, finder, types, frames, random, state, hooks })`의 경계는 다음과 같다.

- `pool`: `edition: 'patch1078'`, `slot(sid)`·`allocated(sid)`로 원본50바이트 슬롯을 제공한다. 할당/해제 검사는 풀 소유자가 맡는다.
- `finder`: 같은 `pool` 참조와 `begin(rect)`·`next()`를 제공한다. SID0은 끝이다. 지면 프레임 복사는 일반 finder의 첫 island 부류를 사용한다. 별도 좌표 근사 검색을 넣지 않았다.
- `types`: 실제188개 타입 레코드. 현재 flags1/2·container/content listFlags를 읽는다. 비용/HP 계산은 이 로더에 복제하지 않고 가상 조회 장치가 제공한다.
- `frames`:188개의 `{ codes }` 프레임 표, `random.next(limit)`은 같은 게임 난수를 사용한다. 기존 `LocalRandom`은 동일 난수 몸체라 독립 검사에서 사용하지만 실제 세션 연결에서는 세션의 난수를 빌려야 한다.
- `hooks`: 생성·표면 알림·가상 상태/소유자·HP/개수 조회·Pop·삭제·내용물 생성·청크 지역·Terr 허용·깊이 통지·소유자 마감이 **모두 필수**다. 누락/다른 풀/다른 판본/잘못된 표 크기는 첫 쓰기 전에 거부한다.

이 장치들은 실제 게임 월드가 소유해야 한다. 검사의 입력 장치를 제품의 대체 월드로 사용하거나, 빈 Pop/소유자 마감 등으로 실제 연결이 된 것처럼 처리하지 않는다. 기존 정적 지형 뷰에는 아직 연결하지 않았다.

## 3. 원본 순서와 현재 값 재조회

- 저장 프레임·QA 낮은 BYTE/QB WORD·다리 프레임을 좁은 폭으로 직접 쓴다. QA는 WORD의 높은 BYTE를 보존하며, 수량 지정 뒤 현재 타입이 현재 다리 전역과 같을 때만 표면을 알린다.
- 소유자 저장 마스크, 버전0의 state 처리, version>1의 factory state, 유효 소유자 보정, owner 지움, noIsland 소유자0, 큰 섬 assert를 원래 순서로 수행한다. 현재 타입과 판본/관문은 callback 뒤에도 다시 읽는다.
- 가상 HP 조회를 **먼저 끝낸 뒤** 현재 할당 슬롯을 다시 빌려 DWORD HP를 쓴다. 조회가 풀을 재할당할 때 오래된 바이트에 쓰지 않는지 별도 검사했다.
- 지면 프레임 복사에서 패치 DWORD `0xffffffff`는0으로 바뀐다. 랜덤 프레임은 P/P→변환P/P→P/A→P/모든 변형의 첫 후보 집합, 최대256개, 같은 게임 난수1회 소비다. 후보가 없으면 난수를 쓰지 않고0을 저장한다.
- Pop9 뒤의 현재 타입으로 내용물 유무를 결정한다. Local 플래그 변경은 같은 목록의 뒤 형제에게 남으며 중첩 호출에는 값으로 전달한다. `ContentForm` 반환 SID를 새 부모로 사용한다.
- 마지막 Pop/Contents 뒤 FullDeck/유지 마스크를 읽는다. Fort 삭제가 실행되어도 원본처럼 반환 SID가 이미 해제됐을 수 있다.
- 전체 로딩은 각 반복의 현재 지도 크기/지역/선택 정책을 읽고 Terr 뒤 Chaff를 수행한다. 소유자 마감/청크 소진 등 내부 예외에서는 앞선 효과를 남기고 로딩 깊이를 복구한다. 최초 깊이 통지는 원본과 같이 try 밖이다.

## 4. 검증

`node --test webpj/tests/fort-object-load.test.mjs`: **6개 통과, 실패/건너뜀0**.

| 대조/검사 | 결과 |
| --- | --- |
| 원본10.78 독립 실행 관측 | **609입력**(객체573·전체 순회36)의 효과 명령열·모든 생성 root의50바이트·최종 게임 난수·version/depth/scope/삭제 억제값이 저장 관측과 일치 |
| 기존 판독기→새 로더 | 같은 원본 객체 **573건을 다시** packet→Fort Chaff/TypeNames→`parseFort`→로더로 실행해 전체 관측과 일치. 추가 독립573입력으로 합산하지 않음 |
| 계약/오류 | 필수 효과·동일 풀·판본/표 크기·선택 필드 누락, 청크 소진/소유자 마감 예외 뒤 깊이 복구 |
| 현재 저장소 | Pop/Contents 뒤 공유 FullDeck의0/상위 비트/전체 DWORD, 유지 마스크 변경, 판본의 같은 소유자, 가상 HP 조회 뒤 풀 저장소 교체 |

fixture SHA-256: `31fb91aabb4fe3c403e9a9780e0f14f211508cbbce6523cef1b53a5bdf439519` (`cpppj/tests/fixtures/fortobjectload-x86.tsv`). 기대값을 웹 결과로 만들거나 fixture를 수정하지 않았다. fixture가 없는 환경은 두 대조 검사만 건너뛴다. 다른4개 계약/경계 검사는 계속 실행한다.

전체 `npm --prefix webpj test`: **270개 중268개 통과, 기존 실패2·건너뜀0**, 약25.32초. 실패2개는 기존 `fonts/License.txt` 원본 작업 트리CRLF와 웹 복사본LF의 자산/글꼴 비교다. 이번 단위에서 자산/기대값은 바꾸지 않았다.

사용자가 추가한 `be8f80f9`의 Playwright 설정을 그대로 사용해 `npm --prefix webpj run test:e2e`도 실행했다. Chromium 기본 스모크 **1/1 통과**(약3.2초). 페이지 초기화/Netstorm 노출/루트/처리되지 않은 예외 없음 검사이며, 시작 클릭·맵 조작·새 로더의 실제 게임 연결을 검사한 것은 아니다. Node v24.21.0·Windows/HJOW-Athlon 환경이며 다른 브라우저/OS와 실제 세션 L2~L5는 미검증이다.

## 5. 다음 시작점과 완료 경계

읽은 객체를 적용하는 순수 제어 경로가 완료됐으며 **실제 저장 객체 표시·M2·WB04 공유 코어·WB05 세션 시작은 미완료**다. 로더를 실제 화면에 연결하려면 같은 SID 풀/생성자·공간 해시/finder·Pop/내용물/소유자 마감 장치가 필요하다. 따라서 해당 WB04 최소 기반은 M2의 객체 연결에 선행 이식해야 한다. 가짜 성공 장치로 지형 미리보기에 객체를 붙이지 않는다.

다음은 이 필수 월드 기반을 실제 소유자에 모으고, 그 상태에서 가상 Draw/프레임·소유자 색·신전 원소 투표/지역 도장·그림자 표를 연결하는 작업이다. 원시 스트림 로더의 필요 여부는 실제 세션 조립 시 판단한다. 이후 실제 배경/카메라·덱/미니맵·미션 선택 연결과 같은 상태의 원본 캡처 대조를 잇는다. `edgeFarm`도 기존 계획대로 같은 WB04 월드 기반 뒤다.
