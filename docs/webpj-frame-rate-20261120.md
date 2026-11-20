# 옵션 메뉴 Frame Rate(프레임 상한) (WB04-E1094)

원본의 프레임 상한(기본 75, `DAT_005318d8 = 0x4b`)을 웹 프레임 루프에 적용하고, cpppj의 후순위 F1처럼 옵션 메뉴에서 60·75(Original)·120 FPS를 고르게 했다. 이전에는 상한이 없어 144Hz 같은 빠른 표시에서 게임 프레임이 표시 주기마다 한 번씩 돌았다.

## 구현

- **값 해석.** [frame-rate-option.js](../webpj/www/core/frame-rate-option.js)가 cpppj `FrameRateOption.cpp`를 옮긴다: 설정 `maxFPS`가 0·범위 밖이면 원본 기본 75, 1~1000은 수동 값으로 인정; Tell 대상 `Patch1101.SetFrameRate(n)`는 대소문자를 무시하고 접두·닫는 괄호가 정확한 10진수가 선택지(60·75·120)에 있을 때만 값; 옵션 항목은 `60 FPS`·`75 FPS (Original)`·`120 FPS`에 현재 상한 체크.
- **프레임 상한.** `_onFrame`이 `frameDue`로 직전 처리 프레임과의 간격이 상한 간격(1000/상한 ms, 표시 주기 흔들림 허용 1.5ms)보다 짧으면 처리를 건너뛰고 다음 표시 기회를 예약한다. 60Hz 표시에서는 상한 75가 걸리지 않고 144Hz에서는 약 72프레임으로 돈다.
- **메뉴.** 메인 메뉴 Options 맨 앞에 `Frame Rate` 하위 목록(원본 비교용 `includeAll`에는 없음)과 ESC 게임 메뉴 Options 맨 앞의 같은 하위 목록(`frameRates` 장치)을 더했다. 선택은 Tell 대상으로 가서 `_openTell`이 창을 열지 않고 `maxFPS` 설정에 쓰고 저장한다. 제목은 한국어 `프레임 속도`로 번역하고 선택지 라벨은 cpppj처럼 번역하지 않는다.
- **설정.** `maxFPS`를 설정 키(정수, 기본 0)로 올려 `netstorm.settings`에 저장한다(Reset All은 같은 저장소 키를 지운다). 설정 키가 13개가 됐다.

## 검증

- 새 `tests/frame-rate-option.test.mjs` 5건(값 해석·Tell 대상·선택지·프레임 처리 판단[60·144·120Hz 표시 모의]·메인 메뉴 항목). 메뉴 구조 변경으로 `popup.test.mjs`·`korean.test.mjs`(첫 항목이 Frame Rate)의 기대와 `first-run.spec.js` 설정 목록, 한국어 번역표(`translations/xlat.korean`, 자산 사본 동기)를 갱신했다.
- 새 `e2e/frame-rate.spec.js`(Chromium): 메인 메뉴에서 기본 75 체크 → 120 선택(저장 확인) → 다시 열면 120 체크 → 60 선택 뒤 처리한 프레임 간격이 60 FPS 간격 이상; 미션 중 ESC 메뉴 Options 맨 앞의 Frame Rate가 세 항목을 보이고 60을 선택하면 같은 설정에 저장된다.

## 남은 경계

실제 144Hz·240Hz 표시에서의 체감과 게임 속도 동일성은 이 환경(60Hz 헤드리스)에서 확인하지 못했다. 120 FPS 선택 시 게임 로직·애니메이션이 원본 75 프레임 기준과 어떻게 어울리는지는 AGENTS의 "이후 60·120프레임 지원" 단계에서 다룰 항목이다. Language·Difficulty 하위 메뉴는 아직 없다.
