// ==== FUN_00490250 @ 00490250

int __cdecl FUN_00490250(int param_1)

{
  float fVar1;
  int iVar2;
  int *piVar3;
  
  if (DAT_00595344 == 0) {
    FUN_004e0620("playerIdKnown","\\Ns\\O\\Player.cpp",0x351);
  }
  fVar1 = (float)(byte)((-(DAT_0055a480 != 0) & 9U) + 1);
  iVar2 = FUN_004b1fa0((float)(param_1 + 10),fVar1,DAT_005412b8);
  if (iVar2 != 0) {
    FUN_004e0620("!findSquid(bc,playerType)","\\Ns\\O\\Player.cpp",0x354);
  }
  piVar3 = (int *)FUN_004af530(DAT_005412b8,2);
  (**(code **)(*piVar3 + 0x74))(param_1);
  piVar3[9] = 0;
  (**(code **)(*piVar3 + 0x90))((float)(param_1 + 10),fVar1,0);
  return ((int)piVar3 - DAT_005c8464) / 0x32;
}


// ==== FUN_004905d0 @ 004905d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004905d0(void *this,uint param_2)

{
  void *pvVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fb17b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(int *)((uint)*(byte *)((int)this + 0x22) * 0xb4 + 0x9c + DAT_0059534c) =
       ((int)this - DAT_005c8464) / 0x32;
  if ((((param_2 & 1) != 0) && (*(byte *)((int)this + 0x22) == _DAT_00540c70)) &&
     (DAT_00594fb8 == 0)) {
    pvVar1 = operator_new(0x28);
    local_4 = 0;
    if (pvVar1 != (void *)0x0) {
      FUN_00496e80(pvVar1,1,((int)this - DAT_005c8464) / 0x32,0x40a00000);
    }
  }
  local_4 = 0xffffffff;
  if (((param_2 & 1) != 0) && (DAT_0055a480 != 0)) {
    *(undefined4 *)((int)this + 0x24) = 1;
  }
  FUN_004b0d30(this,param_2);
  ExceptionList = local_c;
  return;
}


// ==== FUN_0048fc60 @ 0048fc60

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __cdecl FUN_0048fc60(void)

{
  if (DAT_00595344 == 0) {
    return 0;
  }
  return *(undefined4 *)(_DAT_00540c70 * 0xb4 + 0x9c + DAT_0059534c);
}


// ==== FUN_0048fc90 @ 0048fc90

undefined4 __cdecl FUN_0048fc90(int param_1)

{
  if (DAT_00595344 == 0) {
    FUN_004e0620("playerIdKnown","\\Ns\\O\\Player.cpp",0x29d);
  }
  return *(undefined4 *)(param_1 * 0xb4 + 0x9c + DAT_0059534c);
}


