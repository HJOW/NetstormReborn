# 보관된 후보/기준 사본의 바이트만 확인한다. 제품 적용·빌드·게임 실행은 하지 않는다.
from pathlib import Path
import hashlib
import json

BASE = Path(__file__).resolve().parent  # 검증기와 같은 디렉터리 안의 추적 인계만 읽는다.
manifest = json.loads((BASE / 'MANIFEST.json').read_text(encoding='utf-8'))  # 보관 파일별 실제 바이트 기준이다.
expected = {entry['path'] for entry in manifest['files']}
actual = {path.relative_to(BASE).as_posix() for path in BASE.rglob('*') if path.is_file() and path.name != 'MANIFEST.json'}
assert expected == actual, {'missing': sorted(expected - actual), 'extra': sorted(actual - expected)}
for entry in manifest['files']:  # 누락/추가뿐 아니라 개행·BOM을 포함한 모든 바이트를 비교한다.
    path = (BASE / entry['path']).resolve()
    assert path.is_relative_to(BASE), entry['path']
    assert path.stat().st_size == entry['bytes'], entry['path']
    assert hashlib.sha256(path.read_bytes()).hexdigest() == entry['sha256'], entry['path']
print(json.dumps({'archivedFiles': len(expected), 'byteHashesMatch': True, 'productApplied': False, 'cppCompiledOrRun': False}))
