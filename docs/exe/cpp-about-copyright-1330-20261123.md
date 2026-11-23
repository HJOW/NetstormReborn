# cpppj1330 — 버전 창의 11.01 패치 저작권

2026-11-23, HJOW-Athlon. 마지막 디컴파일 PC도 같으며 원문/디컴파일은 변경하지 않았다.

수정 전 실제 영어·한국어·일본어 버전 창에는 원작1997 저작권과 Ticonderoga의10.78 패치만 표시됐다. 문서에 있던 “11.01이 Ticonderoga 패치로 치환될 수 있다”는 추측은 재현되지 않았다. 실제 `gamemaster/gameminor`는10.78이고 제품 `version`만v11.01이었다.

기존 원작 저작권과10.78 치환/작성자·각 언어 확인 버튼을 그대로 두고 `ProductTellBody`의 About 확장에서 `NetStorm 11.01 Patch by HJOW.`와 `Copyright 2026 HJOW`를 확인 버튼 앞에 추가했다. 한국어는 `NetStorm 11.01 패치: HJOW.`, 일본어는 `NetStorm 11.01 パッチ: HJOW。`다. 같은 저작권 줄이 이미 있으면 중복 삽입하지 않는다. 원본 `tell.english`, 한국어 원문 파일, CD 자료를 수정하지 않는다. 일본어 About의 기존 영어 대체 본문은 D3 정책을 유지하고 새 패치 행만 일본어다.

Release NetstormCpp·NetstormRoot·두 집중 대상 빌드 종료0. 기존 제품 UI25/25와 한국어/일본어47/47, 합계72/72 통과. 실제 메인 Help→Version의 영어/한국어/일본어3경로와 게임 중 ESC→정보→버전1경로를 실행해 모두 종료0을 확인했다. 실제 HTML 본문에서 원작·Ticonderoga·10.78·11.01·HJOW·저작권을 대조했고 세 창의 글자와 버튼 배치를 검토했다. [세 언어 화면](../screens/cpp-about-copyright-1330-20261123/versions.png).

실행 파일 SHA-256 `99514ed7c3b13ada9a155518880ce02421e21ba2d875805b13591f287b69da45`. [검증 JSON](../../cpppj/about-copyright-1330-20261123.json). 원시 입력·전후 보고·BMP와 실행별 JSON은 `extracted/cpp-followups-1328/about-*-before*`, `about-*-final*`, `version-ingame-ko*`, `check_about.py`다. 화면 PNG는 실제 세 BMP의 버전 창 영역을 자른 검토 모음이다.

1330 완료. 원본/설치 보호 지문과 배포 반영은 최종 확인 단위에서 기록한다. DN/WB는 해당 About 표시 경계를 옮길 때 이 계약을 따른다.
