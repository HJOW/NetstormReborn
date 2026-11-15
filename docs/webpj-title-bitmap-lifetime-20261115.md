# 시작 title 그림 수명 이식 (WB04-E533)

5-C의 "메인 메뉴·타이틀 상태" 중 `client/TitleBitmapLifetime`(원문 4ab3e0/4ab370/4ab2d0/4b3500)을
`webpj/www/core/title-bitmap-lifetime.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
시작 title과 일반 BackgroundGif의 소유 수명을 구별한다.

## 옮긴 범위

- `runTitleBitmaps`(4ab3e0): 현재 −1·paint 깊이 +1·전체 clip·pump 뒤 입력이 있으면 건너뛰고, 아니면 `title0.gif` 적재 →
  `title00.col` 팔레트 → 가운데 정렬 표시(Lock·복사·Unlock·Present·Flip) → 현재 0 → 게임 시각 기록 → 3초 deadline 안에
  `title6.gif`·`title7.gif` 적재(적재 시간 포함) → 입력 대기 → `title06.col` → title6 표시 → 3초 대기 → title7 표시 → 3초 대기.
  끝에서 8칸을 앞에서부터 해제하고(0으로 재작성하지 않음) 검정으로 채운 뒤 깊이를 되돌린다. 예외를 정상 cleanup으로 바꾸지 않는다.
- `waitTitleBitmap`(4ab370): 기한 = ms × 0.001 + wall(), 기한 도달은 pump/input을 더 부르지 않고 false, 입력이 있으면 true.
- `redrawTitleBitmap`(4ab2d0 WM_PAINT): 현재 −1은 검정 출력, 그 밖은 현재 bitmap을 전체 SCREEN stride로 복사(0번 표/범위 밖은 거부).
- `requestInitialTitleBitmap`(4b3500): `next[0]=42`, `next[35]=0`만 쓰고 나머지 34DWORD는 보존.
- `TitleBitmapQuit`: 435b30의 WM_QUIT/_exit를 host Run의 정상 반환으로 잇는 제어 예외.
- 전역은 사본 없이 현재 값을 읽고 쓰는 속성 묶음이며, 적재 훅의 치수는 같은 공유 출력 객체이다(6/7의 서로 다른 치수가 같은 마지막 쌍을 쓰는 원문 확인).

## 시험

`webpj/tests/title-bitmap-lifetime.test.mjs` 3건:

1. 원본 관측 `title-bitmap-lifetime-x86.tsv` **33행 전부**: 종료 종류·현재 번호·paint 깊이·게임 시각·8칸 슬롯·장치 호출
   전체 순서(`P·I·N·G·A·L·B·U·V·F·W·R·Z·C·FL·FB·FU`)와 요청 뒤 144BYTE next 블록. **첫 실행에서 전부 일치**했다.
2. State 42(`HTRPS`)와 스플래시 중단(파일 오류/`TitleBitmapQuit`)이 앞선 숨김을 되돌리지 않고 전달됨(E532의 `installNativeIntroState`와 결합).
3. Wait의 기한 도달 시 추가 호출 없음, 재그리기 범위 밖 거부, 요청이 나머지 34DWORD를 보존.

`webpj/tests/fixtures/manifest.json`에 `title-bitmap-lifetime` 항목(원본 10.78, 33행)을 추가했다.

## 남은 경계

- 설치 경로 고리와 GIF/팔레트 파일 읽기(cpppj 세 번째 시험 `FormatTitleBitmapPath`·`ReadBackgroundGifFileImage`)는
  `ClientTitleBitmaps`(5-C의 남은 단위)·GIF 단위 몫이다. 실제 화면 장치(Lock/Present/Flip)와 Client 연결은 아직 없다(PORT_MAP `port`).
