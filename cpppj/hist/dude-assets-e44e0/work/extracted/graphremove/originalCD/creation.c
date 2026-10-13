// ==== FUN_00401130 @ 00401130

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00401130(double *param_1)

{
  undefined4 uVar1;
  uint local_c [2];
  uint local_4;
  
  uVar1 = DAT_0054842c;
  if (DAT_0050f250 != 0) {
    *(undefined4 *)param_1 = DAT_00548428;
    *(undefined4 *)((int)param_1 + 4) = uVar1;
    return;
  }
  FUN_004010c0(&local_4,local_c,1);
  *param_1 = ((double)local_4 + (double)local_c[0] * _DAT_00500008) - _DAT_00548498;
  return;
}


// ==== FUN_004015d0 @ 004015d0

undefined4 FUN_004015d0(void)

{
  return DAT_0050f6c8;
}


// ==== FUN_004071f0 @ 004071f0

void FUN_004071f0(int param_1,undefined4 param_2)

{
  int iVar1;
  int *piVar2;
  
  if (param_1 != 0) {
    piVar2 = (int *)(DAT_0050f834 + param_1 * 0xac);
    if (piVar2 == (int *)0x0) {
      FUN_00490040(&DAT_0050fb04,s__ns_o_Player_cpp_0050faf0,0x2c3);
    }
    else if (piVar2[2] < piVar2[1]) {
      iVar1 = piVar2[2] + 1;
      piVar2[2] = iVar1;
      *(undefined4 *)(*piVar2 + -4 + iVar1 * 4) = param_2;
    }
  }
  return;
}


// ==== FUN_0040bc20 @ 0040bc20

void FUN_0040bc20(int param_1)

{
  (&DAT_005111a0)[param_1] = (&DAT_005111a0)[param_1] + 1;
  return;
}


// ==== FUN_0040bc50 @ 0040bc50

void FUN_0040bc50(int param_1)

{
  (&DAT_005109a0)[param_1] = (&DAT_005109a0)[param_1] + 1;
  (&DAT_00510da0)[param_1] = (&DAT_00510da0)[param_1] + 1;
  return;
}


// ==== FUN_0040f180 @ 0040f180

undefined4 FUN_0040f180(void)

{
  return 1;
}


// ==== FUN_004147a0 @ 004147a0

float10 FUN_004147a0(int param_1)

{
  return (float10)*(float *)(param_1 * 0x1d4 + 0xc4 + DAT_0051c960);
}


// ==== FUN_0041eac0 @ 0041eac0

void FUN_0041eac0(void)

{
  int iVar1;
  int iVar2;
  
  if ((DAT_005134c8 != 0) &&
     (iVar1 = FUN_004eb4b0(DAT_00549b38,DAT_00549b3c,DAT_005134c8), iVar1 != 0)) {
    iVar2 = FUN_0041ea30();
    if ((iVar2 != 0) && (iVar2 = FUN_0041ea30(), iVar2 != iVar1)) {
      return;
    }
    FUN_0041eda0(iVar1,0,1);
    if ((*(byte *)(DAT_005395dc + iVar1 * 0x24 + 0x23) & 9) == 0) {
      FUN_0041eab0();
    }
  }
  return;
}


// ==== FUN_0043f620 @ 0043f620

undefined4 __thiscall FUN_0043f620(float *param_1,float param_2,float param_3,int param_4)

{
  param_2 = param_2 - *param_1;
  param_3 = param_3 - param_1[1];
  if (param_4 != 0) {
    if (ABS(param_2) <= ABS(param_3)) {
      if (0 < (int)param_3) {
        return 4;
      }
      return 0;
    }
    if (0 < (int)param_2) {
      return 2;
    }
    return 6;
  }
  if (0 < (int)param_2) {
    if (0x80000000 < (uint)param_3) {
      return 1;
    }
    if (0 < (int)param_3) {
      return 3;
    }
    return 2;
  }
  if ((uint)param_2 < 0x80000001) {
    if (0x80000000 < (uint)param_3) {
      return 0;
    }
    if (0 < (int)param_3) {
      return 4;
    }
    FUN_00490040(s_false_0051c480,s__ns_o_BoardCoord_cpp_0051c468,0xfa);
    return 0xffffffff;
  }
  if (0x80000000 < (uint)param_3) {
    return 7;
  }
  if (0 < (int)param_3) {
    return 5;
  }
  return 6;
}


// ==== FUN_0043f950 @ 0043f950

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_0043f950(float *param_1)

{
  float fVar1;
  
  fVar1 = *param_1;
  if (*param_1 <= _DAT_00501764) {
    fVar1 = _DAT_00501764;
  }
  *param_1 = fVar1;
  fVar1 = param_1[1];
  if (param_1[1] <= _DAT_00501764) {
    fVar1 = _DAT_00501764;
  }
  param_1[1] = fVar1;
  fVar1 = *param_1;
  if (_DAT_00501768 <= *param_1) {
    fVar1 = _DAT_00501768;
  }
  *param_1 = fVar1;
  fVar1 = param_1[1];
  if (_DAT_00501768 <= param_1[1]) {
    fVar1 = _DAT_00501768;
  }
  param_1[1] = fVar1;
  return;
}


// ==== FUN_00440a50 @ 00440a50

void __fastcall FUN_00440a50(float *param_1)

{
  int iVar1;
  
  iVar1 = __ftol();
  *param_1 = (float)iVar1;
  iVar1 = __ftol();
  param_1[1] = (float)iVar1;
  return;
}


// ==== FUN_00440a80 @ 00440a80

void __fastcall FUN_00440a80(float *param_1)

{
  int iVar1;
  
  iVar1 = __ftol();
  *param_1 = (float)iVar1;
  iVar1 = __ftol();
  param_1[1] = (float)iVar1;
  return;
}


// ==== FUN_00442f30 @ 00442f30

void FUN_00442f30(void)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  undefined4 local_4;
  
  if ((DAT_0051c54c != 0) && ((*(byte *)(DAT_0051c54c + 4) & 1) != 0)) {
    iVar1 = __ftol();
    iVar1 = (int)(iVar1 + (iVar1 >> 0x1f & 3U)) >> 2;
    iVar2 = __ftol();
    piVar3 = &DAT_005654d0;
    iVar4 = 0;
    iVar2 = (int)(iVar2 + (iVar2 >> 0x1f & 3U)) >> 2;
    if (0 < DAT_0051c540) {
      do {
        if ((*piVar3 == iVar1) && (piVar3[1] == iVar2)) {
          return;
        }
        iVar4 = iVar4 + 1;
        piVar3 = piVar3 + 3;
      } while (iVar4 < DAT_0051c540);
    }
    if (99 < DAT_0051c540) {
      DAT_0051c550 = 1;
      return;
    }
    (&DAT_005654d0)[DAT_0051c540 * 3] = iVar1;
    (&DAT_005654d4)[DAT_0051c540 * 3] = iVar2;
    iVar1 = DAT_0051c540 * 3;
    DAT_0051c540 = DAT_0051c540 + 1;
    (&DAT_005654d8)[iVar1] = local_4;
  }
  return;
}


// ==== FUN_004434d0 @ 004434d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004434d0(void)

{
  undefined *puVar1;
  float fVar2;
  float fVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined **ppuVar8;
  undefined4 *unaff_FS_OFFSET;
  char local_114 [256];
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00444218;
  *unaff_FS_OFFSET = &local_10;
  uVar4 = FUN_0049c1a0(0xfffffffe,PTR_DAT_00510940,0xfffffffe,PTR_DAT_0051094c,0xfffffffd,
                       s__shapes_shp_0051d73c,0,0);
  iVar5 = FUN_004bb6d0(uVar4);
  if (iVar5 == 0) {
    FUN_00490040(s_filename_0051d75c,s__ns_o_RiftType_cpp_0051d748,0xa0);
  }
  DAT_0051cbf4 = FUN_004be710(iVar5);
  if (DAT_0051cbf4 == 0) {
    FUN_00490040(s_masterShapeDatabasePtr_0051d77c,s__ns_o_RiftType_cpp_0051d768,0xa2);
  }
  iVar5 = DAT_0051cbf0;
  iVar6 = FUN_004f1650(DAT_0051cbf0 * 0x1d4);
  local_8 = 0;
  local_14 = iVar6;
  if (iVar6 == 0) {
    iVar6 = 0;
  }
  else {
    while (iVar5 = iVar5 + -1, -1 < iVar5) {
      FUN_00444230();
    }
  }
  local_8 = 0xffffffff;
  DAT_0051c960 = iVar6;
  FUN_00446080();
  iVar5 = DAT_0051c960;
  *(undefined1 **)(DAT_0051c968 * 0x1d4 + DAT_0051c960 + 0x1b0) = &LAB_004af5c0;
  *(undefined1 **)(DAT_0051c964 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004af920;
  *(undefined1 **)(DAT_0051c978 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004afe00;
  ppuVar8 = &PTR_s_evilRaySystemProcessType_0051c890;
  *(undefined1 **)(DAT_0051c96c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004afd60;
  iVar6 = 0x1248;
  for (iVar7 = 10; iVar7 < DAT_0051ca58; iVar7 = iVar7 + 1) {
    puVar1 = *ppuVar8;
    ppuVar8 = ppuVar8 + 1;
    FUN_004f2440(iVar6 + iVar5 + 4,puVar1,0x14);
    iVar5 = DAT_0051c960;
    iVar6 = iVar6 + 0x1d4;
  }
  *(undefined1 **)(DAT_0051c9bc * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047f310;
  *(undefined1 **)(DAT_0051c9c0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fc60;
  *(undefined1 **)(DAT_0051c9f8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047f850;
  *(undefined1 **)(DAT_0051c9fc * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047f680;
  *(undefined1 **)(DAT_0051ca00 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fdb0;
  *(undefined1 **)(DAT_0051ca04 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fbd0;
  *(undefined1 **)(DAT_0051ca08 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047f3a0;
  *(undefined1 **)(DAT_0051ca38 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fcf0;
  *(undefined1 **)(DAT_0051ca44 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0045f760;
  *(undefined1 **)(DAT_0051ca5c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e8b10;
  *(undefined1 **)(DAT_0051ca60 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435ff0;
  *(undefined1 **)(DAT_0051ca64 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00497950;
  *(undefined1 **)(DAT_0051ca68 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004979c0;
  *(undefined1 **)(DAT_0051ca70 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00459eb0;
  *(undefined1 **)(DAT_0051ca74 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00459eb0;
  *(undefined1 **)(DAT_0051ca78 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00459eb0;
  *(undefined1 **)(DAT_0051ca7c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c07d0;
  *(undefined1 **)(DAT_0051ca80 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c07f0;
  *(undefined1 **)(DAT_0051ca8c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004491a0;
  *(undefined1 **)(DAT_0051ca90 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435fd0;
  *(undefined1 **)(DAT_0051ca98 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047a190;
  *(undefined1 **)(DAT_0051ca9c * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047a190;
  *(undefined1 **)(DAT_0051caa0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047a190;
  *(undefined1 **)(DAT_0051caa4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047a190;
  *(undefined1 **)(DAT_0051caac * 0x1d4 + 0x1b0 + iVar5) = &LAB_004783c0;
  *(undefined1 **)(DAT_0051cabc * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0330;
  *(undefined1 **)(DAT_0051cac0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0380;
  *(undefined1 **)(DAT_0051cad0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00415d10;
  *(undefined1 **)(DAT_0051cad4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416d60;
  *(undefined1 **)(DAT_0051cad8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416d60;
  *(undefined1 **)(DAT_0051cae0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0048aeb0;
  *(undefined1 **)(DAT_0051cae8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047ca20;
  *(undefined1 **)(DAT_0051cb00 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7bc0;
  *(undefined1 **)(DAT_0051cb04 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416aa0;
  *(undefined1 **)(DAT_0051cb08 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004601a0;
  *(undefined1 **)(DAT_0051cb0c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004601d0;
  *(undefined1 **)(DAT_0051cb10 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460230;
  *(undefined1 **)(DAT_0051cb14 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460260;
  *(undefined1 **)(DAT_0051cb18 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460290;
  *(undefined1 **)(DAT_0051cb1c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004602c0;
  *(undefined1 **)(DAT_0051cb20 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004602f0;
  *(undefined1 **)(DAT_0051cb24 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460320;
  *(undefined1 **)(DAT_0051cb28 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416d80;
  *(undefined1 **)(DAT_0051cb2c * 0x1d4 + 0x1b0 + iVar5) = &LAB_00414d50;
  *(undefined1 **)(DAT_0051cb30 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044fda0;
  *(undefined1 **)(DAT_0051cb34 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044fd70;
  *(undefined1 **)(DAT_0051cb38 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044fdd0;
  *(undefined1 **)(DAT_0051cb3c * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047b540;
  *(undefined1 **)(DAT_0051cb44 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435f40;
  *(undefined1 **)(DAT_0051cb48 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435f40;
  *(undefined1 **)(DAT_0051cb50 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435fa0;
  *(undefined1 **)(DAT_0051cb54 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435fd0;
  *(undefined1 **)(DAT_0051cb5c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0c80;
  *(undefined1 **)(DAT_0051cb60 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7940;
  *(undefined1 **)(DAT_0051cb64 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004783f0;
  *(undefined1 **)(DAT_0051cb68 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7920;
  *(undefined1 **)(DAT_0051cb6c * 0x1d4 + 0x1b0 + iVar5) = &LAB_00497930;
  *(undefined1 **)(DAT_0051cb70 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004979a0;
  iVar6 = DAT_0051cb78;
  *(undefined1 **)(DAT_0051cb74 * 0x1d4 + iVar5 + 0x1b0) = &LAB_00435f60;
  *(undefined1 **)(iVar6 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7bc0;
  *(undefined1 **)(DAT_0051cb7c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c0810;
  *(undefined1 **)(DAT_0051cb80 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00497980;
  *(undefined1 **)(DAT_0051cb84 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00497910;
  *(undefined1 **)(DAT_0051cb88 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00491420;
  *(undefined1 **)(DAT_0051cb8c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7be0;
  *(undefined1 **)(DAT_0051cb90 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c07d0;
  *(undefined1 **)(DAT_0051cb94 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435f80;
  *(undefined1 **)(DAT_0051cb98 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00491440;
  *(undefined1 **)(DAT_0051cb9c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c07b0;
  *(undefined1 **)(DAT_0051cba8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00414110;
  *(undefined1 **)(DAT_0051cbac * 0x1d4 + 0x1b0 + iVar5) = &LAB_00414c20;
  *(undefined1 **)(DAT_0051cbb4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7960;
  *(undefined1 **)(DAT_0051cbb8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0400;
  *(undefined1 **)(DAT_0051cbbc * 0x1d4 + 0x1b0 + iVar5) = &LAB_0040d790;
  *(undefined1 **)(DAT_0051cbc0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044bfb0;
  *(undefined1 **)(DAT_0051cbc4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047c880;
  *(undefined1 **)(DAT_0051cbc8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0046de60;
  *(undefined1 **)(DAT_0051cbd0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004990f0;
  *(undefined1 **)(DAT_0051cbd4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483d80;
  *(undefined1 **)(DAT_0051cbd8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483500;
  *(undefined1 **)(DAT_0051cbdc * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483560;
  *(undefined1 **)(DAT_0051cbe0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483da0;
  *(undefined1 **)(DAT_0051cbe4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460200;
  *(undefined1 **)(DAT_0051cbe8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483520;
  *(undefined1 **)(DAT_0051cbec * 0x1d4 + 0x1b0 + iVar5) = &LAB_00478410;
  *(undefined1 **)(DAT_0051c988 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004b36a0;
  *(undefined1 **)(DAT_0051c98c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004cf190;
  *(undefined1 **)(DAT_0051c990 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004cf290;
  *(undefined1 **)(DAT_0051c994 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00455b90;
  *(undefined1 **)(DAT_0051c9b8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0045d3d0;
  *(code **)(DAT_0051ca28 * 0x1d4 + 0x1b0 + iVar5) = FUN_0048b950;
  *(code **)(DAT_0051ca2c * 0x1d4 + 0x1b0 + iVar5) = FUN_0048ba20;
  *(undefined1 **)(DAT_0051ca40 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004cf210;
  *(undefined1 **)(DAT_0051caa8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044a4c0;
  *(undefined1 **)(DAT_0051caec * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047c5a0;
  *(undefined1 **)(DAT_0051caf4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0046de00;
  *(undefined1 **)(DAT_0051caf8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0046de30;
  *(undefined1 **)(DAT_0051cafc * 0x1d4 + 0x1b0 + iVar5) = &LAB_00401bf0;
  *(undefined1 **)(DAT_0051cb58 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fd90;
  *(undefined1 **)(DAT_0051cba4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00407950;
  iVar6 = DAT_0051cbf0;
  _DAT_0051c6e8 = 0.0;
  _DAT_0051c6ec = 0.0;
  *(undefined1 **)(DAT_0051cbb0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004487a0;
  for (iVar7 = 0; iVar7 < iVar6; iVar7 = iVar7 + 1) {
    fVar2 = _DAT_0051c6e8;
    fVar3 = _DAT_0051c6ec;
    if (*(int *)(iVar5 + 0x9c) == 1) {
      fVar2 = *(float *)(iVar5 + 0x44) + _DAT_0050198c;
      if (fVar2 <= _DAT_0051c6e8) {
        fVar2 = _DAT_0051c6e8;
      }
    }
    else if ((*(int *)(iVar5 + 0x9c) == 0) &&
            (fVar3 = *(float *)(iVar5 + 0x44) + _DAT_00501990, fVar3 <= _DAT_0051c6ec)) {
      fVar3 = _DAT_0051c6ec;
    }
    _DAT_0051c6ec = fVar3;
    _DAT_0051c6e8 = fVar2;
    iVar5 = iVar5 + 0x1d4;
  }
  uVar4 = __ftol();
  _sprintf(local_114,s_global_crossbowCost__d_0051d794,uVar4);
  FUN_004a99b0(local_114);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004442b0 @ 004442b0

void __thiscall FUN_004442b0(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1b4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1c4) + *(int *)(param_1 + 0x1b8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1b8);
  return;
}


// ==== FUN_00444720 @ 00444720

int __fastcall FUN_00444720(int *param_1)

{
  int iVar1;
  
  if (DAT_00511c90 != 0) {
    iVar1 = FUN_004448e0();
    if (iVar1 == DAT_0051cbbc) {
      return (int)(*param_1 + (*param_1 >> 0x1f & 3U)) >> 2;
    }
  }
  return *param_1;
}


// ==== FUN_004448e0 @ 004448e0

int __fastcall FUN_004448e0(int param_1)

{
  return (param_1 - DAT_0051c960) / 0x1d4;
}


// ==== FUN_004449d0 @ 004449d0

float * FUN_004449d0(float *param_1,float param_2,float param_3,uint param_4)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  int local_8;
  int local_4;
  
  FUN_004442b0(&local_8,&local_4,param_4 & 1);
  fVar2 = (float)(1 - local_8) + param_2;
  pfVar1 = param_1 + 2;
  *param_1 = param_2;
  *pfVar1 = param_2;
  param_1[1] = param_3;
  param_1[3] = param_3;
  param_3 = (float)(1 - local_4) + param_3;
  FUN_0043f950();
  if (((((int)*param_1 < 1) || ((float)DAT_0052e9a8 <= *param_1)) || ((int)param_1[1] < 1)) ||
     ((float)DAT_0052e9a8 <= param_1[1])) {
    *param_1 = fVar2;
    param_1[1] = param_3;
  }
  else {
    fVar3 = *param_1;
    if (fVar2 <= *param_1) {
      fVar3 = fVar2;
    }
    *param_1 = fVar3;
    fVar3 = param_1[1];
    if (param_3 <= param_1[1]) {
      fVar3 = param_3;
    }
    param_1[1] = fVar3;
  }
  FUN_0043f950();
  if (((0 < (int)*pfVar1) && (*pfVar1 < (float)DAT_0052e9a8)) &&
     ((0 < (int)param_1[3] && (param_1[3] < (float)DAT_0052e9a8)))) {
    fVar3 = *pfVar1;
    if (*pfVar1 <= fVar2) {
      fVar3 = fVar2;
    }
    *pfVar1 = fVar3;
    fVar2 = param_1[3];
    if (param_1[3] <= param_3) {
      fVar2 = param_3;
    }
    param_1[3] = fVar2;
    return param_1;
  }
  *pfVar1 = fVar2;
  param_1[3] = param_3;
  return param_1;
}


// ==== FUN_00448c10 @ 00448c10

int FUN_00448c10(int param_1)

{
  return DAT_005395dc + param_1 * 0x24;
}


// ==== FUN_00455c90 @ 00455c90

void __thiscall FUN_00455c90(int *param_1,int *param_2)

{
  int iVar1;
  
  iVar1 = __ftol();
  *param_1 = iVar1 - DAT_00565c34;
  iVar1 = __ftol();
  iVar1 = iVar1 - DAT_00565c38;
  param_1[1] = iVar1;
  *param_2 = *param_1;
  param_2[1] = iVar1;
  return;
}


// ==== FUN_00456930 @ 00456930

void FUN_00456930(int *param_1,int param_2,int param_3,undefined4 param_4,undefined4 param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int local_10;
  int local_c;
  undefined1 local_8 [8];
  
  FUN_00455c90(local_8,param_4,param_5);
  iVar2 = *(int *)(param_2 * 0x1d4 + 0xdc + DAT_0051c960);
  if (iVar2 == 0) {
    *param_1 = 0;
    param_1[1] = 0;
    param_1[2] = 0;
    param_1[3] = 0;
    return;
  }
  iVar3 = *(int *)(iVar2 + 8 + param_3 * 8);
  iVar4 = iVar3 + -0x24 + iVar2;
  iVar1 = *(short *)(iVar4 + 0x18) * DAT_00565c30;
  iVar3 = local_10 - (*(short *)(iVar3 + -8 + iVar2) * DAT_00565c30 >> 0x10);
  iVar2 = *(short *)(iVar4 + 0x1a) * DAT_00565c30;
  iVar4 = local_c - (*(short *)(iVar4 + 0x1e) * DAT_00565c30 >> 0x10);
  *param_1 = iVar3;
  param_1[1] = iVar4;
  param_1[2] = (iVar1 >> 0x10) + iVar3 + 1;
  param_1[3] = iVar4 + (iVar2 >> 0x10) + 1;
  return;
}


// ==== FUN_00456a00 @ 00456a00

void FUN_00456a00(int param_1,int param_2,undefined4 param_3,undefined4 param_4,uint param_5)

{
  int iVar1;
  int *piVar2;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined1 local_10 [16];
  
  if (DAT_0052039c == 0) {
    FUN_00456930(&local_20,param_1,param_2,param_3,param_4);
    iVar1 = DAT_00583e28;
    if (local_20 <= DAT_00583e28) {
      iVar1 = local_20;
    }
    local_20 = iVar1;
    if (iVar1 <= DAT_00583e20) {
      local_20 = DAT_00583e20;
    }
    iVar1 = DAT_00583e2c;
    if (local_1c <= DAT_00583e2c) {
      iVar1 = local_1c;
    }
    local_1c = iVar1;
    if (iVar1 <= DAT_00583e24) {
      local_1c = DAT_00583e24;
    }
    iVar1 = DAT_00583e28;
    if (local_18 <= DAT_00583e28) {
      iVar1 = local_18;
    }
    local_18 = iVar1;
    if (iVar1 <= DAT_00583e20) {
      local_18 = DAT_00583e20;
    }
    iVar1 = DAT_00583e2c;
    if (local_14 <= DAT_00583e2c) {
      iVar1 = local_14;
    }
    local_14 = iVar1;
    if (iVar1 <= DAT_00583e24) {
      local_14 = DAT_00583e24;
    }
    if ((param_5 & 0x10) != 0) {
      if (local_18 - local_20 < 0x13) {
        local_20 = local_20 + -9;
        local_18 = local_18 + 9;
      }
      local_1c = local_1c + -9;
      local_20 = local_20 + -3;
      local_18 = local_18 + 3;
    }
    if ((local_18 != local_20 && -1 < local_18 - local_20) &&
       (local_14 != local_1c && -1 < local_14 - local_1c)) {
      FUN_00457320(&local_20,param_5);
    }
    iVar1 = DAT_0051c960 + param_1 * 0x1d4;
    if ((*(byte *)(iVar1 + 0xea) & 4) != 0) {
      piVar2 = (int *)FUN_00456930(local_10,param_1,*(int *)(iVar1 + 0x114) + param_2,param_3,
                                   param_4);
      local_20 = DAT_00583e28;
      if (*piVar2 <= DAT_00583e28) {
        local_20 = *piVar2;
      }
      if (local_20 <= DAT_00583e20) {
        local_20 = DAT_00583e20;
      }
      local_1c = DAT_00583e2c;
      if (piVar2[1] <= DAT_00583e2c) {
        local_1c = piVar2[1];
      }
      if (local_1c <= DAT_00583e24) {
        local_1c = DAT_00583e24;
      }
      local_18 = DAT_00583e28;
      if (piVar2[2] <= DAT_00583e28) {
        local_18 = piVar2[2];
      }
      if (local_18 <= DAT_00583e20) {
        local_18 = DAT_00583e20;
      }
      local_14 = DAT_00583e2c;
      if (piVar2[3] <= DAT_00583e2c) {
        local_14 = piVar2[3];
      }
      if (local_14 <= DAT_00583e24) {
        local_14 = DAT_00583e24;
      }
      if ((local_18 != local_20 && -1 < local_18 - local_20) &&
         (local_14 != local_1c && -1 < local_14 - local_1c)) {
        FUN_00457320(&local_20,param_5);
      }
    }
  }
  return;
}


// ==== FUN_00457320 @ 00457320

void __thiscall FUN_00457320(int param_1,int *param_2,uint param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int *piVar11;
  int iVar12;
  int iVar13;
  uint *local_20;
  int local_14;
  
  iVar1 = *param_2;
  if ((((iVar1 < DAT_00516b10) && (iVar2 = param_2[1], iVar2 < DAT_00516b14)) &&
      (iVar3 = param_2[2], 0 < iVar3)) && (iVar12 = param_2[3], 0 < iVar12)) {
    if (99 < *(int *)(param_1 + 100)) {
      DAT_0052039c = 1;
      return;
    }
    iVar8 = (iVar12 - iVar2) * (iVar3 - iVar1);
    local_14 = 0;
    if (0 < *(int *)(param_1 + 100)) {
      piVar11 = (int *)(param_1 + 0x68);
      local_20 = (uint *)(param_1 + 0x6a8);
      do {
        if (DAT_005203bc != 0) break;
        uVar4 = *local_20;
        if ((uVar4 & 1) == 0) {
          iVar5 = (piVar11[3] - piVar11[1]) * (piVar11[2] - *piVar11);
          iVar13 = *piVar11;
          if (iVar1 <= *piVar11) {
            iVar13 = iVar1;
          }
          iVar10 = piVar11[1];
          if (iVar2 <= piVar11[1]) {
            iVar10 = iVar2;
          }
          iVar6 = piVar11[2];
          if (piVar11[2] <= iVar3) {
            iVar6 = iVar3;
          }
          iVar7 = piVar11[3];
          if (piVar11[3] <= iVar12) {
            iVar7 = iVar12;
          }
          iVar9 = (iVar7 - iVar10) * (iVar6 - iVar13);
          if (((param_3 & 4) == 0) || (iVar8 != iVar9)) {
            if (((uVar4 & 4) != 0) && (iVar9 - iVar5 == 0)) {
              return;
            }
            if (iVar9 - ((int)(iVar9 + (iVar9 >> 0x1f & 7U)) >> 3) < iVar8 + iVar5) {
              *piVar11 = iVar13;
              piVar11[1] = iVar10;
              piVar11[2] = iVar6;
              piVar11[3] = iVar7;
              return;
            }
          }
          else {
            *local_20 = uVar4 | 1;
          }
        }
        piVar11 = piVar11 + 4;
        local_14 = local_14 + 1;
        local_20 = local_20 + 1;
      } while (local_14 < *(int *)(param_1 + 100));
    }
    iVar12 = *(int *)(param_1 + 100) * 0x10 + param_1;
    iVar1 = param_2[1];
    *(int *)(iVar12 + 0x68) = *param_2;
    iVar2 = param_2[2];
    iVar3 = param_2[3];
    *(int *)(iVar12 + 0x6c) = iVar1;
    *(int *)(iVar12 + 0x70) = iVar2;
    *(int *)(iVar12 + 0x74) = iVar3;
    *(uint *)(param_1 + 0x6a8 + *(int *)(param_1 + 100) * 4) = param_3 & 0xfffffffe;
    *(int *)(param_1 + 100) = *(int *)(param_1 + 100) + 1;
  }
  return;
}


// ==== FUN_0045b4d0 @ 0045b4d0

void __fastcall FUN_0045b4d0(undefined2 *param_1)

{
  param_1[1] = 0;
  *param_1 = 0;
  return;
}


// ==== FUN_0045b4e0 @ 0045b4e0

int __fastcall FUN_0045b4e0(int param_1)

{
  return (param_1 - DAT_005207f0) / 6;
}


// ==== FUN_0045b520 @ 0045b520

void FUN_0045b520(void)

{
  return;
}


// ==== FUN_0045b530 @ 0045b530

void __fastcall FUN_0045b530(short *param_1)

{
  short sVar1;
  
  if (((param_1[1] != 0) && (0 < *param_1)) && (sVar1 = *param_1 + -1, *param_1 = sVar1, sVar1 == 0)
     ) {
    FUN_0045b520();
    FUN_0045b4d0();
  }
  return;
}


// ==== FUN_0045b560 @ 0045b560

void FUN_0045b560(void)

{
  return;
}


// ==== FUN_0045b6f0 @ 0045b6f0

undefined4 FUN_0045b6f0(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  
  if (param_1 == 0) {
    return 0;
  }
  iVar3 = DAT_005395dc + param_1 * 0x24;
  uVar1 = FUN_004abac0();
  if ((uVar1 & 0x800) != 0) {
    uVar2 = FUN_004adbc0();
    return uVar2;
  }
  uVar2 = FUN_0045b670(*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12));
  return uVar2;
}


// ==== FUN_0045b750 @ 0045b750

int FUN_0045b750(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = FUN_0045b670(param_1,param_2);
  if (iVar1 == 0xfe) {
    return 0;
  }
  return DAT_005207f0 + iVar1 * 6;
}


// ==== FUN_0045b7a0 @ 0045b7a0

int FUN_0045b7a0(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_0045b6f0(param_1);
  if (iVar1 == 0xfe) {
    return 0;
  }
  return DAT_005207f0 + iVar1 * 6;
}


// ==== FUN_0045b7d0 @ 0045b7d0

int FUN_0045b7d0(int param_1)

{
  short *psVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  bool bVar5;
  int iVar6;
  int iVar7;
  undefined2 uVar11;
  int iVar8;
  int iVar9;
  undefined4 uVar10;
  int iVar12;
  int *piVar13;
  int local_218;
  int local_214;
  int local_1cc;
  int aiStack_104 [65];
  
  FUN_0045b560();
  iVar6 = DAT_005395dc + param_1 * 0x24;
  fVar2 = *(float *)(iVar6 + 0xe);
  fVar3 = *(float *)(iVar6 + 0x12);
  iVar7 = iVar6 + 0xe;
  if (0 < (int)fVar2) {
    fVar4 = (float)DAT_0052e9a8;
    uVar11 = (undefined2)((uint)iVar7 >> 0x10);
    iVar7 = CONCAT22(uVar11,(ushort)(fVar4 < fVar2) << 8 | (ushort)(NAN(fVar4) || NAN(fVar2)) << 10
                            | (ushort)(fVar4 == fVar2) << 0xe);
    if ((((fVar4 >= fVar2 && (fVar4 == fVar2) == 0) && (0 < (int)fVar3)) &&
        (iVar7 = CONCAT22(uVar11,(ushort)(fVar3 < fVar4) << 8 |
                                 (ushort)(NAN(fVar3) || NAN(fVar4)) << 10 |
                                 (ushort)(fVar3 == fVar4) << 0xe), fVar3 < fVar4)) &&
       (iVar7 = CONCAT31((int3)((uint)iVar6 >> 8),*(byte *)(iVar6 + 0xb)),
       (*(byte *)(iVar6 + 0xb) & 7) == 0)) {
      iVar12 = 0;
      piVar13 = aiStack_104;
      local_218 = 0x7fffffff;
      local_214 = -0x7fffffff;
      for (iVar7 = 0x40; piVar13 = piVar13 + 1, iVar7 != 0; iVar7 = iVar7 + -1) {
        *piVar13 = 0;
      }
      FUN_004ebad0(param_1,8);
      if (DAT_005395dc + 0x24U <= (uint)(local_1cc * 0x24 + DAT_005395dc)) {
        do {
          if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= (uint)(local_1cc * 0x24 + DAT_005395dc))
          break;
          bVar5 = true;
          iVar7 = FUN_004adbc0();
          if (iVar7 == DAT_005207f4) {
LAB_0045b92e:
            bVar5 = false;
          }
          else {
            iVar7 = 0;
            if (0 < iVar12) {
              do {
                iVar8 = FUN_004adbc0();
                iVar9 = FUN_004adbc0();
                if (iVar8 == iVar9) goto LAB_0045b92e;
                iVar7 = iVar7 + 1;
              } while (iVar7 < iVar12);
            }
          }
          if (bVar5) {
            iVar12 = iVar12 + 1;
            aiStack_104[iVar12] = (local_1cc * 0x24) / 0x24;
            iVar7 = FUN_004adbc0();
            iVar7 = (int)*(short *)(DAT_005207f0 + iVar7 * 6);
            if (local_214 < iVar7) {
              local_218 = FUN_0045b4e0();
              local_214 = iVar7;
            }
          }
          FUN_004ec030();
        } while (DAT_005395dc + 0x24U <= (uint)(local_1cc * 0x24 + DAT_005395dc));
      }
      if (iVar12 == 0) {
        iVar7 = FUN_0045c390();
        if (*(short *)(iVar7 + 2) != 1) {
          FUN_00490040(s_g_>inUse____true_00520840,s__ns_o_graph_cpp_00520830,0xf3);
        }
        uVar10 = FUN_0045b4e0();
        FUN_0045bb90((iVar6 - DAT_005395dc) / 0x24,uVar10);
      }
      else {
        if ((local_218 < 0) || (0xfa < local_218)) {
          FUN_00490040(s_biggestConnection_>__0____bigges_00520864,s__ns_o_graph_cpp_00520854,0xfb);
        }
        psVar1 = (short *)(DAT_005207f0 + local_218 * 6);
        if (psVar1[1] == 0) {
          psVar1[1] = 1;
        }
        FUN_004adbd0(local_218);
        *psVar1 = *psVar1 + 1;
        if (1 < iVar12) {
          piVar13 = aiStack_104 + 1;
          do {
            iVar6 = *piVar13;
            if ((iVar6 != 0) && (iVar7 = FUN_004adbc0(), iVar7 != local_218)) {
              FUN_0045b7a0(iVar6);
              FUN_0045bb90(iVar6,local_218);
              FUN_0045b4d0();
              FUN_0045b520();
            }
            piVar13 = piVar13 + 1;
          } while (piVar13 < &stack0x00000000);
        }
      }
      uVar10 = FUN_0045c440();
      iVar7 = FUN_0048ca70(0x20,s__d_graphs_in_use_005208a0,uVar10);
    }
  }
  return iVar7;
}


// ==== FUN_0045bb90 @ 0045bb90

int FUN_0045bb90(undefined4 param_1,int param_2)

{
  int iVar1;
  short *psVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  short *psVar6;
  int iVar7;
  undefined4 *puVar8;
  int local_104;
  int local_cc;
  
  FUN_0045b560();
  if (DAT_00520800 == (undefined4 *)0x0) {
    DAT_00520800 = (undefined4 *)FUN_004f1650(0x4000);
    FUN_0048ef00();
    puVar8 = DAT_00520800;
    for (iVar5 = 0x1000; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
  }
  *DAT_00520800 = param_1;
  local_104 = 0;
  iVar5 = 1;
  do {
    iVar7 = iVar5 + -1;
    uVar4 = DAT_00520800[iVar5 + -1];
    iVar1 = FUN_004adbc0();
    if (iVar1 != param_2) {
      psVar6 = (short *)0x0;
      if (iVar1 != 0xfe) {
        psVar6 = (short *)(DAT_005207f0 + iVar1 * 6);
      }
      psVar2 = (short *)0x0;
      if (param_2 != 0xfe) {
        psVar2 = (short *)(DAT_005207f0 + param_2 * 6);
      }
      local_104 = local_104 + 1;
      if (psVar6 != (short *)0x0) {
        *psVar6 = *psVar6 + -1;
      }
      if (psVar2 != (short *)0x0) {
        *psVar2 = *psVar2 + 1;
      }
      FUN_004adbd0(param_2);
      FUN_004ebad0(uVar4,8);
      iVar5 = iVar5 * 4 + -4;
      if (DAT_005395dc + 0x24U <= (uint)(local_cc * 0x24 + DAT_005395dc)) {
        do {
          if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= (uint)(local_cc * 0x24 + DAT_005395dc))
          break;
          iVar3 = FUN_004adbc0();
          iVar1 = iVar5;
          if (iVar3 != param_2) {
            iVar1 = iVar5 + 4;
            iVar7 = iVar7 + 1;
            *(int *)(iVar5 + (int)DAT_00520800) = (local_cc * 0x24) / 0x24;
            if (0x3fff < iVar1) {
              FUN_00490040(s_d_<_FLOOD_STACK_SIZE_005208ec,s__ns_o_graph_cpp_005208dc,0x17a);
            }
          }
          FUN_004ec030();
          iVar5 = iVar1;
        } while (DAT_005395dc + 0x24U <= (uint)(local_cc * 0x24 + DAT_005395dc));
      }
    }
    iVar5 = iVar7;
    if (iVar7 < 1) {
      uVar4 = FUN_0045c440();
      FUN_0048ca70(0x20,s__d_graphs_in_use_00520904,uVar4);
      FUN_0045b560();
      return local_104;
    }
  } while( true );
}


// ==== FUN_0045bfd0 @ 0045bfd0

void FUN_0045bfd0(char *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,int param_5
                 )

{
  short *psVar1;
  int iVar2;
  int iVar3;
  short *psVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;
  int iVar10;
  bool bVar11;
  undefined4 auStack_250 [5];
  undefined4 uStack_23c;
  char *pcStack_238;
  char *pcStack_234;
  undefined1 *puStack_230;
  undefined1 local_210 [8];
  undefined1 local_208 [60];
  int local_1cc;
  int local_100 [64];
  
  if (DAT_005207f8 != 0) {
    puStack_230 = (undefined1 *)0x45bfec;
    FUN_0045b560();
    puStack_230 = (undefined1 *)param_2;
    pcStack_234 = param_1;
    pcStack_238 = (char *)0x45c001;
    psVar1 = (short *)FUN_0045b750();
    iVar2 = DAT_0051c960 + param_5 * 0x1d4;
    if ((psVar1 != (short *)0x0) && (psVar1[1] != 0)) {
      iVar7 = 0;
      piVar6 = local_100;
      for (iVar5 = 0x40; iVar5 != 0; iVar5 = iVar5 + -1) {
        *piVar6 = 0;
        piVar6 = piVar6 + 1;
      }
      puStack_230 = (undefined1 *)0x0;
      puVar9 = auStack_250;
      puVar8 = (undefined4 *)register0x00000010;
      for (iVar5 = 8; puVar8 = puVar8 + 1, iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar9 = *puVar8;
        puVar9 = puVar9 + 1;
      }
      FUN_004ebbe0();
      if (DAT_005395dc + 0x24U <= (uint)(local_1cc * 0x24 + DAT_005395dc)) {
        piVar6 = local_100;
        do {
          if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= (uint)(DAT_005395dc + local_1cc * 0x24))
          break;
          iVar7 = iVar7 + 1;
          *piVar6 = (local_1cc * 0x24) / 0x24;
          if (&stack0x00000000 <= piVar6 + 1) {
            puStack_230 = (undefined1 *)0x1ef;
            pcStack_234 = s__ns_o_graph_cpp_00520918;
            pcStack_238 = s_numConnections_<_MAX_CONNECTIONS_00520928;
            uStack_23c = 0x45c0d2;
            FUN_00490040();
          }
          puStack_230 = (undefined1 *)0x45c0de;
          FUN_004ec030();
          piVar6 = piVar6 + 1;
        } while (DAT_005395dc + 0x24U <= (uint)(local_1cc * 0x24 + DAT_005395dc));
      }
      puStack_230 = local_208;
      pcStack_234 = (char *)0x45c109;
      FUN_00401130();
      bVar11 = DAT_005207fc == 0;
      iVar5 = iVar7;
      if (iVar7 != 0) {
        do {
          iVar10 = 0;
          iVar3 = 0;
          if (0 < iVar7) {
            piVar6 = local_100;
            do {
              if (*piVar6 != 0) {
                iVar10 = local_100[iVar3];
                local_100[iVar3] = 0;
                break;
              }
              piVar6 = piVar6 + 1;
              iVar3 = iVar3 + 1;
            } while (iVar3 < iVar7);
          }
          if (iVar10 == 0) {
            puStack_230 = (undefined1 *)0x20b;
            pcStack_234 = s__ns_o_graph_cpp_0052094c;
            pcStack_238 = s_floodSid____0_0052095c;
            uStack_23c = 0x45c16f;
            FUN_00490040();
          }
          pcStack_234 = (char *)0x45c178;
          puStack_230 = (undefined1 *)iVar10;
          psVar4 = (short *)FUN_0045b7a0();
          if (psVar4 == psVar1) {
            if ((DAT_005207fc == 0) && (iVar5 < 2)) {
              if ((*(byte *)(iVar2 + 0xec) & 8) == 0) {
                bVar11 = true;
              }
            }
            else {
              puStack_230 = (undefined1 *)0x45c1a9;
              FUN_0045c390();
              puStack_230 = (undefined1 *)0x45c1b0;
              puStack_230 = (undefined1 *)FUN_0045b4e0();
              pcStack_238 = (char *)0x45c1b7;
              pcStack_234 = (char *)iVar10;
              FUN_0045bb90();
            }
          }
          iVar5 = iVar5 + -1;
        } while (iVar5 != 0);
        if (param_5 == DAT_0051cbcc) {
          *psVar1 = *psVar1 + -9;
        }
        else {
          *psVar1 = *psVar1 + -1;
        }
      }
      if (!bVar11) {
        puStack_230 = (undefined1 *)0x45c1e6;
        FUN_0045b4d0();
        puStack_230 = (undefined1 *)0x45c1ed;
        FUN_0045b520();
      }
      puStack_230 = local_210;
      pcStack_234 = (char *)0x45c1f7;
      FUN_00401130();
      puStack_230 = (undefined1 *)0x45c1ff;
      FUN_0045b560();
    }
  }
  return;
}


// ==== FUN_0045c210 @ 0045c210

int FUN_0045c210(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  undefined4 local_50;
  undefined4 local_4c;
  undefined **local_40;
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  FUN_004aeec0(&local_50,0);
  local_8 = 0;
  local_40 = &PTR_FUN_005003d0;
  local_4 = 0;
  FUN_00440a50();
  FUN_00440a80();
  uVar1 = __ftol();
  uVar2 = __ftol(uVar1);
  uVar3 = __ftol(uVar2);
  uVar4 = __ftol(uVar3);
  iVar5 = FUN_004eae20(uVar4,uVar3,uVar2,uVar1);
  while (local_c != 0) {
    uVar6 = FUN_004abac0();
    iVar5 = DAT_0052fe48;
    if ((uVar6 & 0x800) != 0) {
      iVar7 = local_c * 0x24 + DAT_005395dc;
      local_4c = *(undefined4 *)(iVar7 + 0x12);
      local_50 = *(undefined4 *)(iVar7 + 0xe);
      iVar7 = __ftol();
      iVar8 = __ftol();
      if ((*(byte *)(iVar7 * 0x100 + iVar8 + iVar5) & 8) != 0) {
        iVar5 = FUN_004adbc0();
        if (iVar5 != DAT_005207f4) {
          FUN_0045b530();
        }
        FUN_004adbd0(DAT_005207f4);
      }
    }
    FUN_004eafe0();
    iVar5 = DAT_005395dc;
  }
  return iVar5;
}


// ==== FUN_0045c390 @ 0045c390

int FUN_0045c390(void)

{
  int iVar1;
  short *psVar2;
  
  iVar1 = 0;
  psVar2 = (short *)(DAT_005207f0 + 2);
  do {
    if (*psVar2 == 0) {
      FUN_0045b4d0();
      iVar1 = DAT_005207f0 + iVar1 * 6;
      *(undefined2 *)(iVar1 + 2) = 1;
      return iVar1;
    }
    psVar2 = psVar2 + 3;
    iVar1 = iVar1 + 1;
  } while (iVar1 < 0xfb);
  FUN_0045bd60(1);
  iVar1 = 0;
  psVar2 = (short *)(DAT_005207f0 + 2);
  do {
    if (*psVar2 == 0) {
      FUN_0045b4d0();
      iVar1 = DAT_005207f0 + iVar1 * 6;
      *(undefined2 *)(iVar1 + 2) = 1;
      return iVar1;
    }
    psVar2 = psVar2 + 3;
    iVar1 = iVar1 + 1;
  } while (iVar1 < 0xfb);
  FUN_00490040(s_false_0052097c,s__ns_o_graph_cpp_0052096c,0x25d);
  return 0;
}


// ==== FUN_0045c440 @ 0045c440

int FUN_0045c440(void)

{
  int iVar1;
  int iVar2;
  short *psVar3;
  
  iVar1 = 0;
  psVar3 = (short *)(DAT_005207f0 + 2);
  iVar2 = 0xfb;
  do {
    if (*psVar3 != 0) {
      iVar1 = iVar1 + 1;
    }
    psVar3 = psVar3 + 3;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return iVar1;
}


// ==== FUN_0045c460 @ 0045c460

void FUN_0045c460(int param_1)

{
  undefined2 *puVar1;
  
  if (param_1 != 0xfe) {
    if ((param_1 < 0) || (0xfa < param_1)) {
      FUN_00490040(s_graphNum_>_0____graphNum_<_MAX_G_00520994,s__ns_o_graph_cpp_00520984,0x26f);
    }
    FUN_0045b560();
    puVar1 = (undefined2 *)(DAT_005207f0 + param_1 * 6);
    puVar1[1] = 0;
    *puVar1 = 0;
  }
  return;
}


// ==== FUN_0045ec90 @ 0045ec90

void FUN_0045ec90(undefined4 param_1)

{
  if (DAT_00566fe8 < DAT_00566fe4) {
    *(undefined4 *)(DAT_00566fe0 + DAT_00566fe8 * 4) = param_1;
    DAT_00566fe8 = DAT_00566fe8 + 1;
  }
  DAT_00520c10 = DAT_00520c10 + 1;
  return;
}


// ==== FUN_00479d20 @ 00479d20

int __thiscall FUN_00479d20(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_00490040(s_level_>_0____level_<_HASH_LEVEL__0052d5bc,s__ns_o_SquidHash_cpp_0052d5a8,0x52);
  }
  iVar1 = (&DAT_0052d598)[param_4];
  iVar2 = __ftol();
  iVar2 = (iVar2 / iVar1) * DAT_0052e9a8;
  iVar3 = __ftol();
  return (iVar2 / iVar1 + iVar3 / iVar1) * 2 + *(int *)(param_1 + 0xc + param_4 * 4);
}


// ==== FUN_00479d90 @ 00479d90

int __thiscall FUN_00479d90(int param_1,int param_2,int param_3,int param_4)

{
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_00490040(s_level_>_0____level_<_HASH_LEVEL__0052d5f8,s__ns_o_SquidHash_cpp_0052d5e4,0x5c);
  }
  return ((DAT_0052e9a8 * param_3) / (int)(&DAT_0052d598)[param_4] + param_2) * 2 +
         *(int *)(param_1 + 0xc + param_4 * 4);
}


// ==== FUN_00487810 @ 00487810

uint FUN_00487810(int param_1,int param_2,int param_3,int param_4)

{
  byte bVar1;
  uint uVar2;
  byte *pbVar3;
  int iVar4;
  int iVar5;
  byte *pbVar6;
  
  uVar2 = 0xffff;
  if (param_2 < 2) {
    param_2 = 1;
  }
  if (0xfe < param_2) {
    param_2 = 0xff;
  }
  if (param_4 < 2) {
    param_4 = 1;
  }
  if (0xfe < param_4) {
    param_4 = 0xff;
  }
  if (param_1 < 2) {
    param_1 = 1;
  }
  if (0xfe < param_1) {
    param_1 = 0xff;
  }
  if (param_3 < 2) {
    param_3 = 1;
  }
  if (0xfe < param_3) {
    param_3 = 0xff;
  }
  if (param_2 <= param_4) {
    iVar5 = (param_4 - param_2) + 1;
    pbVar6 = (byte *)(param_2 * 0x100 + DAT_0052fe48 + param_1);
    do {
      if (param_1 <= param_3) {
        iVar4 = (param_3 - param_1) + 1;
        pbVar3 = pbVar6;
        do {
          bVar1 = *pbVar3;
          pbVar3 = pbVar3 + 1;
          uVar2 = uVar2 & bVar1;
          iVar4 = iVar4 + -1;
        } while (iVar4 != 0);
      }
      pbVar6 = pbVar6 + 0x100;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
  }
  return uVar2;
}


// ==== FUN_0048a360 @ 0048a360

void FUN_0048a360(int param_1)

{
  bool bVar1;
  int iVar2;
  int *piVar3;
  
  iVar2 = 0;
  piVar3 = DAT_00567648;
  if (0 < DAT_00567650) {
    do {
      if (*piVar3 == param_1) {
        bVar1 = true;
        goto LAB_0048a393;
      }
      iVar2 = iVar2 + 1;
      piVar3 = piVar3 + 1;
    } while (iVar2 < DAT_00567650);
  }
  bVar1 = false;
LAB_0048a393:
  if ((!bVar1) && (DAT_00567650 < DAT_0056764c)) {
    DAT_00567648[DAT_00567650] = param_1;
    DAT_00567650 = DAT_00567650 + 1;
  }
  DAT_00520c10 = DAT_00520c10 + 1;
  return;
}


// ==== FUN_0048ca70 @ 0048ca70

void FUN_0048ca70(int param_1,char *param_2)

{
  char cVar1;
  bool bVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  char *pcVar6;
  char *pcVar7;
  
  if ((-1 < param_1) && (param_1 <= DAT_005308cc)) {
    cVar1 = *param_2;
    pcVar6 = (char *)&DAT_00567668;
    uVar4 = DAT_005308d4;
    while (DAT_005308d4 = uVar4, cVar1 == '~') {
      *pcVar6 = *param_2;
      pcVar6[1] = param_2[1];
      param_2 = param_2 + 2;
      pcVar6 = pcVar6 + 2;
      uVar4 = DAT_005308d4;
      cVar1 = *param_2;
    }
    if (0 < (int)uVar4) {
      uVar3 = uVar4 >> 2;
      pcVar7 = pcVar6;
      while (uVar3 != 0) {
        uVar3 = uVar3 - 1;
        builtin_strncpy(pcVar7,"    ",4);
        pcVar7 = pcVar7 + 4;
      }
      for (uVar3 = uVar4 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
        *pcVar7 = ' ';
        pcVar7 = pcVar7 + 1;
      }
      pcVar6 = pcVar6 + uVar4;
    }
    if (pcVar6 != param_2) {
      FUN_004bcd80(pcVar6,param_2,&stack0x0000000c);
    }
    if (DAT_005308d0 == (FILE *)0x0) {
      DAT_005308d0 = (FILE *)&DAT_00546bf0;
    }
    uVar4 = 0xffffffff;
    pcVar6 = (char *)&DAT_00567668;
    do {
      if (uVar4 == 0) break;
      uVar4 = uVar4 - 1;
      cVar1 = *pcVar6;
      pcVar6 = pcVar6 + 1;
    } while (cVar1 != '\0');
    bVar2 = false;
    for (iVar5 = ~uVar4 - 2;
        (-1 < iVar5 &&
        ((*(char *)((int)&DAT_00567668 + iVar5) == '\n' ||
         (*(char *)((int)&DAT_00567668 + iVar5) == '\r')))); iVar5 = iVar5 + -1) {
      *(undefined1 *)((int)&DAT_00567668 + iVar5) = 0;
      bVar2 = true;
    }
    if (bVar2) {
      iVar5 = -1;
      pcVar6 = (char *)&DAT_00567668;
      do {
        pcVar7 = pcVar6;
        if (iVar5 == 0) break;
        iVar5 = iVar5 + -1;
        pcVar7 = pcVar6 + 1;
        cVar1 = *pcVar6;
        pcVar6 = pcVar7;
      } while (cVar1 != '\0');
      *(undefined2 *)(pcVar7 + -1) = DAT_005308e0;
    }
    FUN_004aa990(&DAT_00567668);
    if ((DAT_005308d0 != (FILE *)0x0) && (DAT_005308dc != 0)) {
      FID_conflict__fwprintf((FILE *)&DAT_00546c10,(wchar_t *)&DAT_005308e4,&DAT_00567668);
    }
    FID_conflict__fwprintf(DAT_005308d0,(wchar_t *)&DAT_005308e8,&DAT_00567668);
    if (DAT_005308d8 != 0) {
      _fflush(DAT_005308d0);
    }
  }
  return;
}


// ==== FUN_004952f0 @ 004952f0

void __thiscall FUN_004952f0(int param_1,char *param_2)

{
  char cVar1;
  char *pcVar2;
  
  cVar1 = *param_2;
  while (cVar1 != '\0') {
    cVar1 = *param_2;
    pcVar2 = *(char **)(param_1 + 8);
    param_2 = param_2 + 1;
    *(char **)(param_1 + 8) = pcVar2 + 1;
    *pcVar2 = cVar1;
    cVar1 = *param_2;
  }
  return;
}


// ==== FUN_004aaad0 @ 004aaad0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004aaad0(int param_1)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  byte *pbVar5;
  undefined4 *puVar6;
  int iVar7;
  
  if (0 < param_1) {
    DAT_005395f4 = param_1;
    DAT_005395dc = (undefined4 *)FUN_0048ef10(param_1 * 0x24);
  }
  iVar2 = DAT_005395f4;
  puVar6 = DAT_005395dc;
  for (uVar4 = (uint)(DAT_005395f4 * 0x24) >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  pbVar5 = (byte *)((int)DAT_005395dc + 0x53);
  for (iVar3 = 2; iVar3 < iVar2; iVar3 = iVar3 + 1) {
    *pbVar5 = *pbVar5 | 1;
    pbVar5 = pbVar5 + 0x24;
  }
  iVar3 = 5;
  DAT_005395e0 = DAT_005395dc;
  DAT_005395e8 = (int)DAT_005395dc + 0xe;
  _DAT_005395f0 = 0;
  for (iVar7 = 0xb4; puVar6 = DAT_005395dc, iVar7 < 0x34bc0; iVar7 = iVar7 + 0x24) {
    iVar3 = iVar3 + 1;
    if (iVar7 / 0x24 == iVar3 % 6000) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(short *)((int)puVar6 + iVar7 + 4) = (short)(iVar3 % 6000);
    _DAT_005395f0 = _DAT_005395f0 + 1;
  }
  iVar3 = 6000;
  for (iVar7 = 0x34bc0; puVar6 = DAT_005395dc, iVar7 < 0x7b0c0; iVar7 = iVar7 + 0x24) {
    iVar3 = iVar3 + 1;
    if (iVar7 / 0x24 == iVar3 % 14000) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(short *)((int)puVar6 + iVar7 + 4) = (short)(iVar3 % 14000);
    _DAT_005395f0 = _DAT_005395f0 + 1;
  }
  iVar3 = 14000;
  iVar7 = 0x7b0c0;
  while (puVar1 = DAT_005395e0, puVar6 = DAT_005395dc, iVar3 < iVar2) {
    iVar3 = iVar3 + 1;
    if (iVar7 / 0x24 == iVar3 % iVar2) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(short *)((int)puVar6 + iVar7 + 4) = (short)(iVar3 % iVar2);
    iVar7 = iVar7 + 0x24;
    _DAT_005395f0 = _DAT_005395f0 + 1;
  }
  if (((int)DAT_005395e0 - (int)DAT_005395dc) / 0x24 == 5) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  *(undefined2 *)(puVar1 + 1) = 5;
  iVar2 = DAT_005395e8;
  DAT_005395e4 = DAT_005395dc + 0xd2e7;
  if ((DAT_005395e8 - (int)DAT_005395dc) / 0x24 == 6000) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  *(undefined2 *)(iVar2 + 4) = 6000;
  DAT_005395ec = DAT_005395dc + 0x1ec27;
  if (*(short *)(DAT_005395dc + 0x1ec28) != 0) {
    FUN_00490040(s_serverFreeTail_>getNext______0_00539634,s__ns_o_Squid_cpp_00539624,0xc0);
  }
  DAT_005395c0 = 1;
  return;
}


// ==== FUN_004aad30 @ 004aad30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004aad30(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int local_8;
  int local_4;
  
  iVar2 = 6000;
  iVar5 = 0;
  local_8 = 0;
  local_4 = 0;
  for (iVar4 = 0x34bc0; iVar4 < 0x7b0c0; iVar4 = iVar4 + 0x24) {
    iVar3 = DAT_005395dc + iVar4;
    if ((*(byte *)(iVar3 + 0xb) & 1) != 0) {
      if (local_8 == 0) {
        iVar1 = DAT_005395e8 - DAT_005395dc;
        iVar5 = DAT_005395e8;
        local_8 = iVar2;
      }
      else {
        iVar1 = iVar5 - DAT_005395dc;
      }
      if (iVar1 / 0x24 == iVar2) {
        FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
      }
      *(short *)(iVar5 + 4) = (short)iVar2;
      _DAT_005395f0 = _DAT_005395f0 + 1;
      iVar5 = iVar3;
      local_4 = iVar2;
    }
    iVar2 = iVar2 + 1;
  }
  if (local_8 == 0) {
    FUN_00490040(s_firstFree_00539664,s__ns_o_Squid_cpp_00539654,0xe1);
  }
  iVar5 = local_4 * 0x24 + DAT_005395dc;
  DAT_005395ec = iVar5;
  if ((iVar5 - DAT_005395dc) / 0x24 == 0) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  *(undefined2 *)(iVar5 + 4) = 0;
  return;
}


// ==== FUN_004aae40 @ 004aae40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_004aae40(uint param_1)

{
  undefined4 *puVar1;
  byte bVar2;
  ushort uVar3;
  ushort uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int *piVar8;
  int iVar9;
  int *piVar10;
  int iVar11;
  undefined4 *puVar12;
  
  if (DAT_005395c4 != 0) {
    FUN_00490040(s_disallowSquidAllocation____0_00539680,s__ns_o_Squid_cpp_00539670,0xe8);
  }
  iVar6 = DAT_005395dc;
  if ((param_1 & 1) != 0) {
    iVar11 = DAT_005395dc + 0x7b0c0;
    if ((*(byte *)(DAT_005395dc + 0x7b0cb) & 1) == 0) {
      FUN_00490040(s_predictableHead_>isFree___005396b0,s__ns_o_Squid_cpp_005396a0,0xf0);
    }
    uVar5 = DAT_005395c0 + 14000;
    if (DAT_005395f4 <= (int)uVar5) {
      FUN_00490040(s_sid_<_numSquids_005396dc,s__ns_o_Squid_cpp_005396cc,0xf2);
    }
    puVar1 = (undefined4 *)(uVar5 * 0x24 + DAT_005395dc);
    if ((*(byte *)((int)puVar1 + 0xb) & 1) == 0) {
      FUN_00490040(s_me_>isFree___005396fc,s__ns_o_Squid_cpp_005396ec,0xf4);
    }
    if (DAT_00540a28 != 0) {
      if (*(ushort *)(puVar1 + 1) - uVar5 != 1) {
        FUN_00490040(s_me_>getNext______sid_1_0053971c,s__ns_o_Squid_cpp_0053970c,0xf7);
      }
      iVar7 = (uint)*(ushort *)(puVar1 + 1) * 0x24 + DAT_005395dc;
      if (iVar7 == 0) {
        FUN_00490040(s_checkMe_00539744,s__ns_o_Squid_cpp_00539734,0xf9);
      }
      if ((*(byte *)(iVar7 + 0xb) & 1) == 0) {
        FUN_00490040(s_checkMe_>isFree___0053975c,s__ns_o_Squid_cpp_0053974c,0xfa);
      }
      uVar3 = *(ushort *)(puVar1 + 1);
      if ((iVar11 - DAT_005395dc) / 0x24 == (uint)uVar3) {
        FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
      }
      *(ushort *)(iVar6 + 0x7b0c4) = uVar3;
    }
    puVar12 = puVar1;
    for (iVar6 = 9; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar12 = 0;
      puVar12 = puVar12 + 1;
    }
    bVar2 = *(byte *)((int)puVar1 + 0xb);
    _DAT_005395f0 = _DAT_005395f0 + -1;
    DAT_005395c0 = DAT_005395c0 + 1;
    *(byte *)((int)puVar1 + 0xb) = bVar2 | 4;
    *(byte *)((int)puVar1 + 0xb) = bVar2 & 0xfe | 4;
    *(byte *)((int)puVar1 + 0xb) = bVar2 & 0xfc | 4;
    return uVar5;
  }
  param_1 = param_1 & 2;
  if (param_1 == 0) {
    if (DAT_00540a28 == 0) {
      FUN_00490040(s_server_00539780,s__ns_o_Squid_cpp_00539770,0x10a);
    }
    piVar10 = &DAT_005395e8;
    piVar8 = &DAT_005395ec;
  }
  else {
    piVar10 = &DAT_005395e0;
    piVar8 = &DAT_005395e4;
  }
  if ((uint)*(ushort *)(*piVar10 + 4) * 0x24 + DAT_005395dc == *piVar8) {
    if (param_1 != 0) {
      FUN_0041f3e0();
      FUN_004875b0(s_alCLIENT__Send_in__NetStorm_squi_00539788);
      FUN_00490040(&DAT_005397d0,s__ns_o_Squid_cpp_005397c0,0x118);
    }
    if (DAT_00540a2c == 0) {
      FUN_0041f3e0();
      FUN_004875b0(s__Server__Send_in__NetStorm_squid_005397e4);
      FUN_00490040(&DAT_0053982c,s__ns_o_Squid_cpp_0053981c,300);
    }
    else {
      iVar11 = 0x32;
      iVar6 = 2;
      if (2 < DAT_005395f4) {
        do {
          if ((*(byte *)(iVar6 * 0x24 + 0xb + DAT_005395dc) & 1) == 0) break;
          iVar6 = iVar6 + 1;
        } while (iVar6 < DAT_005395f4);
      }
      iVar7 = DAT_005395dc;
      iVar9 = DAT_005395f4;
      do {
        piVar8 = (int *)(iVar6 * 0x24 + iVar7);
        if ((piVar8 < (int *)(iVar7 + 0x24U)) || ((int *)(iVar9 * 0x24 + iVar7) <= piVar8)) break;
        if (*(byte *)((int)piVar8 + 10) == DAT_0051ca8c) {
          iVar11 = iVar11 + -1;
          (**(code **)(*piVar8 + 0x10))(0);
          iVar7 = DAT_005395dc;
          iVar9 = DAT_005395f4;
        }
      } while (iVar11 != 0);
      FUN_004c3dc0(s_BridgesKilled_005397d4);
    }
    if (param_1 == 0) {
      if (DAT_00540a28 == 0) {
        FUN_00490040(s_server_00539840,s__ns_o_Squid_cpp_00539830,0x130);
      }
      piVar10 = &DAT_005395e8;
      piVar8 = &DAT_005395ec;
    }
    else {
      piVar10 = &DAT_005395e0;
      piVar8 = &DAT_005395e4;
    }
  }
  uVar3 = *(ushort *)(*piVar10 + 4);
  puVar1 = (undefined4 *)((uint)uVar3 * 0x24 + DAT_005395dc);
  if ((*(byte *)((int)puVar1 + 0xb) & 1) == 0) {
    FUN_00490040(s_p_>isFree___00539858,s__ns_o_Squid_cpp_00539848,0x140);
  }
  if ((undefined4 *)*piVar8 == puVar1) {
    FUN_00490040(s_p_____freeTail_00539874,s__ns_o_Squid_cpp_00539864,0x141);
  }
  iVar6 = *piVar10;
  uVar4 = *(ushort *)(puVar1 + 1);
  if ((iVar6 - DAT_005395dc) / 0x24 == (uint)uVar4) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  *(ushort *)(iVar6 + 4) = uVar4;
  puVar12 = puVar1;
  for (iVar6 = 9; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar12 = 0;
    puVar12 = puVar12 + 1;
  }
  bVar2 = *(byte *)((int)puVar1 + 0xb);
  _DAT_005395f0 = _DAT_005395f0 + -1;
  *(byte *)((int)puVar1 + 0xb) = bVar2 | 4;
  *(byte *)((int)puVar1 + 0xb) = bVar2 & 0xfe | 4;
  *(byte *)((int)puVar1 + 0xb) = bVar2 & 0xfc | 4;
  return (uint)uVar3;
}


// ==== FUN_004ab250 @ 004ab250

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004ab250(int param_1)

{
  undefined1 uVar1;
  byte bVar2;
  int iVar3;
  undefined4 *puVar4;
  int *piVar5;
  undefined4 *puVar6;
  
  puVar4 = (undefined4 *)(DAT_005395dc + param_1 * 0x24);
  if ((*(byte *)((int)puVar4 + 0xb) & 4) == 0) {
    FUN_00490040(s_p_>isVoid___00539894,s__ns_o_Squid_cpp_00539884,0x153);
  }
  if ((*(byte *)((int)puVar4 + 0xb) & 1) != 0) {
    FUN_00490040(s__p_>isFree___005398b0,s__ns_o_Squid_cpp_005398a0,0x154);
  }
  FUN_004cff40(param_1,*(undefined1 *)((int)puVar4 + 10));
  uVar1 = *(undefined1 *)((int)puVar4 + 10);
  puVar6 = puVar4;
  for (iVar3 = 9; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  *(undefined1 *)((int)puVar4 + 10) = uVar1;
  if ((DAT_00540a28 == 0) && (5999 < param_1)) {
    *(byte *)((int)puVar4 + 0xb) = *(byte *)((int)puVar4 + 0xb) | 1;
    return;
  }
  if ((param_1 < 5) || (piVar5 = &DAT_005395e4, 5999 < param_1)) {
    piVar5 = &DAT_005395ec;
  }
  if ((4 < param_1) && (param_1 < 14000)) {
    iVar3 = *piVar5;
    if ((iVar3 - DAT_005395dc) / 0x24 == param_1) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(short *)(iVar3 + 4) = (short)param_1;
    iVar3 = (int)puVar4 - DAT_005395dc;
    *piVar5 = (int)puVar4;
    if (iVar3 / 0x24 == 0) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(undefined2 *)(puVar4 + 1) = 0;
    _DAT_005395f0 = _DAT_005395f0 + 1;
  }
  bVar2 = *(byte *)((int)puVar4 + 0xb);
  *(byte *)((int)puVar4 + 0xb) = bVar2 | 1;
  *(byte *)((int)puVar4 + 0xb) = bVar2 | 3;
  return;
}


// ==== FUN_004ab390 @ 004ab390

int * FUN_004ab390(int param_1,uint param_2)

{
  code *pcVar1;
  int iVar2;
  int *piVar3;
  char *pcVar4;
  char *pcVar5;
  undefined4 uVar6;
  
  iVar2 = FUN_004aae40(param_2);
  if ((param_2 & 1) == 0) {
    if (iVar2 < 14000) goto LAB_004ab3e2;
    uVar6 = 0x18b;
    pcVar5 = s__ns_o_Squid_cpp_005398f0;
    pcVar4 = s_sid_<_FIRST_PREDICTABLE_SID_00539900;
  }
  else {
    if (13999 < iVar2) goto LAB_004ab3e2;
    uVar6 = 0x188;
    pcVar5 = s__ns_o_Squid_cpp_005398c0;
    pcVar4 = s_sid_>__FIRST_PREDICTABLE_SID_005398d0;
  }
  FUN_00490040(pcVar4,pcVar5,uVar6);
LAB_004ab3e2:
  pcVar1 = *(code **)(param_1 * 0x1d4 + 0x1b0 + DAT_0051c960);
  if (pcVar1 == (code *)0x0) {
    piVar3 = (int *)(DAT_005395dc + iVar2 * 0x24);
    if (piVar3 == (int *)0x0) {
      piVar3 = (int *)0x0;
    }
    else {
      *piVar3 = (int)&PTR_LAB_005058d8;
    }
  }
  else {
    piVar3 = (int *)(*pcVar1)(iVar2);
  }
  *(char *)((int)piVar3 + 10) = (char)param_1;
  (**(code **)(*piVar3 + 0x28))();
  return piVar3;
}


// ==== FUN_004ab440 @ 004ab440

int * FUN_004ab440(uint param_1,int param_2)

{
  byte bVar1;
  code *pcVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int *piVar6;
  undefined1 local_100 [256];
  
  if ((param_2 < 6000) || (DAT_005395f4 <= param_2)) {
    FUN_00490040(s__FIRST_SERVER_SID_<__sid_&&_sid_<_0053992c,s__ns_o_Squid_cpp_0053991c,0x19e);
  }
  piVar6 = (int *)(param_2 * 0x24 + DAT_005395dc);
  if ((*(byte *)((int)piVar6 + 0xb) & 1) == 0) {
    if (param_1 != *(byte *)((int)piVar6 + 10)) {
      uVar5 = FUN_00420560(local_100,s_Invalid_take__Server_type__s__cl_0053995c,
                           param_1 * 0x1d4 + DAT_0051c960 + 4,
                           (uint)*(byte *)((int)piVar6 + 10) * 0x1d4 + DAT_0051c960 + 4);
      FUN_004875b0(uVar5);
      FUN_00490040(&DAT_0053999c,s__ns_o_Squid_cpp_0053998c,0x1b1);
    }
    (**(code **)(*piVar6 + 0x48))(0);
  }
  iVar4 = DAT_005395dc;
  iVar3 = DAT_0051c960;
  bVar1 = *(byte *)((int)piVar6 + 0xb);
  *(byte *)((int)piVar6 + 0xb) = bVar1 & 0xfe;
  *(byte *)((int)piVar6 + 0xb) = bVar1 & 0xfc;
  pcVar2 = *(code **)(param_1 * 0x1d4 + 0x1b0 + iVar3);
  if (pcVar2 == (code *)0x0) {
    piVar6 = (int *)(param_2 * 0x24 + iVar4);
    if (piVar6 == (int *)0x0) {
      piVar6 = (int *)0x0;
    }
    else {
      *piVar6 = (int)&PTR_LAB_005058d8;
    }
  }
  else {
    piVar6 = (int *)(*pcVar2)(param_2);
  }
  *(byte *)((int)piVar6 + 0xb) = *(byte *)((int)piVar6 + 0xb) | 4;
  *(char *)((int)piVar6 + 10) = (char)param_1;
  (**(code **)(*piVar6 + 0x2c))();
  return piVar6;
}


// ==== FUN_004abab0 @ 004abab0

void FUN_004abab0(void)

{
  DAT_00539600 = DAT_00539600 + -1;
  return;
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __fastcall FUN_004abac0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004abf50 @ 004abf50

int __fastcall FUN_004abf50(int param_1)

{
  return DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4;
}


// ==== FUN_004abfb0 @ 004abfb0

void __thiscall FUN_004abfb0(int *param_1,uint param_2)

{
  byte bVar1;
  int iVar2;
  
  if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
     ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040(s_isValid___00539cf4,s__ns_o_Squid_cpp_00539ce4,0x352);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
    FUN_00490040(s_isVoid___00539d10,s__ns_o_Squid_cpp_00539d00,0x353);
  }
  iVar2 = DAT_005395dc;
  bVar1 = *(byte *)((int)param_1 + 0xb);
  *(byte *)((int)param_1 + 0xb) = bVar1 & 0xfb;
  if ((bVar1 & 0x10) != 0) {
    FUN_00490040(s__xmitJustCreatedFlag_00539d2c,s__ns_o_Squid_cpp_00539d1c,0x356);
    iVar2 = DAT_005395dc;
  }
  if (DAT_00540a28 == 0) {
    if ((param_2 & 8) == 0) {
      param_2 = param_2 | 0x10;
    }
  }
  else {
    iVar2 = ((int)param_1 - iVar2) / 0x24;
    if ((iVar2 < 5) || (5999 < iVar2)) {
      if ((param_2 & 0x10) == 0) {
        param_2 = param_2 | 8;
      }
    }
    else {
      if ((param_2 & 8) != 0) {
        FUN_00490040(s___popFlags___pfSERVER_POP__00539d54,s__ns_o_Squid_cpp_00539d44,0x35b);
      }
      param_2 = param_2 | 0x10;
    }
    if ((param_2 & 0x50) == 0) {
      if ((param_2 & 7) != 0) {
        *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 0x10;
      }
      FUN_004abb40(0xffffffff,0xffffffff,0,param_2);
      *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) & 0xef;
    }
  }
  iVar2 = DAT_00539600;
  DAT_00539600 = DAT_00539600 + 1;
  (**(code **)(*param_1 + 0x20))(param_2);
  if (iVar2 != DAT_00539600) {
    FUN_00490040(s_curPostPop____basePostPop_00539d80,s__ns_o_Squid_cpp_00539d70,0x389);
  }
  return;
}


// ==== FUN_004aca80 @ 004aca80

void __thiscall FUN_004aca80(int param_1,undefined2 param_2)

{
  uint uVar1;
  
  uVar1 = FUN_004abae0();
  if (((uVar1 & 0x1000006) != 0) && ((uVar1 & 4) == 0)) {
    *(undefined2 *)(param_1 + 8) = param_2;
  }
  return;
}


// ==== FUN_004acb00 @ 004acb00

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004acb00(int param_1)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = FUN_004abf50();
  if ((*(byte *)(iVar4 + 0xec) & 6) != 0) {
    *(undefined1 *)(param_1 + 0x1f) = 0;
    return;
  }
  iVar4 = *(int *)(iVar4 + 0xdc);
  iVar3 = *(int *)(iVar4 + 8 + (uint)*(byte *)(param_1 + 0x22) * 8);
  fVar1 = *(float *)(iVar3 + -0x20 + iVar4);
  fVar2 = *(float *)(iVar3 + -0x24 + iVar4);
  if (fVar1 <= fVar2) {
    fVar1 = fVar2;
  }
  if (fVar1 <= _DAT_005058a4) {
    *(undefined1 *)(param_1 + 0x1f) = 1;
    return;
  }
  if ((int)fVar1 < 0x40800001) {
    *(undefined1 *)(param_1 + 0x1f) = 2;
    return;
  }
  *(undefined1 *)(param_1 + 0x1f) = 3;
  return;
}


// ==== FUN_004acfc0 @ 004acfc0

uint FUN_004acfc0(int param_1,int param_2)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  uint local_c;
  int local_8;
  int local_4;
  
  local_c = FUN_004abae0();
  FUN_004ae480(&local_8,&local_4);
  iVar2 = __ftol();
  iVar3 = __ftol();
  iVar6 = (iVar3 - local_4) + 1;
  iVar5 = (iVar2 - local_8) + 1;
  if ((((param_1 == iVar5) || (param_1 == iVar2)) || (param_2 == iVar6)) ||
     (bVar1 = true, param_2 == iVar3)) {
    bVar1 = false;
  }
  uVar4 = FUN_004abae0();
  if ((((DAT_005395cc & uVar4) != 0) && (bVar1)) &&
     (((uVar4 & 0x400000) == 0 ||
      ((param_1 * 2 - iVar5 != iVar2 || (iVar3 - iVar6 < (param_2 - iVar6) * 2)))))) {
    local_c = local_c | 8;
  }
  return local_c;
}


// ==== FUN_004ad0b0 @ 004ad0b0

void __thiscall FUN_004ad0b0(int *param_1,uint param_2)

{
  ushort uVar1;
  uint uVar2;
  bool bVar3;
  int iVar4;
  int iVar5;
  byte bVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  ushort *puVar14;
  int iVar15;
  int iStack_1c;
  
  if (*(byte *)((int)param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___00539f78,s__ns_o_Squid_cpp_00539f68,0x5bf);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
    FUN_00490040(s__isContained___00539f94,s__ns_o_Squid_cpp_00539f84,0x5c0);
  }
  if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
     ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040(s_isValid___00539fb4,s__ns_o_Squid_cpp_00539fa4,0x5c7);
  }
  iVar5 = DAT_005395dc;
  if ((*(byte *)((int)param_1 + 0xb) & 5) == 0) {
    *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 4;
    iVar7 = FUN_004abf50();
    uVar2 = *(uint *)(iVar7 + 0xec);
    if (((uVar2 & 6) != 0) && ((*(byte *)((int)param_1 + 0x23) & 8) == 0)) {
      FUN_00442f30(*(undefined4 *)((int)param_1 + 0xe),*(undefined4 *)((int)param_1 + 0x12),uVar2);
    }
    if (((((uVar2 & (DAT_005395cc | 0x400ff)) != 0) ||
         (uVar8 = FUN_004abac0(), (uVar8 & 0x800) != 0)) &&
        ((*(byte *)((int)param_1 + 0x23) & 8) == 0)) && ((*(byte *)((int)param_1 + 0xb) & 8) == 0))
    {
      __ftol();
      iVar9 = __ftol();
      __ftol();
      iVar10 = __ftol();
      iVar11 = (iVar9 - *(int *)(iVar7 + 0x1b4)) + 1;
      iVar7 = (iVar10 - *(int *)(iVar7 + 0x1b8)) + 1;
      if (iVar7 <= iVar10) {
        iVar15 = iVar7 * 0x100;
        iVar4 = iVar11;
        iStack_1c = iVar7;
        do {
          for (; iVar4 <= iVar9; iVar4 = iVar4 + 1) {
            bVar6 = FUN_004acfc0(iVar4,iStack_1c);
            if (bVar6 != 0) {
              iVar12 = __ftol();
              iVar13 = __ftol();
              *(byte *)(DAT_0052fe48 + iVar4 + iVar15) =
                   *(byte *)(iVar12 * 0x100 + iVar13 + DAT_0052fe48) & ~bVar6;
            }
            if (((iVar4 != iVar11) && (iVar4 != iVar9)) ||
               ((iStack_1c == iVar7 || (bVar3 = true, iStack_1c == iVar10)))) {
              bVar3 = false;
            }
            if (((DAT_005395cc & uVar2) != 0) && (bVar3)) {
              if ((iVar4 < 0) || (((iVar15 < 0 || (0xff < iVar4)) || (0xffff < iVar15)))) {
                uVar8 = 0;
              }
              else {
                uVar8 = (uint)*(ushort *)(DAT_0052d590 + (iVar15 + iVar4) * 2);
              }
              uVar8 = DAT_005395dc + uVar8 * 0x24;
              if ((DAT_005395dc + 0x24U <= uVar8) &&
                 (uVar8 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
                *(ushort *)(uVar8 + 0xc) = *(ushort *)(uVar8 + 0xc) & 0xfff9;
                FUN_00448c10((int)(uVar8 - DAT_005395dc) / 0x24);
              }
            }
            FUN_004abac0();
          }
          iVar15 = iVar15 + 0x100;
          iStack_1c = iStack_1c + 1;
          iVar4 = iVar11;
        } while (iStack_1c <= iVar10);
      }
    }
    if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
      FUN_004ac160();
      FUN_004acf40(((int)param_1 - DAT_005395dc) / 0x24);
      return;
    }
    bVar3 = false;
    iVar7 = 0;
    do {
      if (bVar3) break;
      puVar14 = (ushort *)
                FUN_00479d20(*(undefined4 *)((int)param_1 + 0xe),
                             *(undefined4 *)((int)param_1 + 0x12),iVar7);
      uVar1 = *puVar14;
      uVar2 = 0;
      while (uVar8 = (uint)uVar1, uVar8 != 0) {
        iVar9 = uVar8 * 0x24 + DAT_005395dc;
        if (((int)param_1 - iVar5) / 0x24 == uVar8) {
          if (uVar2 == 0) {
            *puVar14 = *(ushort *)(iVar9 + 4);
          }
          else {
            uVar1 = *(ushort *)(iVar9 + 4);
            iVar9 = uVar2 * 0x24 + DAT_005395dc;
            if ((iVar9 - DAT_005395dc) / 0x24 == (uint)uVar1) {
              FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
            }
            *(ushort *)(iVar9 + 4) = uVar1;
          }
          bVar3 = true;
          break;
        }
        uVar2 = uVar8;
        uVar1 = *(ushort *)(iVar9 + 4);
      }
      iVar7 = iVar7 + 1;
    } while (iVar7 < 4);
    if ((param_2 & 0x2000) != 0) {
      (**(code **)(*param_1 + 0x8c))();
      return;
    }
    (**(code **)(*param_1 + 0x88))();
  }
  return;
}


// ==== FUN_004ad490 @ 004ad490

void __thiscall FUN_004ad490(int *param_1,float param_2,float param_3,uint param_4)

{
  ushort uVar1;
  uint uVar2;
  bool bVar3;
  bool bVar4;
  int iVar5;
  byte bVar6;
  undefined2 uVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  uint uVar15;
  ushort *puVar16;
  int iVar17;
  int iVar18;
  
  if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
     ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040();
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
    FUN_00490040();
  }
  if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
    FUN_00490040();
  }
  if (((((int)param_2 < 1) || ((float)DAT_0052e9a8 <= param_2)) || ((int)param_3 < 1)) ||
     (bVar3 = false, (float)DAT_0052e9a8 <= param_3)) {
    bVar3 = true;
  }
  if (bVar3) {
    if (DAT_00540a24 == 0) {
      iVar8 = FUN_004abf50((double)param_2,(double)param_3);
      FUN_0048ca70(0x1d,s_Error__s_popped_to___5_1f__5_1f__0053a018,iVar8 + 4);
    }
    param_2 = 10.0;
    param_3 = 10.0;
  }
  *(float *)((int)param_1 + 0xe) = param_2;
  *(float *)((int)param_1 + 0x12) = param_3;
  uVar7 = __ftol();
  *(undefined2 *)((int)param_1 + 0x16) = uVar7;
  uVar7 = __ftol();
  *(undefined2 *)(param_1 + 6) = uVar7;
  iVar8 = FUN_004abf50();
  uVar2 = *(uint *)(iVar8 + 0xec);
  if (((uVar2 & 6) != 0) && ((*(byte *)((int)param_1 + 0x23) & 8) == 0)) {
    FUN_00442f30();
  }
  if ((((uVar2 & (DAT_005395cc | 0x400ff)) != 0) || (uVar9 = FUN_004abac0(), (uVar9 & 0x800) != 0))
     && (((*(byte *)((int)param_1 + 0x23) & 8) == 0 && ((*(byte *)((int)param_1 + 0xb) & 8) == 0))))
  {
    __ftol();
    iVar10 = __ftol();
    __ftol();
    iVar11 = __ftol();
    iVar12 = (iVar10 - *(int *)(iVar8 + 0x1b4)) + 1;
    iVar8 = (iVar11 - *(int *)(iVar8 + 0x1b8)) + 1;
    if (((((int)(float)iVar12 < 1) || ((float)DAT_0052e9a8 <= (float)iVar12)) ||
        ((int)(float)iVar8 < 1)) || ((float)DAT_0052e9a8 <= (float)iVar8)) {
      FUN_00490040();
    }
    if (iVar8 <= iVar11) {
      iVar17 = iVar8 * 0x100;
      iVar5 = iVar12;
      iVar18 = iVar8;
      do {
        for (; iVar5 <= iVar10; iVar5 = iVar5 + 1) {
          bVar6 = FUN_004acfc0(iVar5,iVar18);
          if (((iVar5 == iVar12) || (iVar10 == iVar5)) || ((iVar18 == iVar8 || (iVar18 == iVar11))))
          {
            bVar3 = false;
          }
          else {
            bVar3 = true;
          }
          if (bVar6 != 0) {
            iVar13 = __ftol();
            iVar14 = __ftol();
            *(byte *)(DAT_0052fe48 + iVar5 + iVar17) =
                 *(byte *)(iVar13 * 0x100 + DAT_0052fe48 + iVar14) | bVar6;
          }
          if (((iVar5 != iVar12) && (iVar10 != iVar5)) ||
             ((iVar18 == iVar8 || (bVar4 = true, iVar18 == iVar11)))) {
            bVar4 = false;
          }
          if (((DAT_005395cc & uVar2) != 0) && (bVar4)) {
            if ((iVar5 < 0) || (((iVar17 < 0 || (0xff < iVar5)) || (0xffff < iVar17)))) {
              uVar9 = 0;
            }
            else {
              uVar9 = (uint)*(ushort *)(DAT_0052d590 + (iVar17 + iVar5) * 2);
            }
            uVar9 = DAT_005395dc + uVar9 * 0x24;
            if (((DAT_005395dc + 0x24U <= uVar9) &&
                (uVar9 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) &&
               ((uVar15 = FUN_004abac0(), (uVar15 & 0x800) != 0 && (!bVar3)))) {
              iVar13 = FUN_004bdd80(*(float *)((int)param_1 + 0xe),
                                    *(undefined4 *)((int)param_1 + 0x12));
              if (iVar13 == 0) {
                *(ushort *)(uVar9 + 0xc) = *(ushort *)(uVar9 + 0xc) & 0xfff9;
              }
              else {
                *(ushort *)(uVar9 + 0xc) = *(ushort *)(uVar9 + 0xc) & 0xfffb | 2;
              }
              FUN_00448c10();
            }
          }
          uVar9 = FUN_004abac0();
          if ((uVar9 & 0x800) != 0) {
            iVar13 = __ftol();
            iVar14 = __ftol();
            if (((-1 < iVar13) && (-1 < iVar14)) &&
               ((iVar13 < 0x100 &&
                ((iVar14 < 0x100 && (*(short *)(DAT_005670cc + (iVar14 * 0x100 + iVar13) * 2) != 0))
                )))) {
              FUN_004abf50();
              iVar13 = FUN_004abf50();
              FUN_0048ca70(0x1d,s_ERROR__gridSid__d___s__replacing_0053a06c,
                           ((int)param_1 - DAT_005395dc) / 0x24,iVar13 + 4);
            }
          }
        }
        iVar17 = iVar17 + 0x100;
        iVar18 = iVar18 + 1;
        iVar5 = iVar12;
      } while (iVar18 <= iVar11);
    }
  }
  FUN_004acb00();
  puVar16 = (ushort *)FUN_00479d20();
  uVar1 = *puVar16;
  if (((int)param_1 - DAT_005395dc) / 0x24 == (uint)uVar1) {
    FUN_00490040();
  }
  iVar8 = DAT_005395dc;
  *(ushort *)(param_1 + 1) = uVar1;
  *puVar16 = (ushort)(((int)param_1 - iVar8) / 0x24);
  if (*(char *)((int)param_1 + 0x1f) == '\0') {
    FUN_004aca80();
  }
  if ((param_4 & 0x2000) == 0) {
    (**(code **)(*param_1 + 0x88))();
  }
  else {
    (**(code **)(*param_1 + 0x8c))();
  }
  if ((*(byte *)((int)param_1 + 0x23) & 0x80) == 0) {
    (**(code **)(*param_1 + 0x30))();
    *(byte *)((int)param_1 + 0x23) = *(byte *)((int)param_1 + 0x23) | 0x80;
  }
  FUN_004abfb0();
  if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
    FUN_00490040();
  }
  return;
}


// ==== FUN_004adbc0 @ 004adbc0

undefined1 __fastcall FUN_004adbc0(int param_1)

{
  return *(undefined1 *)(param_1 + 0x1c);
}


// ==== FUN_004adbd0 @ 004adbd0

void __thiscall FUN_004adbd0(int param_1,undefined1 param_2)

{
  *(undefined1 *)(param_1 + 0x1c) = param_2;
  return;
}


// ==== FUN_004ae040 @ 004ae040

void __fastcall FUN_004ae040(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  bool bVar3;
  
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a108,s__ns_o_Squid_cpp_0053a0f8,0x7a6);
  }
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_00490040(s__isContained___0053a124,s__ns_o_Squid_cpp_0053a114,0x7a7);
  }
  bVar3 = DAT_00540a28 != 0;
  *(undefined1 *)(param_1 + 0x21) =
       *(undefined1 *)(DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4 + 0x104);
  if (bVar3) {
    *(undefined1 *)(param_1 + 0x20) = 0;
    iVar1 = FUN_004aecf0();
    if (iVar1 != 0) {
      uVar2 = FUN_00444720();
      FUN_004aed20(uVar2);
    }
  }
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_00490040(s__isContained___0053a144,s__ns_o_Squid_cpp_0053a134,0x7b1);
  }
  return;
}


// ==== FUN_004ae0f0 @ 004ae0f0

uint __fastcall FUN_004ae0f0(int param_1)

{
  byte bVar1;
  uint uVar2;
  
  uVar2 = 0;
  bVar1 = *(byte *)(param_1 + 0x23);
  if (((bVar1 & 1) == 0) && ((bVar1 & 8) == 0)) {
    return 1;
  }
  if ((bVar1 & 1) != 0) {
    uVar2 = 2;
  }
  if ((bVar1 & 8) != 0) {
    uVar2 = uVar2 | 4;
  }
  return uVar2;
}


// ==== FUN_004ae120 @ 004ae120

void __fastcall FUN_004ae120(int param_1)

{
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a164,s__ns_o_Squid_cpp_0053a154,0x7c8);
  }
  *(undefined1 *)(param_1 + 0x21) =
       *(undefined1 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0x104 + DAT_0051c960);
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_00490040(s__isContained___0053a180,s__ns_o_Squid_cpp_0053a170,0x7cc);
  }
  return;
}


// ==== FUN_004ae180 @ 004ae180

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004ae180(int param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  
  if (DAT_00540a24 == 0) {
    uVar1 = FUN_004abae0();
    if ((((((DAT_005395cc | 8) & uVar1) != 0) && (DAT_005207f8 != 0)) && ((param_2 & 3) != 0)) &&
       ((param_2 & 0x2000000) == 0)) {
      FUN_0045c210((param_1 - DAT_005395dc) / 0x24);
    }
    if (DAT_005207f8 != 0) {
      if (((param_2 & 0x203) == 0) || ((param_2 & 0x2000000) != 0)) {
        uVar1 = FUN_004abac0();
        if (((uVar1 & 0x800) != 0) && (((param_2 & 1) != 0 && ((param_2 & 0x2000000) != 0)))) {
          FUN_004adbd0(DAT_005207f4);
        }
      }
      else {
        if ((param_2 & 0x800) != 0) {
          FUN_00490040(s___popFlags_pfDESTROY_GRAPH__0053a1a0,s__ns_o_Squid_cpp_0053a190,0x7dd);
        }
        uVar1 = FUN_004abac0();
        if ((uVar1 & 0x800) != 0) {
          FUN_0045b7d0((param_1 - DAT_005395dc) / 0x24);
        }
      }
    }
    uVar1 = FUN_004abae0();
    if (((param_2 & 1) != 0) && ((*(byte *)(param_1 + 0x23) & 9) == 0)) {
      FUN_004147a0(*(undefined1 *)(param_1 + 10));
      _DAT_00512114 = __ftol();
      uVar2 = FUN_004abac0();
      if ((uVar2 & 0x10000000) != 0) {
        FUN_0048a360((param_1 - DAT_005395dc) / 0x24);
      }
      if ((uVar1 & 0x4200) != 0) {
        FUN_004071f0(*(undefined1 *)(param_1 + 0x20),(param_1 - DAT_005395dc) / 0x24);
        FUN_0045ec90((param_1 - DAT_005395dc) / 0x24);
      }
      FUN_004d8230(*(undefined1 *)(param_1 + 0x20),(param_1 - DAT_005395dc) / 0x24,1);
      uVar1 = FUN_004015d0();
      if (uVar1 == *(byte *)(param_1 + 0x20)) {
        FUN_0040bc50(*(undefined1 *)(param_1 + 10));
      }
      FUN_0040bc20(*(undefined1 *)(param_1 + 10));
      FUN_0041eac0();
    }
  }
  FUN_004abab0(param_2);
  return;
}


// ==== FUN_004ae380 @ 004ae380

void __fastcall FUN_004ae380(int param_1)

{
  undefined1 uVar1;
  
  uVar1 = *(undefined1 *)(param_1 + 0x22);
  FUN_004abf50();
  FUN_00456a00(*(undefined1 *)(param_1 + 10),uVar1,*(undefined4 *)(param_1 + 0xe),
               *(undefined4 *)(param_1 + 0x12),((*(byte *)(param_1 + 0x23) & 4) == 0) - 1U & 0x10);
  return;
}


// ==== FUN_004ae3e0 @ 004ae3e0

void __fastcall FUN_004ae3e0(int *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x004ae3e2. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


// ==== FUN_004ae3f0 @ 004ae3f0

int __fastcall FUN_004ae3f0(int param_1)

{
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a1cc,s__ns_o_Squid_cpp_0053a1bc,0x826);
  }
  return (int)*(char *)(*(int *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0x124 + DAT_0051c960) +
                       (uint)*(byte *)(param_1 + 0x22) * 4);
}


// ==== FUN_004ae480 @ 004ae480

void FUN_004ae480(undefined4 param_1,undefined4 param_2)

{
  FUN_004442b0(param_1,param_2,0);
  return;
}


// ==== FUN_004aecf0 @ 004aecf0

byte __fastcall FUN_004aecf0(int param_1)

{
  byte bVar1;
  uint uVar2;
  
  uVar2 = FUN_004abae0();
  if ((uVar2 & 0x400000) == 0) {
    bVar1 = FUN_004abac0();
    return bVar1 & 0x10;
  }
  return *(byte *)(param_1 + 0x23) & 1;
}


// ==== FUN_004aed20 @ 004aed20

void __thiscall FUN_004aed20(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0x1a) = param_2;
  return;
}


// ==== FUN_004aeec0 @ 004aeec0

undefined4 __thiscall FUN_004aeec0(int param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  
  uVar2 = *(undefined4 *)(param_1 + 0xe);
  uVar3 = *(undefined4 *)(param_1 + 0x12);
  uVar1 = param_2;
  FUN_004abf50(param_2,uVar2,uVar3,param_3);
  FUN_004449d0(uVar1,uVar2,uVar3,param_3);
  return param_2;
}


// ==== FUN_004aef30 @ 004aef30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004aef30(int param_1,float *param_2)

{
  float fVar1;
  int *piVar2;
  int *piVar3;
  undefined4 uVar4;
  int local_10 [3];
  int local_4;
  
  piVar3 = &local_4;
  uVar4 = 0;
  piVar2 = local_10;
  FUN_004abf50(piVar2,piVar3,0);
  FUN_004442b0(piVar2,piVar3,uVar4);
  fVar1 = *(float *)(param_1 + 0x12);
  *param_2 = (float)local_10[0] * _DAT_005058cc + *(float *)(param_1 + 0xe);
  param_2[1] = (float)local_4 * _DAT_005058cc + fVar1;
  return;
}


// ==== FUN_004cff40 @ 004cff40

void FUN_004cff40(int param_1,int param_2)

{
  int *_Src;
  
  if ((param_1 < 5) || (_Src = &DAT_005890b0, 5999 < param_1)) {
    _Src = &DAT_00589010;
  }
  FID_conflict__memcpy(_Src + 2,_Src,0x98);
  *_Src = param_1;
  _Src[1] = param_2;
  return;
}


// ==== FUN_004d2e00 @ 004d2e00

undefined4 FUN_004d2e00(int param_1,int param_2,int param_3,int param_4,uint param_5)

{
  char cVar1;
  char cVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  
  uVar4 = (int)param_5 >> 0x1f;
  if (((param_5 ^ uVar4) - uVar4 & 1 ^ uVar4) == uVar4) {
    cVar1 = *(char *)(*(int *)(param_1 + 0x124) + param_2 * 4);
    cVar2 = *(char *)(*(int *)(param_3 + 0x124) + param_4 * 4);
  }
  else {
    cVar1 = *(char *)(*(int *)(param_1 + 0x124) + 1 + param_2 * 4);
    cVar2 = *(char *)(*(int *)(param_3 + 0x124) + 1 + param_4 * 4);
  }
  iVar6 = (int)cVar2;
  iVar5 = (int)cVar1;
  uVar4 = *(uint *)(param_1 + 0xec);
  if ((uVar4 & 0x40000) != 0) {
    iVar5 = 0x50;
  }
  uVar3 = *(uint *)(param_3 + 0xec);
  if ((uVar3 & 0x40000) != 0) {
    iVar6 = 0x50;
  }
  if (((uVar4 & 0x104) == 0) || ((uVar3 & 0x1044202) == 0)) {
    if (((uVar3 & 0x104) != 0) && ((uVar4 & 0x1044202) != 0)) {
      iVar5 = 0x41;
    }
  }
  else {
    iVar6 = 0x41;
  }
  if ((((&DAT_0051c3d0)[param_5] &
       (uint)*(ushort *)(s_playerId____GAME_SERVER_ID_0053fd2c + iVar5 * 2 + 2)) != 0) &&
     (((&DAT_0051c3d0)[(&DAT_0051c3b0)[param_5]] & (uint)(ushort)(&DAT_0053fdb0)[iVar6 + -0x41]) !=
      0)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004d81f0 @ 004d81f0

int FUN_004d81f0(int param_1)

{
  int iVar1;
  
  if ((((DAT_00540a24 != 0) || (param_1 < 1)) || (iVar1 = DAT_0050f834 + param_1 * 0xac, iVar1 == 0)
      ) || (iVar1 = *(int *)(iVar1 + 0xa8), iVar1 == 0)) {
    iVar1 = 0;
  }
  return iVar1;
}


// ==== FUN_004d8230 @ 004d8230

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004d8230(undefined4 param_1,int param_2,int param_3)

{
  int *piVar1;
  bool bVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  float10 fVar9;
  undefined4 local_10 [2];
  byte *local_8;
  int local_4;
  
  iVar3 = FUN_004d81f0(param_1);
  if (iVar3 != 0) {
    iVar4 = DAT_005395dc + param_2 * 0x24;
    if (param_3 == 0) {
      piVar1 = (int *)(iVar3 + 0x6d4 + (uint)*(byte *)(iVar4 + 10) * 4);
      *piVar1 = *piVar1 + -1;
    }
    else {
      piVar1 = (int *)(iVar3 + 0x6d4 + (uint)*(byte *)(iVar4 + 10) * 4);
      *piVar1 = *piVar1 + 1;
    }
    local_8 = (byte *)(iVar4 + 10);
    uVar5 = FUN_004abae0();
    if (param_3 == 0) {
      if ((uVar5 & 0x200000) != 0) {
        if (*(int *)(iVar3 + 0x2a0) == param_2) {
          *(undefined4 *)(iVar3 + 0x2a0) = 0;
        }
        if (*(int *)(iVar3 + 0x29c) == param_2) {
          *(undefined4 *)(iVar3 + 0x29c) = 0;
        }
      }
      if ((uVar5 & 0x30000) != 0) {
        FUN_004e37e0(param_2,&local_4,local_10);
        if (local_4 != 0) {
          FUN_004e3320(local_4);
        }
        iVar6 = FUN_004dd400(param_2);
        if (iVar6 != 0) {
          FUN_004def00(param_2);
          *(double *)(iVar3 + 0x5e0) = _DAT_005484a8 + _DAT_00506cb0;
        }
      }
      if (((uVar5 & 0x200) != 0) && (*(int *)(iVar3 + 0x294) == param_2)) {
        *(undefined4 *)(iVar3 + 0x294) = 0;
      }
      FUN_004def00(param_2);
      FUN_004def00(param_2);
      if (*(int *)(iVar3 + 0x3f4) == param_2) {
        *(undefined4 *)(iVar3 + 0x3f4) = 0;
      }
      if (*(int *)(iVar3 + 0x3f8) == param_2) {
        *(undefined4 *)(iVar3 + 0x3f8) = 0;
      }
    }
    if ((uVar5 & 0x400000) != 0) {
      if (param_3 == 0) {
        if (*(int *)(iVar3 + 0x2a4) == param_2) {
          *(undefined4 *)(iVar3 + 0x2a4) = 0;
        }
      }
      else {
        *(int *)(iVar3 + 0x2a4) = param_2;
      }
    }
    if ((uVar5 & 0x4000000) != 0) {
      if (param_3 == 0) {
        FUN_004def00(param_2);
      }
      else {
        bVar2 = false;
        if ((*(int *)(iVar3 + 0x294) != 0) &&
           (fVar9 = (float10)FUN_004e33e0(param_2,*(int *)(iVar3 + 0x294)),
           fVar9 < (float10)_DAT_00506cb8)) {
          bVar2 = true;
        }
        iVar6 = 0;
        if (0 < *(int *)(iVar3 + 0x308)) {
          iVar8 = 0;
          do {
            fVar9 = (float10)FUN_004e33e0(param_2,*(undefined4 *)(*(int *)(iVar3 + 0x300) + iVar8));
            if (fVar9 < (float10)_DAT_00506cb8) {
              bVar2 = true;
            }
            iVar8 = iVar8 + 4;
            iVar6 = iVar6 + 1;
          } while (iVar6 < *(int *)(iVar3 + 0x308));
        }
        if (bVar2) {
          FUN_004e32e0(param_2);
        }
      }
    }
    if (((uVar5 & 0x30000) != 0) && ((uVar5 & 0x200000) == 0)) {
      FUN_004d72d0(iVar3 + 0x2dc,param_2,param_3);
      local_10[0] = *(undefined4 *)(iVar4 + 0xe);
      uVar7 = __ftol();
      uVar7 = __ftol(uVar7);
      iVar4 = FUN_004abf50(uVar7);
      FUN_004dabe0(iVar3,DAT_00540c24,s_Built__s_at___d__d__005410fc,iVar4 + 4);
    }
    if ((uVar5 & 0x800) != 0) {
      FUN_004d72d0(iVar3 + 0x2f4,param_2,param_3);
      *(undefined4 *)(iVar3 + 0x5b8) = 0;
      *(undefined4 *)(iVar3 + 0x5bc) = 0;
    }
    if (((uVar5 & 0x4000) != 0) && (*local_8 != DAT_0051cb3c)) {
      FUN_004d72d0(iVar3 + 0x300,param_2,param_3);
      iVar4 = 0;
      FUN_004d72d0(iVar3 + 0x30c,param_2,param_3);
      if (param_3 == 0) {
        FUN_004e32a0();
        FUN_004d72d0(iVar3 + 0x318,param_2,0);
        if (0 < *(int *)(iVar3 + 0x308)) {
          iVar6 = 0;
          do {
            uVar7 = *(undefined4 *)(*(int *)(iVar3 + 0x300) + iVar6);
            iVar8 = FUN_004dd400(uVar7);
            if (iVar8 == 0) {
              FUN_004e2930(uVar7);
            }
            iVar6 = iVar6 + 4;
            iVar4 = iVar4 + 1;
          } while (iVar4 < *(int *)(iVar3 + 0x308));
        }
      }
      *(uint *)(iVar3 + 0x2c8) = (uint)(*(int *)(iVar3 + 0x3b0) <= *(int *)(iVar3 + 0x308));
    }
    if ((uVar5 & 0x40800) != 0) {
      if (param_3 == 0) {
        iVar4 = FUN_004dd400(param_2);
        if (iVar4 != 0) {
          FUN_004def00(param_2);
        }
      }
      else {
        if (*(int *)(iVar3 + 0x2ec) <= *(int *)(iVar3 + 0x2f0)) {
          FUN_004def00(**(undefined4 **)(iVar3 + 0x2e8));
        }
        FUN_004e2930(param_2);
        *(int *)(iVar3 + 0x3c4) = *(int *)(iVar3 + 0x3c4) + 3;
      }
    }
    if ((param_3 != 0) && ((uVar5 & 0x4200) != 0)) {
      *(undefined4 *)(iVar3 + 0x5c0) = 0;
      *(undefined4 *)(iVar3 + 0x5c4) = 0;
    }
  }
  return;
}


// ==== FUN_004eae20 @ 004eae20

void __thiscall FUN_004eae20(int *param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  ushort *puVar5;
  int iVar6;
  int iVar7;
  
  iVar2 = DAT_0052e9a8;
  if (param_4 == -1) {
    param_4 = DAT_0052e9a8;
  }
  if (param_5 == -1) {
    param_5 = DAT_0052e9a8;
  }
  if (param_2 < 1) {
    param_2 = 0;
  }
  param_1[1] = param_2;
  if (param_3 < 1) {
    param_3 = 0;
  }
  param_1[2] = param_3;
  iVar1 = iVar2 + -1;
  if (iVar1 <= param_4) {
    param_4 = iVar1;
  }
  param_1[3] = param_4;
  if (iVar1 <= param_5) {
    param_5 = iVar1;
  }
  param_1[4] = param_5;
  param_1[9] = 1;
  if (param_1[0xe] == 0) {
    param_1[9] = 0;
  }
  iVar1 = param_1[9];
  iVar2 = iVar2 / (int)(&DAT_0052d598)[iVar1] + -1;
  iVar3 = param_2 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_2 / (int)(&DAT_0052d598)[iVar1]) {
    iVar3 = iVar2;
  }
  param_1[5] = iVar3;
  iVar6 = param_3 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_3 / (int)(&DAT_0052d598)[iVar1]) {
    iVar6 = iVar2;
  }
  param_1[6] = iVar6;
  iVar7 = param_4 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_4 / (int)(&DAT_0052d598)[iVar1]) {
    iVar7 = iVar2;
  }
  param_1[7] = iVar7;
  iVar4 = param_5 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_5 / (int)(&DAT_0052d598)[iVar1]) {
    iVar4 = iVar2;
  }
  param_1[8] = iVar4;
  param_1[7] = iVar7 + 1;
  param_1[8] = iVar4 + 1;
  if (iVar7 + 1 < 0) {
    param_1[7] = 0;
  }
  if (iVar4 + 1 < 0) {
    param_1[8] = 0;
  }
  if (iVar2 < param_1[7]) {
    param_1[7] = iVar2;
  }
  if (iVar2 < param_1[8]) {
    param_1[8] = iVar2;
  }
  param_1[10] = iVar3;
  param_1[0xb] = iVar6;
  param_1[0xd] = 0;
  puVar5 = (ushort *)FUN_00479d90(iVar3,iVar6,param_1[9]);
  param_1[0xc] = (uint)*puVar5;
  (**(code **)(*param_1 + 4))();
  return;
}


// ==== FUN_004eafe0 @ 004eafe0

void __fastcall FUN_004eafe0(undefined4 *param_1)

{
  float *pfVar1;
  byte bVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  ushort *puVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int local_8;
  int local_4;
  
  iVar10 = DAT_005395dc;
LAB_004eafef:
  do {
    uVar9 = param_1[0xc];
    while (uVar9 == 0) {
      iVar10 = param_1[10];
      param_1[10] = iVar10 + 1;
      if ((int)param_1[7] < iVar10 + 1) {
        param_1[10] = param_1[5];
        iVar10 = param_1[0xb];
        param_1[0xb] = iVar10 + 1;
        if ((int)param_1[8] < iVar10 + 1) {
          iVar10 = param_1[9] + 1;
          param_1[9] = iVar10;
          if (3 < iVar10) {
            param_1[0xd] = 0;
            return;
          }
          iVar7 = DAT_0052e9a8 / (int)(&DAT_0052d598)[iVar10] + -1;
          iVar8 = (int)param_1[1] / (int)(&DAT_0052d598)[iVar10];
          if (iVar7 <= (int)param_1[1] / (int)(&DAT_0052d598)[iVar10]) {
            iVar8 = iVar7;
          }
          param_1[5] = iVar8;
          iVar11 = (int)param_1[2] / (int)(&DAT_0052d598)[iVar10];
          if (iVar7 <= (int)param_1[2] / (int)(&DAT_0052d598)[iVar10]) {
            iVar11 = iVar7;
          }
          param_1[6] = iVar11;
          iVar12 = (int)param_1[3] / (int)(&DAT_0052d598)[iVar10];
          if (iVar7 <= (int)param_1[3] / (int)(&DAT_0052d598)[iVar10]) {
            iVar12 = iVar7;
          }
          param_1[7] = iVar12;
          iVar5 = (int)param_1[4] / (int)(&DAT_0052d598)[iVar10];
          if (iVar7 <= (int)param_1[4] / (int)(&DAT_0052d598)[iVar10]) {
            iVar5 = iVar7;
          }
          param_1[8] = iVar5;
          param_1[7] = iVar12 + 1;
          param_1[8] = iVar5 + 1;
          if (iVar12 + 1 < 0) {
            param_1[7] = 0;
          }
          if (iVar5 + 1 < 0) {
            param_1[8] = 0;
          }
          if (iVar7 < (int)param_1[7]) {
            param_1[7] = iVar7;
          }
          if (iVar7 < (int)param_1[8]) {
            param_1[8] = iVar7;
          }
          param_1[0xb] = iVar11;
          param_1[0xd] = 0;
          param_1[10] = iVar8;
        }
      }
      puVar6 = (ushort *)FUN_00479d90(param_1[10],param_1[0xb],param_1[9]);
      iVar10 = DAT_005395dc;
      uVar9 = (uint)*puVar6;
      param_1[0xc] = uVar9;
    }
    do {
      iVar7 = param_1[0xc];
      param_1[0xd] = iVar7;
      if (iVar7 == 0) goto LAB_004eafef;
      iVar7 = iVar7 * 0x24 + iVar10;
      bVar2 = *(byte *)(iVar7 + 0x23);
      param_1[0xc] = (uint)*(ushort *)(iVar7 + 4);
    } while ((bVar2 & 8) != 0);
    pfVar1 = (float *)(param_1[0xd] * 0x24 + 0xe + iVar10);
    fVar3 = *pfVar1;
    fVar4 = pfVar1[1];
    if (((((int)fVar3 < 1) || ((float)DAT_0052e9a8 <= fVar3)) || ((int)fVar4 < 1)) ||
       ((float)DAT_0052e9a8 <= fVar4)) {
      FUN_00490040(s_pos_isValid___00541890,s__ns_o_SquidFinder_cpp_00541878,0x89);
    }
    FUN_004442b0(&local_8,&local_4,param_1[0xf]);
    iVar7 = __ftol();
    iVar8 = __ftol();
    iVar12 = (iVar7 - local_8) + 1;
    iVar11 = (iVar8 - local_4) + 1;
    iVar10 = DAT_005395dc;
    if (((((((int)param_1[1] <= iVar7) || ((int)param_1[1] <= iVar12)) &&
          ((iVar7 <= (int)param_1[3] || (iVar12 <= (int)param_1[3])))) &&
         (((int)param_1[2] <= iVar8 || ((int)param_1[2] <= iVar11)))) &&
        ((iVar8 <= (int)param_1[4] || (iVar11 <= (int)param_1[4])))) &&
       (iVar7 = (**(code **)*param_1)(param_1[0xd] * 0x24 + DAT_005395dc), iVar10 = DAT_005395dc,
       iVar7 != 0)) {
      return;
    }
  } while( true );
}


// ==== FUN_004ebad0 @ 004ebad0

undefined4 * __thiscall FUN_004ebad0(undefined4 *param_1,int param_2,uint param_3)

{
  uint uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  undefined1 local_20 [16];
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  *param_1 = &PTR_FUN_005003d0;
  param_1[0xe] = 0;
  param_1[0xf] = 0;
  param_1[0xd] = 0;
  param_1[0x16] = 0xc61c4000;
  param_1[0x18] = 0xc61c4000;
  iVar2 = DAT_005395dc;
  param_1[0x15] = 0xc61c4000;
  param_1[0x17] = 0xc61c4000;
  *param_1 = &PTR_LAB_00507488;
  uVar1 = param_2 * 0x24 + iVar2;
  if ((uVar1 < iVar2 + 0x24U) || ((uint)(DAT_005395f4 * 0x24 + iVar2) <= uVar1)) {
    FUN_00490040(s_s_isValid___00541918,s__ns_o_SquidFinder_cpp_00541900,0x17e);
  }
  param_1[0x12] = param_3;
  if ((param_3 & 8) != 0) {
    param_1[0x19] = 0;
  }
  puVar3 = (undefined4 *)FUN_004aeec0(local_20,0);
  local_10 = *puVar3;
  local_c = puVar3[1];
  local_8 = puVar3[2];
  local_4 = puVar3[3];
  param_1[0x15] = local_10;
  param_1[0x17] = local_8;
  param_1[0x16] = local_c;
  param_1[0x18] = local_4;
  uVar4 = FUN_004abf50();
  param_1[0x10] = uVar4;
  param_1[0x11] = (uint)*(byte *)(uVar1 + 0x22);
  puVar3 = (undefined4 *)FUN_004aef30(local_20);
  uVar4 = puVar3[1];
  param_1[0x13] = *puVar3;
  param_1[0x14] = uVar4;
  FUN_004ebf30();
  return param_1;
}


// ==== FUN_004ebbe0 @ 004ebbe0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 * __thiscall
FUN_004ebbe0(undefined4 *param_1,float param_2,float param_3,undefined4 param_4,undefined4 param_5,
            int param_6,undefined4 param_7,undefined4 param_8,undefined4 param_9,uint param_10)

{
  undefined4 *puVar1;
  int local_28;
  int local_24;
  undefined1 local_20 [16];
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  *param_1 = &PTR_FUN_005003d0;
  param_1[0xe] = 0;
  param_1[0xf] = 0;
  param_1[0xd] = 0;
  param_1[0x16] = 0xc61c4000;
  param_1[0x15] = 0xc61c4000;
  param_1[0x18] = 0xc61c4000;
  param_1[0x17] = 0xc61c4000;
  *param_1 = &PTR_LAB_00507488;
  param_1[0x12] = param_10;
  if ((param_10 & 8) != 0) {
    param_1[0x19] = 0;
  }
  param_1[0x11] = param_9;
  param_1[0x10] = param_6 * 0x1d4 + DAT_0051c960;
  puVar1 = (undefined4 *)FUN_004449d0(local_20,param_2,param_3,0);
  local_10 = *puVar1;
  local_c = puVar1[1];
  local_8 = puVar1[2];
  local_4 = puVar1[3];
  param_1[0x15] = local_10;
  param_1[0x16] = local_c;
  param_1[0x17] = local_8;
  param_1[0x18] = local_4;
  FUN_004442b0(&local_28,&local_24,0);
  param_1[0x13] = (float)local_28 * _DAT_0050747c + param_2;
  param_1[0x14] = (float)local_24 * _DAT_0050747c + param_3;
  FUN_004ebf30();
  return param_1;
}


// ==== FUN_004ebdd0 @ 004ebdd0

void __fastcall FUN_004ebdd0(undefined4 *param_1)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  short *psVar4;
  int iVar5;
  
  iVar2 = DAT_0052d590;
  param_1[0xd] = 0;
  do {
    bVar1 = true;
    iVar3 = param_1[0x1a] + 1;
    iVar5 = param_1[0x1b];
    param_1[0x1a] = iVar3;
    if ((param_1[0x1d] == iVar5) || (param_1[0x1f] == iVar5)) {
      bVar1 = false;
    }
    if (bVar1) {
      if ((int)param_1[0x1c] < iVar3) {
        param_1[0x1b] = iVar5 + 1;
        iVar3 = param_1[0x1e];
        if (param_1[0x1f] == iVar5 + 1) {
          iVar3 = iVar3 + 1;
        }
LAB_004ebe2e:
        param_1[0x1a] = iVar3;
      }
    }
    else if ((int)param_1[0x1c] <= iVar3) {
      param_1[0x1b] = iVar5 + 1;
      if ((int)param_1[0x1f] < iVar5 + 1) {
        return;
      }
      iVar3 = param_1[0x1e];
      goto LAB_004ebe2e;
    }
    iVar5 = param_1[0x1a];
    if ((((iVar5 < 0) || (iVar3 = param_1[0x1b], iVar3 < 0)) || (0xff < iVar5)) || (0xff < iVar3)) {
      param_1[0xd] = 0;
    }
    else {
      param_1[0xd] = (uint)*(ushort *)(iVar2 + (iVar3 * 0x100 + iVar5) * 2);
    }
    if (param_1[0xd] != 0) {
      iVar5 = DAT_005395dc + param_1[0xd] * 0x24;
      if (((((float)param_1[0x15] <= *(float *)(iVar5 + 0xe)) &&
           (*(float *)(iVar5 + 0xe) <= (float)param_1[0x17])) &&
          (((float)param_1[0x16] <= *(float *)(iVar5 + 0x12) &&
           (*(float *)(iVar5 + 0x12) <= (float)param_1[0x18])))) ||
         (iVar5 = (**(code **)*param_1)(iVar5), iVar2 = DAT_0052d590, iVar5 == 0)) {
        param_1[0xd] = 0;
      }
      else {
        iVar3 = 0;
        iVar5 = param_1[0x19];
        if (0 < iVar5) {
          psVar4 = (short *)(param_1 + 0x20);
          do {
            if ((int)*psVar4 == param_1[0xd]) {
              param_1[0xd] = 0;
              break;
            }
            psVar4 = psVar4 + 1;
            iVar3 = iVar3 + 1;
          } while (iVar3 < iVar5);
        }
        if (param_1[0xd] != 0) {
          *(short *)((int)param_1 + iVar5 * 2 + 0x80) = (short)param_1[0xd];
          param_1[0x19] = param_1[0x19] + 1;
          return;
        }
      }
    }
  } while( true );
}


// ==== FUN_004ebf30 @ 004ebf30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004ebf30(int param_1)

{
  undefined4 uVar1;
  uint uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  
  uVar1 = __ftol();
  uVar1 = __ftol(uVar1);
  uVar1 = __ftol(uVar1);
  uVar1 = __ftol(uVar1);
  uVar2 = FUN_00487810(uVar1);
  if ((uVar2 & 8) != 0) {
    *(undefined4 *)(param_1 + 0x34) = 0;
    return;
  }
  if ((*(byte *)(param_1 + 0x48) & 8) != 0) {
    uVar1 = __ftol();
    *(undefined4 *)(param_1 + 0x78) = uVar1;
    uVar3 = __ftol();
    *(undefined4 *)(param_1 + 0x74) = uVar3;
    uVar4 = __ftol();
    *(undefined4 *)(param_1 + 0x70) = uVar4;
    uVar4 = __ftol();
    *(undefined4 *)(param_1 + 0x7c) = uVar4;
    *(undefined4 *)(param_1 + 0x68) = uVar1;
    *(undefined4 *)(param_1 + 0x6c) = uVar3;
    FUN_004ebdd0();
    return;
  }
  uVar1 = __ftol();
  uVar3 = __ftol(uVar1);
  uVar4 = __ftol(uVar3);
  uVar5 = __ftol(uVar4);
  FUN_004eae20(uVar5,uVar4,uVar3,uVar1);
  return;
}


// ==== FUN_004ec030 @ 004ec030

void __fastcall FUN_004ec030(int param_1)

{
  if ((*(byte *)(param_1 + 0x48) & 8) != 0) {
    FUN_004ebdd0();
    return;
  }
  FUN_004eafe0();
  return;
}


// ==== FUN_004ec040 @ 004ec040

undefined4 __thiscall FUN_004ec040(int param_1,int param_2)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  undefined4 *puVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uStack_10;
  undefined4 uStack_c;
  
  uStack_c = *(undefined4 *)(param_2 + 0x12);
  uStack_10 = *(undefined4 *)(param_2 + 0xe);
  iVar2 = __ftol();
  iVar3 = __ftol();
  bVar1 = *(byte *)(iVar2 * 0x100 + iVar3 + DAT_0052fe48);
  uVar4 = FUN_004abac0();
  if (((uVar4 & 0x800) != 0) || ((*(byte *)(param_1 + 0x48) & 2) != 0)) {
    if ((*(byte *)(param_1 + 0x48) & 8) == 0) {
      puVar5 = (undefined4 *)FUN_004aeec0(&uStack_10,0);
      iVar2 = FUN_0043fe10(*puVar5,puVar5[1],puVar5[2],puVar5[3]);
      if (iVar2 == 0) {
        return 0;
      }
    }
    if ((*(byte *)(param_1 + 0x48) & 1) == 0) {
      uVar6 = *(undefined4 *)(param_1 + 0x50);
      uVar8 = 1;
      uVar7 = *(undefined4 *)(param_1 + 0x4c);
      FUN_004aef30(&uStack_10);
      uVar6 = FUN_0043f620(uVar7,uVar6,uVar8);
      uVar6 = FUN_004abf50(*(undefined1 *)(param_2 + 0x22),*(undefined4 *)(param_1 + 0x40),
                           *(undefined4 *)(param_1 + 0x44),uVar6);
      iVar2 = FUN_004d2e00(uVar6);
      if (iVar2 == 0) {
        return 0;
      }
    }
    if (((*(byte *)(param_2 + 0xb) & 2) == 0) &&
       ((((*(byte *)(param_1 + 0x48) & 4) == 0 || ((*(byte *)(param_2 + 0x23) & 1) == 0)) &&
        ((bVar1 & 8) == 0)))) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_004ec150 @ 004ec150

int FUN_004ec150(undefined4 param_1,int *param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int local_cc;
  
  uVar1 = FUN_004abae0();
  if ((uVar1 & 4) == 0) {
    return 0;
  }
  iVar3 = 0;
  FUN_004ebad0(param_1,8);
  while (local_cc != 0) {
    if ((param_2 != (int *)0x0) && (param_2[2] < param_2[1])) {
      iVar2 = param_2[2] + 1;
      param_2[2] = iVar2;
      *(int *)(*param_2 + -4 + iVar2 * 4) = local_cc;
    }
    iVar3 = iVar3 + 1;
    FUN_004ec030();
  }
  return iVar3;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FID_conflict:_memcpy @ 004f1a60

/* Library Function - Multiple Matches With Different Base Names
    _memcpy
    _memmove
   
   Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release */

void * __cdecl FID_conflict__memcpy(void *_Dst,void *_Src,size_t _Size)

{
  uint uVar1;
  int in_EDX;
  uint uVar2;
  undefined4 *puVar3;
  undefined1 *puVar4;
  undefined4 *puVar5;
  undefined1 *puVar6;
  
  if ((_Src < _Dst) && (_Dst < (void *)((int)_Src + _Size))) {
    puVar3 = (undefined4 *)((int)_Src + _Size);
    puVar5 = (undefined4 *)((int)_Dst + _Size);
    if (((uint)puVar5 & 3) == 0) {
      uVar1 = _Size >> 2;
      while( true ) {
        puVar5 = puVar5 + -1;
        puVar3 = puVar3 + -1;
        if (uVar1 == 0) break;
        uVar1 = uVar1 - 1;
        *puVar5 = *puVar3;
      }
      switch(_Size & 3) {
      case 1:
switchD_004f1b29_caseD_1:
        *(undefined1 *)((int)puVar5 + 3) = *(undefined1 *)((int)puVar3 + 3);
        return _Dst;
      case 2:
switchD_004f1b29_caseD_2:
        *(undefined2 *)((int)puVar5 + 2) = *(undefined2 *)((int)puVar3 + 2);
        return _Dst;
      case 3:
switchD_004f1b29_caseD_3:
        *(undefined2 *)((int)puVar5 + 2) = *(undefined2 *)((int)puVar3 + 2);
        *(undefined1 *)((int)puVar5 + 1) = *(undefined1 *)((int)puVar3 + 1);
        return _Dst;
      }
    }
    else {
      puVar4 = (undefined1 *)((int)puVar3 + -1);
      puVar6 = (undefined1 *)((int)puVar5 + -1);
      if (_Size < 0xd) {
        for (; _Size != 0; _Size = _Size - 1) {
          *puVar6 = *puVar4;
          puVar4 = puVar4 + -1;
          puVar6 = puVar6 + -1;
        }
        return _Dst;
      }
      uVar2 = -in_EDX & 3;
      uVar1 = _Size - uVar2;
      for (; uVar2 != 0; uVar2 = uVar2 - 1) {
        *puVar6 = *puVar4;
        puVar4 = puVar4 + -1;
        puVar6 = puVar6 + -1;
      }
      puVar3 = (undefined4 *)(puVar4 + -3);
      puVar5 = (undefined4 *)(puVar6 + -3);
      for (uVar2 = uVar1 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
        *puVar5 = *puVar3;
        puVar3 = puVar3 + -1;
        puVar5 = puVar5 + -1;
      }
      switch(uVar1 & 3) {
      case 1:
        goto switchD_004f1b29_caseD_1;
      case 2:
        goto switchD_004f1b29_caseD_2;
      case 3:
        goto switchD_004f1b29_caseD_3;
      }
    }
    return _Dst;
  }
  puVar3 = _Dst;
  if (((uint)_Dst & 3) == 0) {
    for (uVar1 = _Size >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
      *puVar3 = *(undefined4 *)_Src;
      _Src = (undefined4 *)((int)_Src + 4);
      puVar3 = puVar3 + 1;
    }
    switch(_Size & 3) {
    case 1:
switchD_004f1a90_caseD_1:
      *(undefined1 *)puVar3 = *(undefined1 *)_Src;
      return _Dst;
    case 2:
switchD_004f1a90_caseD_2:
      *(undefined2 *)puVar3 = *(undefined2 *)_Src;
      return _Dst;
    case 3:
switchD_004f1a90_caseD_3:
      *(undefined2 *)puVar3 = *(undefined2 *)_Src;
      *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)_Src + 2);
      return _Dst;
    }
  }
  else {
    puVar4 = _Dst;
    if (_Size < 0xd) {
      for (; _Size != 0; _Size = _Size - 1) {
        *puVar4 = *(undefined1 *)_Src;
        _Src = (undefined1 *)((int)_Src + 1);
        puVar4 = puVar4 + 1;
      }
      return _Dst;
    }
    uVar2 = -(int)_Dst & 3;
    uVar1 = _Size - uVar2;
    for (; uVar2 != 0; uVar2 = uVar2 - 1) {
      *(undefined1 *)puVar3 = *(undefined1 *)_Src;
      _Src = (undefined4 *)((int)_Src + 1);
      puVar3 = (undefined4 *)((int)puVar3 + 1);
    }
    for (uVar2 = uVar1 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
      *puVar3 = *(undefined4 *)_Src;
      _Src = (undefined4 *)((int)_Src + 4);
      puVar3 = puVar3 + 1;
    }
    switch(uVar1 & 3) {
    case 1:
      goto switchD_004f1a90_caseD_1;
    case 2:
      goto switchD_004f1a90_caseD_2;
    case 3:
      goto switchD_004f1a90_caseD_3;
    }
  }
  return _Dst;
}


