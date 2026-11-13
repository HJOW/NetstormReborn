# webpj Client 커서 장기 소유자·첫 메뉴 취소 (WB04-E402, 2026-11-14)

4-D A의 `ClientCursorGlobals.h`(25줄)·`ClientCursorStateCleanup.cpp/.h`(27줄)를 이식했다. 기준선 `1f00a25a` 뒤 변경은 없다. 취소 연결이 직접 요구하는 `CampaignCursorClientEffects`의 **MakeClientCursorPrepareHooks 공개 API만** 함께 앞당겼다(해당 단위는 partial). 웹 파일은 `core/client-cursor-globals.js`와 `core/client-cursor-state-cleanup.js`다.

## 소유권과 연결

- `ClientCursorGlobals`: 같은 Lifecycle/Placement 객체·30DWORD 목록/문맥/치수 배열·두 옵션 상자를 Client 수명 동안 유지한다. 최초 BSS/PE 값만 만들며 Session 실행기나 Renderer를 소유하지 않는다.
- `resetForGeneralWorld`: 기존 배열에 값을 복사하고 화면 점의 주소를 유지한다. 현재 checking 참조·카메라 픽셀 참조와 회전 모드 DWORD는 보존하고 snap만0으로 초기화한다. NativeCancel의 효과를 대신하지 않는다.
- `cancelCursorForState`: 일반 game.prepare 없이 원문 `RawCursorLifecycle.cancel`→`activate`가 요구하는 위치에서 실제 `RawCursorPlacement.prepare`를 실행한다. 호출자 effects를 복사해 준비 경계만 교체하고 원래 묶음은 보존한다.
- `makeClientCursorPrepareHooks`: 호출 시 현재 Client 카메라 snapshot으로 투영하고 현재 Renderer provider로 변경 표를 갱신한다. provider 미지정 시에도 이전 Renderer를 캡처하지 않는다.
- `cancelClientCursorForState`: 현재 snapshot의 같은8바이트 픽셀 저장소를 공유한다. CameraView의 `pixels` 값 사본을 공유 포인터로 쓰지 않으며, 같은 buffer/offset이면 기존 보기의 객체 주소도 유지한다. Session 없는 잠금/빈 목록에서도 실제 Project/dirty 꼬리를 생략하지 않는다.
- Client 최소 계약은 모듈 머리에 명시했다: `gameplayGlobals()`→`camera()/cursorGlobals()/cursor`, `getRenderer()`. Surface는 실제 `pool/types/hash.view(0)`다. 최종 Client 본체의 생성/Acquire 배선은 4-D E와 구별한다.

## 기준선 뒤 판정

`CampaignCursorClientEffects.cpp`는 `b234f02e`에서 `ClientDiagnostics.h` include와 `drop.record` 개발자 건설 기록 연결 두 줄이 늘었다. PrepareHooks는 변경이 없다. include는 결과 영향 없는①, `MakeCampaignCursorClientEffects`의 실제 record 연결은 아직 옮기지 않은 함수의③으로 판정했다. 해당 행 base를 `b234f02e`로 올리고 메모에 남겼다. GUI/번역/글자 측정/마우스/비용/기록 연결을 빈 성공 함수로 만들지 않았다.

## 검사와 최초 교정

`tests/client-cursor-globals.test.mjs` **6건**, 주변 Lifecycle/Placement 포함 집중 **18/18 통과**:

- 최초값·120바이트/30DWORD·높은 WORD·signed count와 같은 상태/옵션 주소·두 Client 소유자 독립성.
- 일반 fresh의 명시 reset을 두 번 실행해 배열/화면 점 주소, 현재 checking·카메라·회전 모드 유지와 snap0·원본 문맥/기본값 복원을 대조했다.
- NativeCancel에서 준비·가상 삭제가 바꾼 현재 count/다음 DWORD·Process를 다시 읽어 호출순서 `[1,100,101,99]`와 전체 목록 정리를 확인했다.
- 큰 목록/현재 미리보기 DWORD는 원문 강조/준비/선택/잠금 prefix 뒤 호스트 경계에서 거부하고 하위 WORD로 접지 않는다.
- 실제 `ClientCameraGlobals`·`RawCursorLifecycle`·`RawCursorPlacement`·두 `Renderer`·같은 풀/해시를 최소 Client 계약으로 묶었다. 첫 메뉴 취소의 두 요청이 한 변경 영역 `(78,53)-(86,69)`로 모이고, 카메라5→6/Renderer 교체 뒤 `(62,42)-(70,58)`만 새 Renderer에 전달된다. 현재 미리보기 DWORD 전체를 지우되 screenX/Y·허용/현재 Gump·hotspot은 보존한다. 기본/명시 provider, 살아 있는 카메라 보기 주소도 확인했다.
- 실제 game.prepare 미호출·호출자 effects 보존과 선택된 Project 누락의 명시 실패를 확인했다.

첫 실행은 **4/6 통과, 2건 실패**였다. ① 일반 복사기가 빌린 카메라 TypedArray까지 소유 배열처럼 취급해 fresh의 null을 `.set`에 넘겼다 → fresh도 TypedArray인 소유 필드만 제자리 복사하고 카메라 참조는 보존하도록 교정. ② C++ 투영 결과의 배열 표현을 그대로 예상했으나 기존 웹 `CameraView.project`는 `{x,y}`를 반환했다 → 기존 반환 문맥을 그대로 전달하도록 교정. 제품 코드 교정 뒤 6/6·주변 집중18/18을 통과했다.

별도 fixture 추가/원본 변경과 게임·브라우저 실행은 없다. Node v26.13.0·v22.23.3 전체 검사 각각 **3873/3873 통과**, 실패/취소/건너뜀0(312.4초/345.6초). 대응표 검사 오류0이며 port650·partial61·todo185다.

## 남은 경계와 다음

cpppj 검사의 실제 `ClientGameplayGlobals.Acquire/AcquirePrepared` 부모 호출과 전체 Client 생성 부작용은 그 소유자/Client 본체에서 재대조한다. 이번 검사는 장기 데이터/원문 취소/현재 카메라·Renderer 효과의 조립이며 앱 수명 연결은 아니다. `CreateOverwritesInactiveDwordsAtActualAppend`는 **RawCursorPreviews** 이식과 함께 옮긴다.

다음은 5-A 목록의 **RawCursorPreviews**다. ClientCursorGlobals의 선행은 해소됐고, 사용자 선택 소유자/정리는 RawUserSelection 상태·본문이 선행이라 4-D A의 묶음 체크는 아직 닫지 않는다.
