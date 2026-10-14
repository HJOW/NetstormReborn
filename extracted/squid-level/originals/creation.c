// ==== FUN_004ad0a0 @ 004ad0a0

undefined4 __thiscall FUN_004ad0a0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if (*(int *)(iVar1 + 0x9c) == 10) {
    iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
    if (((*(uint *)(iVar1 + 0xe8) | 0x4000000) == 0) && (*(byte *)((int)this + 10) != DAT_005412e8))
    {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_004ad0f0 @ 004ad0f0

byte __thiscall FUN_004ad0f0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004ad0a0();
  if (iVar1 != 0) {
    return *(byte *)((int)this + 0xb) >> 5;
  }
  return 0;
}


// ==== FUN_004ad110 @ 004ad110

void __thiscall FUN_004ad110(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004ad0a0();
  if (iVar1 == 0) {
    FUN_004e0620("hasLevel()","\\Ns\\O\\Squid.cpp",0x578);
  }
  if ((param_2 < 0) || (7 < param_2)) {
    FUN_004e0620("0 <= n && n < 8","\\Ns\\O\\Squid.cpp",0x579);
  }
  *(byte *)((int)this + 0xb) = *(byte *)((int)this + 0xb) & 0x1f | (char)param_2 << 5;
  return;
}


