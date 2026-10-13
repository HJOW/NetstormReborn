// ==== FUN_004add10 @ 004add10

undefined4 __thiscall FUN_004add10(void *this,int param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  int unaff_retaddr;
  
  if ((*(byte *)((int)this + 0xb) & 2) != 0) {
    FUN_004e0620("!isDead()","\\Ns\\O\\Squid.cpp",0x9b1);
  }
  if ((*(byte *)((int)this + 0xb) & 3) == 0) {
    if (DAT_0055a48c != 0) {
      param_2 = 0;
    }
    if ((*(byte *)((int)this + 0x28) & 9) == 0) {
      iVar1 = (**(code **)(*(int *)this + 0x9c))(param_4);
      if (iVar1 != 0) {
        return 1;
      }
      FUN_0044bfe0(&stack0x00000000,param_2,*(undefined1 *)((int)this + 0x22));
      iVar1 = *(int *)((int)this + 0x1a) - unaff_retaddr;
      if (iVar1 < 1) {
        if (DAT_00540bc0 != 0) {
          DAT_005c845c = ((int)this - DAT_005c8464) / 0x32;
          DAT_005c8460 = param_2;
          if (param_2 == 0) {
            uVar3 = 0;
          }
          else {
            uVar3 = param_2 << 0x10;
          }
          uVar4 = uVar3 | 0x100000;
          if (param_4 != 0) {
            uVar2 = FUN_004ac200(param_4 * 0x32 + DAT_005c8464);
            if ((uVar2 & 0x100) != 0) {
              uVar4 = uVar3 | 0x4100000;
            }
          }
          (**(code **)(*(int *)this + 0x10))(uVar4);
          return 0;
        }
        *(undefined4 *)((int)this + 0x1a) = 1;
      }
      else {
        *(int *)((int)this + 0x1a) = iVar1;
      }
      FUN_00416760(*(undefined1 *)((int)this + 0x22),((int)this - DAT_005c8464) / 0x32,param_4);
      return 1;
    }
    if (DAT_005e4794 != 0) {
      FUN_004e0620("false","\\Ns\\O\\Squid.cpp",0x9bd);
    }
  }
  return 1;
}


// ==== FUN_0044bfe0 @ 0044bfe0

void __cdecl FUN_0044bfe0(int *param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  
  if (((DAT_00594fa4 == 0) && (param_2 != 0)) && (param_3 != 0)) {
    iVar1 = FUN_0041c5b0(&DAT_0052f7e0,param_2 + -1);
    iVar2 = FUN_0041c5b0(&DAT_0052f7e0,param_3 + -1);
    if (iVar2 < iVar1) {
      *param_1 = (((iVar1 - iVar2) + 0x14) * *param_1) / 0x14;
      return;
    }
    if (iVar1 < iVar2) {
      *param_1 = (*param_1 * 0x14) / ((iVar2 - iVar1) + 0x14);
    }
  }
  return;
}


// ==== FUN_0044c060 @ 0044c060

int __thiscall FUN_0044c060(void *this)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  
  if ((*(byte *)((int)this + 0xb) & 1) == 0) {
    iVar2 = FUN_004ac550();
    if (*(int *)(iVar2 + 0xd4) != 0) {
      iVar2 = *(int *)((int)this + 0x1a);
      uVar3 = FUN_004ac550(this);
      iVar4 = FUN_0049fce0(uVar3);
      iVar1 = ((int)this - DAT_005c8464) / 0x32;
      iVar5 = FUN_004b1a90(iVar1,DAT_005410a8,0,0);
      if (DAT_005e4794 != 0) {
        iVar6 = FUN_004b1a90(iVar1,DAT_005410a8,1,0);
        if (iVar6 != 0) {
          FUN_004e0620("findProcess (mySid, burningSystemProcessType, 1 ) == 0",
                       "\\Ns\\O\\Damageable.cpp",0x210);
        }
      }
      if (iVar2 < (int)(iVar4 * 2 + (iVar4 * 2 >> 0x1f & 3U)) >> 2) {
        if (iVar5 == 0) {
          if (*(byte *)((int)this + 10) == DAT_00541274) {
            FUN_004a9d70(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                         "bulfHurt.wav",0);
          }
          iVar2 = FUN_00424090(iVar1);
          if (iVar2 != 0) {
            iVar5 = *(int *)(iVar2 + 4);
          }
        }
      }
      else if (iVar5 != 0) {
        (**(code **)(*(int *)(iVar5 * 0x32 + DAT_005c8464) + 0x10))(0);
        return 0;
      }
      return iVar5;
    }
  }
  return 0;
}


// ==== FUN_0044c8e0 @ 0044c8e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall FUN_0044c8e0(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  undefined1 local_100 [256];
  
  iVar1 = FUN_004adc60();
  if (iVar1 != 0) {
    if (*(byte *)((int)this + 0x22) == _DAT_00540c70) {
      iVar1 = FUN_0040eaf0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
      iVar3 = iVar1 * 0x32 + DAT_005c8464;
      iVar1 = FUN_004acdf0(iVar3);
      if ((iVar1 != 0) &&
         (iVar4 = (uint)*(byte *)((int)this + 0x22) * 0xb4 + DAT_0059534c,
         iVar1 = FUN_004acda0(iVar3), iVar1 == *(int *)(iVar4 + 0x78))) {
        iVar1 = FUN_004180a0(&DAT_0055a450);
        if (iVar1 != 0) {
          FUN_004d3090(0xc,0,0,0,&DAT_00503c8b);
          FUN_004a9cb0("islandUnderAttack.wav",0,0,0,1,0);
          uVar2 = FUN_004a89c0(local_100,"islandUnderAttack");
          FUN_004cf960(uVar2);
        }
        FUN_00418070(&DAT_0055a450,DAT_005326c8);
      }
    }
    if ((*(byte *)((int)this + 0x28) & 4) != 0) {
      (**(code **)(*(int *)this + 0x88))();
    }
    iVar1 = FUN_004add10(this,param_2,param_3,param_4);
    if (iVar1 != 0) {
      FUN_0044c060(this);
    }
    return iVar1;
  }
  return 0;
}


// ==== FUN_00494380 @ 00494380

undefined4 __thiscall FUN_00494380(void *this,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  if (((*(byte *)((int)this + 0xb) & 2) != 0) &&
     (FUN_004e0620("!isDead()",".\\Priest.cpp",0x1ed), (*(byte *)((int)this + 0xb) & 2) != 0)) {
    return 1;
  }
  if (DAT_0055a48c != 0) {
    param_2 = 0;
  }
  iVar1 = FUN_004afb40(this,0x25d);
  if ((iVar1 != 0) || ((iVar1 = FUN_004ad710(this), 9 < iVar1 && (iVar1 < 0xf)))) {
    FUN_00491ac0(this,0x25c);
    FUN_00491ac0(this,0x25d);
    uVar3 = 0;
    iVar1 = FUN_004ac550(this,0);
    FUN_004acee0(this,*(undefined4 *)(iVar1 + 0x118),uVar3);
  }
  FUN_0044bfe0(&param_2,param_3,*(undefined1 *)((int)this + 0x22));
  iVar1 = *(int *)((int)this + 0x1a) - param_2;
  if (iVar1 < 1) {
    iVar1 = 1;
  }
  FUN_004942b0(this,iVar1);
  FUN_00416760(*(undefined1 *)((int)this + 0x22),((int)this - DAT_005c8464) / 0x32,param_4);
  FUN_0044c060(this);
  if ((DAT_00540bc4 != 0) && (*(int *)((int)this + 0x1a) == 1)) {
    local_4 = 0;
    local_10[0] = &PTR_LAB_00508f14;
    FUN_004b22d0(local_10,((int)this - DAT_005c8464) / 0x32);
    while (local_8 != 0) {
      piVar2 = (int *)FUN_004b2390(local_10);
      (**(code **)(*piVar2 + 0x1c))(((int)this - DAT_005c8464) / 0x32);
      iVar1 = FUN_004b2390(local_10);
      if ((*(byte *)(iVar1 + 0xb) & 1) == 0) {
        uVar3 = FUN_004b2390(local_10,((int)this - DAT_005c8464) / 0x32,0);
        uVar3 = FUN_004ae510(uVar3);
        FUN_004ae780(uVar3);
      }
      FUN_004b1b80(local_10);
    }
    return 0;
  }
  return 1;
}


// ==== FUN_004942b0 @ 004942b0

void __thiscall FUN_004942b0(void *this,undefined4 param_2)

{
  undefined4 uVar1;
  bool bVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = *(int *)((int)this + 0x1a);
  iVar3 = FUN_004adcb0();
  uVar1 = *(undefined4 *)((int)this + 0x1a);
  bVar2 = iVar4 < iVar3 / 2;
  FUN_004adca0(this,param_2);
  iVar4 = *(int *)((int)this + 0x1a);
  iVar3 = FUN_004adcb0(this);
  if (iVar4 < iVar3 / 2 != bVar2) {
    if (DAT_00540bc4 != 0) {
      if ((bVar2) && (*(int *)((uint)*(byte *)((int)this + 0x22) * 0xb4 + 0x7c + DAT_0059534c) == 0)
         ) {
        FUN_004adca0(this,uVar1);
        return;
      }
      (**(code **)(*(int *)this + 0x48))(0);
      iVar4 = *(int *)((int)this + 0x1a);
      iVar3 = FUN_004adcb0(this);
      if (iVar4 < iVar3 / 2) {
        FUN_0041d7a0((int)this + 0xe);
      }
      (**(code **)(*(int *)this + 0x4c))(0);
      return;
    }
    iVar4 = FUN_004adcb0(this);
    iVar4 = iVar4 / 2;
    if (bVar2) {
      iVar4 = iVar4 + -1;
    }
    FUN_004adca0(this,iVar4);
  }
  return;
}


