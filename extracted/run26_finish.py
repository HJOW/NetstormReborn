# -*- coding: utf-8 -*-
"""전체 회귀 성공 후 단위26 인수인계와 검증 수치를 마감한다."""
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 같은 작업공간이다.
data=(ROOT/'extracted/run26-all.log').read_bytes()  # 완료 로그는 변경하지 않는다.
result=data.decode('utf-16' if data.startswith(b'\xff\xfe') else 'utf-8-sig')  # PowerShell 인코딩이다.
assert '1139 test(s), 0 failed check(s)' in result and '100% tests passed' in result
seconds=re.search(r'Total Test time \(real\) =\s*([\d.]+) sec',result).group(1)  # 실제 CTest 완료 시간이다.
path=ROOT/'LEFT_JOBS.md';text=path.read_text(encoding='utf-8')  # 기존 원문/이력을 보존한다.
text=text.replace('## 2026-10-15 진행: 단위26','## 2026-10-15 완료: 단위26',1)
text=text.replace('- [ ] 최종 Release 성공과 집중8개 성공 뒤 전체 회귀 검사를 진행 중이다. 전체 원문/로그/SHA 고정·인수인계 마감·로컬 `20261015-11` 커밋은 완료 결과 후 수행한다.',
    f'- [x] 최종 Release 경고/오류0·집중8개 성공·새 검사4개·전체CTest **1/1·내부1,139개·실패 체크0**({seconds}초). [상세 단위26](docs/exe/cpp-run26-20261015.md), `run26.json`, `tools/check_campaign26.py finalize/verify`로 전체 원문/입력/실행/로그 SHA를 고정한다. 경로는 기존 ignored 파일명41자1건 외 신규 위반0·최장178자다. 인수인계 후 로컬 **`20261015-11 실제 Player 송신 분배와 단독 캠페인 연결 검증`**으로 커밋하며 push하지 않는다.',1)
path.write_text(text,encoding='utf-8',newline='\r\n')
doc=ROOT/'docs/exe/cpp-run26-20261015.md';text=doc.read_text(encoding='utf-8')
text=text.replace('최종 Release/전체 검사는 마감 후 `cpppj/run26.json`, `extracted/run26-*.log` 및 `tools/check_campaign26.py finalize/verify`에 고정한다.',
    f'최종 Release 경고/오류0, 집중8개 성공(신규4개), 전체CTest 1/1·내부1,139개·실패 체크0({seconds}초)다. 전체 원문/입력/실행 SHA는 `cpppj/run26.json`, `extracted/run26-*.log` 및 `tools/check_campaign26.py finalize/verify`에 고정했다.')
doc.write_text(text,encoding='utf-8',newline='\n')
print('단위26 인수인계 마감:',seconds,'초, 전체1139 성공')
