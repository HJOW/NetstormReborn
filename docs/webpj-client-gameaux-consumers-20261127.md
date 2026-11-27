# 실제 Client의 GameAux 소비와 공급자 순환 수명

WB04-E1440, 2026-11-27, HJOW-Athlon. 실제 Client가 제공한 surfaceStorage를 사용하는 Session에서 **건설 수신 차단, 건설/경로/배치 충돌 마스크, Player별 지역 간헐천 요청 수, 공급자 순환 인덱스**를 같은 장기 GameAux에 연결했다. 독립 typed 입력은 기존 지역 값/대입 계약을 유지한다.

| 원문 주소/값 | 연결한 실제 소비자 |
| --- | --- |
| 00558e74 / constructionBlocked | 실제 RawConstructionCommand가 본문보다 먼저 읽는 차단 DWORD |
| 005325b4 / constructionDisplaceGenus | 요청 displaceGenus·건설 pushGenus·경로 currentBlockingGenus·일반/실제 게임 배치 collision의5소비자 |
| 00540bd4·00540bd8 / first/lastLocalGeyserQuantity | Player1~4·5~8의 실제 지역 간헐천 생성량 재조회 |
| 0055a4fc / dudeProviderIndex | 같은 Client의 공급자 보행 순환 signed 인덱스; Session 반납/새 미션에 보존 |

C++ RawCampaignSession.cpp의 Native 소비 결선1057~1059와 공급자 참조676을 대조했다. 브라우저에는 Native 장치 ABI를 만들지 않고, 실제 Client의 장기 Storage 제공 여부로 이 공유 수명을 선택한다. 신규 공급자 칸은 보호10.78 PE의 초기0/32비트 폭으로94번째 scalar에 등록했다. 기존93칸 값/폭/원본 대조를 바꾸지 않았다.

수정 전 새3개 검사가 모두 실패했다. 실제 수신기에 빈/NULL 본문을 주었을 때 현재 차단값을 읽지 않아 예외가 났고, 다섯 소비자의 마스크/요청량과 다음 Session의 공급자 순환값이 실제 장기 owner를 따르지 않았다. 수정 뒤 실제 수신 조기 반환/차단 해제, fallback 대입이 장기 원본을 덮지 않음, signed 수량 재조회와 두 Session/새 Client의 수명을 대조했다. 새 접근자 팩터리는 장기 aux/필드명만 캡처해 짧은 Session을 잡지 않는다.

후보 두 모듈과 적용 소스는 원시 바이트/SHA-256이 같다. 첫 후보에서 원본 PE 각 값은 모두 일치했으나 scalar 개수 메타 단언이93/실제94라1건 실패했다. 새 주소를 그대로 PE에서 읽는 검증을 유지하고 개수만94로 갱신해후보46/46을 통과했다. 본 소스 집중72/72와 실제 raw-construction-command.test.mjs 추가8/8, ESLint 선행 운영 빌드, 재번들 일치1/1, 기존 Chromium L2 화면/덤프1/1을 통과했다. 첫 집중 명령의 이름이 다른 건설 검사 입력은 선택되지 않았으므로72개에 가산하지 않고 올바른8개를 따로 실행했다. 번들은3,515,266바이트다.

같은 본 소스에서 원래 유지 도구 `node tools/mission-memory-profile.cjs 12`로 실제 아군 AI1-1을매회600고정프레임 진행/원래 Leave UI로 반납했다.12회 전부 모든 이전World/Session WeakRef가 회수됐고 게임 오류0이다. 최종 post-GC 사용량32,190,896바이트이며 이를 전체 메모리 무증가로 주장하지 않는다. 도구의 raw JSON에는 오래된 E1404/E1400 문자열이 남아 있어 원문을 보존하고 실행 전 현재 소스SHA를 별도 기록했다. 정상rAF30분(E1435), 물리FPS와 구별한다.

129개 C++ private 이름과 웹 이름을 찾은 초기 감사 JSON은 탐색 자료다. **이름이 없다는 이유만으로 미구현을 확정하지 않는다.** 이번에 확정한4입력/순환1칸만 닫으며, 나머지 Native 헤더·역할·진단 값의 플랫폼 대응 판정은3.6절 후속에 남긴다. GameAux 전체129칸의 내부 저장 정체성을 입증한 결과는 아니다. 현재 v26=3/4·v22=3/8이며 다음 완료 단위 뒤 Node26 전체가 필요하다.

[수정 전](webpj-e1440-evidence/before.log) · [후보 첫 실패](webpj-e1440-evidence/candidate-first.log) · [후보46개](webpj-e1440-evidence/candidate-second.log) · [본 소스72개](webpj-e1440-evidence/webpj-e1440-main-focus.log) · [건설8개](webpj-e1440-evidence/webpj-e1440-command-focus.log) · [빌드](webpj-e1440-evidence/webpj-e1440-build.log) · [L2](webpj-e1440-evidence/webpj-e1440-l2.log) · [12회 메모리](webpj-e1440-evidence/memory12-raw.json) · [출처/해시/범위](webpj-e1440-evidence/provenance.json).
