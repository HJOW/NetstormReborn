# 원본 시작 로딩 화면의 실제 표시 복원

2026-10-23 단위372. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 VM-W11-Codex / 2026-10-23이다. 사용자의 추가 요청에 따라 원본10.78 사본과 클론을 실행하여 시작 로딩을 비교했다. 원본/CD/AGENTS 및 기존 디컴파일 산출물은 변경하지 않았다.

## 결과와 변경

원본 시작 화면은 검정 바탕에 실행 파일의 비트맵147 두루마리 지도를 원래 크기로 중앙에 놓고, 우하단에 Arial20pt의 흰색 `Loading`과 `Please Wait`를 표시한다. 1024×768의 실제 원본·수정 클론 캡처를 대조한 결과 **전체786,432픽셀의 RGB 차이는0개**다. 지도152,192픽셀뿐 아니라 배경·문자·안티앨리어싱까지 같은 호스트/DPI에서 일치한다.

기존 Client에는 같은 비트맵/글꼴/공식의 `PaintLoading`이 있었지만, 초기 창이 표시된 뒤 타입 자료를 읽는 동안 유효한 갱신 영역이 항상 보장되지는 않았다. 제품 변경 전 실제 클론의 초기 visible/foreground 구간에서 이전 창 내용이 관측되고, 이후 메뉴는 정상 표시됐다. 단순 `PaintLoading(windowDc_)`와 `GdiFlush`만 호출한 중간 수정도 로딩을 표시하지 못했다.

최종 변경은 타입 자료를 읽기 직전에 현재 창을 `InvalidateRect(...,FALSE)`로 무효화하고 `UpdateWindow`로 **기존 WM_PAINT→PaintLoading**을 실행한 뒤 GDI와 Windows 합성 화면의 다음 표시를 완료하는 것이다. 원본 그림·크기·폰트·좌표를 재작성하지 않는다. 별도 Sleep이나 최소 로딩 표시 시간도 추가하지 않는다. 이는 현재 Windows 창에서 원본 화면의 표시를 보장하는 호스트 처리이며 원본 창 모드에 같은 추가 호출이 있었다는 주장은 아니다.

## 원본 근거

원본 `00436260`의 검정 FillRect→비트맵 중심 BitBlt→번역 Loading/Please Wait 측정→우하단 TextOut을 사용한다. PE의 `RT_BITMAP(2)/147/1033`은328×464·8bpp이며 DIB SHA-256은 `039d495fee732e6fdf94ef4cf169c30a185e1e8727a8d12898d5ac661c2c2466`이다. 원본 EXE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다.

그림은 확대 없이 `trunc((W-328)/2),trunc((H-464)/2)`에 놓는다. 글자의 x는 `min(W-loadingWidth,W-waitWidth)-10`, y는 각각 `H-loadingHeight-waitHeight-20`, `H-waitHeight-20`이다. CreateFont는 원본과 같은 Arial20pt/보통 굵기이며 실제 호스트 DPI로 픽셀 높이를 정한다.

## 실제 관측과 재실행

`tools/cpp_loading_screen_smoke.py`는 originals 전체를 출력 경로 아래에 독립 복사하고 options/setup의 저장 루트를 해당 사본으로 바꾼 뒤 실행한다. 보호 파일에 쓰지 않는다. PID/NetstormClient가 일치하는 창만 읽고 최초 unique 화면을 최대6PNG, 전체 unique/window 상태 메타를 최대64행 보관한다. 현재 desktop DC에서 ClientToScreen으로 구한 client 사각형만 읽으며 WM_PAINT·활성화·PrintWindow를 검사 도구에서 유도하지 않는다. 정상 WM_CLOSE를 먼저 요청하고 종료 결과와 전체 originals 전후 해시를 보존한다.

```powershell
python -X utf8 tools/cpp_loading_screen_smoke.py --output extracted/<새경로> --original
python -X utf8 tools/cpp_loading_screen_smoke.py --output extracted/<새경로> --width 1280 --height 720
```

현재 호스트는 AGENTS의 게임 실행 확인 예외 시스템이다. 다른 시스템에서는 해당 문서의 실행 확인 규칙을 먼저 따른다.

- `cpp-loading-20261023-gui1`: 원본 지도RGB1.0/두 문구·정상종료0. 최초 도구가 비로딩 앞2PNG만 보관하여 클론 후반 화면을 보존하지 못한 한계를 기록했다.
- `gui2/gui3`: 제품 변경 전 클론의 전체 unique 메타/초기 visible 화면을 보강하여 확인했다. `gui3`에는 초기 이전 창 내용과 정상 메인 메뉴가 함께 남아 있다.
- `gui4`: 직접 PaintLoading만 호출한 중간 수정은 로딩 미관측이다. 원인 규명 과정의 실패 자료로 보존했다.
- `gui5`: 최종 WM_PAINT 갱신 보장 뒤 로딩 관측·정상종료0·전체 originals 해시 동일. 원본1024×768 캡처와 전체RGB 차이0이다. 자세한 비교는 `extracted/cpp-loading-screen-20261023-pixel-comparison.json`에 있다.

640×480과1280×720의중간수정은로딩을관측했지만1280×800한번은미관측이었다. 임시진단빌드에서현재DC의전체갱신영역과328×464리소스성공을확인했고, 최종제품에서는진단을제거하고DwmFlush로합성표시까지완료한다. [Microsoft DwmFlush 문서](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmflush)는현재대기중표시가끝날때까지의동기경계를설명한다. GdiFlush의스레드GDI배치완료와구별한다.

최종 build4는 종료0·경고/오류0이며 제품 SHA-256은 `ef3bdff944a1d1ff2cc2d7d7964e86a590994533c94bc083b5814f0530c24576`이다. 관련15검사가 모두 통과했다. 전체2416/0은 이전 단위371의 결과이며 이번 변경 뒤 전체검사를 다시 실행한 결과는 아니다.

최종 desktop 캡처 `extracted/cpp-loading-screen-20261023-final-desktop`에서1024×768·640×480·1280×720·1280×800 모두 지도 RGB1.0/로딩 관측·정상종료0·전체 originals 동일이다. 앞3해상도는 원본의 같은 지도/문자 좌표를 적용한 전체RGB 차이0이다. 1280×800은 Windows 정품 인증 워터마크가 우하단에 겹쳐2451픽셀 차이가 남으며 이를 게임 그림 차이 또는 전체 픽셀 통과로 취급하지 않는다. 별도1120×700 재시도는 desktop 사용불가로 캡처0/BitBlt 실패915회여서 제품 표시 여부를 판정하지 못했다. 초기 private client DC 관측은 그리기와 캡처 경계의 한계가 있으므로 원인 확정 근거로 삼지 않는다.

실제 메뉴 회귀 `cpp-loading-screen-20261023-menu-final2`는1024×768 core 통과·원본 해시 동일이다. 앞 `menu-final`은 options.cfg 열기 실패로 probe가 종료1했고 재시도 성공과 별도로 보존했다. 임시 진단 코드는 최종 제품에 없다.

## 구별하는 화면과 남은 범위

이 단위의 동일성은 **프로그램 시작의 두루마리 지도 로딩**에 대한 결과다. 원본 `title0/6/7` 스플래시 및 미션/서버 전송의 `004b6090→004cf570` HTML 안내는 별도 경로다. 현재 미션의 중앙 `Starting Mission...` 임시 표시는 원본의400폭 요청/Cancel→LeaveBattle3 안내와 다르다. 준비한 MissionLoadingMessage의 실제 부모 수명/HTML 연결은 후속으로 남긴다. 전체 DirectDraw/전체화면 장치와 모든 미션의 원본 동일성까지 완료한 결과로 해석하지 않는다.
