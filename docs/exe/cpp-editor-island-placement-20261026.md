# 편집기의 섬 배치 가능 검사와 남은 실제 GUI 경계

작업 PC는 HJOW-Athlon이며 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 기존 `extracted/refined/{originals,originalCD}`를 읽기만 했고 디컴파일 결과와 보호 원본을 변경하지 않았다.

이번 구현은 `CanPlaceIsland`의 실제 판정만 완료한다. Add Island 창과 실제 섬 추가·저장 완료를 뜻하지 않는다. 기존 `RawIslandBuilder`와 Place/PlaceAll의 과거 fixture·보고서는 변경하지 않았다.

## 복원한 함수

| 역할 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 임시 청크 지도에서 Territory 배치 가능 검사 | `0046de50` | `00490a50` |
| 실제 Add Island 공장 | `00480090` | `00477300` |
| Add Island 창 생성 | `0047f450` | 별도 분석 필요 |
| 현재 Fort로 임시 지도/목록 재구성 | `0047f9b0` | 별도 분석 필요 |
| 섬 드래그·놓기 입력 | `0047fb90` | 별도 분석 필요 |

`RawIslandPlacement.h/.cpp`는 기존 `ChunkMap`, `RawIslandBuilderState`, TYPE 프레임과 `CanonDecoder`를 같은 참조로 사용한다. 원문 stack 값 전달인 Territory 6바이트는 진단 앞에서 고정하고, 초기 bounds 진단 뒤 현재 원점을 계산한다. 방향 high2를 그대로 전달하고, CD의 홀수 방향 진단은 decoder 원점 인자를 계산한 뒤 실행한다.

첫 유효 칸부터 현재 right/bottom 배타 경계를 확인한 다음 현재 청크의 섬 번호를 읽는다. 원문에 없는 left/top 하한 거부를 추가하지 않는다. 10.78은 청크 getter의 범위 진단 뒤 기존 clamp를 실행하고 CD는 같은 clamp를 무진단으로 수행한다. 충돌하면 즉시 false이며 decoder에 유효 프레임이 없으면 원문처럼 true다. 판정 중 지도·builder·섬 목록을 변경하지 않는다.

## 독립 관찰과 검증 방법

`tools/decomp_islandplacement_oracle.py`는 보호 PE 두 개의 실제 CanPlace를 Unicorn에서 실행한다. 현재 호스트 `functions.tsv`의 해당 함수 불연속 범위와 기존 완료 디코더 body만 허용한다. 진단 보고는 기록 후 원본 흐름으로 반환하며 판정 결과를 C++이나 callback으로 계산하지 않는다.

- 양판본 각각 2,448개, 합계 **4,896개 고유 입력**이다.
- x87 `0x027f/0x037f` 두 설정에서 합계 **9,792회 정상 반환**을 대조한다.
- 모양64·방향4·내부/우측/하단 경계·한 점/전체 충돌을 교차한다. 빈/역순/일부 누락 프레임과 음수 원점도 별도 입력한다.
- 매 실행 전후 실제 3,084바이트 ChunkMap과 24바이트 builder가 같은지 확인한다. 실제 게임/OS 호출은 0회다.
- C++의 `IslandPlacement_ReplaysNativeReadOnlyCollisionAndBounds`가 모든 고유 입력의 반환·진단·지도/builder 불변을 재생한다. `IslandPlacement_CurrentMapAndBoundsAreBorrowed`는 같은 참조의 현재 변경을 다음 호출에서 읽는지 확인한다.

생성/출처감사는 각각 `python tools/decomp_islandplacement_oracle.py`, `python tools/decomp_islandplacement_oracle.py --verify`다. 최종 Release 빌드 경고/오류0, 새 C++2개 통과, 같은 제품 전체 C++3,165개·실패0을 확인했다. [통합 작업 문서](cpp-editor-runtime-integration-20261026.md)와 `cpppj/editor-runtime-integration-20261026.json`에 실제 결과를 기록했다. Add Island GUI 실행 완료로 합산하지 않는다.

## 다음 실제 Add Island 구현 계약

1. 원본 주 입력 부모는 `00542780`의 명령(`0x02000022`)에서 `00480090`을 생성하고 가상 `+0x20`에 `(0,0,0)`을 전달한 뒤 `00466600`의 실제 모달 수명에 등록한다. 0개 섬으로 편집기에 들어오면 기존 tail이 이 명령을 입력 큐에 예약한다.
2. 창 생성 `0047f450`은 432×412 기본 창·임시 지도·목록·그림·Help `PlacingIslands`와 초기 dirty/drag 상태를 만든다. 원본 생성자의 창 자식/위치/캡처를 임시 HTML 목록으로 대체하면 동일하지 않다.
3. `0047f9b0`은 현재 Fort Territory를 14×14 임시 지도에 다시 놓고 소유 영역과 남은 영역 목록을 만든다. 실제 `0046e4a0`의 남은 모양 목록과 임시 지도 복사가 필요하다.
4. `0047fb90`은 목록 선택에서 `0047f2a0` drag image를 만들고 실제 캡처/깊이/위치를 바꾼다. 놓을 때 화면 좌표를 16픽셀 청크로 바꿔 x/y 0..13을 검사하고 packed 위치 BYTE를 쓴다.
5. 놓기 가능 판정은 이번 `0046de50`이다. true일 때 record flag bit1을 내리고 `0046da20`으로 같은 builder를 실제 world에 연결한 뒤 `0046e0d0`을 실행한다. 그 부모는 **Place → 실제 지형 생성 `0046da70` → population `0046e000` → `00463110(0)` → Territory 저장 `004bf6d0`** 순서다. 현재 Place만 호출해서 빈 섬 추가를 완료로 처리하면 안 된다.
6. 성공 뒤 원본 카메라 갱신·drag Kill·목록 이미지 반납·행 제거·선택−1·dirty1·맵 revision 증가를 실행한다. 목록 안에 놓는 취소와 창 닫기는 월드 배치/저장을 실행하지 않는다.

Save as는 원본 `005428dc` 명령 → `004d3490`/`004d3250`의 실제 텍스트 입력 창이다. Test Battle은 `005429fc`의 인자0에서 `004bfb50` 저장 → Guide `00463b70` → 현재 mission+0x64 쓰기 `004639d0`로 진행한다. 인자1인 Return to Editing은 `00463b20` → pending name `00460bb0` → 모드2 → `004b3560`의 State1 예약이다. 이 경계들은 이번 판정 모듈에 섞거나 완료 처리하지 않았다.
