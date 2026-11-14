# dotnetpj 0.523 — 사제 월드 선행 Carrier 전처리와 현재 내용물 조회

2026-11-14, HJOW-Athlon. cpppj 기준 커밋은 `824b4ea2d`다. 기존 디컴파일 산출물은 읽거나 변경하지 않았다.

## 조립 검토에서 확인한 선행

실제 사제 월드 `RawPriestWorldBindings`의 core/state/effects 조립을 검토하니 기존 C# Carrier 전처리/내용물 검색기가 현행 cpppj의 Native 경로·공유 권한·현재 checking/복귀 진단·개수 함수·사제 어댑터를 포함하지 않았다. 전체 월드를 이 이전 계약에 맞춰 억지로 연결하지 않고 두 선행 모듈을 먼저 갱신했다. 전체 사제 월드 묶음/캠페인 호스트는 아직 잔여다.

`RawContainedFinder.h/.cpp`를 현행에 맞췄다. 현재 권한은 공유 getter 또는 독립값에서 읽는다. checking은 원문 `RawCheckingFlag`의 전체 현재DWORD이며 값 사본이 같은 장기 owner를 빌린다. 기존 bool 입력4곳은 `new(true)`로 이행했다. PATCH는checking→dead→현재boss→allowDead 순서로 진단을 선택하고, 선택한 장치를 저장해 호출한다. 정상 복귀 뒤 현재 부모head를 처음 읽는다. CD에는PATCH의dead 진단을 추가하지 않는다. Next는반환 전에다음SID를 저장하고 현재 containedTYPE의 전체DWORD를 후보BYTE와 비교한다.

`CountContained`는같은 커서로 종류DWORD·전체maskWORD를 순서대로 판정한다. free/dead/void/contained 추가 필터가 없으며 kind65536을0으로 축소하지 않는다. 이 함수는현행C++소스 계약과 직접 검사로 확인했고 별도Count 원본PE관찰을 새로 생성한 것은 아니다.

`RawCarrierPreDestroy.h/.cpp`는PATCH에서만 Native를 켤 수 있고, Native의Validate는raw/하위typed 사전검사를 앞당기지 않는다. 실제Damageable부모 후 현재boss를읽고 필요할 때만내용물을 조회한다. 독립/CD 경로는순수 선행검사를 유지하며 CD 타입 상한171을 적용한다. 함수 객체를복사하고 같은현재FinderState를 노출한다. 사제어댑터는같은풀의Validate/PreDestroy만 연결하고 보호막 함수는 보존한다.

## 원본 대조와 합성

`carrierpredestroy-x86.tsv`의세판본2880입력을 cpppj 원문에 읽기 전용으로 연결했다. LF정규화 SHA-256은 `e549025d8c54eedddbca3958c4b0def795a0fc08bb215749f8c5626567440bf1`이다. 반환·현재boss·Damageable후효과/사건·다섯raw슬롯 전부를 독립 원본과 비교했다. CD의36BYTE와 C#나머지14BYTE 입력보존은 따로확인했다. 원본 입력은처음부터 모두일치했다.

새 `CarrierCurrentOracleTests` 9검사는 원본 재생3개 외에 다음을 포함한다.

- Native의typed사전검사 생략→Damageable부모→현재공유boss 관문. 잘못된head0xffff도부모가boss를false로바꾼뒤조회하지 않는다.
- 두판본의효과 뒤 공유boss 재조회, PATCHdead진단/CD생략, 정상복귀후 바뀐head, checking0x100·0·명시진단뷰 사본.
- 저장한next/현재TYPE, 모든상태 후보허용, 종류DWORD/전체WORDmask 개수와 미등록 큰TYPE.
- 같은풀/함수사본·CD의Native거부, 실제사제PreDestroy→Carrier어댑터→공급한Damageable기록경계. 중복사제목록을압축하고 검증2회/부모1회를확인한다. Damageable 전체Native몸체/부모사제삭제까지의 검증으로 확대해 주장하지 않는다.

추가 공유권한 검사2개에서합성raw를할당전에써서FIFO의free표식을깨뜨린입력오류가있었다. 원본과같이client할당을먼저끝내고raw를준비하도록고쳤으며제품/golden은변경하지않았다. 이후새9/9검사와실제사제어댑터확장을통과했다. 공유boss대신독립값을읽는변이는9검사중3개가검출했다. 제품은SHA-256 `6e586cb3bf2f302b8cbfa16ae3e6b213b53525e556c5fb03a017c11674ad88d7`까지바이트원복했다. 로컬근거는`extracted/dotnet-carrier-current-0523/`의TRX·변이스크립트/결과다(Git제외). 원복뒤전체비증분Release빌드0경고/0오류·최종관련Core527/527·Client32/32통과(실패0·건너뜀0·각종료0)를확인했다. 전체회귀완주로합산하지않는다. 문서명충돌로임시덮어쓴기존0.508문서는Git원문바이트로복원했고새기록은이별도파일로분리했다.

PORT_MAP은793행(L1 379+port414), 공유fixture275개다. 기존3행을현행기준으로재대조하고Carrier헤더1행을추가했다. 이번모듈의새10차NB항목은없다.

## 보호와 잔여

실제게임/MP를실행하지않았다. 사제포획/희생·특정적공격·시나리오승리재현은생략했다. originals/originalCD·AGENTS.md·README개요·cpppj·webpj는변경하지않았고외부접속코드를추가하지않았다. 실제사제월드묶음/HostSession·일반지역정책과M1전체일치0/34는잔여다. 다음에는 `RawDamageablePreDestroy`의현행Native/현재TYPE 경계 등 남은실제하위계약을확인한뒤묶음조립을계속한다. 사제월드설계에서 확인한 해시0표면의동일저장소·네소비자의권한owner동일성·필수효과검증후후처리등록/반납 규칙도그대로지킨다.
