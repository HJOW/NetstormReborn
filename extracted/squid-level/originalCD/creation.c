// ==== FUN_004acdc0 @ 004acdc0

undefined4 __thiscall FUN_004acdc0(void *this)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_004acd90();
  if (iVar1 == 0) {
    uVar2 = FUN_004abac0(this);
    if (((uVar2 | 0x4000000) == 0) && (*(byte *)((int)this + 10) != DAT_0051cbd4)) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_004ace00 @ 004ace00

byte __thiscall FUN_004ace00(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004acdc0();
  if (iVar1 != 0) {
    return *(byte *)((int)this + 0xb) >> 5;
  }
  return 0;
}


// ==== FUN_004ace20 @ 004ace20

void __thiscall FUN_004ace20(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004acdc0();
  if (iVar1 == 0) {
    FUN_00490040(s_hasLevel___00539f3c,s__ns_o_Squid_cpp_00539f2c,0x561);
  }
  if ((param_2 < 0) || (7 < param_2)) {
    FUN_00490040(s_0_<__n_&&_n_<_8_00539f58,s__ns_o_Squid_cpp_00539f48,0x562);
  }
  *(byte *)((int)this + 0xb) = *(byte *)((int)this + 0xb) & 0x1f ^ (char)param_2 << 5;
  return;
}


