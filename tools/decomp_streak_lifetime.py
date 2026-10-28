#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 마나12의 실제458680 생성자를 격리 실행한다. 게임/창 실행은 없다."""
import csv
import json
import struct
from pathlib import Path
from decomp_fierytail_lifetime import LifetimeOracle, ROOT, OBJECT, PARTICLES, POOL, TABLE, MERGED, semantic, slots
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP


class StreakOracle(LifetimeOracle):
    """공통 원본 등록/입자/타이머 초기화는 유지하고 실제 Host 부착만12로 구별한다."""

    def __init__(self):
        """기존 허용 범위에 실제 마나 생성자와 공유 빔 팔레트 조회만 추가한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in (0x458680,0x458480):
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """원본 BaseProcess의 부착 인자를 관찰하고 외부 Form 번호만 공급한다."""
        if address==0x41bd20:
            values=struct.unpack('<4I',mu.mem_read(mu.reg_read(UC_X86_REG_ESP)+4,16))
            assert mu.reg_read(UC_X86_REG_ECX)==OBJECT and values==(12,self.parent,0,16)
            self.events.append({'attach':list(values)})
            mu.mem_write(OBJECT+4,struct.pack('<2I',7,51 if self.mutate else self.parent))
            self.ret(OBJECT,16)
            return
        super().instruction(mu,address,size,data)

    def observe(self,parent,mutate,control):
        """입력만 준비한 뒤 원본 필드/120입자/등록/공유 owner 보존을 수집한다."""
        self.parent,self.mutate,self.capacity=parent,mutate,40
        mu=self.mu
        mu.mem_write(OBJECT,b'\xa5'*0x68)
        mu.mem_write(PARTICLES,b'\x5a'*2880)
        mu.mem_write(POOL,bytes(2600))
        mu.mem_write(POOL+50*50+35,bytes([255]))
        mu.mem_write(POOL+51*50+35,bytes([127]))
        mu.mem_write(0x5c8464,struct.pack('<I',POOL))
        mu.mem_write(0x5410a0,struct.pack('<I',12))
        mu.mem_write(0x55a758,struct.pack('<I',MERGED))
        mu.mem_write(0x55a750,struct.pack('<I',0x12345678))
        mu.mem_write(MERGED,struct.pack('<2I',11,MERGED+0x1000))
        mu.mem_write(0x595138,struct.pack('<2I',TABLE,40))
        mu.mem_write(TABLE,struct.pack('<40I',*slots(40,20)))
        self.events=[]
        self.call(0x458680,OBJECT,struct.pack('<I',parent),4,control,OBJECT)
        raw=bytes(mu.mem_read(OBJECT,0x68))
        assert bytes(mu.mem_read(PARTICLES,2880))==bytes(2880)
        assert self.integer(0x55a758)==MERGED and self.integer(0x55a750)==0x12345678
        assert struct.unpack_from('<i',raw,0x20)[0]==-20
        assert struct.unpack_from('<i',raw,0x54)[0]==2 and struct.unpack_from('<i',raw,0x60)[0]==-8
        return semantic(raw).hex()


def main():
    """두 x87 정밀도와 부모/부착 중 변경을 교차하고 기존 fixture는 변경하지 않는다."""
    rows=[]
    for parent in (0,50):
        for mutate in (0,1):
            pair=[]
            for control in (0x027f,0x037f):
                oracle=StreakOracle()
                observed=oracle.observe(parent,mutate,control)
                pair.append(observed)
                rows.append((parent,mutate,hex(control),observed))
            assert pair[0]==pair[1]
    target=ROOT/'cpppj/tests/fixtures/streak-lifetime-x86.tsv'
    target.write_text('# parent mutate cw nativeFields\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf8',newline='\n')
    print(json.dumps({'observations':len(rows),'precisionDifferences':0,'gameExecutions':0,'fixture':str(target)},ensure_ascii=False))


if __name__=='__main__':
    main()
