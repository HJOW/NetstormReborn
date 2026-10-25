"""실제 Screen의 구름 픽셀 장치를 명시 검사하며 자연 사용자 플레이와 구별한다."""
import argparse,json
from pathlib import Path
from cpp_bridge_ritual_runtime import execute,save
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots

ROOT=Path(__file__).resolve().parents[1]  # 같은 실제 제품과 보호 원본의 작업 공간이다.
EXE=ROOT/'cpppj/build/bin/Release/netstorm_cloud_render_probe.exe'  # 일반 CTest에서 창을 열지 않는 별도 장치 도구다.


def main():
    """설정을 복구하고 실제 종료·픽셀 비교·행/clip/같은 cache와 난수를 기록한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();directory=args.output.resolve();directory.mkdir(parents=True,exist_ok=False)
    before=snapshots(ROOT/'originals');save(directory/'originals-before.json',before);result=dict(completed=False,passed=False,apiDeviceProbe=True,userPlayVerified=False,goalComplete=False)
    try:
        with preserve_game_settings(ROOT/'originals','d',directory/'settings-backup'):
            result.update(execute([str(EXE),str(ROOT/'originals'),str(directory)],directory,EXE,90))
    finally:
        after=snapshots(ROOT/'originals');save(directory/'originals-after.json',after);result['originalsUnchanged']=before==after;save(directory/'execution.json',result)
    assert result['returnCode']==0 and not result['timedOut'] and result['originalsUnchanged']
    rows={}
    for line in (directory/'device.tsv').read_text('utf-8').splitlines():  # 실제 각 API 호출 뒤 같은 현재 표면과 전역이다.
        fields=line.split('\t');assert len(fields)==11;rows[fields[0]]=[int(value) for value in fields[1:]]
    assert list(rows)==['built','full','drained','partial','cleaned','zero','alternate-prefix','resized','finished']
    assert rows['built'][0] and rows['full'][1:6]==[0,1,1,0,0] and rows['drained'][1:6]==[0,0,0,0,0]
    assert rows['partial'][1:6]==[2,0,0,64,64] and rows['cleaned'][1:6]==[2,0,0,0,0]
    assert rows['cleaned']==rows['zero']==rows['alternate-prefix'] and rows['resized'][8:]==[800,600]
    assert rows['resized'][1:6]==[2,1,1,0,0] and rows['finished'][1:6]==[2,0,0,0,0]
    assert len({row[0] for row in rows.values()})==1 and len({row[6] for row in rows.values()})==1
    assert all((directory/name).is_file() for name in ['full.bmp','partial.bmp','resized.bmp'])
    result.update(completed=True,passed=True);save(directory/'execution.json',result);save(directory/'results.json',dict(passed=True,apiDeviceProbe=True,fullPixelComparison=True,userPlayVerified=False,goalComplete=False,binarySha256=result['binarySha256'],device=rows));print(json.dumps(dict(passed=True,steps=len(rows),originalsUnchanged=True,userPlayVerified=False)))


if __name__=='__main__':main()
