# Renderer 앞단의 화면 옵션·override·구름 스캔 복원 — 단위618

원문 `004994b0..00499758`의 화면 flags 변경·세계 redraw·실제 override 조회·구름 갱신 관문과16구간 행 스캔·dirty 준비를 `RawRendererPrelude`로 복원했다. **독립 본체와 원문 회귀의 완료이며 실제 Client Frame/구름 표시 연결은 다음 단계다.** [원문 근거](../../cpppj/renderer-prelude-native-20261030.json)와 [검증 집계](../../cpppj/renderer-prelude-runtime-20261030.json), 원시 `extracted/cpp-renderer-prelude-618-20261030/`를 따른다.

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. 앞617에서 AGENTS.md·공통/DN 인계를 다시 읽고 사용자 새 목표 요청으로 재개했으며 이번에도 같은 작업본에서 계속했다. 디컴파일 산출물은 변경하지 않았다.

**619 후속 정정:** 명시 하위였던 `4977f0`은 별도 구름 dirty4칸/count 저장이다. API 이름을 `queueCloudDirty`로 고쳤으며 일반 화면 dirty 표와 구별한다. [619 하위 복원](cpp-renderer-clouds-20261030.md)을 따르고, 실제 Client Frame/픽셀·정리 연결은620에서 진행한다. 아래618의 관측/회귀 수와 기존 fixture는 당시 근거로 유지한다.

## 원문 순서

1. 현재 화면 flags가 이전값과 다르면 실제 구름 dirty→현재 전체 화면 invalidate→효과 뒤 flags 저장 순서다. 세계 redraw가 비영이면 별도로 구름 dirty/전체 invalidate를 다시 수행한다.
2. clouds 비트8이 켜져 있을 때만 `00463fe0`의 실제 overrideHead를 조회한다. override가 있으면 구름 갱신/스캔을 생략한다. 일반 모달 개수로 대신하지 않는다.
3. animate 비트32가 있으면 현재 구름 Update를 실행하고 화면 flags를 다시 읽는다. parallax 비트16이 없을 때만 초기 높이 `00531864`로 행 표를 처음 구성한다. 현재 논리 화면 높이 `005c78f4`와 구분한다.
4. 원문은16행 중0번 행을 쓰지 않는다. 현재 index의30행 구간에서 첫 dirty를 찾고, 각 조회 뒤 바뀐 index/상한·현재 카메라Y·화면 폭/지도 clip을 반영한다. 행 요청 뒤 index는 signed 나머지16으로 진행한다.
5. 구름 dirty를 표시하고, parallax16·fullscreen1·비DIB2의 조합이면 별도 배경 invalidate 뒤 Renderer+60에1을 쓴다. 캡처한 flags BYTE는 하위 효과 뒤 임의로 다시 선택하지 않는다.
6. 정상 경로는 표시 카운터를0으로 만든다. 현재 세계 redraw가 여전히 비영이면 dirty 개수를0으로 만든 뒤 현재 크기의 전체 사각형을 추가한다. 이 앞단은 세계 redraw 자체를 지우지 않는다.

index·16행·초기화·이전 flags는 원문처럼 인접19DWORD로 제공한다. 따라서 입력 index=-1/16/17의 실제 인접 읽기도 유지한다. 그 물리 범위 밖의 임의 접근은 호스트 예외로 거부하며 배열을 임의 확장하거나 index를 양수로 고치지 않는다. 실제 PE의19DWORD 초기값은 모두0임을 별도 읽기로 확인했다.

## 독립 원문 관측

`tools/decomp_renderer_prelude.py`는 보호 PE의 실제 상위 구간과 `00470c80` MarkDirty·`00463fe0` getter·`00445290` 정수 clip 명령을 Unicorn x86에서 실행한다. 구름 Update/RowDirty, dirty 목록 처리와 시차 배경 장치는 명시 동기 경계다. 구름의 실제 생성/이동·픽셀 표시 전체를 이 관측에서 실행했다고 합산하지 않는다.

720개 입력에 대해 기본 x87 제어어027f 최초/재관측과037f의2160관측이 같다. 별도 프로세스의 재관측 및 도구 정리 후 최종 관측도 입력/최초·재관측·확장 출력/fixture가 모두 바이트 동일했다. 세 프로세스 합계6480관측이며 게임/OS 실행과 구별한다. [fixture](../../cpppj/tests/fixtures/renderer-prelude-x86.tsv)는 제품 본체 작성 전에 원문 출력으로 만들었고 이후 변경하지 않았다.

입력은 화면 옵션 여섯 비트와 실제 호환 모드, 이전 flags 변경·세계 redraw·override 교차, 모든16행/초기화, 첫·4번째·30번째 dirty/없음, 음수/감김 크기, 인접 alias와 하위 효과 뒤 현재값 변경을 포함한다. 캡처한 flags와 현재 전역 재조회가 필요한 위치를 각각 비교한다.

## C++와 다음 실제 연결

새 검사2개는720행의 인접 상태19DWORD·현재 전역11DWORD·Renderer/표시/구름 DWORD 전체와 호출 순서를 대조하고, 생략 경로의 장치 요구 및 선택된 하위 실패의 부분 효과를 검사한다. 기존 KencloudRender·ClientKencloudGlobals·DrawSubmission·ScreenScroll·CameraPan 회귀도 함께 실행했다. 최종 Release 게임/검사는 경고/오류0이며 고유 집중25개/실패0·등록3524개다. 게임/검사 및 근거 SHA는 집계를 따른다. 전체 C++ 검사는 반복하지 않았다.

새 fixture를 제외한 기존 보호2618파일/fixture759개는 그대로다. 현재 fixture는760개다. 기존 CMake 본문은 보존하고 끝에 등록만 추가했다. 기존 게임 동작에 연결하지 않은 본체이므로 새 게임창·ZIP·전체34경로를 실행하지 않았다. 마지막 실제 배포본/화면비 검증은617, 마지막 전체34는615제품/616검증이다.

다음619에서는 실제 Client 수명의19DWORD와 Renderer 필드, 같은 Kencloud cache/난수/옵션, 현재 overrideHead, 화면 옵션/지도 clip을 이 본체에 연결한다. 이어 dirty별 실제 배경 Draw와 프레임 말미 CleanDirty를 원문 순서로 연결해야 한다. 기존 정적 GIF를 동적 구름의 증거로 세거나 무조건 Step을 실행해 관문을 우회하지 않는다. 비DIB DirectDraw 시차는 현재 Windows10/11의 DIB 호환 출력과 구분하고 차이/이유를 코드 주석에 남긴다.

전체 목표는 active·미완료다. 남은 UI/Edit·Native·시나리오/파일·수명·Windows10/11 호환성 뒤 MP/Outpust를 디컴파일 근거만으로 무시험 사전 구현하고4~7차를 진행한다. 실제 포획/희생·특정 적 공격·승리/자연 완주와 공중3종 전용·신규MP 검사는 수행하지 않았다. 612의 스택 미정값 자체 동일성 한계도 유지한다.
