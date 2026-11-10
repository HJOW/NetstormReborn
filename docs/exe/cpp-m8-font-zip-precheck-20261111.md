# M8 선행 점검: 글꼴 외부 배포 계약과 배포 ZIP 바이트 — 작업1274

2026-11-11 / HJOW-Athlon. AGENTS의 글꼴 지침("글꼴 파일은 게임 실행 파일에 내장되어서는 안 되고, exe와 같은 경로의 fonts 디렉토리에 별도 파일로 배치")을 현재 제품과 배포 스크립트에서 확인했다. M8 전체(최종 감사·최종 ZIP·checking 목록·외부 검증 비활성 전수)는 M6-n·M7이 끝난 뒤의 최종 관문이라 **미완료**이며, 이 단위는 그 관문의 글꼴/ZIP 선행 점검이다. 마지막 디컴파일 2026-11-03/HJOW-Athlon, 새 디컴파일 없음, MP 실행 0.

## 결과

- **exe 내장 여부:** 최종 `Netstorm.exe`(14.7MB)에 `ttcf`(TTC)·`wOF2`·`OTTO` 글꼴 시그니처가 0개이고 소스에도 글꼴 리소스(`RT_FONT`/`AddFontMemResource`)가 없다. 글꼴은 `AddFontResourceExW(FR_PRIVATE)`로 **파일 경로**에서만 등록한다(`BitmapFont.cpp`). 탐색 경로는 `CloneDataRoots(게임 폴더, 실행 파일 폴더)`의 `fonts/`이며 가까운 배포 폴더를 먼저 본다(`FindKoreanFontFile`). 문자열 "D2Coding"은 글꼴 이름 지정일 뿐이다.
- **배포 ZIP(실제 생성·압축 해제 없이 바이트 대조):** `package-cpppj.ps1 -NoBuild`가 1430항목/230.7MB(압축 193.8MB)를 만들며 `fonts/D2Coding-Ver1.4.0-20261003-all.ttc`와 `lang/korean/xlat.korean`, `lang/korean/d/*.korean` 43개, `Netstorm.exe`를 저장소 파일과 SHA-256으로 대조해 46개 모두 일치했다. 옛 `lang/xlat.korean`은 들어가지 않는다.
- **결함 발견·교정:** 배포 스크립트가 `fonts\License.txt`를 찾아 **없으면 조용히 건너뛰고** 있었는데, 저장소 글꼴 폴더의 라이선스는 `D2Coding-License.txt`로 이름이 바뀌어 있어 **글꼴 라이선스가 ZIP에 들어가지 않았다.** `D2Coding-License.txt`를 필수로(없으면 스크립트 오류) 동봉하도록 고쳤고, 재생성한 ZIP(1430항목)에서 바이트 일치를 확인했다.

## 한계 (남은 M8)

- 일본어 등 다른 언어 글꼴(Noto Sans JP, NotoSans)은 L 단계에서 같은 방식으로 외부 배치/라이선스를 점검해야 한다.
- 압축을 푼 배포본의 실제 GUI 실행, 읽기 전용 설치 위치 실행, 최종 ZIP의 Netstorm/R/에셋 전체 바이트 정합성, checking 목록 0/새 누락 감사, 외부 접속 비활성화 전수 감사는 최종 관문에서 수행한다.
