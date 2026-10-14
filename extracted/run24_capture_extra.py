# -*- coding: utf-8 -*-
"""아직 편집하지 않은 SP/공용CMake 직접 원문을 현재 단위 기준에 추가한다."""
import hashlib
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]  # 같은 저장소의 현재 전체 바이트다.
history=root/'cpppj/hist/run24-base'
manifest=history/'capture.json'
data=json.loads(manifest.read_text(encoding='utf-8'))
# 현재까지 무편집인 직접 영향 원문만 추가한다.
for name in ('cpppj/src/o/CMakeLists.txt','cpppj/src/o/SpStore.h','cpppj/src/o/SpStore.cpp'):
    assert name not in data['files']
    target=history/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((root/name).read_bytes())
    data['files'][name]=hashlib.sha256(target.read_bytes()).hexdigest()
manifest.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
print('단위24 직접 영향 기준:',len(data['files']))
