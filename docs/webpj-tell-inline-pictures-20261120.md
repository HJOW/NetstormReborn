# Tell 본문 그림 연결

WB04-E1063은 Tell HTML의 그림 태그를 기존 원본 `HtmlPictureText`와 StyleText 그림 해석기에 연결한다. 같은 텍스트 줄의 그림 크기와 기준점은 기존 ScrollingText가 측정·출력한다. 도움말에서 이미 사용하던 실제 TYPE·SHP 해석을 재사용한다.

실제 한국어 Tutorial2의 두 번째 안내 페이지에서 신전과 썬 워크샾 그림 조각 두 개가 양수 크기를 갖는 것을 확인하고 Chromium 캡처를 직접 살폈다. Tell·한국어·도움말 관련 Node 22·26 각 42건이 통과했다. 최초 Chromium 검사에서 존재하지 않는 `lines` 대신 실제 `softLines`를 읽도록 고쳤다. 상자 맞춤·색 치환 그림과 다른 미션의 원본 화면 대조는 남았다.
