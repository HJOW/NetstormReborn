// ==== FUN_00416720 @ 00416720

void __cdecl FUN_00416720(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0xb4;
  do {
    if ((DAT_0059534c + iVar2 != 0) && (iVar1 = *(int *)(DAT_0059534c + iVar2 + 0xb0), iVar1 != 0))
    {
      FUN_004153c0(iVar1,param_1,param_2);
    }
    iVar2 = iVar2 + 0xb4;
  } while (iVar2 < 0x654);
  return;
}


// ==== FUN_004153c0 @ 004153c0

void __thiscall FUN_004153c0(void *this,undefined4 param_2,int param_3)

{
  int iVar1;
  
  if (param_3 == 0) {
    FUN_0040ea00((int)this + 0x324,param_2);
    FUN_0040ea00((int)this + 0x330,param_2);
    FUN_0040ea00((int)this + 0x33c,param_2);
    FUN_0040ea00((int)this + 0x348,param_2);
    FUN_0040ea00((int)this + 0x354,param_2);
    return;
  }
  iVar1 = FUN_004152b0(this,param_2);
  if (iVar1 == 0) {
    if (*(int *)((int)this + 0x32c) < *(int *)((int)this + 0x328)) {
      *(undefined4 *)(*(int *)((int)this + 0x324) + *(int *)((int)this + 0x32c) * 4) = param_2;
      *(int *)((int)this + 0x32c) = *(int *)((int)this + 0x32c) + 1;
    }
    FUN_00411000(this,param_2);
    return;
  }
  FUN_004105b0(this,0x1d,"Not closer to geyser %d.\n",param_2);
  return;
}


