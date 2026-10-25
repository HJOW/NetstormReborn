"""실제 Client/Screen의 팔레트 장치를 명시 실행하며 자연 사용자 플레이와 구별한다."""
import argparse
import json
from pathlib import Path
from cpp_bridge_ritual_runtime import execute,save
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots

ROOT=Path(__file__).resolve().parents[1]  # 현재 빌드한 실제 검사 장치와 보호 원본 위치다.
EXE=ROOT/'cpppj/build/bin/Release/netstorm_cloud_probe.exe'  # 일반 CTest에서 실행하지 않는 실제 Win32 장치 검사다.


def main():
    """설정 원시를 복원하고 실제 단계/제품/종료와 장치·캐시 수명을 기록한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();directory=args.output.resolve();directory.mkdir(exist_ok=False,parents=True)
    before=snapshots(ROOT/'originals');save(directory/'originals-before.json',before);result=dict(completed=False,passed=False,apiDeviceProbe=True,userPlayVerified=False,goalComplete=False)
    try:
        with preserve_game_settings(ROOT/'originals','d',directory/'settings-backup'):
            result.update(execute([str(EXE),str(ROOT/'originals'),str(directory/'device.tsv')],directory,EXE,90))
    finally:
        after=snapshots(ROOT/'originals');save(directory/'originals-after.json',after);result['originalsUnchanged']=before==after;save(directory/'execution.json',result)
    assert result['returnCode']==0 and not result['timedOut'] and result['originalsUnchanged']
    rows={}
    for line in (directory/'device.tsv').read_text('utf-8').splitlines():  # 실제 API 호출 뒤의 같은 Client/Screen 전역을 읽는다.
        fields=line.split('\t');assert len(fields)==12;rows[fields[0]]=[int(value) for value in fields[1:]]
    assert list(rows)==['ready','first','same','second','resolution','third','finished']
    assert rows['ready'][0]==0 and rows['ready'][3]==0 and rows['ready'][4]!=0
    assert rows['first']==rows['same'] and rows['first'][0] and rows['first'][2]==2 and rows['first'][3]==1
    assert rows['second'][0]!=rows['first'][0] and rows['second'][2]==4 and rows['second'][3]==1
    assert rows['resolution'][:4]==rows['second'][:4] and rows['resolution'][4]!=rows['second'][4] and rows['resolution'][6]==rows['second'][6] and rows['resolution'][7:10]==[800,600,1]
    assert rows['third'][0]!=rows['second'][0] and rows['third'][2]==6 and rows['third'][3]==1 and rows['third'][9]==1
    result.update(completed=True,passed=True);save(directory/'execution.json',result);save(directory/'results.json',dict(passed=True,apiDeviceProbe=True,userPlayVerified=False,goalComplete=False,binarySha256=result['binarySha256'],device=rows));print(json.dumps(dict(passed=True,steps=len(rows),originalsUnchanged=True,userPlayVerified=False)))


if __name__=='__main__':
    main()
