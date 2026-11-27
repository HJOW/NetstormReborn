# SID 관측 자료 등록 누락과 전체 검사 재실행

2026-11-27, HJOW-Athlon. E1438 Node26 정기 전체에서 새 `sid-allocation-recovery-x86.tsv`를 이식 추적 목록에 등록하지 않은 누락1건이 확인됐다. E1433의 집중84건에는 이 추적 검사를 포함하지 못했다. 제품/원본 기대값을 바꾸는 대신47행 자료를 필수 항목으로 등록했다.

첫 전체는 실패를 확인한 뒤 자체 Node 트리만 중단했고, 종료 당시3541개 입력의 원시 해시는 전부 같았다. 전체 요약이 없는 부분 실행이므로 최종 총수/통과수를 만들어 기록하지 않는다. 중단 원문·시작 고정·종료 대조를 그대로 보존했다.

공식 `fixture-manifest.mjs --write --only sid-allocation-recovery`로 계산 칸을 채웠다. 기존17개의 worktree 표시는 실제 fixture/web 커밋으로만 갱신됐고 자료 SHA·크기·줄 수와 기대값은 바뀌지 않았다. 목록은1331개 항목이며 추적3건+SID 회복2건 **5/5**를 통과했다. 원본 TSV/화면·기존 단언은 그대로다.

이번 보완은 E1433의 추적 등록 후속이며 구현 단위를 더하지 않는다. v26=4/4·v22=4/8을 유지하고 E1438 전체를 다시 실행한다. 전체 회귀 완료 기록은 재실행의 실제 결과가 나온 뒤 별도로 남긴다.

[첫 부분 실행 원문](webpj-e1438-first-evidence/node26.log)·[입력 고정](webpj-e1438-first-evidence/source-freeze.json)·[종료 해시](webpj-e1438-first-evidence/source-freeze-check.json)·[중단 경위](webpj-e1438-first-evidence/manual-stop.json)·[등록 자료](webpj-e1438-first-evidence/registered-fixture.json)·[보완 뒤5건](webpj-e1438-first-evidence/manifest-focus.log)·[17개 계산 칸/출처](webpj-e1438-first-evidence/provenance.json).
