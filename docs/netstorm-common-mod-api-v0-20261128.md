# Netstorm 공통 모드 API v0.1.0

2026-11-28 사용자 결정으로 **webpj가 명세와 기준 구현을 주도**한다. DN 구현은 사용자가 재개할 때까지 중지하며, 이 명세를 QuickJS 바인딩의 입력으로 쓴다. cpppj는 이번 모딩 대상에 넣지 않는다. 이 문서와 [기계 판독 계약](../shared/modding/v0/contract.json), [선언 스키마](../shared/modding/v0/manifest.schema.json)가 MOD-1의 확정 산출물이다. 구현 완료와 엔진 간 적합성 통과는 각각 별도 기록한다.

## 1. 범위와 버전

이벤트를 받아 상태를 읽고 일반 플레이 입력을 예약하는 단일 플레이 모드다. 원래 게임 로직·난수열·입력 처리기는 호스트가 소유한다. 메모리 주소, SID, Raw 객체, 쓰기 가능한 게임 참조, 임의 원본 명령 번호를 공개하지 않는다. 조회는 새 값 사본이며 모드가 바꿔도 게임에 반영되지 않는다.

공개 객체 이름은 `netstorm`, 클래스가 필요하면 `ModRuntime`·`ModHost`, 메서드는 camelCase다. 공개 객체는 기존 내부 클래스의 이름을 일괄 변경하는 방식으로 만들지 않는다. `0.x`에서 minor 변경은 호환성을 깨뜨릴 수 있으므로 현재 `apiVersion`은 정확히 `0.1.0`만 받는다. 호환 추가는 patch, 삭제·의미/순서 변경은 minor를 올린다. 게임 버전 11.01과 독립이다. v0에 없는 전역/기능은 호환 API가 아니다.

v0 조회 범위는 현재 미션·플레이어·단일 대표 선택이다. 전체 객체 검색, 객체 식별자 기반 명령, 직접 피해/SP/기술 변경, 모드 UI/파일/네트워크, 편집기, 멀티플레이는 추가 버전에서 설계할 사항이다. 단일 대표 선택의 필드는 식별자가 아니므로 수명 지난 대상을 명령할 수 없다. 다중 선택·위치 이동·생산/배치는 원래 UI와 아래 입력 명령을 쓴다.

## 2. 패키지·위치·로드

웹은 사용자가 파일 선택기로 고른 UTF-8 `.nsmod.json` 한 파일을 읽는다. 형식은 `{format:"netstorm-mod-package",formatVersion:1,manifest,modules}`이며 다른 최상위 키는 거절한다. `modules`는 정규화된 상대 경로에서 UTF-8 ES 모듈 원문으로 가는 사전이다. [실행 가능한 관찰 예제](../shared/modding/v0/examples/session-observer.nsmod.json)를 기준으로 한다. 선언은 스키마의 모든 조건을 만족해야 한다.

경로는 ASCII 영숫자/`_`/`-`인 디렉터리와 `.js` 파일만 허용한다. 절대 경로·역슬래시·빈 구간·`.`/`..`·URL·인코딩 경로·쿼리·해시는 거절한다. 모듈은 패키지 내 `./` 또는 `../` 상대 **정적** import/export만 허용하며, 정규화 결과가 패키지 밖이면 거절한다. Windows 예약 이름·대소문자만 다른 파일 경로·중복 JSON 키도 양쪽에서 거절한다. bare import·동적 import·외부 URL·import assertions는 거절한다. 순환 정적 ES 모듈은 허용한다. 코드 텍스트의 단순 문자열 치환으로 import를 판별하지 않고 구문 분석한다.

엔트리는 `export default function registerMod(netstorm) { ... }`다. 등록·핸들러는 동기로 종료해야 한다. Promise/thenable 반환과 비동기 작업은 지원하지 않는다. v0 구문 검증은 async 함수/화살표·await·import.meta도 거절한다. 호스트가 검증한 자체 모듈만 로드하고 `eval`·Function·Wasm 추가 컴파일을 허용하지 않는다.

DN의 디렉터리형 표현은 Windows `%USERPROFILE%\Saved Games\Netstorm\Mods\<id>\`, Linux `$HOME/.netstorm/Mods/<id>/` 안에 `manifest.json`과 같은 상대 모듈들을 둔다. 디렉터리 열거·심볼릭 링크/재분석 지점·실제 경로 탈출 검증은 DN 호스트가 담당하며 모드에는 파일 기능을 주지 않는다. 웹 패키지와 DN 디렉터리를 정규화한 manifest+modules 내용은 같아야 한다.

설치는 실행과 분리한다. 새 설치/교체는 항상 꺼짐이며 같은 id를 조용히 덮어쓰지 않는다. 켜짐 선택은 명시적으로 저장한다. 웹 저장소는 `netstorm.mods.v0`이고 용량/접근 오류 시 기존 정상 자료를 보존하고 UI에 실패를 표시한다. 한 키의 `{version:1,mods:[{package,enabled,lastError}]}`로 보관하며 package는 정규화한 패키지 JSON 문자열이다. 다른 탭이 마지막 읽기 뒤 값을 바꾸면 재읽기 전 쓰기를 거절한다. 손상 저장소는 자동으로 비우거나 덮어쓰지 않고 오류를 표시한다. Options의 관리 화면에서 목록·가져오기·켜기/끄기·삭제·마지막 오류를 제공한다. 미션 중 변경은 다음 미션부터 적용한다. Reset All은 이 키도 지운 뒤 기존 재로딩 절차를 따른다. 원본 대조/고정 입력 fixture 실행은 강제로 모두 꺼짐이다.

## 3. 공개 이름과 값

`netstorm`과 하위 객체는 동결한다. 조회값은 JSON 가능한 유한 숫자·문자열·불리언·null·배열·평범한 객체만 담는다. NaN/Infinity/BigInt/함수/접근자/순환 참조는 경계를 넘지 못한다. 문자열 길이는 별도 표기가 없으면 JS UTF-16 code unit 수이며 파일·메시지 한도는 UTF-8 바이트다.

| 이름 | 인수/결과·의미 |
| --- | --- |
| `apiVersion`, `modId` | 읽기 전용 문자열 |
| `on(name, handler)` | 알려진 사건과 동기 함수 등록, 양의 정수 구독 번호 반환. 총 32개 이하. 등록 순서로 호출 |
| `off(subscription)` | 자기 구독을 제거했으면 true, 이미 없으면 false. 유효한 양의 정수 필요 |
| `getState()` | `{sessionId,mission,gameSeconds,localOwner,paused,logicalSize:{width,height},mapRect:{left,top,right,bottom}}`. 세션이 없으면 sessionId/mission/localOwner=null, gameSeconds=0. sessionId는 실행 중 증가하는 문자열 `session-1` 등이며 원본 SID와 무관 |
| `getPlayers()` | owner 오름차순 `{owner,stormPower,allies}` 배열. owner=1..8, allies는 해당 owner와 동맹인 owner의 정렬 배열(자신 제외). 로그인 참가자 또는 현재 AI brain이 있는 참가자를 포함한다. 반납 뒤 남는 AI 표시 bit만으로 포함하지 않는다. 비참가자는 제외. 세션 없으면 `[]`. 자원은 정수이며 조회로 검산/재시드하지 않음 |
| `getSelection()` | `{typeId,owner,health}` 또는 null. 현재 대표 선택이 살아 있는 자산일 때만 반환. typeId=원본 TYPE 번호, owner=0..8, health=현재 정수 HP. SID/주소/원문 비트는 미공개 |
| `randomInt(limit)` | limit=1..2^32-1, `[0,limit)` 정수. §5의 모드 전용 난수만 전진 |
| `log(level,text)` | level=info/warn/error, text 문자열 ≤1024자. 사건당 8회. 플랫폼 로그에 modId/사건 번호를 붙임. 게임 채팅으로 보내지 않음 |
| `commands` | 아래 동결 객체. 명령당 현재 사건 내 양의 정수 번호를 반환하며 결과는 뒤 사건으로 통지 |

모든 조회와 명령은 등록 또는 사건 콜백 실행 중에만 호출할 수 있다. 등록 시 조회는 미션 없는 초기 상태이며 명령은 `noSession`이다. 저장해 둔 API를 비동기 작업에서 호출하면 `permissionDenied`다. 핸들러 목록은 사건 시작 때 고정하여, 콜백에서 등록한 것은 다음 사건부터 받고 제거는 이후 사건부터 적용한다.

| 명령 | 인수·원래 입력과의 대응 |
| --- | --- |
| `click(x,y,button='left',modifiers={})` | 현재 논리 화면 안의 정수 좌표. button=left/right. modifiers는 shift/control 불리언만 허용. 해당 위치의 버튼 누름/뗌을 원래 큐에 원자적으로 추가. 같은 위치를 각 사건에 보존하며 OS/DOM 합성 입력을 공개하지 않음 |
| `pressKey(key,modifiers={})` | 계약 JSON keyNames의 정확한 이름. 문자 키는 원래 문자 누름/뗌, 방향키는 원래 가상 키 누름/뗌. 반복·임의 명령 번호·OS 키 없음 |
| `selectPriest()` | `pressKey('p')`와 동일 |
| `nextUnit(add=false)` | `pressKey('n',{shift:add})`와 동일 |
| `produceBridge(slot)` | 정수0..5 → q/w/a/s/z/x와 동일. 현재 덱 항목 취득이며 SP나 배치를 직접 변경하지 않음 |
| `repeatProduction()` | `pressKey('d')`와 동일 |
| `rotatePiece()` | `pressKey('c')`와 동일. 회전 방향 옵션을 원래 처리기가 적용 |

클릭을 포함한 명령은 원래 UI 상태·기술·SP·배치 판정을 따른다. 성공 결과는 **입력 큐 수용**을 뜻하며 생산/이동/건설 성공을 보장하지 않는다. 게임 영역 클릭은 mapRect로 판별할 수 있다. 일시 정지·브리핑/모달·채팅 입력·미완료 로그인에서는 `inputBlocked`로 거절하여 안내 창을 자동으로 넘기지 않는다. 화면 크기/세션이 바뀐 명령은 새 상태로 재검증한다.

## 4. 사건·순서·수명

같은 호스트 경계의 효과는 사건 종류(시작 → 직전 명령 결과 → tick → 종료) → 모드 id의 ASCII 코드 → 콜백 등록 → 호출 순서로 확정한다. sequence는 모드마다 독립이므로 서로 다른 모드의 번호를 섞어 정렬하지 않는다. 일시 정지나 호스트 대기 시간을 게임 경과로 세지 않는다. 모든 모드는 같은 경계의 상태 사본을 읽는다. 해당 사건에서 요청한 입력은 **다음 원래 입력 단계 직전**에 추가하며 기존 사람/게임 입력을 추월하거나 버리지 않는다.

| 사건 | 추가 필드와 시점 |
| --- | --- |
| `sessionStart` | `{sessionId,mission,gameSeconds}`. 실제 Session Ready/미션 시작 준비 완료 뒤 첫 경계. 모드 인스턴스/구독/RNG를 미션마다 새로 만든 후 통지 |
| `tick` | `{sessionId,gameSeconds}`. 시작 시각으로부터 처음1게임초 뒤, 이후 1초 간격. 프레임이 여러 간격을 넘으면 현재 상태로 **한 번만** 통지하고 다음 기한을 현재 시각+1로 정함. 매 유닛/매 표시 프레임 호출 금지 |
| `commandResult` | `{sessionId,results:[{eventSequence,commandId,status,code}]}`. eventSequence는 요청을 만든 사건 번호다. 다음 입력 단계에서 확정한 모드별 결과를 다음 모드 경계에 한 번 통지(최대16개). 요청 사건/명령 번호의 오름차순이며 중복은 거절한다. status=queued/rejected, queued의 code=null. 자체적으로 매초 tick을 발생시키지 않음 |
| `sessionEnd` | `{sessionId,mission,gameSeconds,reason}`. reason=leave/restart/replace/destroy. 마지막 사본으로 한 번 통지하고 명령은 거절. 남은 입력 요청을 staleSession으로 폐기한 뒤 컨텍스트·포트·구독·난수·타이머·강한 Session 참조를 반납 |

모든 사건은 `{name,sequence,...}`이며 sequence는 **각 모드 인스턴스의 미션 안에서** 1부터 정확히 1씩 증가한다. 결과를 받지 않은 다른 모드의 번호를 건너뛰지 않는다. gameSeconds는 이전 경계보다 작을 수 없다. 결과 사건 때문에 다시 생성되는 결과는 다음 경계로 미루어 한 경계에서 무한 재진입하지 않는다. 종료 뒤 늦게 도착한 메시지는 세션/인스턴스 번호로 폐기한다. 한 결과 사건에는 명령 결과를 최대16개 담고 원래 요청 사건 번호도 보존한다. 호스트는 한 경계에서 모드별 commandResult를 최대 한 번, tick을 최대 한 번 호출한다. 각각 새 명령8개 이하를 만들므로 다음 경계의 결과도 최대16개에 머물며 결과 사건의 자기 증폭을 막는다.

웹 Worker는 비동기이므로 미완료 사건을 둔 채 다음 게임 입력/시계/Kernel을 진행하지 않는다. 호스트가 대기 동안 게임 일시 정지를 유지하고 완료 뒤 원래 다음 프레임에서 재개한다. 이 장벽이 있어야 CPU/메시지 도착 순서가 명령을 넣는 프레임을 바꾸지 않는다. 화면/관리 UI는 계속 응답해야 한다. DN은 같은 사건 경계를 동기로 처리할 수 있으나 명령을 즉시 원본 객체에 실행해서는 안 된다.

## 5. 결정론과 수학

게임 시간은 사건의 `gameSeconds`만 사용한다. Date·performance·호스트 타이머·Math.random·crypto 난수에 의존하는 모드는 계약 위반이다. 실행기는 이 장치들을 노출하지 않거나 명시적으로 거절한다. 모드 지연은 게임의 난수 소비와 시계를 전진시키지 않는다.

모드 전용 RNG는 게임 초기 seed(uint32)의 **십진 문자열** + LF + modId의 UTF-8을 FNV-1a32(offset2166136261, prime16777619, 각 단계 uint32 곱)로 해시해 초기화한다. 상태0은 `0x6d2b79f5`로 바꾼다. 매 호출마다 `s ^= s << 13; s ^= s >>> 17; s ^= s << 5`를 uint32로 보존하고 `s % limit`를 반환한다(편향 제거 재시도 없음). 원래 게임 RNG를 추가 소비하지 않으며 다른 모드의 호출 수에 영향받지 않는다. 최초 미션 seed를 복사해서 쓰고 현 시각의 변하는 RNG 상태를 seed로 재사용하지 않는다.

정수·비트 연산·IEEE754 기본 연산·Math.imul/fround/abs/min/max/floor/ceil/trunc/round/sqrt는 JS 의미를 따른다. Math.sin/cos/atan2/pow의 **double 결과는 WB의 V8 기준**으로 하고 DN MOD-3에서 같은 fdlibm 경로를 주입한다. 단순 float32 변환이나 허용 오차를 비트 일치의 대체로 쓰지 않는다. 수학 격자와 ±0/경계 입력을 공통 적합성 자료에 포함한다. 나머지 초월함수의 엔진 간 비트 일치는 v0에서 보장하지 않으며 재현 명령 판단에 쓰면 안 된다. 이는 DN 실행 검증이 완료되었다는 뜻이 아니다.

## 6. 오류와 자원·격리

검증 실패는 `ModError`의 `{name:'ModError',code,message}`로 전달한다. code 목록은 계약 JSON과 같다. 인수/버전/경로/권한/한도는 첫 효과 전에 검사한다. 전체 큐 여유가 명령의 두 사건보다 적으면 `queueFull`이며 누름만 넣지 않는다. 여러 명령 중 앞서 수용된 것은 뒤 실패로 되돌리지 않는다. 원본 고리 큐의 39개 유효 용량/소비 순서를 바꾸지 않는다.

선언·구문·로드·등록 실패, 처리되지 않은 예외/Promise, 무한 실행/시간 초과는 **해당 모드만 이번 실행에서 비활성화**하고 id·오류·사건·가능한 스택을 기록한다. 그 사건에서 해당 모드가 만든 아직 미반영 명령/로그는 폐기한다. 다른 모드는 계속한다. 재시작 때 자동으로 오류 모드를 반복 실행하지 않도록 저장된 활성 상태도 끈다. 저장 실패는 별도로 기록하고 메모리에서 먼저 끈다.

패키지 최대1MiB, 모듈 하나256KiB/총32개, 설치8개, 구독32개, 사건당 명령8개·로그8개, 명령 결과 사건은 최대16개 항목, 경계 메시지64KiB, 등록/사건 한 번1000ms다. 시간 제한은 복잡도/지연에 따른 안전 중단 장치이며 같은 입력의 시간 초과 여부까지 엔진 간 결정론으로 주장하지 않는다. 전송 크기·행 수·명령 개수를 호스트도 다시 검사한다.

DN은 모드당 QuickJS runtime을 분리하고 quickjs-libc/std/os/파일/네트워크/프로세스를 등록하지 않는다. 메모리16MiB·스택256KiB·평가1000ms는 CPP `ScriptEngineLimits`와 같다. 웹은 불투명 origin iframe(`sandbox=allow-scripts`) 안에서 전용 Worker를 만들고 CSP로 외부 연결/외부 스크립트를 막는다. 추가 Worker 생성 capability도 모드에 주지 않는다. 모드 코드를 메인 페이지/iframe DOM에서 실행하지 않고, 검증된 메시지 전용 포트만 준다. 초과/비정상 종료는 Worker와 iframe을 폐기한다. 네트워크 함수 이름을 지우는 것만으로 격리를 대신하지 않는다.

**플랫폼 보장 차이:** 표준 브라우저 Worker에는 QuickJS와 같은 JS 힙/네이티브 스택 바이트 상한 설정 API가 없다. 따라서 웹은 시간·패키지·메시지·호스트 객체 한도를 적용하고 `hardMemoryLimit=false`, `hardStackLimit=false`로 기록한다. DN의 16MiB를 웹에서도 강제한다고 주장하지 않는다. 웹 VM 내부 OOM까지 페이지 생존을 보장하는 계약은 아니다. 공통 적합성은 이 차이를 프로필로 확인하고 숨기지 않는다.

격리 설계 근거는 [HTML sandbox의 불투명 origin 규칙](https://html.spec.whatwg.org/multipage/document-sequences.html)과 [CSP의 로컬 URL 정책 상속](https://www.w3.org/TR/CSP/#security-inherit-csp)이다. blob Worker의 정책 상속과 connect-src/worker-src 제약은 [W3C Worker 정책](https://www.w3.org/TR/CSP11/#processing-model-workers)에 명시되어 있다. 실제 기준 구현은 이 규칙을 실제 브라우저의 탈출/무한 루프 검사로 검증한 뒤 완료로 표시한다.

## 7. 구현 연결과 완료 관문

| 계약 | WB 연결 위치 | DN 재개 후 연결 |
| --- | --- | --- |
| 읽기 전용 상태 | Netstorm의 현재 World/Guide/GameClock/Layout, Session.localOwner/player/SP/alliance, 대표 선택의 자산 필드만 복사 | ClientHostGame/HostSession의 같은 공개 값 복사 |
| 입력 | InputQueue.tryPushBatch → 기존 CampaignUserInput/생산/회전 분배. 기존 사람 입력 앞에 삽입 금지 | 기존 InputEventQueue/Raw 입력 분배. 스크립트에서 Core 메서드 직접 호출 금지 |
| 사건 | Ready 뒤 sessionStart, 원래 루프 경계의 게임 시간 tick, 종료 전 sessionEnd, 다음 입력 전 결과 확정 | R6 실제 Session 결선이 된 동일 경계에 바인딩 |
| 실행/로드 | native JS 격리 실행·파일 선택·localStorage·기본 꺼짐 | MOD-2 엔진 → MOD-3 로더/수학 → MOD-4 바인딩 → MOD-5 GUI/LAN |

진행: E1470 명세·E1471 순수API/공통 입력·E1472 실제 사본조회/큐·E1473 패키지/보관·E1474 분류등록·E1475/1476 두 전체를 완료했다. E1477은 AI 참가자 조회와 모드별 sequence/유계 결과16개를 보강한다. 남은 구현은 **격리 실행·실제 사건/시계 장벽·Options 관리·브라우저 적합성**이다. 각 단위는 인계 갱신 후 커밋한다. 새 JS 소스는 한국어 설명 주석·ESLint 선행 빌드를 따른다.

공통 적합성에는 이름 정확 일치·조회 사본·구독 순서·독립 RNG golden·입력 FIFO/포화/수명/모달·오류 모드 단독 해제·기본 꺼짐·허용 모듈/경로 탈출·네트워크/DOM/컴파일 차단·무한 루프 종료·호스트 대기 중 게임 시계 보존을 포함한다. **같은 패키지**와 **같은 정규화 상태/사건/입력열**을 WB와 DN에 공급하고 결과 JSON을 비교한다. WB 통과와 DN 미실행을 분리 기록하며 DN 중지 중에는 양쪽 일치를 완료로 표시하지 않는다.

실제 플레이는 기존 사용자 제한을 유지한다. 아군 AI를 활성화하며 1-3·1-4·1-6 및 그 이후의 승리/포획/희생/특정 적 공격은 사용자에게 이월한다. 모든 미션 진입/안내 확인과 단순 대기/생산/건설은 가능하다. 모딩 관문 때문에 이월 승리 검사를 다시 시작하지 않는다.
