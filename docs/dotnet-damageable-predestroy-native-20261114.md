# dotnetpj 0.524 — 피해 전처리의 현재 Native·사제TYPE·flatSpot 경계

2026-11-14, HJOW-Athlon. cpppj 기준 커밋 `4c29c9e35`의 `RawDamageablePreDestroy.h/.cpp` 전체를 현행화했다. 기존 디컴파일 산출물은 읽거나 변경하지 않았다.

## 구현

`EnableNativeBody`는PATCH에서만 실제 진입을 선택한다. Native의Validate는typed 사전검사를 끼우지 않으며, 직접 `PreDestroyNative`는현재extra→붕괴/폭발·소리→현재지도→현재내용물/사제TYPE→현재boss/선택해방→공통부모 순서를 실행한다. 독립typed/CD는기존선행검사를 유지하고 CD의타입상한171/extra35를 반영했다. 진입extra는한번만 분기하고 효과중 바뀌었다고 재검사하지 않는다. 각소리는효과뒤 현재좌표를 읽는다.

Native지도는Y→X를원문float상수0.9999와넓은정밀도로더하고signed64절삭후lowDWORD를취한다. `uint32(y*256+x)`의감김을 보존하고 결과가도달한flatBYTE만 물리범위를 확인한다. 좌표별0~255관문을 추가하지 않았다. typed는좌표별범위를 먼저검사한다. NaN/Inf·실제CRT표현범위/도달한지도범위밖입력은도달시점에 진단한다.

사제TYPE는같은실제`RawGameAuxState.PriestTypeBox`를대여할수있고 내용물kind 비교마다전체현재DWORD를다시읽는다. 현재공유boss도효과뒤조회한다. 직접Native의대체해방함수는권한/flags0x800관문을통과할때만요구하며, 다른경우에는비어있어도조회하지않는다. 구성시함수객체를복사하고 Carrier어댑터는동일풀의Validate/PreDestroy만교체한다.

## 검증

기존독립원본`damageablepredestroy-x86.tsv`의4896입력을세판본typed로대조했고, PATCH1632입력을Native로추가대조했다. 합계6528실행이며고유관찰은4896개다. 기존PATCH1632검사도유지한다. fixture를다시생성하거나golden을변경하지않았다. LF정규화SHA-256은`a0dd75327aa55f130acabb28d848a86c8f4beee563aadcfa1d2784442d50db8c`이다.

재생장면을판본별extra·CDraw36BYTE에맞추고나머지.NET14BYTE보존을별도로확인했다. 모든행의입력을초기화하므로원본C++처럼장면을반복재사용한다. 이는이전행의기대값을공유하지않으며반환·모든raw/전역·효과순서·선행검사개수를독립관찰과비교한다.

집중13검사(기존PATCH1·새재생4·Native계약8)가처음부터통과했다. Native계약은다음을포함한다.

- `(300,-2)→44`, `(-2,1)→255`, `(9,16777216)→9`의감긴실제지도BYTE와typed의선행거부.
- 붕괴/소리뒤도달한지도범위실패의부분효과, extra9가생략하는NaN/권한/해방읽기.
- 첫소리뒤바뀐공유사제TYPE/저장한next·부모head변경, 큰DWORD를WORD로줄이지않음, 독립999보존.
- 현재boss/flags뒤선택한해방만요구함, 직접대체해방/구성시함수사본, 효과뒤권한변경.
- 실제NativeCarrier→NativeDamageable어댑터와동일풀/정책보존, CD의Native선택거부.

Native지도인덱스의x/y전치변이는13검사중4개(원본Native재생1·감김3)가검출했다. 소스를SHA-256 `6b81b89291a72b273160ef1d5c3a5b7c4407263a6282f071d51d9d79fa525c69`까지바이트원복했다. 로컬근거는`extracted/dotnet-damageable-native-0524/`의TRX·변이스크립트/결과다(Git제외). 원복 뒤 전체 비증분 Release 빌드는 경고0·오류0, 관련 Core539/539·Client32/32가 통과했다(실패0·건너뜀0·각종료0). 전체 회귀 완주로 합산하지 않는다. PORT_MAP은794행(L1 380+port414)이며, 공유fixture는275개로유지한다. 이번모듈에해당하는새10차NB항목은없다.

## 보호와 잔여

실제게임/MP실행없이부품과합성만검사한다. 요청한사제포획/희생·특정적공격·시나리오승리실제재현은생략했다. originals/originalCD·AGENTS.md·README개요·cpppj·webpj를변경하지않았고외부접속코드를추가하지않았다. 실제사제월드묶음/HostSession·일반지역정책과M1전체일치0/34는잔여다.
