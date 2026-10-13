// ==== FUN_00443160 @ 00443160

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00443160(void *this,double *param_2)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_004ac420();
  uVar2 = FUN_004abae0(this);
  if ((((uVar2 & 0x100000) == 0) && (iVar1 != 1)) && (iVar1 != 0)) {
    FUN_00490040(s__getGenus_____gFLYER_____group___0051c5a0,s__Ns_o_unit_cpp_0051c590,0x3c);
  }
  *param_2 = (double)((*(ushort *)((int)this + 0xc) & 0xe) >> 1) * _DAT_00501960 +
             (double)(*(ushort *)((int)this + 0xc) >> 4);
  return;
}


// ==== FUN_004431f0 @ 004431f0

ushort __cdecl FUN_004431f0(ushort param_1)

{
  return param_1 & 0x7fff;
}


// ==== FUN_00443200 @ 00443200

void __thiscall FUN_00443200(void *this)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  
  iVar2 = FUN_004ac420();
  uVar3 = FUN_004abae0(this);
  if ((((uVar3 & 0x100000) == 0) && (iVar2 != 1)) && (iVar2 != 0)) {
    FUN_00490040(s__getGenus_____gFLYER_____group___0051c5f0,s__Ns_o_unit_cpp_0051c5e0,0x49);
  }
  iVar2 = __ftol();
  uVar3 = __ftol();
  sVar1 = FUN_004431f0(uVar3 | iVar2 << 3);
  *(ushort *)((int)this + 0xc) = *(ushort *)((int)this + 0xc) & 1 ^ sVar1 * 2;
  FUN_00448c10(((int)this - DAT_005395dc) / 0x24);
  return;
}


// ==== FUN_004432a0 @ 004432a0

void __thiscall FUN_004432a0(void *this,int param_2,undefined4 param_3)

{
  int *piVar1;
  
  piVar1 = (int *)(param_2 * 0x24 + DAT_005395dc);
  if ((param_2 != 0) && ((*(byte *)((int)piVar1 + 0xb) & 3) == 0)) {
    (**(code **)(*piVar1 + 0x98))
              (param_3,*(undefined1 *)((int)this + 0x20),((int)this - DAT_005395dc) / 0x24);
  }
  return;
}


// ==== FUN_004432f0 @ 004432f0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004432f0(void *this,undefined4 param_2)

{
  uint uVar1;
  double *pdVar2;
  undefined4 uVar3;
  uint uVar4;
  float10 fVar5;
  float afStack_10 [2];
  float fStack_8;
  
  fVar5 = (float10)(**(code **)(*(int *)this + 0xbc))(param_2);
  fStack_8 = (float)fVar5;
  afStack_10[0] = (float)__ftol();
  uVar4 = (int)afStack_10[0] << 3;
  uVar1 = __ftol();
  uVar1 = FUN_004431f0(uVar1 | uVar4);
  pdVar2 = (double *)FUN_00443160(this,afStack_10);
  afStack_10[0] =
       ((float)(uVar1 & 7) * (float)_DAT_00501960 + (float)((int)uVar1 >> 3)) - (float)*pdVar2;
  if (_DAT_00501970 <= afStack_10[0] * fStack_8) {
    uVar3 = __ftol();
    (**(code **)(*(int *)this + 0xc0))(param_2,uVar3);
  }
  FUN_00443200(this,DAT_005484a8,DAT_005484ac);
  return;
}


// ==== FUN_004433e0 @ 004433e0

void __thiscall FUN_004433e0(void *this)

{
  FUN_00443200(this,DAT_005484a8,DAT_005484ac);
  return;
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


