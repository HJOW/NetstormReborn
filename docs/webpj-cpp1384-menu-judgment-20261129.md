# CP1384 메뉴 변경 판정 — WB04-E1490

2026-11-29, HJOW-Athlon. `RawGameMenus.cpp`의 행 기준 `840d92a8`과 CP1384 `3f12a2bd4`를 대조했다.

차이는 FSR 해상도 반복문의 주석 한 줄이다. 설명을 1280 폭·같은 화면비 정책으로 바꿨으며 실행 문장·분기·메뉴 항목은 같다. 따라서 3.6의 **① 웹 영향 없음**으로 판정한다. FSR·해상도 선택 메뉴는 웹 제외이고, 웹 논리 화면 상한은 E1488에서 이미 반영했다.

`webpj/PORT_MAP.tsv`의 해당 행 base를 `3f12a2bd`로 올리고 판정 근거를 추가했다. 고정 기준선 `225553df`와 오라클은 유지한다. 소스·자산·운영 번들은 변경하지 않았다.

대응표 집중 Node 검사는 7/7 통과했다(`webpj/extracted/webpj-e1490-port-map-tests.log`). `node webpj/tools/port-map.mjs`도 1,160행·오류 0이며 `RawGameMenus.cpp`는 변경 보고에서 사라졌다(`webpj/extracted/webpj-e1490-port-map.log`). 남은 보고는 생산 격자와 제외 분류의 `ScreenResolutions.h`다.

E1490에 도달한 정기 Node26 전체 회귀는 실행 중이다(`webpj/extracted/webpj-e1490-node26-full.log`). 제품 소스를 고치기 전에 그 결과를 확인한다. 이후 추가하는 부모 소실 재현 검사는 이 실행이 시작할 때 열거한 검사 목록에 포함되지 않는다.
