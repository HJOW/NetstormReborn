// ==== FUN_00448930 @ 00448930

void __thiscall FUN_00448930(void *this,uint param_2)

{
  void *pvVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f9f5b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  if ((param_2 & 1) != 0) {
    ExceptionList = &local_c;
    pvVar1 = operator_new(0x28);
    local_4 = 0;
    if (pvVar1 != (void *)0x0) {
      FUN_00496e80(pvVar1,0x304,((int)this - DAT_005c8464) / 0x32,0x3c23d70a);
    }
  }
  local_4 = 0xffffffff;
  if ((param_2 & 2) != 0) {
    iVar2 = FUN_00448a20(this);
    uVar3 = iVar2 * 0x32 + DAT_005c8464;
    if ((DAT_005c8464 + 0x32U <= uVar3) && (uVar3 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
      iVar4 = (int)this - DAT_005c8464;
      iVar2 = FUN_00448340();
      FUN_00443fe0(iVar4 / 0x32,(float)iVar2);
    }
  }
  FUN_004b0d30(this,param_2);
  ExceptionList = local_c;
  return;
}


// ==== FUN_00448a20 @ 00448a20

int __thiscall FUN_00448a20(void *this)

{
  int iVar1;
  undefined1 local_40 [52];
  int local_c;
  
  FUN_00422230(local_40,*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),1,1,0);
  while( true ) {
    if (local_c == 0) {
      return 0;
    }
    iVar1 = DAT_005c8464 + local_c * 0x32;
    if ((*(byte *)(iVar1 + 10) == DAT_005412ec) &&
       (*(char *)(iVar1 + 0x22) == *(char *)((int)this + 0x22))) break;
    FUN_004b1810(local_40);
  }
  return local_c;
}


// ==== FUN_00448340 @ 00448340

char __thiscall FUN_00448340(void *this)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412e8);
  uVar2 = iVar1 * 0x32 + DAT_005c8464;
  if ((((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) &&
      (*(ushort *)(uVar2 + 0xc) != 0)) && (DAT_00594fa4 == 0)) {
    return (-((*(byte *)((uint)*(ushort *)(uVar2 + 0xc) * 0xb4 + DAT_0059534c + 0x50) & 1) != 0) &
           0x12U) + 7;
  }
  return '\n';
}


