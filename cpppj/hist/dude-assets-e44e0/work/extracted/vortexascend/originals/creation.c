// ==== FUN_004db8e0 @ 004db8e0

undefined4 __thiscall FUN_004db8e0(void *this,int param_2)

{
  undefined4 uVar1;
  void *_Memory;
  int iVar2;
  int iVar3;
  undefined1 local_4c [52];
  int local_18;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fc1e8;
  local_c = ExceptionList;
  if (0x20 < param_2) {
    param_2 = 0x40;
  }
  ExceptionList = &local_c;
  uVar1 = FUN_0046f530(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
  FUN_00422230(local_4c,*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),param_2,
               param_2,0);
  _Memory = operator_new(0x50);
  iVar3 = 0;
  local_4 = 0;
  if (local_18 != 0) {
    do {
      iVar2 = FUN_004db680(local_18,uVar1,*(undefined1 *)((int)this + 0x22));
      if (iVar2 != 0) {
        if (0x13 < iVar3) break;
        *(int *)((int)_Memory + iVar3 * 4) = (local_18 * 0x32) / 0x32;
        iVar3 = iVar3 + 1;
        if (0x13 < iVar3) break;
      }
      FUN_004b1810(local_4c);
    } while (local_18 != 0);
    if (iVar3 != 0) {
      iVar3 = FUN_004558c0(iVar3);
      (**(code **)(*(int *)(*(int *)((int)_Memory + iVar3 * 4) * 0x32 + DAT_005c8464) + 0x10))
                (0x100000);
      goto LAB_004db9de;
    }
  }
  if (param_2 == 0x40) {
    local_4 = 0xffffffff;
    _free(_Memory);
    ExceptionList = local_c;
    return 0;
  }
LAB_004db9de:
  local_4 = 0xffffffff;
  _free(_Memory);
  ExceptionList = local_c;
  return 1;
}


// ==== FUN_004db680 @ 004db680

bool __cdecl FUN_004db680(int param_1,int param_2,uint param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  bool bVar7;
  
  uVar1 = *(undefined4 *)(param_1 * 0x32 + 0x12 + DAT_005c8464);
  iVar6 = param_1 * 0x32 + DAT_005c8464;
  uVar2 = *(undefined4 *)(iVar6 + 0xe);
  uVar3 = FUN_004ac200(iVar6);
  if (((uVar3 & 0x128b302) == 0) && (iVar4 = FUN_0046f530(uVar2,uVar1), iVar4 == param_2)) {
    iVar4 = FUN_004e49c0();
    iVar5 = FUN_004e49c0();
    if ((*(byte *)(iVar5 * 0x100 + DAT_005c7c44 + iVar4) & 2) == 0) {
      uVar3 = FUN_004ac1e0(iVar6);
      bVar7 = (uVar3 & 0x200000) == 0;
    }
    else {
      iVar4 = FUN_0040da00(iVar4,iVar5);
      if (iVar4 == 0) {
        return false;
      }
      bVar7 = *(byte *)(iVar4 * 0x32 + 10 + DAT_005c8464) == DAT_005412cc;
    }
    if (!bVar7) {
      return *(byte *)(iVar6 + 0x22) != param_3;
    }
  }
  return false;
}


// ==== FUN_0046f480 @ 0046f480

int __thiscall FUN_0046f480(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  
  iVar1 = *(int *)this;
  iVar2 = 0;
  iVar3 = 0;
  if (0 < iVar1) {
    piVar4 = (int *)((int)this + 8);
    do {
      if (((-1 < iVar3) && (iVar3 < iVar1)) && (*piVar4 != 0)) {
        if ((iVar3 < *(int *)this) && (*piVar4 != 0)) {
          iVar1 = piVar4[1];
        }
        else {
          iVar1 = 0;
        }
        if ((iVar1 != param_2) && (iVar1 = FUN_0046f320(this,iVar3), iVar1 != 0)) {
          iVar2 = iVar2 + 1;
        }
      }
      iVar1 = *(int *)this;
      iVar3 = iVar3 + 1;
      piVar4 = piVar4 + 8;
    } while (iVar3 < iVar1);
  }
  return iVar2;
}


// ==== FUN_0046f320 @ 0046f320

undefined4 __thiscall FUN_0046f320(void *this,int param_2)

{
  if (((param_2 < 0) || (*(int *)this <= param_2)) ||
     (*(int *)(param_2 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2fb);
  }
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     (*(int *)(param_2 * 0x20 + 8 + (int)this) != 0)) {
    return *(undefined4 *)((int)this + param_2 * 0x20 + 0xc);
  }
  return 0;
}


// ==== FUN_0040da00 @ 0040da00

undefined2 __cdecl FUN_0040da00(int param_1,int param_2)

{
  if ((((-1 < param_1) && (-1 < param_2)) && (param_1 < 0x100)) && (param_2 < 0x100)) {
    return *(undefined2 *)(DAT_005c84bc + (param_2 * 0x100 + param_1) * 2);
  }
  return 0;
}


// ==== FUN_004ac1e0 @ 004ac1e0

undefined4 __thiscall FUN_004ac1e0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xe8);
}


// ==== FUN_00422230 @ 00422230

void * __thiscall
FUN_00422230(void *this,undefined4 param_2,undefined4 param_3,int param_4,int param_5,uint param_6)

{
  int iVar1;
  int iVar2;
  
  *(uint *)((int)this + 0x38) = param_6 & 1;
  *(undefined ***)this = &PTR_FUN_00502318;
  *(uint *)((int)this + 0x3c) = param_6 & 4;
  FUN_0041d7a0(&param_2);
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  FUN_004b16d0(this,iVar2 - param_4,iVar1 - param_5,iVar2 + param_4,iVar1 + param_5);
  return this;
}


// ==== FUN_0041d7a0 @ 0041d7a0

void __thiscall FUN_0041d7a0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *(float *)this = (float)iVar1;
  iVar1 = FUN_004e49c0();
  *(float *)((int)this + 4) = (float)iVar1;
  return;
}


