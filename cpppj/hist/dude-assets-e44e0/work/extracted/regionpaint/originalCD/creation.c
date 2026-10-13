// ==== FUN_004bd2d0 @ 004bd2d0

void __thiscall FUN_004bd2d0(void *this,int param_2,int param_3,undefined4 param_4)

{
  uint uVar1;
  
  *(undefined4 *)((int)this + 4) = 0x43808000;
  *(undefined4 *)((int)this + 8) = 0x43808000;
  *(undefined4 *)((int)this + 0xc) = 0xbf800000;
  *(undefined4 *)((int)this + 0x10) = 0xbf800000;
  if ((((param_2 < 0) || (param_3 < 0)) || (0xff < param_2)) || (0xff < param_3)) {
    uVar1 = 0;
  }
  else {
    uVar1 = (uint)*(ushort *)(DAT_0052d590 + (param_3 * 0x100 + param_2) * 2);
  }
  if ((uVar1 != 0) && (*(byte *)(uVar1 * 0x24 + 10 + DAT_005395dc) == DAT_0051cac8)) {
    *(undefined4 *)((int)this + 0x14) = param_4;
    FUN_004bd0b0(this,param_2,param_3);
  }
  DAT_0052039c = DAT_0052039c + 1;
  return;
}


// ==== FUN_004bd0b0 @ 004bd0b0

void __thiscall FUN_004bd0b0(void *this,int param_2,int param_3)

{
  uint uVar1;
  int *piVar2;
  
  while( true ) {
    if ((((param_2 < 0) || (param_3 < 0)) || (0xff < param_2)) || (0xff < param_3)) {
      uVar1 = 0;
    }
    else {
      uVar1 = (uint)*(ushort *)(DAT_0052d590 + (param_3 * 0x100 + param_2) * 2);
    }
    if (((uVar1 == 0) ||
        (piVar2 = (int *)(DAT_005395dc + uVar1 * 0x24), *(byte *)((int)piVar2 + 10) != DAT_0051cac8)
        ) || ((uint)*(byte *)(piVar2 + 8) == *(uint *)((int)this + 0x14))) break;
    (**(code **)(*piVar2 + 0x74))(*(uint *)((int)this + 0x14));
    uVar1 = *(uint *)(s_Error_in_command____s___code______0051c2dc +
                     *(char *)(*(int *)this + (uint)*(byte *)((int)piVar2 + 0x22) * 4) * 4 + 0x20);
    if ((uVar1 & 1) != 0) {
      FUN_004bd0b0(this,param_2,param_3 + -1);
    }
    if ((uVar1 & 2) != 0) {
      FUN_004bd0b0(this,param_2 + 1,param_3);
    }
    if ((uVar1 & 4) != 0) {
      FUN_004bd0b0(this,param_2,param_3 + 1);
    }
    if ((uVar1 & 8) == 0) {
      return;
    }
    param_2 = param_2 + -1;
  }
  return;
}


// ==== FUN_004bd070 @ 004bd070

void * __thiscall FUN_004bd070(void *this)

{
  *(undefined4 *)((int)this + 8) = 0xc61c4000;
  *(undefined4 *)((int)this + 4) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x10) = 0xc61c4000;
  *(undefined4 *)((int)this + 0xc) = 0xc61c4000;
  *(undefined4 *)this = *(undefined4 *)(DAT_0051cac8 * 0x1d4 + 0x124 + DAT_0051c960);
  return this;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __cdecl __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


