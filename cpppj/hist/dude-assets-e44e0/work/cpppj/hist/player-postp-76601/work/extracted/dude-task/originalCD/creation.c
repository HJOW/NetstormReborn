// ==== FUN_004e8ba0 @ 004e8ba0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8ba0(int param_1,int param_2,float param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_004e8c8a;
  *unaff_FS_OFFSET = &local_10;
  if ((param_1 != 0) && (iVar3 = DAT_005395dc + param_1 * 0x24, (*(byte *)(iVar3 + 0xb) & 3) == 0))
  {
    if ((*(short *)(iVar3 + 0xc) == 0x5105) && (param_2 != 0x5105)) {
      param_2 = 0x5105;
    }
    iVar1 = FUN_004ac9c0(iVar3,0xfff0,0x5100);
    if (iVar1 == 0) {
      iVar2 = FUN_004f1650(0x28);
      iVar1 = 0;
      local_8 = 0;
      if (iVar2 != 0) {
        iVar1 = FUN_0048efd0(iVar2,param_2,(iVar3 - DAT_005395dc) / 0x24,0x40a00000);
      }
      local_8 = 0xffffffff;
    }
    else {
      *(undefined4 *)(iVar1 + 0x20) = 0;
      *(int *)(iVar1 + 0x18) = param_2;
    }
    *(double *)(iVar1 + 0x10) = _DAT_005484a8 + (double)param_3;
    FUN_004abb00(iVar3,param_2);
  }
  *unaff_FS_OFFSET = local_10;
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


// ==== FUN_0048efd0 @ 0048efd0

void * __thiscall FUN_0048efd0(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_0048f073;
  *unaff_FS_OFFSET = &local_10;
  *(undefined ***)this = &PTR_FUN_00500d68;
  puVar1 = (undefined4 *)((int)this + 0x10);
  local_8 = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *puVar1 = 0;
  uVar2 = DAT_0051ca18;
  *(undefined4 *)((int)this + 0x14) = 0;
  *puVar1 = 0;
  *(undefined ***)this = &PTR_FUN_00504670;
  *(undefined4 *)((int)this + 0x14) = 0;
  FUN_0048f590(this,uVar2,param_3,0,0x50);
  *(undefined4 *)((int)this + 0x1c) = param_4;
  uVar2 = DAT_005484ac;
  *puVar1 = DAT_005484a8;
  *(undefined4 *)((int)this + 0x14) = uVar2;
  *(undefined4 *)((int)this + 0x18) = param_2;
  *(undefined4 *)((int)this + 0x20) = 0;
  *unaff_FS_OFFSET = local_10;
  return this;
}


