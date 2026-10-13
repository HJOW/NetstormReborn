// ==== FUN_0049a4e0 @ 0049a4e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0049a4e0(void *this,undefined4 param_2,undefined4 param_3,float param_4)

{
  char cVar1;
  bool bVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  uint uVar6;
  float10 fVar7;
  int local_4c;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  switch(param_2) {
  case 0:
    *(ushort *)((int)this + 0xc) =
         *(ushort *)((int)this + 0xc) ^
         (byte)(1U - ((*(byte *)((int)this + 0xc) & 0xf) != 0) ^ *(byte *)((int)this + 0xc)) & 0xf;
    FUN_004214a0(((int)this - DAT_005c8464) / 0x32);
    FUN_004acee0(this,(*(byte *)((int)this + 0xc) & 0xf) + (*(int *)((int)this + 0x24) / 2) * 2,0);
    fVar7 = (float10)FUN_00455930(0x41700000,0x420c0000);
    param_4 = (float)fVar7;
    break;
  case 1:
    uVar4 = FUN_004e49c0();
    uVar4 = FUN_004e49c0(uVar4);
    iVar3 = FUN_004558f0(uVar4);
    uVar4 = FUN_004e49c0();
    uVar4 = FUN_004e49c0(uVar4);
    iVar5 = FUN_004558f0(uVar4);
    if (iVar3 < 10) {
      iVar3 = 10;
    }
    if (iVar5 < 10) {
      iVar5 = 10;
    }
    if (0xf6 < iVar3) {
      iVar3 = 0xf6;
    }
    if (0xf6 < iVar5) {
      iVar5 = 0xf6;
    }
    local_40[0] = &PTR_FUN_00502318;
    local_8 = 0;
    local_4 = 0;
    uVar4 = FUN_004e49c0();
    FUN_004b16d0(local_40,iVar3 + -1,iVar5 + -3,iVar3 + 1,uVar4);
    bVar2 = false;
    local_4c = 0;
    while ((local_c != 0 && (!bVar2))) {
      local_4c = local_4c + 1;
      iVar3 = local_c * 0x32 + DAT_005c8464;
      bVar2 = false;
      if (*(byte *)(iVar3 + 10) == DAT_005411f0) {
        uVar6 = FUN_004ad060(iVar3);
        if ((uVar6 & 8) == 0) {
          uVar6 = FUN_004ad060(iVar3);
          if ((uVar6 & 0x10) == 0) goto LAB_0049a7b6;
          iVar5 = *(int *)(iVar3 + 0x24) + -1;
        }
        else {
          iVar5 = *(int *)(iVar3 + 0x24) + 1;
        }
        FUN_004acee0(iVar3,iVar5,0);
        bVar2 = true;
      }
LAB_0049a7b6:
      FUN_004b1810(local_40);
    }
    if (DAT_0059a9e4 <= local_4c) {
      DAT_0059a9e4 = local_4c;
    }
    param_4 = 1.0;
    break;
  case 2:
    FUN_00450a90(((int)this - DAT_005c8464) / 0x32);
    break;
  case 3:
    iVar3 = FUN_0040eaf0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
    if (iVar3 != 0) {
      cVar1 = *(char *)(iVar3 * 0x32 + 0x22 + DAT_005c8464);
      if (cVar1 == '\0') {
        (**(code **)(*(int *)this + 0x74))(0);
      }
      else {
        (**(code **)(*(int *)this + 0x74))(cVar1);
      }
    }
    iVar3 = 0;
    if (0 < DAT_005e18a4) {
      do {
        if (*(char *)(*(int *)(DAT_005e189c + iVar3 * 4) * 0x32 + DAT_005c8464 + 0x22) ==
            *(char *)((int)this + 0x22)) {
          uVar4 = FUN_004ac970();
          iVar3 = FUN_004544a0(uVar4);
          if (iVar3 == 1) {
            iVar3 = *(int *)((int)this + 0x2a) + 6;
            goto LAB_0049a608;
          }
          if (iVar3 == 2) {
            iVar3 = *(int *)((int)this + 0x2a) + 0xc;
            goto LAB_0049a608;
          }
          if (iVar3 == 3) {
            iVar3 = *(int *)((int)this + 0x2a) + 0x12;
            goto LAB_0049a608;
          }
          break;
        }
        iVar3 = iVar3 + 1;
      } while (iVar3 < DAT_005e18a4);
    }
    iVar3 = *(int *)((int)this + 0x2a);
LAB_0049a608:
    FUN_004acee0(this,iVar3,0);
    FUN_004acee0(this,(*(byte *)((int)this + 0xc) & 0xf) + (*(int *)((int)this + 0x24) / 2) * 2,0);
    param_4 = 0.1;
  }
  FUN_00496e70(param_2,param_3,param_4);
  return;
}


// ==== FUN_00485640 @ 00485640

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00485640(void *this,int param_2,undefined4 param_3,undefined4 param_4)

{
  ushort uVar1;
  float fVar2;
  int iVar3;
  
  (**(code **)(*(int *)this + 0x88))();
  if (param_2 != 0) goto LAB_00485787;
  uVar1 = *(ushort *)((int)this + 0xc);
  iVar3 = FUN_004ac550(this);
  if ((float)uVar1 / *(float *)(iVar3 + 0xc4) <= _DAT_0050db54) {
LAB_004856dd:
    uVar1 = *(ushort *)((int)this + 0xc);
    iVar3 = FUN_004ac550(this);
    if (_DAT_00500ee4 < (float)uVar1 / *(float *)(iVar3 + 0xc4)) {
      uVar1 = *(ushort *)((int)this + 0xc);
      iVar3 = FUN_004ac550(this);
      fVar2 = (float)uVar1 / *(float *)(iVar3 + 0xc4);
      if (fVar2 < _DAT_0050db54 != (fVar2 == _DAT_0050db54)) {
        if (*(int *)((int)this + 0x24) < 0x20) {
          *(int *)((int)this + 0x24) = *(int *)((int)this + 0x24) + 1;
          param_4 = 0x3d75c28f;
        }
        else {
          *(undefined4 *)((int)this + 0x24) = 0x11;
          param_4 = 0x3d75c28f;
        }
        goto LAB_0048577d;
      }
    }
    if (*(int *)((int)this + 0x24) < 0x30) {
      *(int *)((int)this + 0x24) = *(int *)((int)this + 0x24) + 1;
    }
    else {
      *(undefined4 *)((int)this + 0x24) = 0x21;
    }
    param_4 = 0x3db851ec;
  }
  else {
    uVar1 = *(ushort *)((int)this + 0xc);
    iVar3 = FUN_004ac550(this);
    fVar2 = (float)uVar1 / *(float *)(iVar3 + 0xc4);
    if (fVar2 < _DAT_005012d0 == (fVar2 == _DAT_005012d0)) goto LAB_004856dd;
    if (*(int *)((int)this + 0x24) < 0x10) {
      *(int *)((int)this + 0x24) = *(int *)((int)this + 0x24) + 1;
      param_4 = 0x3cf5c28f;
    }
    else {
      *(undefined4 *)((int)this + 0x24) = 0;
      param_4 = 0x3cf5c28f;
    }
  }
LAB_0048577d:
  (**(code **)(*(int *)this + 0x88))();
LAB_00485787:
  FUN_0044b900(this,param_2,param_3,param_4);
  return;
}


// ==== FUN_004dc220 @ 004dc220

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_004dc220(void *this,undefined4 param_2,int param_3,float param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  bool bVar4;
  float10 fVar5;
  undefined4 uVar6;
  undefined1 local_108 [4];
  undefined1 local_104;
  short local_103;
  undefined1 local_101;
  undefined1 local_100 [256];
  
  switch(param_2) {
  case 0:
    iVar3 = FUN_004db8e0(this,param_3);
    if (iVar3 == 0) {
      return (float10)_DAT_00502350;
    }
    break;
  case 1:
    if (DAT_00594fb8 == 0) {
      iVar3 = 1;
    }
    else {
      iVar3 = FUN_004ad710(this);
      iVar1 = FUN_004ac550(this);
      iVar3 = *(int *)(iVar1 + 0x170 + iVar3 * 4) + -1;
    }
    FUN_004afc90(this,iVar3,0x2000);
    break;
  case 2:
    FUN_004ac550(this);
    if (((byte)param_3 & 0x1f) != 0x1f) {
      if (*(int *)((int)this + 0x24) == 0) {
        FUN_004acee0(this,1,0x2000);
      }
      uVar6 = 0x2000;
      iVar3 = FUN_004558c0(2,0x2000);
      FUN_004afc90(this,iVar3 + 1,uVar6);
      return (float10)_DAT_005022ec;
    }
    FUN_004acee0(this,0,0x2000);
    return (float10)_DAT_00501444;
  case 3:
    if (param_3 == 0) {
      if (*(short *)((int)this + 0xc) == 2) goto LAB_004dc491;
      FUN_004ac220(this,2);
      FUN_00484480(local_100,*(undefined1 *)((int)this + 0x22));
      FUN_004c2630(0x1d,"Player %s is leaving!\n",local_100);
    }
    FUN_0046f480(&DAT_00568b60,*(undefined1 *)((int)this + 0x22));
    if (DAT_00540bc4 == 0) {
      bVar4 = SBORROW4(param_3,0x17);
      iVar3 = -0x17;
    }
    else {
      bVar4 = SBORROW4(param_3,3);
      iVar3 = -3;
    }
    if (bVar4 != param_3 + iVar3 < 0) {
LAB_004dc449:
      return (float10)_DAT_005012d0;
    }
    if (DAT_00594fa0 == 0) {
      FUN_0040aa20(local_108);
      iVar3 = (int)this - DAT_005c8464;
      local_103 = ((short)(iVar3 / 0x32) + (short)(iVar3 >> 0x1f)) -
                  (short)((longlong)iVar3 * 0x51eb851f >> 0x3f);
      local_104 = FUN_0046f530(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
      local_101 = 1;
      FUN_004e2e30(local_108);
      FUN_004dc050(9,0,local_108);
      if ((*(byte *)((int)this + 0xb) & 1) != 0) {
        return (float10)_DAT_00502350;
      }
    }
    goto LAB_004dc491;
  case 4:
    if (param_3 == 0) {
      FUN_004ac220(this,4);
    }
    if ((DAT_005e4794 == 0) && ((float)param_3 <= _DAT_00502314)) goto LAB_004dc449;
    uVar2 = FUN_0046f650(&DAT_00568b60,*(undefined4 *)((int)this + 0xe),
                         *(undefined4 *)((int)this + 0x12));
    uVar2 = DAT_0054db4c << 0x10 | uVar2;
    FUN_00452fe0(DAT_00542744,uVar2,uVar2);
    FUN_004ac220(this,0);
LAB_004dc491:
    return (float10)_DAT_00500fc4;
  default:
    fVar5 = (float10)FUN_0044b900(this,param_2,param_3,param_4);
    return fVar5;
  }
  return (float10)param_4;
}


// ==== FUN_0044b900 @ 0044b900

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0044b900(void *this,int param_2,uint param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  float10 fVar3;
  
  if (param_2 == DAT_005326c0) {
    FUN_00470ac0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
    iVar1 = (**(code **)(*(int *)this + 0x98))(param_3 & 0xffff,(int)param_3 >> 0x10,0);
    if (iVar1 == 0) {
      uVar2 = FUN_004ac200(this);
      if ((uVar2 & 0x200000) == 0) {
        return (float10)_DAT_00502350;
      }
    }
  }
  else {
    if (param_2 != DAT_005326c4) {
      fVar3 = (float10)FUN_00496e70(param_2,param_3,param_4);
      return fVar3;
    }
    if ((*(byte *)((int)this + 0x28) & 1) != 0) {
      iVar1 = FUN_004b1a90(((int)this - DAT_005c8464) / 0x32,DAT_005410e8,0,0);
      if (iVar1 == 0) {
        FUN_00443fe0(((int)this - DAT_005c8464) / 0x32,0);
      }
    }
  }
  return (float10)_DAT_00500fc4;
}


// ==== FUN_004558c0 @ 004558c0

uint __cdecl FUN_004558c0(uint param_1)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % param_1;
}


// ==== FUN_004558f0 @ 004558f0

int __cdecl FUN_004558f0(int param_1,int param_2)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % (uint)(param_2 - param_1) + param_1;
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


// ==== FUN_00496e70 @ 00496e70

float10 FUN_00496e70(undefined4 param_1,undefined4 param_2,float param_3)

{
  return (float10)param_3;
}


// ==== FUN_004ad710 @ 004ad710

int __thiscall FUN_004ad710(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4) + -0x41;
}


// ==== FUN_004ad060 @ 004ad060

int __thiscall FUN_004ad060(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return (int)*(char *)(*(int *)(iVar1 + 0x124) + 3 + *(int *)((int)this + 0x24) * 4);
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


// ==== FUN_004ac970 @ 004ac970

undefined4 __thiscall FUN_004ac970(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0x98);
}


