// ==== FUN_0044a710 @ 0044a710

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0044a710(void *this,int param_2,undefined4 param_3,float param_4)

{
  ushort uVar1;
  bool bVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  float10 fVar8;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  if (param_2 == 0) {
    uVar1 = *(ushort *)((int)this + 0xc);
    *(ushort *)((int)this + 0xc) = ((uVar1 & 0xf) == 0 ^ uVar1) & 0xf ^ uVar1;
    FUN_00448c10(((int)this - DAT_005395dc) / 0x24);
    FUN_004acbc0(this,(*(byte *)((int)this + 0x22) & 0xfffe) + (*(ushort *)((int)this + 0xc) & 0xf),
                 0);
    fVar8 = (float10)FUN_0048cc50(0x41700000,0x420c0000);
    param_4 = (float)fVar8;
  }
  else if (param_2 == 1) {
    uVar3 = __ftol();
    uVar3 = __ftol(uVar3);
    iVar4 = FUN_0048cc10(uVar3);
    uVar3 = __ftol();
    uVar3 = __ftol(uVar3);
    iVar5 = FUN_0048cc10(uVar3);
    if (iVar4 < 10) {
      iVar4 = 10;
    }
    if (iVar5 < 10) {
      iVar5 = 10;
    }
    if (0xf6 < iVar4) {
      iVar4 = 0xf6;
    }
    if (0xf6 < iVar5) {
      iVar5 = 0xf6;
    }
    uVar3 = __ftol();
    local_40[0] = &PTR_FUN_005003d0;
    iVar7 = 0;
    bVar2 = false;
    local_8 = 0;
    local_4 = 0;
    FUN_004eae20(local_40,iVar4 + -1,iVar5 + -3,iVar4 + 1,uVar3);
    while ((local_c != 0 && (!bVar2))) {
      iVar7 = iVar7 + 1;
      iVar4 = local_c * 0x24 + DAT_005395dc;
      bVar2 = false;
      if (*(byte *)(iVar4 + 10) == DAT_0051cadc) {
        uVar6 = FUN_004acd60(iVar4);
        if ((uVar6 & 8) == 0) {
          uVar6 = FUN_004acd60(iVar4);
          if ((uVar6 & 0x10) == 0) goto LAB_0044a8eb;
          iVar5 = *(byte *)(iVar4 + 0x22) - 1;
        }
        else {
          iVar5 = *(byte *)(iVar4 + 0x22) + 1;
        }
        bVar2 = true;
        FUN_004acbc0(iVar4,iVar5,0);
      }
LAB_0044a8eb:
      FUN_004eafe0(local_40);
    }
    if (iVar7 <= DAT_0051f12c) {
      iVar7 = DAT_0051f12c;
    }
    param_4 = 1.0;
    DAT_0051f12c = iVar7;
  }
  else if (param_2 == 2) {
    FUN_004e8cb0(((int)this - DAT_005395dc) / 0x24);
  }
  FUN_0048f430(param_2,param_3,param_4);
  return;
}


// ==== FUN_00416070 @ 00416070

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00416070(void *this,int param_2,undefined4 param_3,float param_4)

{
  ushort uVar1;
  int iVar2;
  float10 fVar3;
  
  if (param_2 == 0) {
    FUN_004acce0(this,1,0);
    uVar1 = *(ushort *)((int)this + 0xc);
    iVar2 = FUN_004abf50(this);
    if (DAT_00546778 == 0) {
      fVar3 = (float10)uVar1 / (float10)*(float *)(iVar2 + 0xc4);
    }
    else {
      fVar3 = (float10)__adj_fdivr_m32((float)uVar1);
    }
    fVar3 = (float10)_DAT_00500674 - fVar3 * (float10)_DAT_00500670;
    if ((float10)_DAT_00500678 <= fVar3) {
      fVar3 = (float10)_DAT_00500678;
    }
    param_4 = (float)fVar3;
  }
  FUN_00461a40(this,param_2,param_3,param_4);
  return;
}


// ==== FUN_0044efd0 @ 0044efd0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0044efd0(void *this,undefined4 param_2,int param_3,float param_4)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  float10 fVar5;
  undefined4 uVar6;
  undefined1 local_108 [4];
  undefined1 local_104;
  undefined2 local_103;
  undefined1 local_101;
  undefined4 local_100 [64];
  
  switch(param_2) {
  case 0:
    iVar3 = FUN_0044ee30(this,param_3);
    if (iVar3 == 0) {
      fVar5 = (float10)_DAT_00501d10;
    }
    else {
      fVar5 = (float10)param_4;
    }
    break;
  case 1:
    iVar3 = 1;
    if (DAT_00540a1c != 0) {
      iVar3 = FUN_004ae4b0(this);
      iVar2 = FUN_004abf50(this);
      iVar3 = *(int *)(iVar2 + 0x170 + iVar3 * 4) + -1;
    }
    FUN_004acce0(this,iVar3,0x2000);
    fVar5 = (float10)param_4;
    break;
  case 2:
    FUN_004abf50(this);
    if (((byte)param_3 & 0x1f) == 0x1f) {
      FUN_004acbc0(this,0,0x2000);
      param_4 = 2.0;
    }
    else {
      if (*(char *)((int)this + 0x22) == '\0') {
        FUN_004acbc0(this,1,0x2000);
      }
      uVar6 = 0x2000;
      iVar3 = FUN_0048cbd0(2,0x2000);
      FUN_004acce0(this,iVar3 + 1,uVar6);
      param_4 = 0.1;
    }
    fVar5 = (float10)param_4;
    break;
  case 3:
    if (param_3 == 0) {
      if (*(short *)((int)this + 0xc) == 2) goto LAB_0044f276;
      FUN_004abb00(this,2);
      FUN_00401920(local_100,*(undefined1 *)((int)this + 0x20));
      FUN_0048ca70(0x1d,s_Player__s_is_leaving__0051f9c4,local_100);
    }
    FUN_004bdba0(&DAT_00583188,*(undefined1 *)((int)this + 0x20));
    if (DAT_00540a2c == 0) {
      bVar1 = 0x16 < param_3;
    }
    else {
      bVar1 = true;
      if (param_3 < 3) {
        bVar1 = false;
      }
    }
    if (!bVar1) {
      return (float10)_DAT_00501d1c;
    }
    if (DAT_0052e16c == 0) {
      FUN_0040a890(local_108);
      local_103 = (undefined2)(((int)this - DAT_005395dc) / 0x24);
      local_100[0] = *(undefined4 *)((int)this + 0xe);
      local_104 = FUN_004bdc50(&DAT_00583188,local_100[0],*(undefined4 *)((int)this + 0x12));
      local_101 = 1;
      FUN_004c0420(local_108);
      FUN_0044ea00(9,0,local_108);
      if ((*(byte *)((int)this + 0xb) & 1) != 0) {
        return (float10)_DAT_00501d10;
      }
    }
    goto LAB_0044f276;
  case 4:
    if (param_3 == 0) {
      FUN_004abb00(this,4);
    }
    if ((DAT_00531454 == 0) && ((float)param_3 <= _DAT_00501d04)) {
      return (float10)_DAT_00501d1c;
    }
    local_100[0] = *(undefined4 *)((int)this + 0xe);
    uVar4 = FUN_004bdcf0(&DAT_00583188,local_100[0],*(undefined4 *)((int)this + 0x12));
    uVar4 = DAT_0052e8f0 << 0x10 | uVar4;
    FUN_004a6aa0(DAT_005128cc,uVar4,uVar4);
    FUN_004abb00(this,0);
LAB_0044f276:
    fVar5 = (float10)_DAT_00501d0c;
    break;
  default:
    fVar5 = (float10)FUN_00461a40(this,param_2,param_3,param_4);
  }
  return fVar5;
}


// ==== FUN_00461a40 @ 00461a40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00461a40(void *this,int param_2,uint param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  float10 fVar3;
  
  if (param_2 == DAT_00520fdc) {
    FUN_00455b40(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
    iVar1 = (**(code **)(*(int *)this + 0x98))(param_3 & 0xffff,(int)param_3 >> 0x10,0);
    if (iVar1 == 0) {
      uVar2 = FUN_004abae0(this);
      if ((uVar2 & 0x200000) == 0) {
        return (float10)_DAT_00502b74;
      }
    }
    return (float10)_DAT_00502b78;
  }
  if (param_2 != DAT_00520fe0) {
    fVar3 = (float10)FUN_0048f430(param_2,param_3,param_4);
    return fVar3;
  }
  if ((*(byte *)((int)this + 0x23) & 1) == 0) {
    return (float10)_DAT_00502b78;
  }
  iVar1 = FUN_004eb310(((int)this - DAT_005395dc) / 0x24,DAT_0051c9d8,0,0);
  if (iVar1 == 0) {
    FUN_004d06a0(((int)this - DAT_005395dc) / 0x24,0);
  }
  return (float10)_DAT_00502b78;
}


// ==== FUN_0048cbd0 @ 0048cbd0

uint __cdecl FUN_0048cbd0(uint param_1)

{
  if (DAT_005308ec == 0) {
    DAT_005308ec = 0xbad0bad;
  }
  DAT_005308ec = DAT_005308ec * 0x10003 + 3;
  return (DAT_005308ec >> 0x10 & 0xffffU) % param_1;
}


// ==== FUN_0048cc10 @ 0048cc10

int __cdecl FUN_0048cc10(int param_1,int param_2)

{
  if (DAT_005308ec == 0) {
    DAT_005308ec = 0xbad0bad;
  }
  DAT_005308ec = DAT_005308ec * 0x10003 + 3;
  return param_1 + (DAT_005308ec >> 0x10 & 0xffffU) % (uint)(param_2 - param_1);
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


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __thiscall FUN_004abae0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_0048f430 @ 0048f430

float10 FUN_0048f430(undefined4 param_1,undefined4 param_2,float param_3)

{
  return (float10)param_3;
}


// ==== FUN_004ae4b0 @ 004ae4b0

int __thiscall FUN_004ae4b0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004abf50();
  return *(char *)(*(int *)(iVar1 + 0x124) + (uint)*(byte *)((int)this + 0x22) * 4) + -0x41;
}


// ==== FUN_004acd60 @ 004acd60

int __thiscall FUN_004acd60(void *this)

{
  return (int)*(char *)(*(int *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0x124 + DAT_0051c960) + 3
                       + (uint)*(byte *)((int)this + 0x22) * 4);
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


// ==== FUN_004ac440 @ 004ac440

undefined4 __thiscall FUN_004ac440(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0x98 + DAT_0051c960);
}


