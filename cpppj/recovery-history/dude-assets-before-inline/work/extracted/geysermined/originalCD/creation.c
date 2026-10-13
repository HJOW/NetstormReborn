// ==== FUN_004e41e0 @ 004e41e0

void __cdecl FUN_004e41e0(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined4 uVar2;
  int *piVar3;
  undefined4 uStack_104;
  uint local_100;
  undefined4 local_fc;
  
  piVar3 = (int *)(DAT_005395dc + (uint)*(ushort *)(param_3 + 4) * 0x24);
  if ((*(byte *)((int)piVar3 + 0xb) & 1) == 0) {
    iVar1 = FUN_0048cc10(1000,7000);
    uVar2 = FUN_00420560(&local_100,s_geyserMined_d_wav_00541628,iVar1 / 1000);
    FUN_00438c00(*(uint *)((int)piVar3 + 0xe),*(undefined4 *)((int)piVar3 + 0x12),uVar2,0);
    local_100 = (uint)*(ushort *)(piVar3 + 3);
    uVar2 = __ftol();
    FUN_004abb00(piVar3,uVar2);
    if ((DAT_00540a2c != 0) && ((short)piVar3[3] == 0)) {
      local_100 = *(uint *)((int)piVar3 + 0xe);
      local_fc = *(undefined4 *)((int)piVar3 + 0x12);
      (**(code **)(*piVar3 + 0x10))(0x1000);
      iVar1 = FUN_004bdd80(&DAT_00583188,uStack_104,local_100);
      if (iVar1 == 0) {
        piVar3 = (int *)FUN_004ab390(DAT_0051ca94,0);
        (**(code **)(*piVar3 + 0x90))(uStack_104,local_100,0);
      }
    }
  }
  return;
}


// ==== FUN_004abb00 @ 004abb00

void __thiscall FUN_004abb00(void *this,undefined2 param_2)

{
  *(undefined2 *)((int)this + 0xc) = param_2;
  if (*(byte *)((int)this + 10) == DAT_0051ca8c) {
    FUN_00448c10(((int)this - DAT_005395dc) / 0x24);
  }
  return;
}


// ==== FUN_0048cc10 @ 0048cc10

int __cdecl FUN_0048cc10(int param_1,int param_2)

{
  if (DAT_005308ec == 0) {
    DAT_005308ec = 0xbad0bad;
  }
  DAT_005308ec = DAT_005308ec * 0x10003 + 3;
  return param_1 + (DAT_005308ec >> 0x10 & 0xffffU) % (uint)(param_2 - param_1);
}


// ==== FUN_004bdd80 @ 004bdd80

void __thiscall FUN_004bdd80(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = __ftol();
  uVar2 = __ftol(uVar1);
  FUN_004bdd20(uVar2,uVar1);
  return;
}


// ==== FUN_004bdd20 @ 004bdd20

undefined4 FUN_004bdd20(int param_1,int param_2)

{
  uint uVar1;
  
  if ((((param_1 < 0) || (param_2 < 0)) || (0xff < param_1)) || (0xff < param_2)) {
    uVar1 = 0;
  }
  else {
    uVar1 = (uint)*(ushort *)(DAT_0052d590 + (param_2 * 0x100 + param_1) * 2);
  }
  if ((uVar1 != 0) && (*(byte *)(uVar1 * 0x24 + 10 + DAT_005395dc) == DAT_0051cac8)) {
    return 1;
  }
  return 0;
}


// ==== FUN_00448c10 @ 00448c10

int __cdecl FUN_00448c10(int param_1)

{
  return DAT_005395dc + param_1 * 0x24;
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


