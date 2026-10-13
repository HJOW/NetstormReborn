// ==== FUN_0040cf10 @ 0040cf10

void __cdecl FUN_0040cf10(int param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  
  uVar1 = param_1 * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    uVar3 = 0;
    iVar2 = FUN_004abf50(uVar1,0);
    FUN_004acbc0(uVar1,*(undefined4 *)(iVar2 + 0x158 + param_2 * 4),uVar3);
  }
  return;
}


// ==== FUN_0040cf60 @ 0040cf60

void __cdecl FUN_0040cf60(int param_1,undefined4 param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0040d00a;
  *unaff_FS_OFFSET = &local_10;
  uVar1 = param_1 * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    FUN_0040cf10(param_1,param_2);
    iVar2 = FUN_004f1650(0x28);
    local_8 = 0;
    if (iVar2 != 0) {
      FUN_0048f1d0(iVar2,0x25c,(int)(uVar1 - DAT_005395dc) / 0x24,0x3c23d70a);
    }
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


