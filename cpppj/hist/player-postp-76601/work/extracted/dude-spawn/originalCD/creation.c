// ==== FUN_004e8b30 @ 004e8b30

int __cdecl FUN_004e8b30(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  undefined4 unaff_retaddr;
  
  if ((DAT_005b72c0 < DAT_005b72bc) && (DAT_005417c8 == 0)) {
    piVar1 = (int *)FUN_004ab390(DAT_0051ca5c,2);
    *(undefined1 *)((int)piVar1 + 0x22) = 0;
    (**(code **)(*piVar1 + 0x74))(param_3);
    (**(code **)(*piVar1 + 0x90))(unaff_retaddr,param_1,0);
    return ((int)piVar1 - DAT_005395dc) / 0x24;
  }
  return 0;
}


// ==== FUN_004e9230 @ 004e9230

void __thiscall FUN_004e9230(void *this,uint param_2)

{
  int iVar1;
  
  if ((param_2 & 1) != 0) {
    FUN_004abb00(this,0x5101);
    if (DAT_005b72c0 < DAT_005b72bc) {
      *(int *)(DAT_005b72b8 + DAT_005b72c0 * 4) = ((int)this - DAT_005395dc) / 0x24;
      DAT_005b72c0 = DAT_005b72c0 + 1;
    }
    iVar1 = FUN_004abf50(this);
    FUN_004acbc0(this,*(undefined4 *)(iVar1 + 0x150),0);
  }
  FUN_004ae180(this,param_2);
  return;
}


// ==== FUN_004e8b10 @ 004e8b10

undefined4 * __cdecl FUN_004e8b10(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(DAT_005395dc + param_1 * 0x24);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_005072f0;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004e8d20 @ 004e8d20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8d20(int param_1,int param_2)

{
  float fVar1;
  float fVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  float10 fVar6;
  float10 fVar7;
  
  if ((param_1 != 0) && (iVar5 = DAT_005395dc + param_1 * 0x24, (*(byte *)(iVar5 + 0xb) & 1) == 0))
  {
    fVar1 = *(float *)(iVar5 + 0x12);
    fVar2 = *(float *)(iVar5 + 0xe);
    iVar3 = FUN_004abf50(iVar5);
    fVar6 = (float10)FUN_0048cc50(0x3dcccccd,(float)*(int *)(iVar3 + 0x1b4) - _DAT_005072c8);
    iVar3 = FUN_004abf50(iVar5);
    fVar7 = (float10)FUN_0048cc50(0x3dcccccd,(float)*(int *)(iVar3 + 0x1b8) - _DAT_005072c8);
    uVar4 = FUN_004e8b30((float)((float10)fVar2 - fVar6),(float)((float10)fVar1 - fVar7),
                         *(undefined1 *)(iVar5 + 0x20),0x5103,(float)param_2 * _DAT_005072c8);
    FUN_004e8ba0(uVar4);
  }
  return;
}


// ==== FUN_004e8e00 @ 004e8e00

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8e00(int param_1,int param_2)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  int iVar3;
  undefined1 local_8 [8];
  
  if ((param_1 != 0) && (iVar3 = param_1 * 0x24 + DAT_005395dc, (*(byte *)(iVar3 + 0xb) & 1) == 0))
  {
    puVar1 = (undefined4 *)
             FUN_004e8a30(local_8,param_1,*(undefined1 *)(iVar3 + 0x20),0x5104,
                          (float)param_2 * _DAT_005072e8);
    uVar2 = FUN_004e8b30(*puVar1,puVar1[1]);
    FUN_004e8ba0(uVar2);
  }
  return;
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_004abb00 @ 004abb00

void __thiscall FUN_004abb00(void *this,undefined2 param_2)

{
  *(undefined2 *)((int)this + 0xc) = param_2;
  if (*(byte *)((int)this + 10) == DAT_0051ca8c) {
    FUN_00448c10(((int)this - DAT_005395dc) / 0x24);
  }
  return;
}


// ==== FUN_0048cc50 @ 0048cc50

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __cdecl FUN_0048cc50(float param_1,float param_2)

{
  if (DAT_005308ec == 0) {
    DAT_005308ec = 0xbad0bad;
  }
  DAT_005308ec = DAT_005308ec * 0x10003 + 3;
  return ABS((float10)param_2 - (float10)param_1) *
         (float10)(DAT_005308ec >> 0x10 & 0xffff) * (float10)_DAT_005045f8 + (float10)param_1;
}


// ==== FUN_004e8a30 @ 004e8a30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8a30(float *param_1,int param_2)

{
  float fVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  float local_c;
  
  if (param_2 == 0) {
    FUN_00490040(&DAT_00541818,s_Dude_cpp_0054180c,0x29);
  }
  iVar4 = DAT_005395dc + param_2 * 0x24;
  local_c = *(float *)(iVar4 + 0x12);
  FUN_004abf50(iVar4);
  iVar2 = __ftol();
  fVar1 = (float)iVar2 + _DAT_005072b4;
  iVar2 = FUN_004abf50(iVar4);
  local_c = local_c - (float)*(int *)(iVar2 + 0x1b8);
  uVar3 = FUN_004abae0(iVar4);
  if ((DAT_005395cc & uVar3) == 0) {
    local_c = local_c - _DAT_005072b4;
  }
  else {
    uVar3 = FUN_004abae0(iVar4);
    if ((uVar3 & 0x10040000) != 0) {
      local_c = local_c + _DAT_005072b8;
    }
  }
  *param_1 = fVar1;
  param_1[1] = local_c;
  return;
}


