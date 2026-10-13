// ==== FUN_004558f0 @ 004558f0

int __cdecl FUN_004558f0(int param_1,int param_2)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % (uint)(param_2 - param_1) + param_1;
}


// ==== FUN_004ac140 @ 004ac140

undefined4 __cdecl FUN_004ac140(int param_1,int param_2)

{
  int iVar1;
  
  if (param_2 == 0) {
    if (param_1 < 1) {
      return 0;
    }
    if (0xfffe < param_1) {
      return 0;
    }
  }
  else {
    if ((param_1 < 1) || (0xfffe < param_1)) {
      return 0;
    }
    if ((DAT_005e4794 != 0) && (iVar1 = FUN_004ac040(param_1 * 0x32 + DAT_005c8464,1), iVar1 == 0))
    {
      FUN_004e0620("resolve( sid ).isRobust()","\\Ns\\O\\Squid.cpp",0x218);
    }
  }
  return 1;
}


// ==== FUN_004adc60 @ 004adc60

uint __thiscall FUN_004adc60(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x400000) != 0) {
    return *(byte *)((int)this + 0x28) & 1;
  }
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(uint *)(iVar1 + 0xe8) & 0x10;
}


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __thiscall FUN_004ac200(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_0049fce0 @ 0049fce0

int __thiscall FUN_0049fce0(void *this)

{
  if (((DAT_005e4794 != 0) && ((*(byte *)((int)this + 0xe8) & 0x10) == 0)) &&
     (((int)this - DAT_0059ab20) / 500 != DAT_005412ec)) {
    FUN_004e0620("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType",
                 "\\Ns\\O\\Rifttype.cpp",0x1e3);
  }
  if ((DAT_005ca8f0 != 0) && (((int)this - DAT_0059ab20) / 500 == DAT_005412d0)) {
    return (int)(*(int *)this + (*(int *)this >> 0x1f & 3U)) >> 2;
  }
  return *(int *)this;
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong __cdecl FUN_004e49c0(void)

{
  ulonglong uVar1;
  uint uVar2;
  float fVar3;
  float10 in_ST0;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  uVar1 = (ulonglong)ROUND(in_ST0);
  local_20 = (uint)uVar1;
  uStack_1c = (float)(uVar1 >> 0x20);
  fVar3 = (float)in_ST0;
  if ((local_20 != 0) || (fVar3 = uStack_1c, (uVar1 & 0x7fffffff00000000) != 0)) {
    if ((int)fVar3 < 0) {
      uVar1 = uVar1 + (0x80000000 < (uint)-(float)(in_ST0 - (float10)(longlong)uVar1));
    }
    else {
      uVar2 = (uint)(0x80000000 < (uint)(float)(in_ST0 - (float10)(longlong)uVar1));
      uVar1 = CONCAT44((int)uStack_1c - (uint)(local_20 < uVar2),local_20 - uVar2);
    }
  }
  return uVar1;
}


