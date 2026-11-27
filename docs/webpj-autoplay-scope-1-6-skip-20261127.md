# 유지 승리 수집기의1-6 검사 제외

WB04-E1423, 2026-11-27. 사용자의 최신 지시를 `tools/ally-ai-campaign-flow.cjs`에 반영했다. 실제 승리 검사 대상은1-1~1-5의 다섯 캠페인이며1-6을 인자로 주면 브라우저 실행 전에 사용자 직접 확인으로 이월됐다는 오류로 거부한다. 이미 종료한 E1419 후속이나 준비만 한1-6 적 재선정 검사는 재실행하지 않았다.

승리 대상과 다음 미션의 첫 안내 허용 집합을 분리했다.1-5 완료 뒤의 실제 Next Mission은1-6 World/첫 안내까지 확인할 수 있으며 Play Mission을 누르지 않는다.1-6 승리 뒤 저장/나가기를 검사하던 분기는 제거했다. 일반 게임의1-6 플레이·저장·메뉴는 변경하지 않았다. [현재 검사 안내](testing/webpj-autoplay-test-guide.md)도 갱신했다.

직접 CLI에서1-6과 후반 `TheNoose`를 각각 지정해 모두 예상 범위 오류·종료1을 확인했다. SyntaxError 같은 우연한 실패로 세지 않도록 실제 오류 문구도 대조했다. 실제 도구의 상수 선언만 평가한 별도 검사에서는 승리 대상5개·1-6 제외와 `thunderingpower → dissolvedalliance` 첫 안내 허용을 확인했다. 브라우저나 가짜 미션은 실행하지 않았다. 전역 ESLint도 종료0이다. 제품 소스를 바꾸지 않아 E1416 운영 번들3,508,534바이트·SHA-256 `72ea8d094b55216452615ed982bceb46e82ab3166957eb47a5225108e2bd7b25`는 그대로다.

[1-6 거부 원문](webpj-e1423-evidence/denied-1-6.log)·[후반 거부](webpj-e1423-evidence/denied-later.log)·[정책 집합](webpj-e1423-evidence/scope-policy.json)·[ESLint](webpj-e1423-evidence/lint.log)·[증빙 해시](webpj-e1423-evidence/provenance.json)를 보존했다. E1420 입력 고정 종료 뒤 수행했으며 완료 후 주기는 v26=2/4·v22=7/8이다.
