# 공용 일본어 자산의 웹 준비

WB04-E1410, 2026-11-27, HJOW-Athlon. 공용 `assets/lang/japanese`의 공식 언어 파일43개와 UI 번역표를 웹 공개 루트로 변환 없이 복사했다. 별도의 일본어 번역 사본을 만들지 않고 공용 원문을 계속 따른다. `japanese` 묶음은44파일이며 기존 영어·한국어·음향 자산의 묶음은 유지한다. 전체 공개 자산은377파일이다.

`japanese-asset-config.js`의 같은 목록을 준비 도구와 런타임이 공유한다. `buildJapaneseSupport`는 번역표·UnicodeFontProvider·일본어 느슨한 파일 Map을 제공하며, 실제 글꼴 준비는 호출자의 비동기 공급자가 맡는다. 번역표/글꼴 오류는 앱이 진단·영어 대체를 처리할 수 있게 전달한다. 현재 앱의 언어 선택/시작에는 아직 연결하지 않았다. 웹 배포에 원본TTF를 넣지 않았고, 실제 CDN 글꼴 제공자는 다음 단위다.

새 검사에서 공용43개 목록·44개 배포 바이트·엄격UTF-8 해독, UI 번역과 튜토리얼39키, 실제 MissionScript32개 실행 헤더, 파일 누락 때 GameData 영어 대체, 필수 번역표/글꼴 오류 전파를 확인했다. 일본어/기존 자산/한국어 집중 검사는41/41 통과했다. 별도 공용 일본어 본문 감사는 원본 TAFF 영어43파일과 명령·조건·치환·링크 구조를 대조해 통과했다. 이는 텍스트/데이터 검사이며 미션 플레이나 일본어 글꼴 화면 확인이 아니다.

전체377개 복사/manifest 검사와 선행ESLint/운영 webpack 빌드도 통과했다. 새 지원 모듈은 앱 시작에 아직 연결하지 않아 운영 번들은 E1404와 같은3,500,925바이트·SHA-256 `1bd47729…e096`이다. 다음 단위에서 CDN 글꼴·언어 선택·제품 추가 문구를 연결하고 브라우저로 확인한다. 그 뒤 모든 미션의 일본어 진입/초기 안내를 확인하되 실제 플레이는 사용자 범위를 따른다.

[원본 구조 감사](webpj-e1410-evidence/japanese-audit.json), [집중 검사](webpj-e1410-evidence/focused.log), [복사](webpj-e1410-evidence/assets-prepare.log), [377개 동일성 검사](webpj-e1410-evidence/assets-check.log), [선행 검사/빌드](webpj-e1410-evidence/build.log), [배포 목록·출처·해시](webpj-e1410-evidence/provenance.json)를 보존했다. 완료 주기는 v26의3/4·v22의8/8이므로 E1411 Node22 전체 회귀를 바로 실행한다.

E1411 최초 Node22 전체에서는6,901개 중6,899통과·2실패였다(823.051초·종료1). 제가 새 자원 모듈 두 개의 대응표 등록을 빠뜨려 같은 완전성 검사 두 곳이 실패했다. `japanese-assets.js`는 기존 `KoreanResources.cpp` 언어 자원 적응 행에, `japanese-asset-config.js`는 웹 전용 배포 설정 목록에 등록했다. 게임 코드나 검사 규칙을 바꾸지 않았다. 보완 뒤 대응표 집중7/7과 전체ESLint가 통과했으며 E1411 전체 재실행은 남는다. 같은 E1410 단위의 누락 보완으로 주기를 중복 가산하지 않는다. [최초 전체 원문](webpj-e1411-evidence/first-node22.log), [2982파일 원시해시 고정](webpj-e1411-evidence/first-source-freeze-check.json), [등록 집중 검사](webpj-e1410-evidence/port-map-focused.log), [등록 보완/한계](webpj-e1410-evidence/port-map-provenance.json)를 보존했다.
