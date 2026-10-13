// ==== FUN_00432430 @ 00432430

void __thiscall FUN_00432430(void *this,undefined4 *param_2,int param_3)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined *puVar5;
  
  iVar3 = 0;
  iVar4 = 0;
  puVar5 = &DAT_0054c8a0;
  if (this != (void *)0x0) {
    puVar5 = this;
  }
  pcVar1 = (char *)FUN_00432180(puVar5,0,0);
  iVar2 = *(int *)(puVar5 + 8);
  if (0 < iVar2) {
    do {
      if (*pcVar1 == param_3) {
        *param_2 = *(undefined4 *)(pcVar1 + 2);
        *(undefined2 *)(param_2 + 1) = *(undefined2 *)(pcVar1 + 6);
        return;
      }
      iVar3 = iVar3 + 1;
      if (*(int *)(puVar5 + 4) <= iVar3) {
        iVar3 = 0;
        iVar4 = iVar4 + 1;
      }
      if (iVar4 < iVar2) {
        pcVar1 = (char *)FUN_00432180(puVar5,iVar3,iVar4);
      }
      else {
        pcVar1 = (char *)0x0;
      }
      iVar2 = *(int *)(puVar5 + 8);
    } while (iVar4 < iVar2);
  }
  *param_2 = 0;
  *(undefined2 *)(param_2 + 1) = 0;
  return;
}


// ==== FUN_00432180 @ 00432180

void * __thiscall FUN_00432180(void *this,uint param_2,uint param_3)

{
  uint uVar1;
  
  if (*(int *)this == 0) {
    FUN_004e0620("isValid()","\\Ns\\O\\Chunkmap.cpp",0xb7);
  }
  if (((((int)param_2 < 0) || ((int)param_3 < 0)) || (*(int *)((int)this + 4) <= (int)param_2)) ||
     (*(int *)((int)this + 8) <= (int)param_3)) {
    FUN_004e0620("cc.x>=0 && cc.y>=0 && cc.x<xLen && cc.y<yLen","\\Ns\\O\\Chunkmap.cpp",0xb8);
  }
  param_2 = ((int)param_2 < 1) - 1 & param_2;
  uVar1 = *(int *)((int)this + 4) - 1;
  if ((int)uVar1 <= (int)param_2) {
    param_2 = uVar1;
  }
  param_3 = ((int)param_3 < 1) - 1 & param_3;
  uVar1 = *(int *)((int)this + 8) - 1;
  if ((int)uVar1 <= (int)param_3) {
    param_3 = uVar1;
  }
  return (void *)((int)this + (param_2 * 0x10 + 1 + param_3) * 0xc);
}


// ==== FUN_004bca20 @ 004bca20

undefined4 __thiscall FUN_004bca20(void *this,int param_2)

{
  int iVar1;
  
  if (param_2 < 1) {
    return 0;
  }
  if (*(int *)this == 0) {
    *(undefined4 *)this = 0xbad0bad;
  }
  iVar1 = *(int *)this * 0x10003 + 3;
  *(int *)this = iVar1;
  return (int)((longlong)(ulonglong)(iVar1 >> 0x10 & 0xffff) % (longlong)param_2);
}


