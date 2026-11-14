# webpj 카메라 입력 세 부품의 원본 관측 연결 (WB04-E465, 2026-11-15)

이 단위는 새 모듈이 없다. E333(`ClientCameraGlobals`)·E437(`RawCameraPan`)·E442(`RawEdgeScroll`)이 기준선 소스 계약과 손 계산만으로 이식됐고 "cpppj에도 직접 시험이 없다"고 기록했지만, 실제로는 cpppj에 직접 시험과 원본 관측 fixture가 있었다(`CameraCenterTests.inc`·`CameraPanTests.cpp`·`EdgeScrollTests.cpp`). 이를 처음으로 연결해 세 이식이 원본 관측과 일치하는지 확인하고, 잘못 적힌 기록을 바로잡는다.

## 연결한 관측

- **`camera-center-x86.tsv` 140행** (`CameraCenter_Native140PreservesCurrentViewsAndOnlyWritesTarget`): `497f20` 중심 요청(`requestCenter`)이 Renderer 160BYTE 중 현재 view 64BYTE와 `+48..5b` 목표 20BYTE, snapshot 64BYTE를 어떻게 바꾸는지의 전후 전체 바이트. 목표 X/Y와 변환 번호(`005c85dc`)만 바뀌고 현재 view/snapshot/별도 분모 두 DWORD(23/17로 설정)는 보존됨을 확인했다.
- **`camera-pan-x86.tsv` 259행** (`CameraPan_Original259AndCurrentCallbacks`): `processCameraPan`의 실제 관문(창 밖/뗌/대화상자)·중심 거리·Shift 축 선택·목표 요청과 callback 뒤 재조회(팝업 닫기가 전역 viewport를 바꾸는 경우, 폴링이 현재 view/변환 번호를 바꾸는 경우)를 20열 입력으로 재생해 선택 여부·현재 view·목표·변환 번호·호출 순서와 snapshot 보존을 대조한다. 선택 장치/비활성 큐 선택 시험(`OnlySelectedDevicesAndInactiveQueueSelection`)도 옮겼다.
- **`edge-scroll-x86.tsv` 135행** (`EdgeScroll_MatchesNativeInputAndCameraTarget`): `processEdgeScroll`의 flags 비트 4 초기화·가장자리 2픽셀 관문·시간 가속·팝업·Shift 축 선택·최종 방향(float32 비트)·목표·변환 번호·호출 순서와 `since` 저장소(double 비트).
- cpppj `CameraViewTests`의 두 시험을 더 옮겼다: `WorldSetterAssertionRequeriesCurrentFieldsAfterAllStores`(X 진단 하위가 Y를 바꾸면 두 번째 진단은 현재 필드를 다시 읽음)와 `PlacementCostRequeriesSharedPixelsAfterDirtyEffect`(먼저 보낸 dirty 효과가 원점을 바꾸면 비용 사각형이 같은 snapshot의 변경 후 값을 읽음 — 실제 `RawCursorPlacement`, 공유 원점은 snapshot 픽셀을 직접 읽는 객체). `EdgeScrollTests`의 나머지 두 시험(목표 요청만 바꾸고 변환 번호가 감김, 연속 호버/중앙 복귀)도 옮겼다.

## 결과

세 관측 모두 **첫 실행에서 전부 일치**했고 모듈을 고치지 않았다(E333/E437/E442의 이식이 원본과 같음을 확인). 대신 다음을 정정했다.

- `raw-camera-pan.js`·`raw-edge-scroll.js` 머리 주석과 두 시험 파일 머리의 "cpppj에도 직접 시험이 없다" 문구를 E465의 원본 관측 연결로 바꿨다(주석 전용 변경).
- `PORT_MAP.tsv`의 `RawCameraPan`·`RawEdgeScroll`·`ClientCameraGlobals` 메모에 원본 일치 행 수를 더하고, `RawCameraView`는 partial → **port**로 올렸다. `RawCameraView.cpp`의 본체 함수(`InitializeCameraView`·`SetCameraViewWorld`·`SetCameraViewPixels`·`CopyCameraViewSnapshot`·`ProjectCameraCoordinates/View`·`CameraFtolLow`)는 모두 웹 `CameraView`/`projectCameraCoordinates`에 있고 cpppj `CameraViewTests` 5건 중 4건이 웹에 대응한다. 남은 `FirstMenuActualPrepareRequeriesSnapshotAndRenderer`는 실제 `Client`의 커서 준비 훅(`MakeClientCursorPrepareHooks`)과 Renderer 교체 통합 시험이라 RawCameraView 본체와 무관하며 실제 Client 단계로 이월한다.
- manifest `camera-center`·`camera-pan`·`edge-scroll`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.

## 남은 것

실제 `Client`의 커서 준비 훅과 Renderer 교체 시험, `CameraViewCommands`·`CampaignViewCommands` 시험(카메라 명령 입력)은 후속 단위다.
