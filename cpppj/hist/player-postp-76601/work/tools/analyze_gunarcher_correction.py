#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Archer 전체 두 checkpoint와 방향90/실제반올림/서버 송신 교정을 연결한다."""
import argparse
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT, digest

# 최초1690행/전체 보고서·source/test/oracle를 수정 전에 보존한 경로다.
BEFORE=ROOT/'cpppj/hist/gunarcher-initial'
CHECKPOINT=ROOT/'cpppj/hist/gunarcher-ai-fdb97'
REPORT=ROOT/'cpppj/gunarcher-correction.json'
CPP=ROOT/'cpppj/src/o/RawArcherAnimation.cpp'
TEST=ROOT/'cpppj/tests/ArcherAnimationTests.cpp'
# 검증 가능한 현재 source 두 수정 및 설명과 입력 수정의 역변환이다.
CHANGES=(
    ('// target0 방향 BYTE는 실제 0050afd8/CD00505354의 90도 계수로 변환한다.\nconstexpr float kAimAngleScale=90.0f;\n',''),
    ('angle=hooks_.aimDirection(fields.Parent())*kAimAngleScale;','angle=hooks_.aimDirection(fields.Parent())*45.0f;'),
)
# 두 번째 수정은 원본 helper의 float0.5 가산과 그 함수 설명뿐이다.
SECOND_CPP=(
    ('// 원본firehelper의fmod결과를float에저장한뒤실제0.5f 가산/절삭과sprung offset2/frame16을유지한다.','// 원본firehelper의fmod결과와16방향 sprung offset2/frame16을유지한다.'),
    ('static_cast<std::int32_t>(value+0.5f)','static_cast<std::int32_t>(value)'),
)
# 실제 Host 검사만 서버 풀 및 GunProcess의 기존 writer를 사용하도록 변경한다.
OLD_HOST='SquidProcessHost host(pool,types,kernel,destroy,hostState);RawGunProcess gunLogic'
NEW_HOST='''
        // 실제 GunProcess writer의 패킷을 보존하며 로컬 Archer Form은 원본0x10 부착을 사용한다.
        std::array<std::uint8_t,64> packet{};std::vector<std::vector<std::uint8_t>> packets;ProcessFormTransmission transmit;
        transmit.writeData=[&](SquidProcess& process,bool parentChanged) {
            if (auto* gun=dynamic_cast<GunProcess*>(&process)) return static_cast<std::int32_t>(gun->WritePacket(packet,parentChanged));
            throw std::logic_error("Archer 검사 실제 GunProcess writer 타입 오류");
        };
        transmit.transmit=[&](std::int32_t size) { packets.emplace_back(packet.begin(),packet.begin()+size); };
        SquidProcessHost host(pool,types,kernel,destroy,hostState,{}, {},transmit);RawGunProcess gunLogic'''
SECOND_TEST=(
    ('// 실제 서버 풀과 GunProcess 패킷 송신에서 두 판본의 Form/부모 연결과 Kill을 방문한다.','// 서버 송신과 분리한 실제 풀에서 두 판본의 Form/부모 연결과 Kill을 방문한다.'),
    ('SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types','SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types'),
    (NEW_HOST,OLD_HOST),
    ('CHECK(!packets.empty() && destroy.PreDepth()==0 && destroy.PostDepth()==0);','CHECK(destroy.PreDepth()==0 && destroy.PostDepth()==0);'),
)


def reverse_changes(value,changes):
    """명시된 각 교정을 한 번씩만 역변환해 실제 이전 바이트와 비교한다."""
    for now,old in changes:
        if value.count(now)!=1:raise RuntimeError('Archer 교정 분기 누락 '+now[:100])
        value=value.replace(now,old,1)
    return value


def rounding_evidence():
    """보호 PE의 실제 helper 명령과 float0.5 상수를 읽으며 원본 파일을 수정하지 않는다."""
    from decomp_gunarcher_oracle import ArcherOracle
    result={}
    for edition,address,constant,length,expected in (
        ('originals',0x43fa50,0x500ee4,15,'d9442404d805e40e5000e9614f0a00'),
        ('originalCD',0x48ee10,0x504654,15,'d9442404d80554465000e9fd270600'),
    ):
        oracle=ArcherOracle(edition);code=bytes(oracle.mu.mem_read(address,length)).hex();raw=bytes(oracle.mu.mem_read(constant,4))
        if code!=expected or raw.hex()!='0000003f':raise RuntimeError('Archer 실제 round helper 바이트 불일치')
        result[edition]=dict(address=f'{address:08x}',bytes=code,constant_address=f'{constant:08x}',constant_bytes=raw.hex(),value=struct.unpack('<f',raw)[0])
    return result


def audit():
    """이전 전체 SHA를 실제 snapshot 또는 불변 파일과 검사하고 명시 수정만 허용한다."""
    # 두 실제 전체 보고서의 당시 source/test/oracle/fixture SHA를 각각 검사한다.
    for snapshot in (BEFORE,CHECKPOINT):
        previous=json.loads((snapshot/'gunarcher.json').read_text(encoding='utf-8'))
        for name,expected in previous['files'].items():
            saved=snapshot/Path(name).name;path=saved if saved.exists() else ROOT/name
            if digest(path)!=expected:raise RuntimeError('Archer 이전 근거 SHA 불일치 '+name)
        if (snapshot/'gunarcher-x86.tsv').read_bytes()!=(ROOT/'cpppj/tests/fixtures/gunarcher-x86.tsv').read_bytes():raise RuntimeError('Archer 같은1690개 fixture 바이트 변경')
    current=CPP.read_text(encoding='utf-8')
    checkpoint_cpp=reverse_changes(current,SECOND_CPP)
    if checkpoint_cpp!=(CHECKPOINT/CPP.name).read_text(encoding='utf-8'):raise RuntimeError('Archer round 교정 외 source 변경')
    if reverse_changes(checkpoint_cpp,CHANGES)!=(BEFORE/CPP.name).read_text(encoding='utf-8'):raise RuntimeError('Archer 명시 계수 외 source 변경')
    current_test=TEST.read_text(encoding='utf-8')
    checkpoint_test=reverse_changes(current_test,SECOND_TEST)
    if checkpoint_test!=(CHECKPOINT/TEST.name).read_text(encoding='utf-8'):raise RuntimeError('Archer server/writer 교정 외 test 변경')
    if checkpoint_test.count('types[71].group=0;')!=1 or checkpoint_test.replace('types[71].group=0;','',1)!=(BEFORE/TEST.name).read_text(encoding='utf-8'):raise RuntimeError('Archer Host group 입력 외 test 변경')
    return rounding_evidence()


def generate():
    """현재 source/report/로그와 전체 초기 snapshot을 새 관찰0 교정 근거로 저장한다."""
    native= audit();paths=[Path(__file__),CPP,TEST,ROOT/'cpppj/gunarcher.json',ROOT/'cpppj/tests/fixtures/gunarcher-x86.tsv',ROOT/'extracted/run8-ArcherAnimation_-first.log',ROOT/'extracted/run8-ArcherAnimation_fixed.log',*[path for snapshot in (BEFORE,CHECKPOINT) for path in snapshot.iterdir() if path.is_file()]]
    REPORT.write_text(json.dumps(dict(schema=2,host='HJOW-Athlon',native_rows=1690,normal=1688,prefix=2,new_unique_rows=0,changes=['실제 target0 방향 계수90을 적용(초안45 교정)','실제Host 검사 입력 type71.group=0 공급','실제 helper float0.5 가산 후 절삭 적용','실제 Host 서버 풀 및 GunProcess.WritePacket 송신 연결'],initial_full_snapshot=True,aim_corrected_full_snapshot=True,rounding_pe=native,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(verified=True,native_rows=1690,new_unique_rows=0),ensure_ascii=False))


def verify():
    """전체 초기 snapshot/명시 수정/현재 SHA와 동일 입력을 다시 감사한다."""
    native=audit();report=json.loads(REPORT.read_text(encoding='utf-8'))
    if report['rounding_pe']!=native:raise RuntimeError('Archer round 실제 PE 관찰 변경')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('Archer 교정 SHA 불일치 '+name)
    print(json.dumps(dict(verified=True,native_rows=1690,new_unique_rows=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
