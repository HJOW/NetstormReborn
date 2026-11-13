# webpj 커서 내려놓기 (WB04-E398, 2026-11-14)

`client/RawCursorDrop.cpp/.h`(기준선 `1f00a25a`, 199줄)를 `core/raw-cursor-drop.js`로 이식했다. 기준선 이후 해당 소스와 `CursorDropTests.cpp` 변경은 없다. `ClientUserSelectionGlobals`·`ClientCursorGlobals`와 5-A의 `ClientRendererFrame`이 요구하는 커서 표시를 연결하기 위해 6단계 커서 선행 목록을 열었다. 앱/실제 입력에는 아직 연결하지 않았다.

## 구현 계약

- `CursorDropState`: 잠금·배치 판정·TYPE·인자·방향·입력/비용 플래그·전체 미리보기 DWORD·월드 float 좌표·서로 다른 두 Gump 토큰·기록·전투·로컬 소유자·보정 화면 좌표를 같은 상태 객체에서 공유한다.
- `RawCursorDrop.drop`: 잠금 생략 → 현재 판정 → 미리보기 번호 캡처 → 준비/반올림/준비 → 강조/소리 → 월드 배치 또는 GUI 전달 → 비용 → 잠금/선택 해제/Process 닫기/미리보기 해제 순서다. GUI 거부는 후속 정리를 하지 않는다. 현재 상태는 원문과 같은 효과 뒤에 다시 읽는다.
- 월드 좌표는 double 중간값의 `floor(float + 0.5)` 뒤 float32로 저장한다. 비용 표시는 CRT signed64 절삭의 하위 signed DWORD, 화면 정수 감김, signed `/2`의 0 방향 절삭을 보존한다. 비유한 위치는 소비 위치에서 거부하지만 비유한 비용은 원문처럼 SP 경계에 전달하고 표시 정수는 0이다.
- 미리보기는 실제 SID 풀 소비 위치에서 5..capacity−1 및 16비트 범위를 검사한다. 준비·소리·잠금 등의 선행 효과를 보존하며 `0x10032`를 SID50으로 잘못 읽지 않는다. GUI 경로는 두 번 조회한 연관 번호, 조회 뒤 현재 raw WORD/품질을 캡처하고 detach 뒤에는 WORD 두 바이트만 쓴다.
- `makeCursorDropHooks`: 같은 풀의 `RawConstructionLocal.place`·`SquidReward.addSp`·호출자 SoundPlayer `playByName`을 새 훅 묶음으로 연결한다. 원래 훅은 보존하고 로컬 소유자는 호출 때마다 읽는다. 브라우저 `AudioSystem.playSound`와 원본 SoundPlayer의 인수 계약을 혼동하지 않으며 그 음향 연결은 H 영역에서 한다.

## 대조와 최초 실패

`tests/cursor-drop.test.mjs` **8건**:

- cpppj `CursorDrop_ReplaysOriginals`: `cursordrop-x86.tsv` originals **686행** 전체 사건/인수와 최종 전역/raw 14개가 첫 실행부터 일치했다.
- cpppj `CursorPreviewDword_Native280CaptureAndPreparePrefix`: `cursor-preview-width-x86.tsv` 중 originals **140행**, 명시 준비 중단 **20건**, 큰 DWORD·변경 시점·완료 표식·사건/최종 상태가 첫 실행부터 일치했다(CD 140행 제외).
- cpppj 거부·GUI 보존·두 잠금 이후 재호출·큰 DWORD의 세 소비 경로에서 하위 WORD 슬롯 보존 시험을 이식했다.
- 웹 보강: 각 필수 훅 누락, 같은 풀·훅 사본·현재 로컬 소유자 계약, detach 뒤 전체 50바이트 보존, float32 비용/CRT 절삭·음수 나눗셈·화면 감김·NaN/무한대 비용. 처음 보강한 detach 시험은 입력 장치가 슬롯 전체를 `0xa5`로 채워 수명 BYTE의 free 비트까지 켜 실패했다. 수명 BYTE11을 할당 상태0으로 보존하도록 **시험 입력만** 교정했고 제품 코드는 바꾸지 않았다. 보강 뒤 8/8 통과했다.

원본 자료·fixture는 수정하지 않았다. 정규화 SHA-256은 `cursordrop` `9a89077c528939f3450d4137a553d727a6d42869fe83b3ea226ffae962006d73`(파일2060행), `cursor-preview-width` `5a63b41fce76031c6525e722832e482f62096d7de141e7f189461e18f90844ad`(파일281행)이며 manifest에도 기록했다. 대응표 오류0, port645·partial60·todo191이다. Node v26.13.0·v22.23.3 전체 검사도 각각 **3850/3850 통과·실패/건너뜀0**(313.9초·349.4초, 종료 코드0)이다. Node v24는 미설치다. 실제 게임/브라우저·청취·자연 플레이를 실행하지 않았다.

## 남은 경계와 다음 시작점

`CursorDropTests.ReturnPiece_ConnectsGuiDrop`의 실제 생산 칸 반환은 **다음 `RawCursorReturnPiece` 단위**에서 옮긴다. 이번 훅 계약 검사를 실제 GUI/건설/SP/음향 세션 통합으로 세지 않는다. 이어 `RawCursorLifecycle`·`RawCursorPlacement`와 커서 전역·미리보기 표시를 같은 상태에 연결한다. `RawCampaignCursorDisplay`와 `ClientRendererFrame`·Tutorial1 Load/Start는 여전히 미완료다. CD·10.37 분기는 웹 범위 밖이다.
