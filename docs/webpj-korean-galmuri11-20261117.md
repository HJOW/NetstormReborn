# webpj 한국어 갈무리11 글꼴 전환 (2026-11-17)

작은 한국어 글씨의 가독성을 높이도록 사용자 제공 `fonts/Galmuri11.woff2`, `Galmuri11-Bold.woff2`와 라이선스를 웹 데이터로 복사한다. `www/assets/korean-font-config.js`가 등록 이름과 세 경로를 정의하고 자산 준비 도구, 한국어 묶음, 래스터라이저와 시작 화면이 공유한다. FontFace는 일반 400·굵은 700을 등록한다. 로딩 실패 시 기존 영어 대체 경로를 유지한다. 예전 D2Coding의 웹 공개 복사본은 배포 목록에서 제거했으며 저장소 공용 원본 글꼴은 보존한다.

작은 슬롯(논리 높이 12·13·14)은 글꼴의 11픽셀 크기를 사용한다. Chromium에서 기존 13픽셀은 획이 뭉치는 것을 확인했으며, 11픽셀은 시작 대화상자와 일반 도움말에서 확인했다. 영어 `.chfnt`의 슬롯과 글줄 높이·기준선은 바꾸지 않았고 알파 문턱 110도 유지했다. 큰 슬롯은 기존 크기 비율을 유지한다. 실제 미션이 아직 연결되지 않아 브리핑의 줄바꿈·혼합 제목은 확인하지 못했다. 미션 연결 후 실제 화면을 확인해야 한다.

검증은 `tests/korean.test.mjs`, `tests/korean-tell.test.mjs`, `tests/external-access.test.mjs`, `npm run check-assets`, Chromium `e2e/korean-font.spec.js`로 글꼴 두께·실제 픽셀과 시작 대화상자·옵션·도움말·실패 시 영어 대체를 확인했다. 시작 대화상자는 Windows HJOW-Athlon, Chromium 헤드리스, 논리 4:3, 1024×768 CSS, DPR 1에서 보았고 도움말은 기본 Playwright 화면에서 확인했다. 원본과 화면 동일성이나 브리핑 검증으로 간주하지 않는다.

Node v22.23.3·v26.13.0 전체 각각 6,068건 통과(실패·취소·건너뜀 0). 첫 전체 실행에서 신규 `korean-font-config.js`의 웹 전용 모듈 등록이 빠져 대응표 두 검사가 실패했고, `WEB_ONLY` 등록 후 두 버전의 해당 검사와 전체를 다시 통과했다. 전체 기록은 `extracted/webpj-galmuri-node22-final.log`·`extracted/webpj-galmuri-node26-final.log`(Git 제외)이다. Node 24는 사용자 지시대로 검사하지 않고 호환을 가정한다.
