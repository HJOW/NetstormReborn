// ==== FUN_0040d900 @ 0040d900

void __thiscall FUN_0040d900(void *this,float *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = *(int *)((int)this + 4);
  iVar2 = *(int *)((int)this + 8);
  iVar3 = *(int *)((int)this + 0xc);
  *param_2 = (float)(*(int *)this << 4);
  param_2[1] = (float)(iVar1 << 4);
  param_2[2] = (float)(iVar2 << 4);
  param_2[3] = (float)(iVar3 << 4);
  return;
}


// ==== FUN_00432210 @ 00432210

void __thiscall FUN_00432210(void *this,int *param_2,int param_3)

{
  int iVar1;
  char *pcVar2;
  int iVar3;
  int iVar4;
  int local_10;
  int local_c;
  int local_4;
  
  iVar1 = FUN_0046ee30(&DAT_00568b60,param_3);
  if (iVar1 == 0) {
    FUN_004e0620("islandList.exists(islandId)","\\Ns\\O\\Chunkmap.cpp",0xc2);
  }
  iVar4 = 0;
  local_10 = 0x10;
  local_c = 0x10;
  iVar3 = 0;
  local_4 = 0;
  iVar1 = 0;
  if (0 < *(int *)((int)this + 8)) {
    do {
      iVar1 = 0;
      if (0 < *(int *)((int)this + 4)) {
        do {
          pcVar2 = (char *)FUN_00432180(this,iVar1,iVar4);
          if (*pcVar2 == param_3) {
            if (iVar1 <= local_10) {
              local_10 = iVar1;
            }
            if (iVar4 <= local_c) {
              local_c = iVar4;
            }
            if (iVar3 <= iVar1) {
              iVar3 = iVar1;
            }
            if (local_4 <= iVar4) {
              local_4 = iVar4;
            }
          }
          iVar1 = iVar1 + 1;
        } while (iVar1 < *(int *)((int)this + 4));
      }
      iVar4 = iVar4 + 1;
      iVar1 = local_4;
    } while (iVar4 < *(int *)((int)this + 8));
  }
  *param_2 = local_10;
  param_2[1] = local_c;
  param_2[2] = iVar3 + 1;
  param_2[3] = iVar1 + 1;
  return;
}


