// ==== FUN_004047c0 @ 004047c0

void __thiscall FUN_004047c0(void *this,undefined4 *param_2,int param_3)

{
  undefined2 uVar1;
  int iVar2;
  char *pcVar3;
  undefined *puVar4;
  int local_10;
  
  iVar2 = FUN_004bd4c0(&DAT_00583188,param_3);
  if (iVar2 == 0) {
    FUN_00490040(s_islandList_exists_islandId__0050f5ac,s__Ns_o_ChunkMap_cpp_0050f598,0x126);
  }
  iVar2 = 0;
  local_10 = 0;
  puVar4 = &DAT_005484c0;
  if (this != (void *)0x0) {
    puVar4 = this;
  }
  pcVar3 = (char *)FUN_00404090(puVar4,0,0);
  if (0 < *(int *)(puVar4 + 8)) {
    do {
      if (*pcVar3 == param_3) {
        uVar1 = *(undefined2 *)(pcVar3 + 6);
        *param_2 = *(undefined4 *)(pcVar3 + 2);
        *(undefined2 *)(param_2 + 1) = uVar1;
        return;
      }
      local_10 = local_10 + 1;
      if (*(int *)(puVar4 + 4) <= local_10) {
        iVar2 = iVar2 + 1;
        local_10 = 0;
      }
      pcVar3 = (char *)0x0;
      if (iVar2 < *(int *)(puVar4 + 8)) {
        pcVar3 = (char *)FUN_00404090(puVar4,local_10,iVar2);
      }
    } while (iVar2 < *(int *)(puVar4 + 8));
  }
  FUN_00490040(&DAT_0050f5dc,s__Ns_o_ChunkMap_cpp_0050f5c8,300);
  *param_2 = 0;
  *(undefined2 *)(param_2 + 1) = 0;
  return;
}


// ==== FUN_00404090 @ 00404090

int __thiscall FUN_00404090(void *this,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  
  iVar2 = *(int *)((int)this + 4) + -1;
  if (param_2 < 1) {
    param_2 = 0;
  }
  if (param_2 <= iVar2) {
    iVar2 = param_2;
  }
  iVar1 = *(int *)((int)this + 8) + -1;
  if (param_3 < 1) {
    param_3 = 0;
  }
  if (param_3 <= iVar1) {
    iVar1 = param_3;
  }
  return (int)this + (iVar2 * 0x10 + iVar1) * 0xc + 0xc;
}


// ==== FUN_004bd4c0 @ 004bd4c0

undefined4 __thiscall FUN_004bd4c0(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004bd480(param_2);
  if (iVar1 != 0) {
    return *(undefined4 *)(param_2 * 0x20 + 8 + (int)this);
  }
  return 0;
}


// ==== FUN_004bd480 @ 004bd480

undefined4 __thiscall FUN_004bd480(int *param_1,int param_2)

{
  if ((-1 < param_2) && (param_2 < *param_1)) {
    return 1;
  }
  return 0;
}


// ==== FUN_0042c010 @ 0042c010

void __cdecl FUN_0042c010(float *param_1,undefined4 param_2,float param_3,float param_4)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  
  uVar1 = __ftol();
  iVar2 = FUN_004a59d0(uVar1);
  uVar1 = __ftol();
  iVar3 = FUN_004a59d0(uVar1);
  param_1[1] = (float)iVar2 + param_4;
  *param_1 = (float)iVar3 + param_3;
  return;
}


// ==== FUN_004a59d0 @ 004a59d0

int FUN_004a59d0(int param_1)

{
  int iVar1;
  
  if (param_1 < 1) {
    return 0;
  }
  iVar1 = FUN_004a59a0();
  return iVar1 % param_1;
}


// ==== FUN_004a59a0 @ 004a59a0

uint __fastcall FUN_004a59a0(int *param_1)

{
  int iVar1;
  
  if (*param_1 == 0) {
    *param_1 = 0xbad0bad;
  }
  iVar1 = *param_1 * 0x10003 + 3;
  *param_1 = iVar1;
  return iVar1 >> 0x10 & 0xffff;
}


// ==== FUN_0042cf7b @ 0042cf7b

void __cdecl FUN_0042cf7b(void)

{
  int unaff_EBP;
  
  FUN_00407c50(unaff_EBP + -0x44);
  return;
}


// ==== FUN_00407c50 @ 00407c50

void __thiscall FUN_00407c50(void *this)

{
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_00407c87;
  *unaff_FS_OFFSET = &local_10;
  local_8 = 0xffffffff;
  FUN_00407c91(this);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00407c91 @ 00407c91

void __cdecl FUN_00407c91(void)

{
  int unaff_EBP;
  
  FUN_00407c40(*(undefined4 *)(unaff_EBP + -0x10));
  return;
}


// ==== FUN_00407c40 @ 00407c40

void __thiscall FUN_00407c40(void *this)

{
  FUN_004f1a50(*(undefined4 *)this);
  return;
}


