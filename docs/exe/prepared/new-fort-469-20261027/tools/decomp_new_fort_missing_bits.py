#!/usr/bin/env python3
"""기존 A5 입력에서 이미 켜진 bit0의 쓰기를 별도0/A4/FE 원시 관측으로 보강한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
from decomp_new_fort import NewFortOracle,STAGE,STATES


class MissingBitsOracle(NewFortOracle):
    """동일 보호 부모와 같은 장치에서 State 초기 BYTE만 명시 입력으로 바꾼다."""
    def reset(self,action,mutation=0):
        """상위의 동일 전역 초기화 뒤9개 State 전체에 행 입력 패턴을 넣는다."""
        super().reset(action,mutation)
        self.mu.mem_write(STATES,bytes([self.state_fill])*288)


def main():
    """추가78행만 새로 만들거나 같은 BYTE인지 재실행하며 이전 세 fixture는 읽거나 쓰지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    oracle=MissingBitsOracle()
    rows=[]
    # 두 owner·13개 외부 재진입 효과·bit0이 모두0인 세 원시 패턴을 독립 교차한다.
    for owner,mutation,pattern in itertools.product((1,8),range(13),(0,0xa4,0xfe)):
        oracle.state_fill=pattern
        rows.append('\t'.join(map(str,(owner,mutation,pattern,*oracle.missing(owner,mutation)))))
    payload=('# owner/mutation/stateFill/alternate/states/trace\n'+'\n'.join(rows)+'\n').encode('utf-8')
    target=STAGE/'fixtures'/'new-fort-missing-bits-x86.tsv'
    if args.verify:
        if target.read_bytes()!=payload:
            raise RuntimeError('NewFort 추가 State bit0 관측 불일치')
    else:
        if target.exists():
            raise RuntimeError('NewFort 추가 State bit0 fixture 덮어쓰기 금지')
        target.write_bytes(payload)
    report=dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon / 2026-10-23',originalGameRuns=0,cloneGameRuns=0,osCalls=0,binarySha256=oracle.sha256,toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),baseToolSha256=hashlib.sha256((STAGE/'tools'/'decomp_new_fort.py').read_bytes()).hexdigest(),rows=len(rows),sha256=hashlib.sha256(payload).hexdigest(),limits=['004b5ca0 파일부재 부모만 실제 실행, manager/NewData 및 큰 외부하위는 명시장치','기존세fixture 불변, bit0=0인 State 입력의 실제 선택 owner쓰기만 보강','제품 C++/빌드/GUI 미검증'])
    (STAGE/'new-fort-missing-bits-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
