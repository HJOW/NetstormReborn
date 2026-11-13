# webpj 커서 미리보기 생성·재사용 (WB04-E403, 2026-11-14)

5-A의 `client/RawCursorPreviews.cpp/.h` 185줄을 `core/raw-cursor-previews.js`로 이식했다. 기준선 `1f00a25a` 판과 현재 HEAD 사이 변경은 없다. 4-D 소유자 E402가 갖춘 같은 Lifecycle/Placement/30DWORD 목록을 사용하며 별도 미리보기 월드를 만들지 않는다.

## 구현과 경계

- Query 뒤 첫 DWORD로 생성/재사용 모드를 한 번 결정한다. Canon 칸 순번과 연결 객체를 넣는 현재 count는 분리한다. 생성 콜백 뒤 현재 count에 append하므로 덮어쓸 비활성 DWORD를 SID16으로 먼저 해석하지 않는다.
- 각 raw의 임시 비트·프레임·WORD·소유자·선택된 HP0·float32 위치와 화면 WORD를 원문 순서로 갱신한다. 가상 호출 뒤 현재 argument/owner/TYPE를 다시 읽는다.
- 다리 품질은 현재 Form의 BYTE30에서 읽는다. 코드 flags는 signed BYTE로 확장해 실제 FrameCodeTable 검색기에 넘긴다. 정수 맞춤 후 생긴 연결 좌표에서 float32 차이만 빼고 본체 위치를 복원한다. 연결7개 진단은 좌표 복구 뒤이며 count 증가/Prepare는 하지 않는다.
- 받침 둘은 연결 개수가 아닌 Canon 칸 순번으로 재사용하며 새 생성에서만 현재 count에 추가한다. 공간 Pop은 호출하지 않는다.
- `bindCursorPreviews`는 Lifecycle 생성 효과만, `bindCursorPreviewPlacement`는 같은 위치 판정·준비·Canon만 바꾼다. `bindCursorPreviewWorld`는 같은 풀의 실제 Factory/Reward/RawBridgeConnect와 현재 프레임 표·WORD/HP 저장을 연결한다. 파생 가상 소유자 효과는 호출자가 공급하며 기존 장치를 보존한다.

## 검사

`tests/cursor-previews.test.mjs` **5/5 통과**:

1. 원본 `cursor-previews-x86.tsv` **522행**의 사건·상태52DWORD·접근한 모든50바이트 raw를 첫 대조에서 전부 일치했다. 그중 **32행**은 실제 RawCursorPlacement Query/Prepare 사슬이며 반복 생성은 첫 호출 결과를 재사용한다.
2. 누락 효과·다른 판본/풀·Canon 누락을 명시 실패로 처리한다.
3. E402에서 넘긴 `ClientCursorGlobals_CreateOverwritesInactiveDwordsAtActualAppend`: 첫0/다음0xffffffff/끝0x87654321 목록에서 실제 생성은 앞 두 칸만100/101로 덮어쓰고 끝의 비활성 값은 보존한다.
4. 연결7개 진단에서 마지막 연결 위치와 본체 위치가 먼저 복구되고 현재 count1과 Prepare 미호출이 남는다.
5. 같은 풀의 실제 Factory 생성·Reward의 현재 HP 판정·공통 HP setter·FrameCodeTable·RawBridgeConnect 꼬리 출력까지 연결했다. 현재 TYPE 변경, WORD 저장 뒤 선택된 통지 누락 실패, 실제 연결 꼬리 초과도 확인했다. 소유자/Pop/표면 효과는 명시 경계이며 전체 Session 시험이 아니다.

최초 집중 실행은3/4 통과였다. 추가 소유자 시험에서 `owner.lifecycle`을 속성으로 취급한 오류를 `owner.lifecycle()` 호출로 교정했다. 실제 월드 추가 시험의 첫 실행은 기존 모듈 이름을 확인하지 않은 import 경로 두 곳에서 로드 실패했다. 저장소의 `sp-store.js`·`random.js` 경로로 고친 뒤5/5를 통과했다. 제품 코드의 원본522행 대조 실패나 기대값 변경은 없었다.

최초 전체 회귀는 두 버전 모두 **3877/3878 통과**였다. 새 실제 월드 시험이 함께 읽는 `bridgeconnect-x86.tsv`의 소비 검사 목록에 새 파일을 등록하지 않아 추적 감사1건만 실패했다. 기존 manifest 항목의 consumers에 추가한 뒤 추적 집중3/3을 통과했다. fixture 바이트나 제품 코드/기대값은 바꾸지 않았다.

공유 입력/경계 도구는 `tests/cursor-preview-support.mjs`로 분리했다. 원본 fixture를 읽기만 하며 게임·브라우저 실행은 없다. 최종 전체 Node v26.13.0·v22.23.3은 각각 **3878/3878 통과**, 실패/취소/건너뜀0(292.1초/330.0초). 대응표 오류0, port651·partial61·todo184다.

## 다음

E386·E387에서 남긴 PreviewCollection/PreviewDraw의 실제 공유 커서 연결과 비용 그림 공급자를 이식한 뒤 RawCampaignCursorDisplay 조립을 진행한다. ClientRendererFrame·ClientGameplayGlobals와 실제 앱/미션 수명은 아직 연결되지 않았다.
