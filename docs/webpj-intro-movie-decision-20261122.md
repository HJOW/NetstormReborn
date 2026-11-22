# 인트로 영상의 웹 빌드 제외 결정 (WB04-E1202)

## 결정

사용자가 2026-11-22 질문에 **"영상 제외"**를 선택했다. 인트로 영상(`englishintro2x.smk`·`germanintro2x.smk`)은 웹 빌드에 넣지 않는다.

## 근거(조사 결과)

- 인트로는 10.78 설치본(`originals`)에 없고 10.72 설치 CD(`originalCD/MOVIE`)에만 있다. Smacker 640×320, 126초, 영어 판 약 31MB.
- 브라우저는 SMK 를 재생하지 못해 WebM 으로 바꿔야 하는데 이 PC 의 ffmpeg(VP9 crf 34)로 바꾼 크기가 약 21MB 라 저장소·배포판이 커진다(영어·독일어 모두면 약 42MB).

## 처리

- `tools/port-map.mjs` 에 `exclude-movie`(`exclude-host`) 규칙을 `adapt-sound` 앞에 더해 `RawMovieControls`·`MoviePlayback`·`MoviePcmOutput`·`MovieLifecycle`·`SmackerDecoder` 다섯 단위를 이식 대상 todo 에서 제외로 옮겼다(`tests/port-map.test.mjs` 예외 목록에 같은 다섯 키 추가). 앞서 이식한 `MovieTimer`(`www/core/movie-timer.js`)는 그대로 둔다.
- 남은 양: 109단위·29,212줄 → 104단위·28,712줄.
- 다시 포함하기로 하면(`?intro` 같은 선택지 포함) 흐름은 클릭하여 시작 → 인트로(입력으로 건너뜀) → title 그림([E1184](webpj-title-screens-20261122.md)) → 메인 메뉴이고, 영상은 `webpj/www/data`에 WebM 으로 동봉해 `tools/prepare-assets.js` 목록에 더해야 한다.
