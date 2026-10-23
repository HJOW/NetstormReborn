#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초saved-state자료는보존하고SEHcleanup의잘못쓴6BYTE표기만정적으로교정한다."""
import argparse
import json
from decomp_priestmenu import ROOT, digest
import pefile
import capstone

# 기존fixture/보고서/도구에쓰기없이별도근거만생성한다.
OUTPUT=ROOT/'cpppj/saved-state-read-cleanup-scope.json'


def main():
    """실제PE의lea6+jmp5와이미실행한범위11을대조한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(OUTPUT.read_text(encoding='utf-8'))
        for name,expected in record['sha256'].items():  # 최초관측자료도교정때와같이유지되어야한다.
            if digest(ROOT/name)!=expected:raise RuntimeError('SavedStatecleanup교정SHA불일치:'+name)
        print('SavedStatecleanup실제11BYTE확인');return
    if OUTPUT.exists():raise RuntimeError('SavedStatecleanup최초교정근거덮어쓰기금지')
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));raw=pe.get_data(0xfb5d0,11);instructions=list(capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32).disasm(raw,0x4fb5d0));assert [item.size for item in instructions]==[6,5] and instructions[-1].op_str=='0x41b410'
    names=('originals/Netstorm.exe','tools/decomp_saved_state_read.py','cpppj/saved-state-read-native.json','cpppj/tests/fixtures/saved-state-read-x86.tsv','tools/audit_saved_state_read_cleanup_size.py')
    record={'correction':'최초보고서entries와도구설명의cleanup6BYTE는표기오류다.실행범위는처음부터004fb5d0<=PC<004fb5db로11BYTE였으며원시관측은변경하지않았다.','entry':'004fb5d0','end_exclusive':'004fb5db','actual_bytes':11,'original_report_label':6,'machine_code':raw.hex(),'instructions':[{'address':f'{item.address:08x}','size':item.size,'operation':item.mnemonic+' '+item.op_str} for item in instructions],'native_observations_unchanged':542,'separate_cleanups_unchanged':24,'sha256':{name:digest(ROOT/name) for name in names}}
    OUTPUT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(record,ensure_ascii=False,indent=2))


if __name__=='__main__':main()
