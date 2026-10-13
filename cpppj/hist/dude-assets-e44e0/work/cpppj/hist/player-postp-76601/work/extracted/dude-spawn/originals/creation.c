// ==== FUN_00450a20 @ 00450a20

int __cdecl FUN_00450a20(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  undefined4 unaff_retaddr;
  
  if ((DAT_0055a4f8 < DAT_0055a4f4) && (DAT_0055a4e8 == 0)) {
    piVar1 = (int *)FUN_004af530(DAT_00541170,2);
    piVar1[9] = 0;
    (**(code **)(*piVar1 + 0x74))(param_3);
    (**(code **)(*piVar1 + 0x90))(unaff_retaddr,param_1,0);
    return ((int)piVar1 - DAT_005c8464) / 0x32;
  }
  return 0;
}


// ==== FUN_00450cd0 @ 00450cd0

void __thiscall FUN_00450cd0(void *this,uint param_2)

{
  if ((param_2 & 1) != 0) {
    FUN_004ac220(this,0x5101);
    if (DAT_0055a4f8 < DAT_0055a4f4) {
      *(int *)(DAT_0055a4f0 + DAT_0055a4f8 * 4) = ((int)this - DAT_005c8464) / 0x32;
      DAT_0055a4f8 = DAT_0055a4f8 + 1;
    }
    FUN_00450d40(this);
  }
  FUN_004b0d30(this,param_2);
  return;
}


// ==== FUN_00451450 @ 00451450

undefined4 * __cdecl FUN_00451450(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(param_1 * 0x32 + DAT_005c8464);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00508cc8;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_00450b30 @ 00450b30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00450b30(int param_1,int param_2)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  float10 fVar5;
  float10 fVar6;
  
  if (param_1 == 0) {
    FUN_004e0620("srcSid",".\\Dude.cpp",0x1c4);
  }
  iVar4 = param_1 * 0x32 + DAT_005c8464;
  if ((*(byte *)(param_1 * 0x32 + 0xb + DAT_005c8464) & 1) == 0) {
    fVar1 = *(float *)(iVar4 + 0x12);
    fVar2 = *(float *)(iVar4 + 0xe);
    iVar3 = FUN_004ac550(iVar4);
    fVar5 = (float10)FUN_00455930(0x3dcccccd,(float)*(int *)(iVar3 + 0x1d4) - _DAT_005022ec);
    iVar3 = FUN_004ac550(iVar4);
    fVar6 = (float10)FUN_00455930(0x3dcccccd,(float)*(int *)(iVar3 + 0x1d8) - _DAT_005022ec);
    iVar4 = FUN_00450a20((float)((float10)fVar2 - fVar5),(float)((float10)fVar1 - fVar6),
                         *(undefined1 *)(iVar4 + 0x22));
    if (iVar4 != 0) {
      if ((*(byte *)(iVar4 * 0x32 + 0xb + DAT_005c8464) & 3) == 0) {
        FUN_00450670(iVar4 * 0x32 + DAT_005c8464,0x5103,(float)param_2 * _DAT_005022ec);
      }
    }
  }
  return;
}


// ==== FUN_00450c20 @ 00450c20

void __cdecl FUN_00450c20(int param_1,int param_2)

{
  undefined4 *puVar1;
  int iVar2;
  undefined1 local_8 [8];
  
  if (param_1 == 0) {
    FUN_004e0620("destSid",".\\Dude.cpp",0x1d5);
    FUN_004e0620("destSid",".\\Dude.cpp",0x1db);
  }
  iVar2 = param_1 * 0x32 + DAT_005c8464;
  if ((*(byte *)(param_1 * 0x32 + 0xb + DAT_005c8464) & 1) == 0) {
    puVar1 = (undefined4 *)FUN_004505a0(local_8,param_1);
    iVar2 = FUN_00450a20(*puVar1,puVar1[1],*(undefined1 *)(iVar2 + 0x22));
    if (iVar2 != 0) {
      if ((*(byte *)(iVar2 * 0x32 + 0xb + DAT_005c8464) & 3) == 0) {
        FUN_00450670(iVar2 * 0x32 + DAT_005c8464,0x5104,
                     (float)param_2 * (float)s_333_drop_value____0_00501784._0_4_);
      }
    }
  }
  return;
}


// ==== FUN_00450d40 @ 00450d40

void __thiscall FUN_00450d40(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004ac550();
  FUN_004acee0(this,*(undefined4 *)(iVar1 + 0x150),0);
  return;
}


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_004ac220 @ 004ac220

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004ac220(void *this,undefined2 param_2)

{
  *(undefined2 *)((int)this + 0xc) = param_2;
  if (*(byte *)((int)this + 10) == _DAT_005411a0) {
    FUN_004214a0(((int)this - DAT_005c8464) / 0x32);
  }
  return;
}


// ==== FUN_00455930 @ 00455930

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __cdecl FUN_00455930(float param_1,float param_2)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (float10)(DAT_00532710 >> 0x10) * (float10)_DAT_005092cc *
         ABS((float10)param_2 - (float10)param_1) + (float10)param_1;
}


// ==== FUN_004505a0 @ 004505a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004505a0(float *param_1,int param_2)

{
  float fVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  float local_4;
  
  if (param_2 == 0) {
    FUN_004e0620(&DAT_00501390,".\\Dude.cpp",0x29);
  }
  local_4 = *(float *)(param_2 * 0x32 + 0x12 + DAT_005c8464);
  iVar4 = param_2 * 0x32 + DAT_005c8464;
  FUN_004ac550(iVar4);
  iVar2 = FUN_004e49c0();
  fVar1 = (float)iVar2 + _DAT_00500ee4;
  iVar2 = FUN_004ac550(iVar4);
  local_4 = local_4 - (float)*(int *)(iVar2 + 0x1d8);
  uVar3 = FUN_004ac200(iVar4);
  if ((DAT_005424b8 & uVar3) == 0) {
    local_4 = local_4 - _DAT_00500ee4;
  }
  else {
    uVar3 = FUN_004ac200(iVar4);
    if ((uVar3 & 0x10040000) != 0) {
      local_4 = local_4 + _DAT_005012d0;
    }
  }
  *param_1 = fVar1;
  param_1[1] = local_4;
  return;
}


