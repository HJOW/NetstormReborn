#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""분리된 게임 자료 사본에서 실제 인트로 표시/스킵/자연 종료·PCM·메뉴 복귀를 검사한다."""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_world_smoke import states


def verify_frame(media, capture, index, output):
    """독립 FFmpeg의 원시 pal8에 원본 Screen의0/255 고정만 적용해 실제 화면 영상 영역 전체를 비교한다."""
    from PIL import Image
    ffmpeg=shutil.which('ffmpeg')
    if not ffmpeg: raise RuntimeError('영상 픽셀 독립 검증용 FFmpeg 필요')
    command=[ffmpeg,'-nostdin','-v','error','-i',str(media),'-vf',f'select=eq(n\\,{index})','-frames:v','1','-pix_fmt','pal8','-f','rawvideo','-']
    raw=subprocess.check_output(command,timeout=60);width,height=640,320
    if len(raw)!=width*height+1024: raise RuntimeError('독립 pal8 프레임 길이 불일치')
    # FFmpeg의 원래 BGRA256색을 RGB로 바꾸고 Screen의 두 고정색만 적용한다.
    colors=[bytes((raw[width*height+i*4+2],raw[width*height+i*4+1],raw[width*height+i*4])) for i in range(256)];colors[0]=bytes(3);colors[255]=bytes((255,255,255))
    # 영상의 모든 인덱스를 실제 RGB로 펼쳐 검정 구간 이후의 전체 픽셀을 비교한다.
    expected=b''.join(colors[index] for index in raw[:width*height]);assert index>=110 and len(set(expected))>8,'검정 화면만의 약한 비교 금지';image=Image.open(capture).convert('RGB');left=(image.width-width)//2;top=(image.height-height)//2;crop=image.crop((left,top,left+width,top+height));crop.save(output/'movie-crop.png');actual=crop.tobytes()
    result={'frameIndex':index,'same':actual==expected,'actualSha256':hashlib.sha256(actual).hexdigest(),'expectedSha256':hashlib.sha256(expected).hexdigest(),'command':command,'videoRect':[left,top,width,height]};save(output/'pixel-comparison.json',result)
    if actual!=expected: raise AssertionError('실제 인트로 영역과 독립 FFmpeg 프레임 불일치')
    return result


def verify_presentation(logical, physical, scene, output):
    """실제 창 DC의 전체 RGB/레터박스를 논리 화면의 픽셀 중심 최근접 출력과 구별해 기록한다."""
    from PIL import Image
    source=Image.open(logical).convert('RGB');actual=Image.open(physical).convert('RGB');left,top,right,bottom=map(int,scene['screen_output']);width,height=right-left,bottom-top;raw=source.tobytes();rows={};columns=[(2*x+1)*source.width//(2*width) for x in range(width)]
    # 실제 호환 출력과 같은 정수 표본 좌표로 각 원래 행을 한 번만 펼친다.
    for y in set((2*y+1)*source.height//(2*height) for y in range(height)):
        row=raw[y*source.width*3:(y+1)*source.width*3];rows[y]=b''.join(row[x*3:x*3+3] for x in columns)
    scaled=b''.join(rows[(2*y+1)*source.height//(2*height)] for y in range(height));expected=Image.new('RGB',actual.size);expected.paste(Image.frombytes('RGB',(width,height),scaled),(left,top));result={'same':actual.tobytes()==expected.tobytes(),'actualSha256':hashlib.sha256(actual.tobytes()).hexdigest(),'expectedSha256':hashlib.sha256(expected.tobytes()).hexdigest(),'dimensions':list(actual.size),'outputRect':[left,top,right,bottom]};save(output/'presentation-comparison.json',result);return result


def main():
    """실제 입력만 보내고 원시 보고·캡처·프로세스/PID·실패를 새 경로에 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--game-dir',type=Path,required=True);parser.add_argument('--binary',type=Path,default=GAME);parser.add_argument('--intro',type=Path,default=ROOT/'originalCD/MOVIE/englishintro2x.smk');parser.add_argument('--natural',action='store_true');parser.add_argument('--skip',choices=('esc','space','left','right'),default='esc');parser.add_argument('--width',type=int,default=1024);parser.add_argument('--height',type=int,default=768);parser.add_argument('--fullscreen',action='store_true');parser.add_argument('--audio-mute',action='store_true')
    parser.add_argument('--expect-audio-unavailable',action='store_true',help='실제 출력 장치0개인 환경에서 실패 후 영상/메뉴 복귀만 별도 검증');args=parser.parse_args()
    data=args.game_dir.resolve();binary=args.binary.resolve();media=args.intro.resolve();out=args.output.resolve();device_count=None
    if args.expect_audio_unavailable:
        import ctypes
        if not args.audio_mute: parser.error('--expect-audio-unavailable은 --audio-mute와 함께 사용해야 합니다')
        device_count=ctypes.WinDLL('winmm').waveOutGetNumDevs()
        if device_count!=0: raise RuntimeError('실제 출력 장치가 존재하므로 장치 부재 기대값을 사용할 수 없습니다')
    for protected in (ROOT/'originals',ROOT/'originalCD'):  # 보호 트리와 그 아래를 실제 실행 데이터로 사용하지 않는다.
        if data.is_relative_to(protected.resolve()): raise ValueError('영상 실행 검사는 보호 자료와 분리한 사본이 필요합니다')
    out.mkdir(parents=True,exist_ok=False);last=4500 if args.natural else 700
    steps=[(520,'report','playing'),(520,'snapshot',str(out/'playing.bmp')),(520,'snapshot-output',str(out/'playing-output.bmp'))]
    if not args.natural:
        operation,argument={'esc':('esc',''),'space':('key',' '),'left':('click','screen:700,600'),'right':('right','screen:700,600')}[args.skip];steps.append((580,operation,argument))
    steps += [(last-50,'click-if','OK'),(last-45,'click-if','Main Menu'),(last-20,'report','final'),(last-20,'snapshot',str(out/'final.bmp'))]
    script=out/'steps.tsv';script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    settings=f'SCREENW={args.width};SCREENH={args.height};maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0;intro=0;introMovie={media.as_posix()};WelcomeGiven=1;userUpdated=1;startInFullScreen='+('1' if args.fullscreen else '0')
    command=[str(binary),'--run',str(data),'--frames',str(last),'--set',settings,'--ui-script',str(script),'--ui-report',str(out/'states.tsv')]
    if not args.fullscreen: command.append('--window')
    command.append('--mute-audio' if args.audio_mute else '--no-audio')
    if args.audio_mute: os.environ['NETSTORM_CPP_NO_AUDIO']='0'  # 공용 무음 검사 모듈의 기본 장치 비활성화를 이 명시 PCM 검사 프로세스에서만 해제한다.
    if args.audio_mute: command.extend(['--audio-report',str(out/'audio-final.tsv')])
    run=execute(command,out,binary,300 if args.natural else 90);save(out/'run.json',run)
    assert run['returnCode']==0 and not run['timedOut'],run
    observed=states(out/'states.tsv');playing=observed['playing'];final=observed['final'];stream=playing['movie_stream']
    assert int(playing['movie_runtime'][0])!=0 and int(stream[2])>0 and stream[3:5]==['1','0'],playing.get('movie_runtime')
    assert playing['movie_update_depth']==['1'] and playing['paused']==['1']
    if args.audio_mute and not args.expect_audio_unavailable: assert int(stream[5])>0,'실제 영상 PCM 장치 수락 없음'
    else: assert stream[5]=='0'
    assert final['movie_runtime'][:3]==['0','0','1'] and final['movie_update_depth']==['0'] and final['paused']==['0'] and final['phase']==['0'] and final['page']==['main'],{key:final.get(key) for key in ('movie_runtime','movie_update_depth','paused','phase','page')}
    for scene in (playing,final):  # 실제 Screen의 전체화면 비트/논리 해상도가 입력과 같은지 확인한다.
        assert bool(int(scene['screen_mode'][0])&1)==args.fullscreen and scene['screen_mode'][1:3]==[str(args.width),str(args.height)],scene['screen_mode']
    pixels=verify_frame(media,out/'playing.bmp',int(stream[2])-1,out)
    presentation=verify_presentation(out/'playing.bmp',out/'playing-output.bmp',playing,out)
    if args.natural: assert final['movie_runtime'][3]=='1889','원문 마지막 프레임 관문 차이'
    if args.audio_mute:
        audio=dict(line.split('\t',1) for line in (out/'audio-final.tsv').read_text(encoding='utf-8').splitlines());ready='0' if args.expect_audio_unavailable else '1';assert audio['deviceReady']==ready and audio['soundEnabled']==ready and audio['musicRuntime']==ready and audio['muteDepth']=='1',audio
    intro=subprocess.check_output([str(binary),'--config-get',str(data),'intro'],cwd=ROOT,text=True,encoding='utf-8');assert intro.strip()=='1','인트로 완료 표식의 디스크 저장 실패'
    # userUpdated는 설정 키가 아닌 실제 사용자 상태다. 원문 첫 메뉴 안내를 실제 버튼 입력으로 닫는다.
    restart=out/'restart';restart.mkdir();restart_plan=restart/'steps.tsv';restart_plan.write_text('3\tclick-if\tOK\n5\tclick-if\tOK\n40\treport\tmain\n',encoding='utf-8',newline='\n')
    restart_command=[str(binary),'--run',str(data),'--frames','55','--no-audio','--set','WelcomeGiven=1;userUpdated=1;autoDemo=0;tellTips=0;sleepPerLoop=25','--ui-script',str(restart_plan),'--ui-report',str(restart/'states.tsv')]
    restart_run=execute(restart_command,restart,binary,60);save(restart/'run.json',restart_run);assert restart_run['returnCode']==0 and not restart_run['timedOut'];main_state=states(restart/'states.tsv')['main'];assert main_state['phase']==['0'] and main_state['page']==['main'] and main_state['paused']==['0'] and 'movie_runtime' not in main_state,{key:main_state.get(key) for key in ('phase','page','paused','movie_runtime')}
    summary={'passed':True,'logicalPlaybackAndLifecyclePassed':True,'physicalOutputVerified':presentation['same'],'presentation':presentation,'gameSha256':sha(binary),'mediaSha256':sha(media),'natural':args.natural,'skip':None if args.natural else args.skip,'fullscreen':args.fullscreen,'audioMute':args.audio_mute,'audioDeviceCount':device_count,'audioFallbackVerified':args.expect_audio_unavailable,'pcmDevicePlaybackVerified':args.audio_mute and not args.expect_audio_unavailable,'dimensions':[args.width,args.height],'run':run,'restart':restart_run,'playing':{k:playing.get(k) for k in ('movie_runtime','movie_stream','movie_update_depth','paused')},'final':{k:final.get(k) for k in ('movie_runtime','movie_update_depth','paused','phase','page')},'pixels':pixels,'scope':'실제 인트로/완료 저장/재시작만 실행. 포획/희생/공격/승리·멀티플레이 실행 없음.'};save(out/'summary.json',summary);print(json.dumps({'passed':True,'seconds':run['seconds'],'natural':args.natural,'audioMute':args.audio_mute,'pcmDevicePlaybackVerified':summary['pcmDevicePlaybackVerified'],'physicalOutputVerified':presentation['same']},ensure_ascii=False))


if __name__=='__main__': main()
