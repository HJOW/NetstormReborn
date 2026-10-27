#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 OS 확대 출력과 별도 Win32 GDI8비트 좌표 표본을 대조한다. 원본 게임/파일은 변경하지 않는다."""
import argparse
import ctypes
from ctypes import wintypes
import json
import struct
from pathlib import Path
import numpy as np
from PIL import Image
gdi=ctypes.WinDLL('gdi32',use_last_error=True)  # 실제 Windows8비트 DIB 확대 표본을 별도 메모리 DC에서 읽는다.
gdi.CreateCompatibleDC.argtypes=[wintypes.HDC];gdi.CreateCompatibleDC.restype=wintypes.HDC
gdi.CreateDIBSection.argtypes=[wintypes.HDC,ctypes.c_void_p,wintypes.UINT,ctypes.POINTER(ctypes.c_void_p),wintypes.HANDLE,wintypes.DWORD];gdi.CreateDIBSection.restype=wintypes.HBITMAP
gdi.SelectObject.argtypes=[wintypes.HDC,wintypes.HGDIOBJ];gdi.SelectObject.restype=wintypes.HGDIOBJ
gdi.DeleteObject.argtypes=[wintypes.HGDIOBJ];gdi.DeleteDC.argtypes=[wintypes.HDC]
gdi.SetStretchBltMode.argtypes=[wintypes.HDC,ctypes.c_int]
gdi.StretchDIBits.argtypes=[wintypes.HDC,*([ctypes.c_int]*8),ctypes.c_void_p,ctypes.c_void_p,wintypes.UINT,wintypes.DWORD]

def header(width,height,bits):
    """원본과 같은 top-down DIB 머리를 생성하며 좌표값은 결과가 아닌 표본 입력이다."""
    return struct.pack('<IiiHHIIIIII',40,width,-height,1,bits,0,0,0,0,0,0)

def samples(width,height,output_width,output_height):
    """각 source 좌표의 하위/상위8비트를 네 실제 GDI 호출로 확대해 실제 샘플 선택을 복원한다."""
    dc=gdi.CreateCompatibleDC(None);pixels=ctypes.c_void_p();target=ctypes.create_string_buffer(header(output_width,output_height,32)+bytes(4));bitmap=gdi.CreateDIBSection(dc,target,0,ctypes.byref(pixels),None,0)
    if not dc or not bitmap: raise RuntimeError('독립 실제 GDI 표본 생성 실패')
    previous=gdi.SelectObject(dc,bitmap);result=[]
    try:
        gdi.SetStretchBltMode(dc,3);source_header=ctypes.create_string_buffer(header(width,height,8)+b''.join(struct.pack('<I',i*0x010101) for i in range(256)))
        source=np.zeros((height,(width+3)&~3),dtype=np.uint8)
        for axis,shift in ((0,0),(0,8),(1,0),(1,8)): # X/Y의 하위/상위8비트를 각각 별도 실제 GDI 호출로 확대한다.
            values=np.arange(width if axis==0 else height,dtype=np.uint16)>>shift
            source[:,:width]=(values[None,:] if axis==0 else values[:,None])&255
            returned=gdi.StretchDIBits(dc,0,0,output_width,output_height,0,0,width,height,source.ctypes.data,source_header,0,0x00cc0020)
            if not returned: raise RuntimeError('독립 실제 GDI 표본 확대 실패')
            gdi.GdiFlush();result.append((np.ctypeslib.as_array((ctypes.c_uint32*(output_width*output_height)).from_address(pixels.value)).reshape(output_height,output_width)&255).copy())
        x=result[0]+result[1]*256;y=result[2]+result[3]*256
        if not np.all(x==x[0:1,:]) or not np.all(y==y[:,0:1]): raise RuntimeError('실제 GDI 표본의 행/열 분리가 달라짐')
        return x[0,:],y[:,0]
    finally:
        gdi.SelectObject(dc,previous);gdi.DeleteObject(bitmap);gdi.DeleteDC(dc)

def main():
    """실제 캡처와 별도 메모리 GDI 좌표 표본을 읽고 결과 파일을 덮어쓰지 않고 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--captures',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--cases',nargs='+',required=True);args=parser.parse_args()
    rows=[]
    for name in args.cases: # 같은 프레임의 논리/실제 출력 두 캡처를 사례마다 독립 대조한다.
        if name.startswith('menu'): logical,physical='logical-full.bmp','output-full.bmp'
        elif name.startswith('stage'): logical,physical='logical-stage.bmp','output-stage.bmp'
        elif name.startswith(('wide','nine','ten')): logical,physical='logical.bmp','output.bmp'
        else: raise ValueError('실제 캡처 사례 이름 필요')
        root=args.captures/name;source=np.array(Image.open(root/logical).convert('RGB'));screen=np.array(Image.open(root/physical).convert('RGB'));source_height,source_width=source.shape[:2];screen_height,screen_width=screen.shape[:2]
        # 제품의 출력 보고값을 기대값으로 쓰지 않고 두 실제 이미지 크기에서 비율을 독립 계산한다.
        width,height=screen_width,screen_height
        if screen_width*source_height<=screen_height*source_width: height=max(1,screen_width*source_height//source_width)
        else: width=max(1,screen_height*source_width//source_height)
        left,top=(screen_width-width)//2,(screen_height-height)//2;right,bottom=left+width,top+height;actual=screen[top:bottom,left:right]
        x,y=samples(source_width,source_height,width,height);expected=source[y[:,None],x[None,:]];different=int(np.any(expected!=actual,axis=2).sum())
        outside=screen.copy();outside[top:bottom,left:right]=0;black=not np.any(outside)
        rows.append(dict(case=name,logicalSize=[source_width,source_height],physicalSize=[screen_width,screen_height],viewport=[left,top,right,bottom],differentPixels=different,viewportPixels=width*height,letterboxBlack=bool(black)))
    with args.output.open('x',encoding='utf-8') as stream: json.dump(rows,stream,indent=2);stream.write('\n')
    print(json.dumps(rows));return int(any(row['differentPixels'] or not row['letterboxBlack'] for row in rows))


if __name__=='__main__':
    raise SystemExit(main())
