# webpj 커서 취득·취소·미리보기 정리 (WB04-E400, 2026-11-14)

`client/RawCursorLifecycle.cpp/.h`(기준선 `1f00a25a`, 235줄)를 `core/raw-cursor-lifecycle.js`로 이식했다. 기준선 뒤 소스와 `CursorLifecycleTests.cpp` 변경은 없다. E398의 같은 `CursorDropState`를 빌려 취득/취소/정리를 실행하고, 나머지 전역만 `CursorLifecycleState`에서 소유한다. 앱/미션에 연결한 것은 아니다.

## 계약

- 상태: abstract TYPE·방향 인자·현재 Gump, 30DWORD/120바이트 미리보기 목록과 signed 개수, 두 8DWORD 글자 문맥, 다섯 개별 경고 치수와 최대값, 비용 문자열·그림 토큰·signed WORD 기준점/높이를 소유한다. 마나/사정거리 Process·허용 Gump·현재 Gump는 별개다.
- `activate`: 준비 효과 뒤 flags/force를 판단하고, 가상 삭제마다 현재 signed 개수/다음 전체 DWORD를 다시 읽는다. 정상 종료만 목록30칸과 개수를 지운다. 큰 DWORD를 하위 WORD에 별칭하지 않는다.
- `cancel`: 강조 뒤 현재 잠금/번호, 준비 뒤 목록, 현재 Form WORD/부모 flags를 순서대로 소비한다. 캡처한 Form과 현재 미리보기 번호를 구별한다. 반환음 뒤 checking, 진단 복귀 뒤 현재 TYPE 비용을 다시 읽으며 중단된 진단의 후속 정리를 합성하지 않는다. 정상 꼬리에서 미리보기·Process·잠금/개수를 정리한다.
- `take`: 취소→팝업 정리→현재 Form의 TYPE/인자/입력 WORD→source 통지→flags/미리보기→마나/사정거리→다섯 경고 측정→CRT 절삭 비용/실제 그림 헤더→현재 마우스 꼬리 호출 순서다. float32·DWORD/WORD·signed 감김은 원문 위치를 보존한다.
- `checkingValue`는 현재 Client 진단 상자 또는 getter를 읽고, 독립 입력일 때만 `teleportBlocked`를 쓴다. `bindCursorLifecycle`은 사제 메뉴 취득과 내려놓기 활성화/GUI 거부를 같은 몸체에 연결한다.
- `bindCursorLifecycleAssets(hooks, assets, types, fonts?)`: 웹 `GameAssets`는 그림 정의/블록을 소유하고 세션의 mutable TYPE 레코드는 따로 있으므로 **실제 현재188칸 types를 명시 인자로 빌린다**. 표를 내부에서 재생성하지 않는다. 같은 자산의 defaultFrame·frameCheck·SHP 헤더와 현재 TYPE 사정거리를 연결한다. 선택적 FontSet은 NUL 종료와 8DWORD 문맥을 기존 비트맵/유니코드 측정 입력으로 바꾼다. 생략하면 기존 측정 장치를 보존한다.

## 대조

`tests/cursor-lifecycle.test.mjs` 신규 **8건**, 커서 세 모듈 집중 검사 **21/21 통과**:

- `cursor-lifecycle-x86.tsv` **565행**: 취득/취소/정리의 전체 사건·78DWORD·비용 문자열·raw150바이트가 첫 실행부터 일치했다.
- `cursor-cancel-checking-x86.tsv` **156행**: 반환음/진단 뒤 현재 checking·TYPE 변경, 음수0 비용, 진단 중단과 부분 상태가 첫 실행부터 일치했다.
- `cursor-type-range-x86.tsv` **854행**: 현재 그룹/flags2 사정거리 판정이 첫 실행부터 일치했다.
- cpppj 메뉴/내려놓기 공유 바인딩·선택 장치/판본/크기 거부, 실제 비용 SHP와 글꼴 측정, 모든188TYPE 번호 변환과 TYPE128 취득의 사정거리 생성 시험을 옮겼다. 웹 준비 자료를 읽기만 한다.
- 웹 보강: 실제 `ClientAiDebugGlobals`의 현재 상자/함수 대여와 누락 진단의 반환음 직후 실패, 큰 DWORD 목록, 삭제 효과 뒤 개수 감소/다음 번호 변경과 실패 시 목록 보존, 현재 TYPE 변경·측정 장치 보존·NUL 종료를 확인했다. 추가 raw 대여 검토에서 defaultArgument 콜백 뒤 Form WORD를 재조회하는 독립 검사를 넣자 이전 복사본 값8을 읽어 기대0x3456과 달라 실패했다. `Raw`를 C++ span처럼 같은 풀의 살아 있는 `physical` 보기로 바꿔 교정했고 8건·커서 집중21건을 재통과했다. 기존1575행은 처음부터 일치했다.

원본/클론 게임·브라우저·청취를 실행하지 않았고 fixture를 수정하지 않았다. manifest의 `cursor-lifecycle`·`cursor-cancel-checking`·`cursor-type-range`에 정규화 SHA·행 수를 기록한다. 수정 후 Node v26.13.0·v22.23.3의 전체 검사를 다시 완주해 각각 **3863/3863 통과·실패/건너뜀0**(347.7초·381.2초, 종료 코드0)으로 확정했다. Node v24는 미설치다.

| fixture | 파일 행 수(주석 포함) | 정규화 SHA-256 |
| --- | --- | --- |
| cursor-lifecycle | 566 | `153f07eb922205fa0a2adf2a4d365bbbffe82d4822d4814c79fdd539d5a4c53a` |
| cursor-cancel-checking | 157 | `4c02fa86dc6cf5f00849ffafd9f77f6a2e0d81c4c348164b1a62cde8e537faa9` |
| cursor-type-range | 855 | `cf2a3db28cd78c5089e7102a33b86b5ab3908e2f7137bf09de592d33df69b824` |

대응표 검사는 오류0(port647·partial60·todo189)이다. 수정 전의 전체 검사 도중 독립 회귀 검사를 추가해 raw 대여를 교정했으므로, 그 이전 실행과 최종 검증을 구별한다. 수정 전 Node22 실행은 중단했고 수정된 코드의 두 버전 전체 검사를 다시 실행했다.

## 남은 경계

cpppj `CursorLifecycle_CurrentClientCheckingSurvivesResetAndSelectedMissingDiagnostic`의 **실제 ClientGameplayGlobals Acquire/AcquirePrepared 대여·반납** 부분은 그 소유자가 미이식이라 4-D A로 이월한다. 현재 AiDebug 상자 대조와 실제 Client 조립 대조를 혼동하지 않는다. 실제 PriestMenu/커서 전역/미리보기 생성·위치 판정/글자 출력/음향/Session은 각 단위에서 같은 상태에 연결한다.

다음은 `RawCursorPlacement`다. 그 뒤 커서 전역·미리보기 표시를 묶어 `RawCampaignCursorDisplay`와 4-D/5-A의 막힘을 순서대로 해소한다.
