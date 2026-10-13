// ==== FUN_0049e4c0 @ 0049e4c0

void __thiscall FUN_0049e4c0(void *this)

{
  FUN_0049d890(this,0x78,0x3d);
  FUN_0049d2c0(this,0x78);
  FUN_0049d9a0(this);
  return;
}


// ==== FUN_0049e5e0 @ 0049e5e0

undefined4 __cdecl FUN_0049e5e0(void)

{
  int iVar1;
  
  if (((DAT_00535ae0 != 0) && (DAT_00540a20 != 0)) && (DAT_0052e16c == 0)) {
    iVar1 = FUN_004bd4f0(&DAT_00583188);
    if (iVar1 == 1) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_00490520 @ 00490520

bool __thiscall FUN_00490520(void *this)

{
  return *(undefined **)this == &DAT_005484c0;
}


// ==== FUN_004040d0 @ 004040d0

void __thiscall FUN_004040d0(void *this,int *param_2,int param_3)

{
  bool bVar1;
  int iVar2;
  char *pcVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int local_18;
  int local_c;
  
  iVar2 = FUN_004bd4c0(&DAT_00583188,param_3);
  if (iVar2 == 0) {
    FUN_00490040(s_islandList_exists_islandId__0050f540,s__Ns_o_ChunkMap_cpp_0050f52c,0xbe);
  }
  iVar5 = 0x10;
  iVar6 = 0;
  iVar2 = 0;
  local_c = 0x10;
  bVar1 = false;
  local_18 = 0;
  if (0 < *(int *)((int)this + 8)) {
    do {
      iVar4 = 0;
      if (0 < *(int *)((int)this + 4)) {
        do {
          pcVar3 = (char *)FUN_00404090(this,iVar4,local_18);
          if (*pcVar3 == param_3) {
            bVar1 = true;
            if (iVar4 <= iVar5) {
              iVar5 = iVar4;
            }
            if (local_18 <= local_c) {
              local_c = local_18;
            }
            if (iVar6 <= iVar4) {
              iVar6 = iVar4;
            }
            if (iVar2 <= local_18) {
              iVar2 = local_18;
            }
          }
          iVar4 = iVar4 + 1;
        } while (iVar4 < *(int *)((int)this + 4));
      }
      local_18 = local_18 + 1;
    } while (local_18 < *(int *)((int)this + 8));
  }
  if (!bVar1) {
    FUN_00490040(s_found_0050f570,s__Ns_o_ChunkMap_cpp_0050f55c,0xce);
  }
  *param_2 = iVar5;
  param_2[1] = local_c;
  param_2[2] = iVar6 + 1;
  param_2[3] = iVar2 + 1;
  return;
}


