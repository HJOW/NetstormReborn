#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""분리 자산의 실제 클론 창에 Win32 포인터/버튼/Shift를 보내 가장자리·메뉴·DIB 이동을 짧게 관찰한다."""
import argparse
import ctypes
from ctypes import wintypes
import json
import platform
import shutil
import subprocess
import time
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, sha, save
from cpp_world_smoke import parse, states

USER=ctypes.WinDLL('user32',use_last_error=True)  # 이 도구가 시작한 클론 PID의 창만 제어한다.
CALLBACK=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)  # 현재 Windows 포인터 폭을 보존한다.
USER.EnumWindows.argtypes=[CALLBACK,wintypes.LPARAM]
USER.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
USER.SendMessageW.argtypes=[wintypes.HWND,wintypes.UINT,wintypes.WPARAM,wintypes.LPARAM]
USER.SendMessageW.restype=wintypes.LPARAM
USER.ClientToScreen.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.POINT)]
USER.SetForegroundWindow.argtypes=[wintypes.HWND]
USER.SetProcessDPIAware()


def button(scene,label):
    """현재 보고의 실제 표시 버튼 사각형 중앙을 읽고 없으면 반환하지 않는다."""
    rows=[row for group in scene.get('html_buttons',{}).values() for row in group if row[-1]==label and row[-2]=='1']
    if rows:
        left,top,right,bottom=map(int,rows[0][5:9])
        return ((left+right)//2,(top+bottom)//2)
    if label in scene.get('controls',{}):
        left,top,right,bottom=map(int,scene['controls'][label][2:6])
        return ((left+right)//2,(top+bottom)//2)
    return None


def pointer(window,scene,x,y):
    """논리 가장자리를 현재 실제 출력에 투영해 OS 포인터를 옮긴다. 폴링 공급자를 덮어쓰지 않는다."""
    left,top,right,bottom=map(int,scene['screen_output']);width,height=map(int,scene['size'])
    px=right-1 if x==width-1 else left+x*(right-left)//width
    py=bottom-1 if y==height-1 else top+y*(bottom-top)//height
    point=wintypes.POINT(px,py);assert USER.ClientToScreen(window,ctypes.byref(point));assert USER.SetCursorPos(point.x,point.y)
    return [px,py,point.x,point.y]


def main():
    """실제 입력·최초 실패/종료와 소스 실행 파일 해시를 새 디렉터리에 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--case',choices=('full','disabled','window','wide','ten','soft'),required=True)
    args=parser.parse_args();root=args.output.resolve();root.mkdir(parents=True,exist_ok=True);directory=root/args.case;directory.mkdir(exist_ok=False);data=root/'game-data'
    if not data.exists():
        shutil.copytree(ROOT/'originals',data)
    long_case=args.case=='full'
    schedule=[(5,'boot1'),(12,'boot2'),(22,'briefing'),(35,'stage'),(40,'baseline'),(45,'right-start'),(60,'right-first'),(75,'right-last')]
    if long_case:
        schedule += [(80,'center-start'),(88,'center-first'),(95,'center-last'),(100,'hold-start'),(115,'held'),(125,'hold-end'),(135,'menu-start'),(140,'menu-open'),(142,'top-start'),(160,'top-blocked'),(165,'menu-end'),(180,'top-free'),(185,'shift-start'),(202,'shift-end'),(220,'diagonal'),(225,'end-center'),(230,'final')]
    script=directory/'steps.tsv';script.write_text(''.join(f'{frame}\treport\t{"camera:" if frame>=35 else ""}{label}\n' for frame,label in schedule)+f'{230 if long_case else 80}\tsnapshot-output\t{directory/"output.bmp"}\n',encoding='utf-8',newline='\n')
    # 관측 프레임/캡처는 같은 프레임 번호 순서로 기록한다. 실제 입력은 아래 Win32 경로만 쓴다.
    rows=script.read_text(encoding='utf-8').splitlines();rows.sort(key=lambda row:int(row.split('\t')[0]));script.write_text('\n'.join(rows)+'\n',encoding='utf-8')
    dimensions={'wide':(1366,768),'ten':(1280,800)}.get(args.case,(1024,768));width,height=dimensions
    setting=f'SCREENW={width};SCREENH={height};startInFullScreen=1;workingFullScreenFlags={111 if args.case=="soft" else 11};maxFPS=20;sleepPerLoop=1;autoDemo=0;tellTips=0;edgeScroll={0 if args.case=="disabled" else 1};edgeScrollSpeed=20;currentLanguage=english'
    command=[str(GAME),'--run',str(data),'--mission','TheWarBegins','--no-audio','--frames',str(240 if long_case else 90),'--set',setting,'--ui-script',str(script),'--ui-report',str(directory/'states.tsv')]
    if args.case=='window':
        command.append('--window')
    events=[];error=None;windows=[];seen=set();deadline=time.monotonic()+100;held=False;shift=False;started=time.monotonic()
    startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=1 # 실제 창 모드도 표시하여 자기 PID의 보이는 창을 찾는다.
    with (directory/'stdout.log').open('xb') as stdout,(directory/'stderr.log').open('xb') as stderr:
        process=subprocess.Popen(command,cwd=ROOT,stdout=stdout,stderr=stderr,startupinfo=startup);save(directory/'running.json',dict(pid=process.pid,command=command,binarySha256=sha(GAME)))
        @CALLBACK
        def find(window,parameter):
            """자신이 생성한 실제 PID의 표시 창만 찾는다."""
            owner=wintypes.DWORD();USER.GetWindowThreadProcessId(window,ctypes.byref(owner))
            if owner.value==process.pid and USER.IsWindowVisible(window):
                windows.append(window)
            return True
        try:
            while process.poll() is None and time.monotonic()<deadline:  # 실제 프로세스 핸들이 살아 있는 동안 새 관측만 처리한다.
                if not windows:
                    USER.EnumWindows(find,0)
                report=directory/'states.tsv'
                if not windows or not report.exists():
                    time.sleep(.02);continue
                try:
                    text=report.read_text(encoding='utf-8');last=text.rsplit('\nframe\t',1)[-1]
                    if not last.startswith('frame\t'):
                        last='frame\t'+last
                    header,body=last.split('\n',1);label=header.split('\t')[2].removeprefix('camera:')
                    if label in seen:
                        time.sleep(.02);continue
                    scene=parse(body)
                    if not all(key in scene for key in ('screen_output','size','raw_camera_frame','page')):
                        time.sleep(.02);continue
                except (FileNotFoundError,ValueError):  # 같은 프로세스가 보고서를 쓰는 도중이면 완료된 같은 파일을 다시 읽는다.
                    time.sleep(.02);continue
                window=windows[0];seen.add(label);USER.SetForegroundWindow(window)
                action=None
                if label in ('boot1','boot2','briefing','stage'):
                    for caption in ('OK','Play Mission'):  # 필요한 실제 안내 확인과 Play Mission만 클릭한다.
                        point=button(scene,caption)
                        if point is None:
                            continue
                        actual=pointer(window,scene,*point);position=(actual[1]&65535)<<16|(actual[0]&65535)
                        USER.SendMessageW(window,0x201,1,position);USER.SendMessageW(window,0x202,0,position);action=dict(click=caption,logical=point,actual=actual);break
                elif label=='right-start':
                    action=dict(pointer=pointer(window,scene,width-1,height//2))
                elif label in ('center-start','hold-end','end-center'):
                    if held:
                        USER.mouse_event(4,0,0,0,0);held=False
                    action=dict(pointer=pointer(window,scene,width//2,height//2))
                elif label=='hold-start':
                    pointer(window,scene,width//2,height//2);USER.mouse_event(2,0,0,0,0);held=True;action=dict(pointer=pointer(window,scene,width-1,height//2),leftHeld=True)
                elif label in ('menu-start','menu-end'):
                    USER.SendMessageW(window,0x102,27,0);action=dict(escape=True)
                elif label=='top-start':
                    action=dict(pointer=pointer(window,scene,width//2,0))
                elif label=='shift-start':
                    USER.keybd_event(0x10,0,0,0);shift=True;action=dict(pointer=pointer(window,scene,width-1,height-1),shiftHeld=True)
                elif label=='shift-end':
                    USER.keybd_event(0x10,0,2,0);shift=False;action=dict(shiftHeld=False)
                if action:
                    events.append(dict(label=label,action=action))
            if process.poll() is None:
                raise TimeoutError('실제 가장자리 검사 종료 대기 초과')
        except Exception as problem:
            error=repr(problem)
        finally:
            if held:
                USER.mouse_event(4,0,0,0,0)
            if shift:
                USER.keybd_event(0x10,0,2,0)
            if process.poll() is None:
                process.terminate()
            code=process.wait(timeout=15)
    observed={label.removeprefix('camera:'):scene for label,scene in states(directory/'states.tsv').items()} if (directory/'states.tsv').exists() else {}
    missing=sorted({label for _,label in schedule}-seen)
    if missing and error is None:
        error='관측 시점을 놓친 입력 도구: '+','.join(missing)
    clip=wintypes.RECT();assert USER.GetClipCursor(ctypes.byref(clip)) # 게임 종료 뒤 실제 OS 포인터 제한의 해제 상태만 관측한다.
    result=dict(command=command,binarySha256=sha(GAME),returnCode=code,error=error,events=events,seen=sorted(seen),os=platform.platform(),host=platform.node(),stageCompletion=False,elapsedSeconds=time.monotonic()-started,exitCursorClip=[clip.left,clip.top,clip.right,clip.bottom],
        camera={name:scene.get('raw_camera_frame') for name,scene in observed.items()},pages={name:scene.get('page') for name,scene in observed.items()})
    save(directory/'result.json',result);print(json.dumps(result,ensure_ascii=False))
    return code or int(error is not None)


if __name__=='__main__':
    raise SystemExit(main())
