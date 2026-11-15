# 일반 Sprite 정기 이식 (WB04-E529)

`o/RawSpriteRegular`(원문 4794b0: 일반 Sprite의 정기)를 `webpj/www/core/raw-sprite-regular.js`의
`runSpriteRegular`로 옮겼다. 재기준선에서 추가된 `[rebaseline-port]` 단위이며 기준선 `225553df`와 현재
cpppj 소스가 같음을 확인했다.

## 옮긴 범위

- 부모의 현재 프레임 DWORD(+36)를 32비트 전체로 먼저 저장하고, 현재 TYPE의 `frameCount`(+114)로
  `(프레임 + 1)`의 **부호 있는 나머지**를 구해 실제 `SquidFrame.set(parent, 나머지, 0)`로 지정한 뒤
  원래 payload를 그대로 반환한다. event/count와 무관하다.
- `frameCount`가 0이거나 `(프레임 + 1)`이 −2^31이고 `frameCount`가 −1이면 원본 IDIV 오류이므로 어떤
  지정(표시 갱신)도 하기 전에 `RangeError`로 거부한다.
- 같은 10.78 풀·188 TYPE 표·같은 풀의 지정기가 아니면 거부한다.

## 시험

`webpj/tests/sprite-regular.test.mjs` 2건(`nan-exact.mjs` 선행 import):

1. 원본 관측 `sprite-refresh-x86.tsv` 225행 중 **정기(모드 R) 147행 전부**: IDIV 오류 여부·BYTE 35 보존·최종 프레임
   DWORD·반환 payload 비트·`SquidFrame.set`의 옛/새 표시 두 번 중 두 번째의 프레임(`S:<signed 프레임>`)을
   대조했다. **첫 실행에서 전부 일치**했다. 같은 파일의 표시(모드 D) 78행은 메타 Sprite 표시
   (`NativeMetaSpriteRefresh`, 웹 범위 밖 MP/메타 단위)의 몫이라 소비하지 않는다.
2. 다른 풀/지정기/TYPE 표 거부, frameCount 0의 사전 거부(프레임 불변, 표시 갱신 없음), 음수 나머지의 부호.

`webpj/tests/fixtures/manifest.json`에 `sprite-refresh` 항목(범위: 정기 147행)을 추가했다.

## 남은 경계

- 실제 일반 Sprite의 정기 분배(`RawSurface*` 소유자·Kernel Regular 등록)는 4-D/5단계 월드 연결의 몫이다.
  앱 수명에는 연결하지 않았다(PORT_MAP `port`).
