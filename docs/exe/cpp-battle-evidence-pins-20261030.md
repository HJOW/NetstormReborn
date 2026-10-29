# 전투 종료 관측의 Git 바이트 보존 — 단위589

2026-10-30 / HJOW-Athlon. 588 커밋 `64ca9886` 후 새 TSV 두 개의 Git blob이 최초 관측 CRLF를 LF로 바꾼 것을 확인했다. 작업 파일과 원문 관측 해시는 그대로였지만, 다음 체크아웃에서는 고정한 SHA와 달라질 수 있었다.

원문 관측 바이트를 수정하지 않고 두 파일의 Git 속성을 `-text`로 지정했다. 마지막 빈 열을 표현하는 TSV의 끝 탭은 `whitespace=-blank-at-eol`로 명시했다. 일반 add는 기존 index 통계를 유지했으므로 해당 두 파일만 `add --renormalize`로 다시 저장했다. 새 index는 최초 관측 SHA와 정확히 같다. 588의 정규화된 과거 blob은 히스토리에 남아 있다.

587/588의 실행·Native 보고서, 새 소스/검사/실행 도구, 기존 혼합 줄바꿈인 UberEditor.inc도 현재 바이트에 맞는 속성으로 고정했다. 게임 본문·검사 기대값·최초 Native 도구/보고서는 바꾸지 않았다.

관련19경로에서 작업 파일=index를 확인하고 `core.autocrlf=true/false`의 실제 Git checkout 필터 출력38개가 모두 같은 바이트임을 확인했다. 원문16행/8행을 새로 재관측해 차이0이며 보호2599개(기존fixture738 포함)는 불변이다. 이번에는 새 게임 빌드/GUI/C++ 검사를 반복하지 않았다. 실행 제품/ZIP SHA와 실제 복귀/저장 전체화면 재실행 근거는588 그대로다. 전체34의 최신 근거는586제품/587이다.

[집계](../../cpppj/battle-evidence-pins-20261030.json), 원시 `extracted/cpp-battle-evidence-pins-589-20261030/`와 [588 기능/실행 문서](cpp-battle-leave-20261030.md)를 따른다. 남은 UI/Edit·시나리오·Native 선택·호환성과 후순위 작업의 완료를 뜻하지 않는다.
