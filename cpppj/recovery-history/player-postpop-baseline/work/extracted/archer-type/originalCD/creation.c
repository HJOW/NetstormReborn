// ==== FUN_004434d0 @ 004434d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004434d0(void)

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
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_00444218;
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
  iVar7 = iVar6;
  local_14 = iVar6;
  if (iVar6 == 0) {
    iVar6 = 0;
  }
  else {
    while (iVar5 = iVar5 + -1, -1 < iVar5) {
      FUN_00444230(iVar7);
      iVar7 = iVar7 + 0x1d4;
    }
  }
  local_8 = 0xffffffff;
  DAT_0051c960 = iVar6;
  FUN_00446080();
  iVar5 = DAT_0051c960;
  *(undefined1 **)(DAT_0051c968 * 0x1d4 + DAT_0051c960 + 0x1b0) = &LAB_004af5c0;
  *(code **)(DAT_0051c964 * 0x1d4 + 0x1b0 + iVar5) = FUN_004af920;
  *(code **)(DAT_0051c978 * 0x1d4 + 0x1b0 + iVar5) = FUN_004afe00;
  ppuVar8 = &PTR_s_evilRaySystemProcessType_0051c890;
  *(code **)(DAT_0051c96c * 0x1d4 + 0x1b0 + iVar5) = FUN_004afd60;
  iVar7 = 0x1248;
  for (iVar6 = 10; iVar6 < DAT_0051ca58; iVar6 = iVar6 + 1) {
    puVar1 = *ppuVar8;
    ppuVar8 = ppuVar8 + 1;
    FUN_004f2440(iVar7 + iVar5 + 4,puVar1,0x14);
    iVar5 = DAT_0051c960;
    iVar7 = iVar7 + 0x1d4;
  }
  *(code **)(DAT_0051c9bc * 0x1d4 + 0x1b0 + iVar5) = FUN_0047f310;
  *(code **)(DAT_0051c9c0 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047fc60;
  *(code **)(DAT_0051c9f8 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047f850;
  *(code **)(DAT_0051c9fc * 0x1d4 + 0x1b0 + iVar5) = FUN_0047f680;
  *(code **)(DAT_0051ca00 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047fdb0;
  *(code **)(DAT_0051ca04 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047fbd0;
  *(code **)(DAT_0051ca08 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047f3a0;
  *(code **)(DAT_0051ca38 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047fcf0;
  *(code **)(DAT_0051ca44 * 0x1d4 + 0x1b0 + iVar5) = FUN_0045f760;
  *(code **)(DAT_0051ca5c * 0x1d4 + 0x1b0 + iVar5) = FUN_004e8b10;
  *(code **)(DAT_0051ca60 * 0x1d4 + 0x1b0 + iVar5) = FUN_00435ff0;
  *(code **)(DAT_0051ca64 * 0x1d4 + 0x1b0 + iVar5) = FUN_00497950;
  *(code **)(DAT_0051ca68 * 0x1d4 + 0x1b0 + iVar5) = FUN_004979c0;
  *(code **)(DAT_0051ca70 * 0x1d4 + 0x1b0 + iVar5) = FUN_00459eb0;
  *(code **)(DAT_0051ca74 * 0x1d4 + 0x1b0 + iVar5) = FUN_00459eb0;
  *(code **)(DAT_0051ca78 * 0x1d4 + 0x1b0 + iVar5) = FUN_00459eb0;
  *(code **)(DAT_0051ca7c * 0x1d4 + 0x1b0 + iVar5) = FUN_004c07d0;
  *(code **)(DAT_0051ca80 * 0x1d4 + 0x1b0 + iVar5) = FUN_004c07f0;
  *(code **)(DAT_0051ca8c * 0x1d4 + 0x1b0 + iVar5) = FUN_004491a0;
  *(code **)(DAT_0051ca90 * 0x1d4 + 0x1b0 + iVar5) = FUN_00435fd0;
  *(code **)(DAT_0051ca98 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047a190;
  *(code **)(DAT_0051ca9c * 0x1d4 + 0x1b0 + iVar5) = FUN_0047a190;
  *(code **)(DAT_0051caa0 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047a190;
  *(code **)(DAT_0051caa4 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047a190;
  *(code **)(DAT_0051caac * 0x1d4 + 0x1b0 + iVar5) = FUN_004783c0;
  *(code **)(DAT_0051cabc * 0x1d4 + 0x1b0 + iVar5) = FUN_004d0330;
  *(code **)(DAT_0051cac0 * 0x1d4 + 0x1b0 + iVar5) = FUN_004d0380;
  *(undefined1 **)(DAT_0051cad0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00415d10;
  *(undefined1 **)(DAT_0051cad4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416d60;
  *(undefined1 **)(DAT_0051cad8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416d60;
  *(undefined1 **)(DAT_0051cae0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0048aeb0;
  *(code **)(DAT_0051cae8 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047ca20;
  *(code **)(DAT_0051cb00 * 0x1d4 + 0x1b0 + iVar5) = FUN_004e7bc0;
  *(code **)(DAT_0051cb04 * 0x1d4 + 0x1b0 + iVar5) = FUN_00416aa0;
  *(code **)(DAT_0051cb08 * 0x1d4 + 0x1b0 + iVar5) = FUN_004601a0;
  *(code **)(DAT_0051cb0c * 0x1d4 + 0x1b0 + iVar5) = FUN_004601d0;
  *(code **)(DAT_0051cb10 * 0x1d4 + 0x1b0 + iVar5) = FUN_00460230;
  *(code **)(DAT_0051cb14 * 0x1d4 + 0x1b0 + iVar5) = FUN_00460260;
  *(code **)(DAT_0051cb18 * 0x1d4 + 0x1b0 + iVar5) = FUN_00460290;
  *(code **)(DAT_0051cb1c * 0x1d4 + 0x1b0 + iVar5) = FUN_004602c0;
  *(code **)(DAT_0051cb20 * 0x1d4 + 0x1b0 + iVar5) = FUN_004602f0;
  *(code **)(DAT_0051cb24 * 0x1d4 + 0x1b0 + iVar5) = FUN_00460320;
  *(code **)(DAT_0051cb28 * 0x1d4 + 0x1b0 + iVar5) = FUN_00416d80;
  *(code **)(DAT_0051cb2c * 0x1d4 + 0x1b0 + iVar5) = FUN_00414d50;
  *(code **)(DAT_0051cb30 * 0x1d4 + 0x1b0 + iVar5) = FUN_0044fda0;
  *(code **)(DAT_0051cb34 * 0x1d4 + 0x1b0 + iVar5) = FUN_0044fd70;
  *(code **)(DAT_0051cb38 * 0x1d4 + 0x1b0 + iVar5) = FUN_0044fdd0;
  *(code **)(DAT_0051cb3c * 0x1d4 + 0x1b0 + iVar5) = FUN_0047b540;
  *(code **)(DAT_0051cb44 * 0x1d4 + 0x1b0 + iVar5) = FUN_00435f40;
  *(code **)(DAT_0051cb48 * 0x1d4 + 0x1b0 + iVar5) = FUN_00435f40;
  *(code **)(DAT_0051cb50 * 0x1d4 + 0x1b0 + iVar5) = FUN_00435fa0;
  *(code **)(DAT_0051cb54 * 0x1d4 + 0x1b0 + iVar5) = FUN_00435fd0;
  *(undefined1 **)(DAT_0051cb5c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0c80;
  *(code **)(DAT_0051cb60 * 0x1d4 + 0x1b0 + iVar5) = FUN_004e7940;
  *(code **)(DAT_0051cb64 * 0x1d4 + 0x1b0 + iVar5) = FUN_004783f0;
  *(code **)(DAT_0051cb68 * 0x1d4 + 0x1b0 + iVar5) = FUN_004e7920;
  *(code **)(DAT_0051cb6c * 0x1d4 + 0x1b0 + iVar5) = FUN_00497930;
  *(code **)(DAT_0051cb70 * 0x1d4 + 0x1b0 + iVar5) = FUN_004979a0;
  iVar7 = DAT_0051cb78;
  *(code **)(DAT_0051cb74 * 0x1d4 + iVar5 + 0x1b0) = FUN_00435f60;
  *(code **)(iVar7 * 0x1d4 + 0x1b0 + iVar5) = FUN_004e7bc0;
  *(code **)(DAT_0051cb7c * 0x1d4 + 0x1b0 + iVar5) = FUN_004c0810;
  *(code **)(DAT_0051cb80 * 0x1d4 + 0x1b0 + iVar5) = FUN_00497980;
  *(code **)(DAT_0051cb84 * 0x1d4 + 0x1b0 + iVar5) = FUN_00497910;
  *(code **)(DAT_0051cb88 * 0x1d4 + 0x1b0 + iVar5) = FUN_00491420;
  *(code **)(DAT_0051cb8c * 0x1d4 + 0x1b0 + iVar5) = FUN_004e7be0;
  *(code **)(DAT_0051cb90 * 0x1d4 + 0x1b0 + iVar5) = FUN_004c07d0;
  *(code **)(DAT_0051cb94 * 0x1d4 + 0x1b0 + iVar5) = FUN_00435f80;
  *(code **)(DAT_0051cb98 * 0x1d4 + 0x1b0 + iVar5) = FUN_00491440;
  *(code **)(DAT_0051cb9c * 0x1d4 + 0x1b0 + iVar5) = FUN_004c07b0;
  *(code **)(DAT_0051cba8 * 0x1d4 + 0x1b0 + iVar5) = FUN_00414110;
  *(code **)(DAT_0051cbac * 0x1d4 + 0x1b0 + iVar5) = FUN_00414c20;
  *(code **)(DAT_0051cbb4 * 0x1d4 + 0x1b0 + iVar5) = FUN_004e7960;
  *(code **)(DAT_0051cbb8 * 0x1d4 + 0x1b0 + iVar5) = FUN_004d0400;
  *(code **)(DAT_0051cbbc * 0x1d4 + 0x1b0 + iVar5) = FUN_0040d790;
  *(code **)(DAT_0051cbc0 * 0x1d4 + 0x1b0 + iVar5) = FUN_0044bfb0;
  *(code **)(DAT_0051cbc4 * 0x1d4 + 0x1b0 + iVar5) = FUN_0047c880;
  *(code **)(DAT_0051cbc8 * 0x1d4 + 0x1b0 + iVar5) = FUN_0046de60;
  *(code **)(DAT_0051cbd0 * 0x1d4 + 0x1b0 + iVar5) = FUN_004990f0;
  *(code **)(DAT_0051cbd4 * 0x1d4 + 0x1b0 + iVar5) = FUN_00483d80;
  *(code **)(DAT_0051cbd8 * 0x1d4 + 0x1b0 + iVar5) = FUN_00483500;
  *(code **)(DAT_0051cbdc * 0x1d4 + 0x1b0 + iVar5) = FUN_00483560;
  *(code **)(DAT_0051cbe0 * 0x1d4 + 0x1b0 + iVar5) = FUN_00483da0;
  *(code **)(DAT_0051cbe4 * 0x1d4 + 0x1b0 + iVar5) = FUN_00460200;
  *(undefined1 **)(DAT_0051cbe8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483520;
  *(code **)(DAT_0051cbec * 0x1d4 + 0x1b0 + iVar5) = FUN_00478410;
  *(code **)(DAT_0051c988 * 0x1d4 + 0x1b0 + iVar5) = FUN_004b36a0;
  *(code **)(DAT_0051c98c * 0x1d4 + 0x1b0 + iVar5) = FUN_004cf190;
  *(code **)(DAT_0051c990 * 0x1d4 + 0x1b0 + iVar5) = FUN_004cf290;
  *(undefined1 **)(DAT_0051c994 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00455b90;
  *(undefined1 **)(DAT_0051c9b8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0045d3d0;
  *(code **)(DAT_0051ca28 * 0x1d4 + 0x1b0 + iVar5) = FUN_0048b950;
  *(code **)(DAT_0051ca2c * 0x1d4 + 0x1b0 + iVar5) = FUN_0048ba20;
  *(code **)(DAT_0051ca40 * 0x1d4 + 0x1b0 + iVar5) = FUN_004cf210;
  *(code **)(DAT_0051caa8 * 0x1d4 + 0x1b0 + iVar5) = FUN_0044a4c0;
  *(code **)(DAT_0051caec * 0x1d4 + 0x1b0 + iVar5) = FUN_0047c5a0;
  *(code **)(DAT_0051caf4 * 0x1d4 + 0x1b0 + iVar5) = FUN_0046de00;
  *(code **)(DAT_0051caf8 * 0x1d4 + 0x1b0 + iVar5) = FUN_0046de30;
  *(code **)(DAT_0051cafc * 0x1d4 + 0x1b0 + iVar5) = FUN_00401bf0;
  *(undefined1 **)(DAT_0051cb58 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fd90;
  *(code **)(DAT_0051cba4 * 0x1d4 + 0x1b0 + iVar5) = FUN_00407950;
  iVar7 = DAT_0051cbf0;
  _DAT_0051c6e8 = 0.0;
  _DAT_0051c6ec = 0.0;
  *(code **)(DAT_0051cbb0 * 0x1d4 + 0x1b0 + iVar5) = FUN_004487a0;
  for (iVar6 = 0; iVar6 < iVar7; iVar6 = iVar6 + 1) {
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


// ==== FUN_00444230 @ 00444230

void * __thiscall FUN_00444230(void *this)

{
  int iVar1;
  undefined4 *puVar2;
  
  puVar2 = this;
  for (iVar1 = 0x75; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined4 *)((int)this + 0x98) = 4;
  *(undefined4 *)((int)this + 0x120) = 0xffffffff;
  *(undefined4 *)((int)this + 0x128) = 0xffffffff;
  *(undefined4 *)((int)this + 0x9c) = 10;
  *(undefined4 *)((int)this + 0x1cc) = 0xffffffff;
  return this;
}


// ==== FUN_00446080 @ 00446080

void __cdecl FUN_00446080(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = 0;
  FUN_00445f40();
  DAT_0052d910 = (FILE *)0x0;
  DAT_0051cc08 = 0;
  FUN_00445e70(PTR_DAT_0051c6f8);
  FUN_0043b270();
  _fclose(DAT_0052d910);
  if (0 < DAT_0051cbf0) {
    iVar3 = 0;
    do {
      iVar1 = iVar3 + DAT_0051c960;
      iVar3 = iVar3 + 0x1d4;
      iVar2 = iVar2 + 1;
      FUN_00444e10(iVar1);
    } while (iVar2 < DAT_0051cbf0);
  }
  FUN_00446040();
  return;
}


// ==== FUN_004460f0 @ 004460f0

void __cdecl FUN_004460f0(undefined4 param_1,int *param_2,int *param_3,int *param_4)

{
  char cVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  undefined1 *puVar5;
  undefined4 *puVar6;
  uint uVar7;
  uint uVar8;
  undefined **ppuVar9;
  int *piVar10;
  char *pcVar11;
  int *piVar12;
  undefined4 *unaff_FS_OFFSET;
  char *pcVar13;
  undefined1 local_58 [4];
  int local_54;
  uint local_40;
  int *local_38;
  int local_34;
  int *local_28;
  int local_24;
  int *local_1c;
  char *local_18;
  int *local_14;
  undefined4 local_10;
  code *pcStack_c;
  uint local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0044832c;
  *unaff_FS_OFFSET = &local_10;
  pcVar11 = *(char **)(&DAT_0051c6f4 + DAT_0051cc08 * 4);
  uVar7 = 0xffffffff;
  local_18 = (char *)(DAT_0051cc08 * 0x1d4 + 0x7e24 + DAT_0051c960);
  local_18[0x1b4] = '\x01';
  local_18[0x1b5] = '\0';
  local_18[0x1b6] = '\0';
  local_18[0x1b7] = '\0';
  local_18[0x1b8] = '\x01';
  local_18[0x1b9] = '\0';
  local_18[0x1ba] = '\0';
  local_18[0x1bb] = '\0';
  do {
    pcVar13 = pcVar11;
    if (uVar7 == 0) break;
    uVar7 = uVar7 - 1;
    pcVar13 = pcVar11 + 1;
    cVar1 = *pcVar11;
    pcVar11 = pcVar13;
  } while (cVar1 != '\0');
  uVar7 = ~uVar7;
  pcVar13 = pcVar13 + -uVar7;
  pcVar11 = local_18;
  for (uVar8 = uVar7 >> 2; pcVar11 = pcVar11 + 4, uVar8 != 0; uVar8 = uVar8 - 1) {
    *(undefined4 *)pcVar11 = *(undefined4 *)pcVar13;
    pcVar13 = pcVar13 + 4;
  }
  for (uVar7 = uVar7 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
    *pcVar11 = *pcVar13;
    pcVar13 = pcVar13 + 1;
    pcVar11 = pcVar11 + 1;
  }
  local_18[0x11c] = -1;
  local_18[0x11d] = -1;
  local_18[0x11e] = -1;
  local_18[0x11f] = -1;
  local_18[0xe8] = '\0';
  local_18[0xe9] = '\0';
  local_18[0xea] = '\0';
  local_18[0xeb] = '\0';
  local_18[0xec] = '\0';
  local_18[0xed] = '\0';
  local_18[0xee] = '\0';
  local_18[0xef] = '\0';
  if (param_2 != (int *)0x0) {
    local_14 = param_2;
    if ((*param_2 != 1) && (param_2[1] != 1)) {
      FUN_00490040(s_flag_>btype____BT_IDENTIFIER____f_0051dadc,s__ns_o_RiftType_cpp_0051dac8,0x558)
      ;
    }
    iVar3 = local_14[1];
    piVar10 = local_14;
    while (piVar10 != (int *)0x0) {
      if ((*piVar10 == 0) && (piVar10[1] == iVar3)) {
        piVar12 = (int *)piVar10[3];
        local_14 = (int *)piVar10[2];
      }
      else {
        piVar12 = (int *)0x0;
        local_14 = piVar10;
      }
      if (*local_14 != 1) {
        FUN_00490040(s_flag_>btype____BT_IDENTIFIER_0051db30,s__ns_o_RiftType_cpp_0051db1c,0x55d);
      }
      iVar2 = FUN_004f2980(local_14[2],s_dontdrawdefault_0051db50);
      if (iVar2 == 0) {
        FUN_00490040(s_false_0051db74,s__ns_o_RiftType_cpp_0051db60,0x563);
      }
      iVar2 = FUN_004f2980(local_14[2],s_carribleInVehicle_0051db7c);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x10000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_container_0051db90);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x20000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_default_hotspot_0051db9c);
      if ((iVar2 == 0) || (iVar2 = FUN_004f2980(local_14[2],s_defaulthotspot_0051dbac), iVar2 == 0))
      {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 1;
      }
      iVar2 = FUN_004f2980(local_14[2],s_shadow_0051dbbc);
      if (iVar2 == 0) {
        if ((local_18[0xea] & 0x40U) != 0) {
          FUN_00490040(s___ts_>typeFlags___TYPE_FLAG_FLYE_0051dbd8,s__ns_o_RiftType_cpp_0051dbc4,
                       0x574);
        }
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x40000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_flyershadow_0051dc04);
      if (iVar2 == 0) {
        if ((local_18[0xea] & 4U) != 0) {
          FUN_00490040(s___ts_>typeFlags___TYPE_FLAG_SHAD_0051dc24,s__ns_o_RiftType_cpp_0051dc10,
                       0x579);
        }
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x400000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_not_selectable_0051dc48);
      if ((iVar2 == 0) || (iVar2 = FUN_004f2980(local_14[2],s_notselectable_0051dc58), iVar2 == 0))
      {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x8000000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_not_real_0051dc68);
      if ((iVar2 == 0) || (iVar2 = FUN_004f2980(local_14[2],s_notreal_0051dc74), iVar2 == 0)) {
        *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x2000000;
        uVar7 = *(uint *)(local_18 + 0xe8);
        *(uint *)(local_18 + 0xe8) = uVar7 | 0x8000000;
        *(uint *)(local_18 + 0xe8) = uVar7 | 0x28000000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_createsisland_0051dc7c);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x400;
      }
      iVar2 = FUN_004f2980(local_14[2],s_mayDropOnRim_0051dc8c);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 4;
      }
      iVar2 = FUN_004f2980(local_14[2],s_mayDropOnIsle_0051dc9c);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 2;
      }
      iVar2 = FUN_004f2980(local_14[2],s_surface_0051dcac);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x800;
      }
      iVar2 = FUN_004f2980(local_14[2],s_randframe_0051dcb4);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x100000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_matchframe_0051dcc0);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x200000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_opaqueCollide_0051dccc);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x800000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_dontSave_0051dcdc);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x20000000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_saveFrame_0051dce8);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x20;
      }
      iVar2 = FUN_004f2980(local_14[2],s_saveQA_0051dcf4);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x1000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_saveQB_0051dcfc);
      piVar10 = piVar12;
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x2000;
      }
      else {
        iVar2 = FUN_004f2980(local_14[2],s_dropblocking_0051dd04);
        if (iVar2 == 0) {
          *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x10;
        }
        else {
          iVar2 = FUN_004f2980(local_14[2],s_blocking_0051dd14);
          if (iVar2 == 0) {
            uVar7 = *(uint *)(local_18 + 0xec);
            *(uint *)(local_18 + 0xec) = uVar7 | 0x10;
            *(uint *)(local_18 + 0xec) = uVar7 | 0x18;
          }
          else {
            iVar2 = FUN_004f2980(local_14[2],s_shotblocking_0051dd20);
            if (iVar2 == 0) {
              *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x20;
            }
            else {
              iVar2 = FUN_004f2980(local_14[2],s_fencePost_0051dd30);
              if (iVar2 == 0) {
                *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 1;
              }
              else {
                iVar2 = FUN_004f2980(local_14[2],s_fence_0051dd3c);
                if (iVar2 == 0) {
                  *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x20000000;
                }
                else {
                  iVar2 = FUN_004f2980(local_14[2],s_buried_0051dd44);
                  if (iVar2 == 0) {
                    uVar7 = 0xffffffff;
                    pcVar11 = (char *)local_14[2];
                    do {
                      if (uVar7 == 0) break;
                      uVar7 = uVar7 - 1;
                      cVar1 = *pcVar11;
                      pcVar11 = pcVar11 + 1;
                    } while (cVar1 != '\0');
                    uVar8 = 8;
                    if (~uVar7 < 8) {
                      uVar8 = 0xffffffff;
                      pcVar11 = (char *)local_14[2];
                      do {
                        if (uVar8 == 0) break;
                        uVar8 = uVar8 - 1;
                        cVar1 = *pcVar11;
                        pcVar11 = pcVar11 + 1;
                      } while (cVar1 != '\0');
                      uVar8 = ~uVar8;
                    }
                    FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                    *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x2000;
                  }
                  else {
                    iVar2 = FUN_004f2980(local_14[2],s_nugget_0051dd4c);
                    if (iVar2 == 0) {
                      uVar7 = 0xffffffff;
                      pcVar11 = (char *)local_14[2];
                      do {
                        if (uVar7 == 0) break;
                        uVar7 = uVar7 - 1;
                        cVar1 = *pcVar11;
                        pcVar11 = pcVar11 + 1;
                      } while (cVar1 != '\0');
                      uVar8 = 8;
                      if (~uVar7 < 8) {
                        uVar8 = 0xffffffff;
                        pcVar11 = (char *)local_14[2];
                        do {
                          if (uVar8 == 0) break;
                          uVar8 = uVar8 - 1;
                          cVar1 = *pcVar11;
                          pcVar11 = pcVar11 + 1;
                        } while (cVar1 != '\0');
                        uVar8 = ~uVar8;
                      }
                      FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                      *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x8000;
                    }
                    else {
                      iVar2 = FUN_004f2980(local_14[2],&DAT_0051dd54);
                      if (iVar2 == 0) {
                        uVar7 = 0xffffffff;
                        pcVar11 = (char *)local_14[2];
                        do {
                          if (uVar7 == 0) break;
                          uVar7 = uVar7 - 1;
                          cVar1 = *pcVar11;
                          pcVar11 = pcVar11 + 1;
                        } while (cVar1 != '\0');
                        uVar8 = 8;
                        if (~uVar7 < 8) {
                          uVar8 = 0xffffffff;
                          pcVar11 = (char *)local_14[2];
                          do {
                            if (uVar8 == 0) break;
                            uVar8 = uVar8 - 1;
                            cVar1 = *pcVar11;
                            pcVar11 = pcVar11 + 1;
                          } while (cVar1 != '\0');
                          uVar8 = ~uVar8;
                        }
                        FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                        *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x100;
                      }
                      else {
                        iVar2 = FUN_004f2980(local_14[2],s_emplacement_0051dd5c);
                        if (iVar2 == 0) {
                          *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x40000;
                        }
                        else {
                          iVar2 = FUN_004f2980(local_14[2],s_bridge_0051dd68);
                          if (iVar2 == 0) {
                            uVar7 = 0xffffffff;
                            pcVar11 = (char *)local_14[2];
                            do {
                              if (uVar7 == 0) break;
                              uVar7 = uVar7 - 1;
                              cVar1 = *pcVar11;
                              pcVar11 = pcVar11 + 1;
                            } while (cVar1 != '\0');
                            uVar8 = 8;
                            if (~uVar7 < 8) {
                              uVar8 = 0xffffffff;
                              pcVar11 = (char *)local_14[2];
                              do {
                                if (uVar8 == 0) break;
                                uVar8 = uVar8 - 1;
                                cVar1 = *pcVar11;
                                pcVar11 = pcVar11 + 1;
                              } while (cVar1 != '\0');
                              uVar8 = ~uVar8;
                            }
                            FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                            *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 4;
                          }
                          else {
                            iVar2 = FUN_004f2980(local_14[2],s_walker_0051dd70);
                            if (iVar2 == 0) {
                              uVar7 = 0xffffffff;
                              pcVar11 = (char *)local_14[2];
                              do {
                                if (uVar7 == 0) break;
                                uVar7 = uVar7 - 1;
                                cVar1 = *pcVar11;
                                pcVar11 = pcVar11 + 1;
                              } while (cVar1 != '\0');
                              uVar8 = 8;
                              if (~uVar7 < 8) {
                                uVar8 = 0xffffffff;
                                pcVar11 = (char *)local_14[2];
                                do {
                                  if (uVar8 == 0) break;
                                  uVar8 = uVar8 - 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                } while (cVar1 != '\0');
                                uVar8 = ~uVar8;
                              }
                              FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                              *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x10000;
                            }
                            else {
                              iVar2 = FUN_004f2980(local_14[2],s_balloon_0051dd78);
                              if (iVar2 == 0) {
                                uVar7 = 0xffffffff;
                                pcVar11 = (char *)local_14[2];
                                do {
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                } while (cVar1 != '\0');
                                uVar8 = 8;
                                if (~uVar7 < 8) {
                                  uVar8 = 0xffffffff;
                                  pcVar11 = (char *)local_14[2];
                                  do {
                                    if (uVar8 == 0) break;
                                    uVar8 = uVar8 - 1;
                                    cVar1 = *pcVar11;
                                    pcVar11 = pcVar11 + 1;
                                  } while (cVar1 != '\0');
                                  uVar8 = ~uVar8;
                                }
                                FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x20000;
                              }
                              else {
                                iVar2 = FUN_004f2980(local_14[2],s_island_0051dd80);
                                if (iVar2 == 0) {
                                  uVar7 = 0xffffffff;
                                  pcVar11 = (char *)local_14[2];
                                  do {
                                    if (uVar7 == 0) break;
                                    uVar7 = uVar7 - 1;
                                    cVar1 = *pcVar11;
                                    pcVar11 = pcVar11 + 1;
                                  } while (cVar1 != '\0');
                                  uVar8 = 8;
                                  if (~uVar7 < 8) {
                                    uVar8 = 0xffffffff;
                                    pcVar11 = (char *)local_14[2];
                                    do {
                                      if (uVar8 == 0) break;
                                      uVar8 = uVar8 - 1;
                                      cVar1 = *pcVar11;
                                      pcVar11 = pcVar11 + 1;
                                    } while (cVar1 != '\0');
                                    uVar8 = ~uVar8;
                                  }
                                  FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                  *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 2;
                                }
                                else {
                                  iVar2 = FUN_004f2980(local_14[2],s_islandThreeByThree_0051dd88);
                                  if (iVar2 == 0) {
                                    *(uint *)(local_18 + 0xec) =
                                         *(uint *)(local_18 + 0xec) | 0x1000000;
                                  }
                                  else {
                                    iVar2 = FUN_004f2980(local_14[2],s_factory_0051dd9c);
                                    if (iVar2 == 0) {
                                      uVar7 = 0xffffffff;
                                      pcVar11 = (char *)local_14[2];
                                      do {
                                        if (uVar7 == 0) break;
                                        uVar7 = uVar7 - 1;
                                        cVar1 = *pcVar11;
                                        pcVar11 = pcVar11 + 1;
                                      } while (cVar1 != '\0');
                                      uVar8 = 8;
                                      if (~uVar7 < 8) {
                                        uVar8 = 0xffffffff;
                                        pcVar11 = (char *)local_14[2];
                                        do {
                                          if (uVar8 == 0) break;
                                          uVar8 = uVar8 - 1;
                                          cVar1 = *pcVar11;
                                          pcVar11 = pcVar11 + 1;
                                        } while (cVar1 != '\0');
                                        uVar8 = ~uVar8;
                                      }
                                      FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                      *(uint *)(local_18 + 0xec) =
                                           *(uint *)(local_18 + 0xec) | 0x4000;
                                    }
                                    else {
                                      iVar2 = FUN_004f2980(local_14[2],s_vortex_0051dda4);
                                      if (iVar2 == 0) {
                                        uVar7 = 0xffffffff;
                                        pcVar11 = (char *)local_14[2];
                                        do {
                                          if (uVar7 == 0) break;
                                          uVar7 = uVar7 - 1;
                                          cVar1 = *pcVar11;
                                          pcVar11 = pcVar11 + 1;
                                        } while (cVar1 != '\0');
                                        uVar8 = 8;
                                        if (~uVar7 < 8) {
                                          uVar8 = 0xffffffff;
                                          pcVar11 = (char *)local_14[2];
                                          do {
                                            if (uVar8 == 0) break;
                                            uVar8 = uVar8 - 1;
                                            cVar1 = *pcVar11;
                                            pcVar11 = pcVar11 + 1;
                                          } while (cVar1 != '\0');
                                          uVar8 = ~uVar8;
                                        }
                                        FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                        *(uint *)(local_18 + 0xec) =
                                             *(uint *)(local_18 + 0xec) | 0x200;
                                      }
                                      else {
                                        iVar2 = FUN_004f2980(local_14[2],s_edgefarm_0051ddac);
                                        if (iVar2 == 0) {
                                          uVar7 = 0xffffffff;
                                          pcVar11 = (char *)local_14[2];
                                          do {
                                            if (uVar7 == 0) break;
                                            uVar7 = uVar7 - 1;
                                            cVar1 = *pcVar11;
                                            pcVar11 = pcVar11 + 1;
                                          } while (cVar1 != '\0');
                                          uVar8 = 8;
                                          if (~uVar7 < 8) {
                                            uVar8 = 0xffffffff;
                                            pcVar11 = (char *)local_14[2];
                                            do {
                                              if (uVar8 == 0) break;
                                              uVar8 = uVar8 - 1;
                                              cVar1 = *pcVar11;
                                              pcVar11 = pcVar11 + 1;
                                            } while (cVar1 != '\0');
                                            uVar8 = ~uVar8;
                                          }
                                          FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                          *(uint *)(local_18 + 0xec) =
                                               *(uint *)(local_18 + 0xec) | 0x80000;
                                        }
                                        else {
                                          iVar2 = FUN_004f2980(local_14[2],s_focus_0051ddb8);
                                          if (iVar2 == 0) {
                                            uVar7 = 0xffffffff;
                                            pcVar11 = (char *)local_14[2];
                                            do {
                                              if (uVar7 == 0) break;
                                              uVar7 = uVar7 - 1;
                                              cVar1 = *pcVar11;
                                              pcVar11 = pcVar11 + 1;
                                            } while (cVar1 != '\0');
                                            uVar8 = 8;
                                            if (~uVar7 < 8) {
                                              uVar8 = 0xffffffff;
                                              pcVar11 = (char *)local_14[2];
                                              do {
                                                if (uVar8 == 0) break;
                                                uVar8 = uVar8 - 1;
                                                cVar1 = *pcVar11;
                                                pcVar11 = pcVar11 + 1;
                                              } while (cVar1 != '\0');
                                              uVar8 = ~uVar8;
                                            }
                                            FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                          }
                                          else {
                                            iVar2 = FUN_004f2980(local_14[2],&DAT_0051ddc0);
                                            if (iVar2 == 0) {
                                              uVar7 = 0xffffffff;
                                              pcVar11 = (char *)local_14[2];
                                              do {
                                                if (uVar7 == 0) break;
                                                uVar7 = uVar7 - 1;
                                                cVar1 = *pcVar11;
                                                pcVar11 = pcVar11 + 1;
                                              } while (cVar1 != '\0');
                                              uVar8 = 8;
                                              if (~uVar7 < 8) {
                                                uVar8 = 0xffffffff;
                                                pcVar11 = (char *)local_14[2];
                                                do {
                                                  if (uVar8 == 0) break;
                                                  uVar8 = uVar8 - 1;
                                                  cVar1 = *pcVar11;
                                                  pcVar11 = pcVar11 + 1;
                                                } while (cVar1 != '\0');
                                                uVar8 = ~uVar8;
                                              }
                                              FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                              *(uint *)(local_18 + 0xec) =
                                                   *(uint *)(local_18 + 0xec) | 0x400;
                                            }
                                            else {
                                              iVar2 = FUN_004f2980(local_14[2],s_geyser_0051ddc4);
                                              if (iVar2 == 0) {
                                                uVar7 = 0xffffffff;
                                                pcVar11 = (char *)local_14[2];
                                                do {
                                                  if (uVar7 == 0) break;
                                                  uVar7 = uVar7 - 1;
                                                  cVar1 = *pcVar11;
                                                  pcVar11 = pcVar11 + 1;
                                                } while (cVar1 != '\0');
                                                uVar8 = 8;
                                                if (~uVar7 < 8) {
                                                  uVar8 = 0xffffffff;
                                                  pcVar11 = (char *)local_14[2];
                                                  do {
                                                    if (uVar8 == 0) break;
                                                    uVar8 = uVar8 - 1;
                                                    cVar1 = *pcVar11;
                                                    pcVar11 = pcVar11 + 1;
                                                  } while (cVar1 != '\0');
                                                  uVar8 = ~uVar8;
                                                }
                                                FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                                *(uint *)(local_18 + 0xec) =
                                                     *(uint *)(local_18 + 0xec) | 0x10000000;
                                              }
                                              else {
                                                iVar2 = FUN_004f2980(local_14[2],s_flyer_0051ddcc);
                                                if (iVar2 == 0) {
                                                  *(uint *)(local_18 + 0xec) =
                                                       *(uint *)(local_18 + 0xec) | 0x100000;
                                                }
                                                else {
                                                  iVar2 = FUN_004f2980(local_14[2],
                                                                       s_residence_0051ddd4);
                                                  if (iVar2 == 0) {
                                                    uVar7 = 0xffffffff;
                                                    pcVar11 = (char *)local_14[2];
                                                    do {
                                                      if (uVar7 == 0) break;
                                                      uVar7 = uVar7 - 1;
                                                      cVar1 = *pcVar11;
                                                      pcVar11 = pcVar11 + 1;
                                                    } while (cVar1 != '\0');
                                                    uVar8 = 8;
                                                    if (~uVar7 < 8) {
                                                      uVar8 = 0xffffffff;
                                                      pcVar11 = (char *)local_14[2];
                                                      do {
                                                        if (uVar8 == 0) break;
                                                        uVar8 = uVar8 - 1;
                                                        cVar1 = *pcVar11;
                                                        pcVar11 = pcVar11 + 1;
                                                      } while (cVar1 != '\0');
                                                      uVar8 = ~uVar8;
                                                    }
                                                    FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                                    *(uint *)(local_18 + 0xec) =
                                                         *(uint *)(local_18 + 0xec) | 0x40000000;
                                                  }
                                                  else {
                                                    iVar2 = FUN_004f2980(local_14[2],
                                                                         s_fringe_0051dde0);
                                                    if (iVar2 == 0) {
                                                      *(uint *)(local_18 + 0xec) =
                                                           *(uint *)(local_18 + 0xec) | 0x1000;
                                                    }
                                                    else {
                                                      iVar2 = FUN_004f2980(local_14[2],
                                                                           s_priest_0051dde8);
                                                      if (iVar2 == 0) {
                                                        *(uint *)(local_18 + 0xec) =
                                                             *(uint *)(local_18 + 0xec) | 0x200000;
                                                      }
                                                      else {
                                                        iVar2 = FUN_004f2980(local_14[2],
                                                                             &DAT_0051ddf0);
                                                        if (iVar2 == 0) {
                                                          *(uint *)(local_18 + 0xec) =
                                                               *(uint *)(local_18 + 0xec) | 0x400000
                                                          ;
                                                        }
                                                        else {
                                                          iVar2 = FUN_004f2980(local_14[2],
                                                                               s_altar_0051ddf8);
                                                          if (iVar2 == 0) {
                                                            *(uint *)(local_18 + 0xec) =
                                                                 *(uint *)(local_18 + 0xec) |
                                                                 0x80000000;
                                                          }
                                                          else {
                                                            iVar2 = FUN_004f2980(local_14[2],
                                                                                                                                                                  
                                                  s_predictable_0051de00);
                                                  if (iVar2 == 0) {
                                                    *(uint *)(local_18 + 0x1c8) =
                                                         *(uint *)(local_18 + 0x1c8) | 99;
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  if (param_3 == (int *)0x0) {
LAB_00448264:
    local_1c = param_4;
    if ((param_4[1] != 2) && (*param_4 != 3)) {
      FUN_00490040(s_frameBud_>utype____UT_FRAME_LIST_0051eea0,s__ns_o_RiftType_cpp_0051ee8c,0x7b3);
    }
    iVar2 = -1;
    iVar3 = 0;
    local_28 = local_1c;
    local_24 = local_1c[1];
    for (piVar10 = local_1c;
        ((piVar10 != (int *)0x0 && (*piVar10 == 0)) && (piVar10[1] == local_24));
        piVar10 = (int *)piVar10[3]) {
      iVar3 = iVar3 + 1;
    }
    iVar3 = (uint)(piVar10 != (int *)0x0) + iVar3;
    *(int *)(local_18 + 0x114) = iVar3;
    puVar6 = (undefined4 *)FUN_004f1650(iVar3 * 4);
    local_40 = 0;
    *(undefined4 **)(local_18 + 0x124) = puVar6;
    for (uVar7 = *(uint *)(local_18 + 0x114) & 0x3fffffff; uVar7 != 0; uVar7 = uVar7 - 1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    while (local_28 != (int *)0x0) {
      iVar2 = iVar2 + 1;
      local_1c = local_28;
      if ((*local_28 == 0) && (local_28[1] == local_24)) {
        local_1c = (int *)local_28[2];
        local_28 = (int *)local_28[3];
      }
      else {
        local_28 = (int *)0x0;
      }
      pcVar11 = (char *)(iVar2 * 4 + *(int *)(local_18 + 0x124));
      if (*local_1c != 3) {
        FUN_00490040(s_frameBud_>btype____BT_FRAME_0051eef4,s__ns_o_RiftType_cpp_0051eee0,0x7bc);
      }
      piVar10 = (int *)local_1c[2];
      if (*piVar10 != 4) {
        FUN_00490040(s_orientVariant_>btype____BT_ORIEN_0051ef24,s__ns_o_RiftType_cpp_0051ef10,0x7c0
                    );
      }
      cVar1 = **(char **)(piVar10[2] + 8);
      *pcVar11 = cVar1;
      if ((cVar1 < 'A') || ('Z' < cVar1)) {
        FUN_00490040(s__A__<__frame_>sideOrientation____0051ef64,s__ns_o_RiftType_cpp_0051ef50,0x7c3
                    );
      }
      iVar3 = piVar10[2];
      pcVar11[1] = 'P';
      cVar1 = *(char *)(*(int *)(iVar3 + 8) + 1);
      if (cVar1 != '\0') {
        pcVar11[1] = cVar1;
      }
      cVar1 = __ftol();
      pcVar11[3] = '\0';
      pcVar11[2] = cVar1;
      piVar10 = (int *)local_1c[3];
      if (piVar10 != (int *)0x0) {
        local_54 = piVar10[1];
        do {
          if ((*piVar10 == 0) && (piVar10[1] == local_54)) {
            piVar12 = (int *)piVar10[3];
            local_14 = (int *)piVar10[2];
          }
          else {
            piVar12 = (int *)0x0;
            local_14 = piVar10;
          }
          if (*local_14 != 1) {
            FUN_00490040(s_frameFlagBud_>btype____BT_IDENTI_0051efb8,s__ns_o_RiftType_cpp_0051efa4,
                         0x7d3);
          }
          iVar3 = FUN_004f2980(local_14[2],s_fringe_0051efe0);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 1;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051efe8);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 2;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051eff0);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 4;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051eff4);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 8;
          }
          iVar3 = FUN_004f2980(local_14[2],s_unlit_0051eff8);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 0x10;
          }
          iVar3 = FUN_004f2980(local_14[2],s_cracked_0051f000);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 0x20;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051f008);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 0x40;
          }
          iVar3 = FUN_004f2980(local_14[2],s_baseframe_0051f010);
          if (iVar3 == 0) {
            *(int *)(local_18 + 0x128) = iVar2;
          }
          iVar3 = FUN_004f2980(local_14[2],s_gumpframe_0051f01c);
          if (iVar3 == 0) {
            *(int *)(local_18 + 0x11c) = iVar2;
          }
          iVar3 = FUN_004f2980(local_14[2],s_default_0051f028);
          if ((iVar3 == 0) && (*(int *)(local_18 + 0x118) = iVar2, local_40 == 0)) {
            *(int *)(local_18 + 0x120) = iVar2;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051f030);
          if (iVar3 == 0) {
            local_40 = 1;
            *(int *)(local_18 + 0x120) = iVar2;
          }
          piVar10 = piVar12;
        } while (piVar12 != (int *)0x0);
      }
    }
    if (DAT_0051cc20 == (int *)0x0) {
      DAT_0051cc20 = DAT_0051cbf4;
    }
    iVar3 = *DAT_0051cc20;
    for (iVar2 = 0; (iVar3 != 0x30312e31 && (iVar2 < 1000)); iVar2 = iVar2 + 1) {
      DAT_0051cc20 = DAT_0051cc20 + 1;
      iVar3 = *DAT_0051cc20;
    }
    if (999 < iVar2) {
      FUN_00490040(s_i_<_1000_0051f04c,s__ns_o_RiftType_cpp_0051f038,0x80a);
    }
    *(int **)(local_18 + 0xdc) = DAT_0051cc20;
    DAT_0051cc20 = DAT_0051cc20 + 1;
    FUN_00445ff0();
    *unaff_FS_OFFSET = local_10;
    return;
  }
  local_14 = param_3;
  if ((*param_3 != 9) && (param_3[1] != 7)) {
    FUN_00490040(s_attributeValue_>btype____BT_ATTR_0051de20,s__ns_o_RiftType_cpp_0051de0c,0x63c);
  }
  local_34 = local_14[1];
  piVar10 = local_14;
joined_r0x00446dcc:
  local_38 = piVar10;
  if (piVar10 == (int *)0x0) goto LAB_00448264;
  if ((*piVar10 == 0) && (piVar10[1] == local_34)) {
    local_38 = (int *)piVar10[3];
    local_14 = (int *)piVar10[2];
  }
  else {
    local_38 = (int *)0x0;
    local_14 = piVar10;
  }
  if (*local_14 != 9) {
    FUN_00490040(s_attributeValue_>btype____BT_ATTR_0051de94,s__ns_o_RiftType_cpp_0051de80,0x641);
  }
  iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxHitPoints_0051dec0);
  if (iVar3 == 0) {
    *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x10;
    if (*(int *)local_14[3] != 2) {
      FUN_00490040(s_attributeValue_>p2_>btype____BT__0051dee4,s__ns_o_RiftType_cpp_0051ded0,0x647);
    }
    uVar4 = __ftol();
    FUN_00444750(local_18,uVar4);
    piVar10 = local_38;
    goto joined_r0x00446dcc;
  }
  iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rangeConst_0051df0c);
  if (iVar3 == 0) {
    uVar4 = 0x64d;
    pcVar13 = s__ns_o_RiftType_cpp_0051df18;
    pcVar11 = s_false_0051df2c;
  }
  else {
    iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_helpText_0051df34);
    if (iVar3 == 0) {
      if (*(int *)local_14[3] != 10) {
        FUN_00490040(s_attributeValue_>p2_>btype____BT__0051df54,s__ns_o_RiftType_cpp_0051df40,0x651
                    );
      }
      uVar4 = FUN_004f26f0(*(undefined4 *)(local_14[3] + 8));
      *(undefined4 *)(local_18 + 0x40) = uVar4;
      for (puVar5 = (undefined1 *)FUN_004f2b70(uVar4,&DAT_0051df84); piVar10 = local_38,
          puVar5 != (undefined1 *)0x0; puVar5 = (undefined1 *)FUN_004f2b70(puVar5,&DAT_0051df88)) {
        *puVar5 = 0x20;
        puVar5[1] = 10;
      }
      goto joined_r0x00446dcc;
    }
    iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rangeVar_0051df8c);
    if (iVar3 == 0) {
      uVar4 = 0x65e;
      pcVar13 = s__ns_o_RiftType_cpp_0051df98;
      pcVar11 = &DAT_0051dfac;
      goto LAB_00448252;
    }
    iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_attackValueConst_0051dfb0);
    if (iVar3 == 0) {
      uVar4 = 0x662;
      pcVar13 = s__ns_o_RiftType_cpp_0051dfc4;
      pcVar11 = &DAT_0051dfd8;
    }
    else {
      iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_attackValueVar_0051dfdc);
      if (iVar3 == 0) {
        uVar4 = 0x666;
        pcVar13 = s__ns_o_RiftType_cpp_0051dfec;
        pcVar11 = &DAT_0051e000;
      }
      else {
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_minUsage_0051e004);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e024,s__ns_o_RiftType_cpp_0051e010,
                         0x66a);
          }
          FUN_00444820(local_18,0,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxUsage_0051e04c);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e06c,s__ns_o_RiftType_cpp_0051e058,
                         0x66f);
          }
          FUN_00444870(local_18,0,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxRainBattleUsage_0051e094);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e0bc,s__ns_o_RiftType_cpp_0051e0a8,
                         0x674);
          }
          FUN_00444870(local_18,0,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxThunderBattleUsage_0051e0e4);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e110,s__ns_o_RiftType_cpp_0051e0fc,
                         0x679);
          }
          FUN_00444870(local_18,1,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxWindBattleUsage_0051e138);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e160,s__ns_o_RiftType_cpp_0051e14c,
                         0x67e);
          }
          FUN_00444870(local_18,2,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_zorder_0051e188);
        if ((iVar3 == 0) ||
           (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_cursorzorder_0051e190),
           iVar3 == 0)) {
          piVar10 = (int *)(local_18 + 0x104);
          iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_cursorzorder_0051e1a0);
          if (iVar3 == 0) {
            local_18[0x108] = '\x01';
            local_18[0x109] = '\0';
            local_18[0x10a] = '\0';
            local_18[0x10b] = '\0';
            piVar10 = (int *)(local_18 + 0x10c);
          }
          iVar3 = *(int *)local_14[3];
          if (iVar3 == 2) {
            iVar3 = __ftol();
            *piVar10 = iVar3;
            piVar10 = local_38;
            goto joined_r0x00446dcc;
          }
          if (iVar3 == 10) {
            iVar3 = ((int *)local_14[3])[2];
            FUN_00463650(local_58,s___A_Za_z0_9_______t___0051e1b0);
            local_8 = 0;
            iVar2 = FUN_00463680(local_58,*(undefined4 *)(local_14[3] + 8));
            if (iVar2 != 0) {
              iVar3 = FUN_004636b0(local_58,1,0,0x50,0);
            }
            local_40 = 0;
            ppuVar9 = &PTR_s_zoNONE_0051c630;
            do {
              iVar2 = FUN_004f2b70(*ppuVar9,iVar3);
              if (iVar2 != 0) {
                FUN_00463650(&local_28,s____t____A_Za_z0_9_______t________0051e1c8);
                local_8 = CONCAT31(local_8._1_3_,1);
                iVar3 = FUN_00463680(&local_28,*(undefined4 *)(local_14[3] + 8));
                if (iVar3 == 0) {
                  *piVar10 = *(int *)(&DAT_0051c634 + local_40 * 8);
                }
                else {
                  pcVar11 = (char *)FUN_004636b0(&local_28,2,0,0x50,0);
                  if (*pcVar11 == '+') {
                    pcVar11 = (char *)FUN_004636b0(&local_28,3,0,0x50,0);
                    iVar3 = _atoi(pcVar11);
                    *piVar10 = *(int *)(&DAT_0051c634 + local_40 * 8) + iVar3;
                  }
                  else {
                    pcVar11 = (char *)FUN_004636b0(&local_28,2,0,0x50,0);
                    if (*pcVar11 == '-') {
                      iVar3 = *(int *)(&DAT_0051c634 + local_40 * 8);
                      pcVar11 = (char *)FUN_004636b0(&local_28,3,0,0x50,0);
                      iVar2 = _atoi(pcVar11);
                      *piVar10 = iVar3 - iVar2;
                    }
                  }
                }
                local_8 = local_8 & 0xffffff00;
                FUN_00448324();
                break;
              }
              ppuVar9 = ppuVar9 + 2;
              local_40 = local_40 + 1;
            } while (ppuVar9 < &DAT_0051c6e8);
            if (0x16 < local_40) {
              FUN_00490040(s_zo<COUNTOF(zOrderDefs)_0051e20c,s__ns_o_RiftType_cpp_0051e1f8,0x6ab);
            }
            local_8 = 0xffffffff;
            FUN_00448336();
            piVar10 = local_38;
            goto joined_r0x00446dcc;
          }
          uVar4 = 0x6ae;
          pcVar13 = s__ns_o_RiftType_cpp_0051e224;
          pcVar11 = &DAT_0051e238;
        }
        else {
          iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_foot_x_0051e23c);
          if ((iVar3 == 0) ||
             (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_footx_0051e244), iVar3 == 0))
          {
            if (*(int *)local_14[3] != 2) {
              FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e260,s__ns_o_RiftType_cpp_0051e24c
                           ,0x6ba);
            }
            iVar2 = __ftol();
            iVar3 = DAT_0052e9bc;
            *(int *)(local_18 + 0x1b4) = iVar2;
            piVar10 = local_38;
            if (iVar2 <= iVar3) goto joined_r0x00446dcc;
            uVar4 = 0x6bc;
            pcVar13 = s__ns_o_RiftType_cpp_0051e288;
            pcVar11 = s_ts_>footX_<__maxFootpadInTilesX_0051e29c;
          }
          else {
            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_foot_y_0051e2bc);
            if ((iVar3 == 0) ||
               (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_footy_0051e2c4), iVar3 == 0)
               ) {
              if (*(int *)local_14[3] != 2) {
                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e2e0,
                             s__ns_o_RiftType_cpp_0051e2cc,0x6c1);
              }
              iVar2 = __ftol();
              iVar3 = DAT_0052e9c0;
              *(int *)(local_18 + 0x1b8) = iVar2;
              piVar10 = local_38;
              if (iVar2 <= iVar3) goto joined_r0x00446dcc;
              uVar4 = 0x6c3;
              pcVar13 = s__ns_o_RiftType_cpp_0051e308;
              pcVar11 = s_ts_>footY_<__maxFootpadInTilesY_0051e31c;
            }
            else {
              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_height_0051e33c);
              if (iVar3 == 0) {
                uVar4 = __ftol();
                *(undefined4 *)(local_18 + 0x1c4) = uVar4;
                piVar10 = local_38;
                goto joined_r0x00446dcc;
              }
              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_hpPerSec_0051e344);
              if (iVar3 == 0) {
                if (*(int *)local_14[3] != 2) {
                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e364,
                               s__ns_o_RiftType_cpp_0051e350,0x6cb);
                }
                *(undefined4 *)(local_18 + 0x48) = *(undefined4 *)(local_14[3] + 8);
                piVar10 = local_38;
                goto joined_r0x00446dcc;
              }
              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rangeVar_0051e38c);
              if ((iVar3 == 0) ||
                 (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rangeConst_0051e398),
                 iVar3 == 0)) {
                uVar4 = 0x6d1;
                pcVar13 = s__ns_o_RiftType_cpp_0051e3a4;
                pcVar11 = s_false_0051e3b8;
              }
              else {
                iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_range_0051e3c0);
                if (iVar3 == 0) {
                  *(undefined4 *)(local_18 + 0x44) = *(undefined4 *)(local_14[3] + 8);
                  piVar10 = local_38;
                  goto joined_r0x00446dcc;
                }
                iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_delayBetweenShots_0051e3c8);
                if (iVar3 == 0) {
                  *(undefined4 *)(local_18 + 0x4c) = *(undefined4 *)(local_14[3] + 8);
                  piVar10 = local_38;
                  goto joined_r0x00446dcc;
                }
                iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_attackValueConst_0051e3dc);
                if (iVar3 == 0) {
                  uVar4 = 0x6dd;
                  pcVar13 = s__ns_o_RiftType_cpp_0051e3f0;
                  pcVar11 = s_false_0051e404;
                }
                else {
                  iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_attackValueVar_0051e40c);
                  if (iVar3 == 0) {
                    uVar4 = 0x6e1;
                    pcVar13 = s__ns_o_RiftType_cpp_0051e41c;
                    pcVar11 = s_false_0051e430;
                  }
                  else {
                    iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rateConst_0051e438);
                    if (iVar3 == 0) {
                      uVar4 = 0x6e5;
                      pcVar13 = s__ns_o_RiftType_cpp_0051e444;
                      pcVar11 = s_false_0051e458;
                    }
                    else {
                      iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rateVar_0051e460);
                      if (iVar3 == 0) {
                        uVar4 = 0x6e9;
                        pcVar13 = s__ns_o_RiftType_cpp_0051e468;
                        pcVar11 = s_false_0051e47c;
                      }
                      else {
                        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                             s_hotFootRatioX_0051e484);
                        if (iVar3 == 0) {
                          if (*(int *)local_14[3] != 2) {
                            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e4a8,
                                         s__ns_o_RiftType_cpp_0051e494,0x6ed);
                          }
                          uVar4 = __ftol();
                          *(undefined4 *)(local_18 + 0x1bc) = uVar4;
                          piVar10 = local_38;
                          goto joined_r0x00446dcc;
                        }
                        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                             s_hotFootRatioY_0051e4d0);
                        if (iVar3 == 0) {
                          if (*(int *)local_14[3] != 2) {
                            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e4f4,
                                         s__ns_o_RiftType_cpp_0051e4e0,0x6f2);
                          }
                          uVar4 = __ftol();
                          *(undefined4 *)(local_18 + 0x1c0) = uVar4;
                          piVar10 = local_38;
                          goto joined_r0x00446dcc;
                        }
                        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_hotFootX_0051e51c);
                        if (iVar3 == 0) {
                          uVar4 = 0x6f7;
                          pcVar13 = s__ns_o_RiftType_cpp_0051e528;
                          pcVar11 = s_false_0051e53c;
                        }
                        else {
                          iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_hotFootY_0051e544)
                          ;
                          if (iVar3 == 0) {
                            uVar4 = 0x6fb;
                            pcVar13 = s__ns_o_RiftType_cpp_0051e550;
                            pcVar11 = s_false_0051e564;
                          }
                          else {
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_priority_0051e56c);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e58c,
                                             s__ns_o_RiftType_cpp_0051e578,0x6ff);
                              }
                              uVar4 = __ftol();
                              *(undefined4 *)(local_18 + 0xd8) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_constructionRate_0051e5b4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e5dc,
                                             s__ns_o_RiftType_cpp_0051e5c8,0x704);
                              }
                              *(undefined4 *)(local_18 + 0x50) = *(undefined4 *)(local_14[3] + 8);
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_orientations_0051e604);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e628,
                                             s__ns_o_RiftType_cpp_0051e614,0x709);
                              }
                              uVar4 = __ftol();
                              *(undefined4 *)(local_18 + 0x110) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_researchDepend_0051e650);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e674,
                                             s__ns_o_RiftType_cpp_0051e660,0x70f);
                              }
                              FUN_00444d20(local_18,*(undefined4 *)(local_14[3] + 8));
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_activeSound_0051e6a4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e6c4,
                                             s__ns_o_RiftType_cpp_0051e6b0,0x716);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x6c) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_fireSound_0051e6f4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e714,
                                             s__ns_o_RiftType_cpp_0051e700,0x71e);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x70) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_impactSound_0051e744);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e764,
                                             s__ns_o_RiftType_cpp_0051e750,0x725);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x74) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_moveSound_0051e794);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e7b4,
                                             s__ns_o_RiftType_cpp_0051e7a0,0x72b);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x78) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_pickupSound_0051e7e4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e804,
                                             s__ns_o_RiftType_cpp_0051e7f0,0x732);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x7c) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_dropSound_0051e834);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e854,
                                             s__ns_o_RiftType_cpp_0051e840,0x739);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x80) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_fallSound_0051e884);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e8a4,
                                             s__ns_o_RiftType_cpp_0051e890,0x740);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x84) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_miscSound0_0051e8d4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e8f4,
                                             s__ns_o_RiftType_cpp_0051e8e0,0x747);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x88) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_miscSound1_0051e924);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e944,
                                             s__ns_o_RiftType_cpp_0051e930,0x74e);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x88) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_buildDoneSound_0051e974);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e998,
                                             s__ns_o_RiftType_cpp_0051e984,0x755);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x90) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_damageEffect_0051e9c8);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e9ec,
                                             s__ns_o_RiftType_cpp_0051e9d8,0x75c);
                              }
                              uVar4 = __ftol();
                              *(undefined4 *)(local_18 + 0xd4) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_speed_0051ea14);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ea30,
                                             s__ns_o_RiftType_cpp_0051ea1c,0x761);
                              }
                              *(undefined4 *)(local_18 + 0xe0) = *(undefined4 *)(local_14[3] + 8);
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_turningSpeed_0051ea58);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ea7c,
                                             s__ns_o_RiftType_cpp_0051ea68,0x766);
                              }
                              *(undefined4 *)(local_18 + 0xe4) = *(undefined4 *)(local_14[3] + 8);
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_theme_0051eaa4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051eac0,
                                             s__ns_o_RiftType_cpp_0051eaac,0x76b);
                              }
                              *(undefined4 *)(local_18 + 0x98) =
                                   (&DAT_00530278)[**(char **)(local_14[3] + 8)];
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_level_0051eaf0);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051eb0c,
                                             s__ns_o_RiftType_cpp_0051eaf8,0x770);
                              }
                              iVar3 = __ftol();
                              *(int *)(local_18 + 0x94) = iVar3 + -1;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),&DAT_0051eb34);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051eb50,
                                             s__ns_o_RiftType_cpp_0051eb3c,0x776);
                              }
                              *(undefined4 *)(local_18 + 0xc4) = *(undefined4 *)(local_14[3] + 8);
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_group_0051eb78);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051eb94,
                                             s__ns_o_RiftType_cpp_0051eb80,0x77b);
                              }
                              iVar3 = FUN_00420500(local_14[3],*(undefined4 *)(local_14[3] + 8));
                              *(int *)(local_18 + 0x9c) = iVar3;
                              piVar10 = local_38;
                              if (iVar3 != 10) goto joined_r0x00446dcc;
                              uVar4 = 0x77d;
                              pcVar13 = s__ns_o_RiftType_cpp_0051ebc4;
                              pcVar11 = s_ts_>group____NO_GROUP_0051ebd8;
                            }
                            else {
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),&DAT_0051ebf0);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 10) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ec0c,
                                               s__ns_o_RiftType_cpp_0051ebf8,0x781);
                                }
                                uVar7 = 0xffffffff;
                                pcVar11 = *(char **)(local_14[3] + 8);
                                do {
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                } while (cVar1 != '\0');
                                if (9 < ~uVar7 - 1) {
                                  FUN_00490040(s_strlen_attributeValue_>p2_>strin_0051ec50,
                                               s__ns_o_RiftType_cpp_0051ec3c,0x782);
                                }
                                uVar7 = 0xffffffff;
                                pcVar11 = *(char **)(local_14[3] + 8);
                                do {
                                  pcVar13 = pcVar11;
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  pcVar13 = pcVar11 + 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar13;
                                } while (cVar1 != '\0');
                                uVar7 = ~uVar7;
                                pcVar11 = pcVar13 + -uVar7;
                                pcVar13 = local_18 + 0xa0;
                                for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
                                  *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
                                  pcVar11 = pcVar11 + 4;
                                  pcVar13 = pcVar13 + 4;
                                }
                                for (uVar7 = uVar7 & 3; piVar10 = local_38, uVar7 != 0;
                                    uVar7 = uVar7 - 1) {
                                  *pcVar13 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                  pcVar13 = pcVar13 + 1;
                                }
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_class_0051ec78
                                                  );
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 10) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ec94,
                                               s__ns_o_RiftType_cpp_0051ec80,0x787);
                                }
                                uVar7 = 0xffffffff;
                                pcVar11 = *(char **)(local_14[3] + 8);
                                do {
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                } while (cVar1 != '\0');
                                if (0x17 < ~uVar7 - 1) {
                                  FUN_00490040(s_strlen_attributeValue_>p2_>strin_0051ecd8,
                                               s__ns_o_RiftType_cpp_0051ecc4,0x788);
                                }
                                uVar7 = 0xffffffff;
                                pcVar11 = *(char **)(local_14[3] + 8);
                                do {
                                  pcVar13 = pcVar11;
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  pcVar13 = pcVar11 + 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar13;
                                } while (cVar1 != '\0');
                                uVar7 = ~uVar7;
                                pcVar11 = pcVar13 + -uVar7;
                                pcVar13 = local_18 + 0xaa;
                                for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
                                  *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
                                  pcVar11 = pcVar11 + 4;
                                  pcVar13 = pcVar13 + 4;
                                }
                                for (uVar7 = uVar7 & 3; piVar10 = local_38, uVar7 != 0;
                                    uVar7 = uVar7 - 1) {
                                  *pcVar13 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                  pcVar13 = pcVar13 + 1;
                                }
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                   s_artifactChanceClass_0051ed14);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 2) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ed3c,
                                               s__ns_o_RiftType_cpp_0051ed28,0x78d);
                                }
                                uVar4 = __ftol();
                                *(undefined4 *)(local_18 + 0xcc) = uVar4;
                                piVar10 = local_38;
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                   s_artifactFrame_0051ed64);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 2) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ed88,
                                               s__ns_o_RiftType_cpp_0051ed74,0x792);
                                }
                                uVar4 = __ftol();
                                *(undefined4 *)(local_18 + 0xd0) = uVar4;
                                piVar10 = local_38;
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                   s_techBit_0051edb0);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 2) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051edcc,
                                               s__ns_o_RiftType_cpp_0051edb8,0x797);
                                }
                                uVar4 = __ftol();
                                *(undefined4 *)(local_18 + 0x1cc) = uVar4;
                                piVar10 = local_38;
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                   s_description_0051edf4);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 10) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ee14,
                                               s__ns_o_RiftType_cpp_0051ee00,0x79c);
                                }
                                FUN_004f2440(local_18 + 0x18,*(undefined4 *)(local_14[3] + 8),0x28);
                                piVar10 = local_38;
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),&DAT_0051ee44);
                              piVar10 = local_38;
                              if ((((iVar3 == 0) ||
                                   (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                         s_basecrew_0051ee4c), piVar10 = local_38,
                                   iVar3 == 0)) ||
                                  (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                        s_joyfactor_0051ee58), piVar10 = local_38,
                                  iVar3 == 0)) ||
                                 (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                       s_foodproduced_0051ee64), piVar10 = local_38,
                                 iVar3 == 0)) goto joined_r0x00446dcc;
                              uVar4 = 0x7aa;
                              pcVar13 = s__ns_o_RiftType_cpp_0051ee74;
                              pcVar11 = &DAT_0051ee88;
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
LAB_00448252:
  FUN_00490040(pcVar11,pcVar13,uVar4);
  piVar10 = local_38;
  goto joined_r0x00446dcc;
}


// ==== FUN_00445ff0 @ 00445ff0

void __cdecl FUN_00445ff0(void)

{
  int *piVar1;
  int *piVar2;
  
  piVar1 = DAT_0051cc1c;
  piVar2 = DAT_0051cc18;
  if (DAT_0051cc18 < DAT_0051cc1c) {
    do {
      if (((*piVar2 == 10) || (*piVar2 == 1)) && (piVar2[2] != 0)) {
        FUN_004f1a50(piVar2[2]);
        piVar1 = DAT_0051cc1c;
      }
      piVar2 = piVar2 + 4;
    } while (piVar2 < piVar1);
  }
  DAT_0051cc1c = DAT_0051cc18;
  return;
}


// ==== FUN_00444e10 @ 00444e10

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00444e10(void *this)

{
  float fVar1;
  undefined *puVar2;
  char cVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  char *pcVar7;
  char *pcVar8;
  int iVar9;
  float10 fVar10;
  
  iVar4 = ((int)this - DAT_0051c960) / 0x1d4;
  if (DAT_0051c964 == iVar4) {
    uVar5 = 0xffffffff;
    pcVar7 = s_DependForm_0051d934;
    do {
      pcVar8 = pcVar7;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      pcVar8 = pcVar7 + 1;
      cVar3 = *pcVar7;
      pcVar7 = pcVar8;
    } while (cVar3 != '\0');
    uVar5 = ~uVar5;
    pcVar8 = pcVar8 + -uVar5;
    pcVar7 = this;
    for (uVar6 = uVar5 >> 2; pcVar7 = pcVar7 + 4, uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pcVar7 = *(undefined4 *)pcVar8;
      pcVar8 = pcVar8 + 4;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *pcVar7 = *pcVar8;
      pcVar8 = pcVar8 + 1;
      pcVar7 = pcVar7 + 1;
    }
  }
  if (DAT_0051c968 == iVar4) {
    uVar5 = 0xffffffff;
    pcVar7 = s_ProcessForm_0051d940;
    do {
      pcVar8 = pcVar7;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      pcVar8 = pcVar7 + 1;
      cVar3 = *pcVar7;
      pcVar7 = pcVar8;
    } while (cVar3 != '\0');
    uVar5 = ~uVar5;
    pcVar8 = pcVar8 + -uVar5;
    pcVar7 = this;
    for (uVar6 = uVar5 >> 2; pcVar7 = pcVar7 + 4, uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pcVar7 = *(undefined4 *)pcVar8;
      pcVar8 = pcVar8 + 4;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *pcVar7 = *pcVar8;
      pcVar8 = pcVar8 + 1;
      pcVar7 = pcVar7 + 1;
    }
  }
  if (DAT_0051c96c == iVar4) {
    uVar5 = 0xffffffff;
    pcVar7 = s_GumpForm_0051d94c;
    do {
      pcVar8 = pcVar7;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      pcVar8 = pcVar7 + 1;
      cVar3 = *pcVar7;
      pcVar7 = pcVar8;
    } while (cVar3 != '\0');
    uVar5 = ~uVar5;
    pcVar8 = pcVar8 + -uVar5;
    pcVar7 = this;
    for (uVar6 = uVar5 >> 2; pcVar7 = pcVar7 + 4, uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pcVar7 = *(undefined4 *)pcVar8;
      pcVar8 = pcVar8 + 4;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *pcVar7 = *pcVar8;
      pcVar8 = pcVar8 + 1;
      pcVar7 = pcVar7 + 1;
    }
  }
  if (DAT_0051c970 == iVar4) {
    uVar5 = 0xffffffff;
    pcVar7 = s_PlayerSquid_0051d958;
    do {
      pcVar8 = pcVar7;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      pcVar8 = pcVar7 + 1;
      cVar3 = *pcVar7;
      pcVar7 = pcVar8;
    } while (cVar3 != '\0');
    uVar5 = ~uVar5;
    pcVar8 = pcVar8 + -uVar5;
    pcVar7 = this;
    for (uVar6 = uVar5 >> 2; pcVar7 = pcVar7 + 4, uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pcVar7 = *(undefined4 *)pcVar8;
      pcVar8 = pcVar8 + 4;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *pcVar7 = *pcVar8;
      pcVar8 = pcVar8 + 1;
      pcVar7 = pcVar7 + 1;
    }
  }
  if (DAT_0051c978 == iVar4) {
    uVar5 = 0xffffffff;
    pcVar7 = s_ContentForm_0051d964;
    do {
      pcVar8 = pcVar7;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      pcVar8 = pcVar7 + 1;
      cVar3 = *pcVar7;
      pcVar7 = pcVar8;
    } while (cVar3 != '\0');
    uVar5 = ~uVar5;
    pcVar8 = pcVar8 + -uVar5;
    pcVar7 = this;
    for (uVar6 = uVar5 >> 2; pcVar7 = pcVar7 + 4, uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pcVar7 = *(undefined4 *)pcVar8;
      pcVar8 = pcVar8 + 4;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *pcVar7 = *pcVar8;
      pcVar8 = pcVar8 + 1;
      pcVar7 = pcVar7 + 1;
    }
  }
  if (iVar4 == DAT_0051cb2c) {
    _DAT_00512120 = __ftol();
  }
  iVar9 = *(int *)((int)this + 0x9c);
  if (iVar9 == 5) {
    *(uint *)((int)this + 0xec) = *(uint *)((int)this + 0xec) | 0x800;
  }
  if ((iVar9 == 1) || (iVar9 == 0)) {
    *(uint *)((int)this + 0xec) = *(uint *)((int)this + 0xec) | 0x8000000;
  }
  if (iVar9 == 2) {
    *(uint *)((int)this + 0xec) = *(uint *)((int)this + 0xec) | 0x4000000;
  }
  uVar5 = *(uint *)((int)this + 0xec);
  if ((uVar5 & 0x34200) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x20000;
    if ((uVar5 & 0x30000) == 0) {
      *(uint *)((int)this + 0xfc) = *(uint *)((int)this + 0xfc) | 0x10;
    }
    else {
      *(uint *)((int)this + 0xfc) = *(uint *)((int)this + 0xfc) | 0x20;
    }
  }
  if ((uVar5 & 0x100) != 0) {
    *(uint *)((int)this + 0x100) = *(uint *)((int)this + 0x100) | 1;
  }
  if ((uVar5 & 0x2000) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x1000;
  }
  if ((uVar5 & 0xa00) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x10000000;
  }
  if ((uVar5 & 0x4000) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x4000000;
  }
  if ((uVar5 & 0x200) != 0) {
    uVar5 = *(uint *)((int)this + 0xe8);
    *(uint *)((int)this + 0xe8) = uVar5 | 0x4000;
    *(uint *)((int)this + 0xe8) = uVar5 | 0x84000;
  }
  iVar9 = 0;
  do {
    fVar10 = (float10)FUN_004448c0(this,iVar9);
    if ((fVar10 != (float10)_DAT_00501988) ||
       (fVar10 = (float10)FUN_004448d0(this,iVar9), fVar10 != (float10)_DAT_00501988)) {
      *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x4000;
    }
    iVar9 = iVar9 + 1;
  } while (iVar9 < 3);
  if (((*(uint *)((int)this + 0xe8) & 0x1000) != 0) && ((*(uint *)((int)this + 0xe8) & 0x2000) != 0)
     ) {
    FUN_00490040(s__typeFlags___TYPE_FLAG_SAVE_Q16__0051d984,s__ns_o_RiftType_cpp_0051d970,0x2cf);
  }
  if ((*(uint *)((int)this + 0xe8) & 4) == 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 2;
  }
  if ((*(uint *)((int)this + 0xec) & 0x40000) != 0) {
    *(uint *)((int)this + 0xec) = *(uint *)((int)this + 0xec) | 0x10;
  }
  uVar5 = *(uint *)((int)this + 0xec);
  if (((((uVar5 & 0x40000) != 0) && ((uVar5 & 0x4200) == 0)) && (*(int *)((int)this + 0x1b4) == 3))
     && (*(int *)((int)this + 0x1b8) == 3)) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 6;
  }
  if ((uVar5 & 0x408100) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 2;
  }
  if (((uVar5 & 0x70000) != 0) && ((uVar5 & 0x4000) == 0)) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x8000;
  }
  if ((*(uint *)((int)this + 0x50) & 0x7fffffff) == 0) {
    *(undefined4 *)((int)this + 0x50) = 0x41200000;
  }
  if (*(int *)((int)this + 0x9c) != 10) {
    if ((*(uint *)((int)this + 0xc4) & 0x7fffffff) == 0) {
      if ((uVar5 & 0x30000) == 0) {
        fVar1 = (float)(*(int *)((int)this + 0x94) + 1) * _DAT_00501998;
      }
      else {
        fVar1 = (float)(*(int *)((int)this + 0x94) + 1) * _DAT_00501994;
      }
      *(float *)((int)this + 0xc4) = fVar1;
    }
    puVar2 = PTR_DAT_00530270;
    if (((uVar5 & 0x200000) == 0) && (pcVar7 = (char *)((int)this + 0xa0), *pcVar7 == '\0')) {
      iVar9 = *(int *)((int)this + 0x94);
      if (-1 < iVar9) {
        *pcVar7 = PTR_DAT_00530270[*(int *)((int)this + 0x98)];
        pcVar7 = (char *)((int)this + 0xa1);
      }
      if (iVar9 == 1) {
        *pcVar7 = puVar2[3];
        pcVar7 = pcVar7 + 1;
      }
      pcVar8 = pcVar7;
      if (iVar9 == 2) {
        pcVar8 = pcVar7 + 2;
        cVar3 = puVar2[3];
        *pcVar7 = puVar2[*(int *)((int)this + 0x98)];
        pcVar7[1] = cVar3;
      }
      *pcVar8 = '\0';
    }
  }
  cVar3 = *(char *)((int)this + 0xa0);
  pcVar7 = (char *)((int)this + 0xa0);
  while (cVar3 != '\0') {
    cVar3 = FUN_004f2a30((int)*pcVar7);
    *pcVar7 = cVar3;
    cVar3 = pcVar7[1];
    pcVar7 = pcVar7 + 1;
  }
  FUN_00444da0(this);
  if (*(int *)((int)this + 0x1b8) == 6) {
    *(undefined4 *)((int)this + 0x1b8) = 8;
  }
  if (*(int *)((int)this + 0x108) == 0) {
    *(undefined4 *)((int)this + 0x10c) = *(undefined4 *)((int)this + 0x104);
  }
  if (iVar4 == DAT_0051cbbc) {
    *(undefined4 *)((int)this + 0xc4) = 0;
  }
  if ((*(byte *)((int)this + 0xed) & 4) != 0) {
    *(undefined4 *)((int)this + 0xc4) = 0;
  }
  return;
}


// ==== FUN_00444da0 @ 00444da0

void __thiscall FUN_00444da0(void *this)

{
  int *piVar1;
  int iVar2;
  char *pcVar3;
  int iVar4;
  
  iVar2 = 0;
  piVar1 = (int *)((int)this + 0x130);
  *(undefined4 *)((int)this + 300) = 0;
  do {
    iVar4 = 0;
    *piVar1 = -1;
    piVar1[0x10] = 0;
    pcVar3 = *(char **)((int)this + 0x124);
    if (0 < *(int *)((int)this + 0x114)) {
      do {
        if (*pcVar3 - iVar2 == 0x41) {
          if (*piVar1 == -1) {
            *(int *)((int)this + 300) = *(int *)((int)this + 300) + 1;
            *piVar1 = iVar4;
          }
          piVar1[0x10] = piVar1[0x10] + 1;
        }
        iVar4 = iVar4 + 1;
        pcVar3 = pcVar3 + 4;
      } while (iVar4 < *(int *)((int)this + 0x114));
    }
    piVar1 = piVar1 + 1;
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x10);
  return;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __cdecl __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_004f2980 @ 004f2980

int FUN_004f2980(byte *param_1,byte *param_2)

{
  uint uVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  char cVar5;
  int _C_00;
  int _C;
  
  if (DAT_005473e8 == 0) {
    bVar4 = 0xff;
    do {
      do {
        cVar5 = '\0';
        if (bVar4 == 0) goto LAB_004f29ce;
        bVar4 = *param_2;
        param_2 = param_2 + 1;
        bVar3 = *param_1;
        param_1 = param_1 + 1;
      } while (bVar3 == bVar4);
      bVar2 = bVar4 + 0xbf + (-((byte)(bVar4 + 0xbf) < 0x1a) & 0x20U) + 0x41;
      bVar3 = bVar3 + 0xbf;
      bVar4 = bVar3 + (-(bVar3 < 0x1a) & 0x20U) + 0x41;
    } while (bVar4 == bVar2);
    cVar5 = (bVar4 < bVar2) * -2 + '\x01';
LAB_004f29ce:
    _C = (int)cVar5;
  }
  else {
    __lock(0x15);
    _C = 0xff;
    _C_00 = 0;
    do {
      do {
        if ((char)_C == '\0') goto LAB_004f2a0f;
        bVar4 = *param_2;
        _C = CONCAT31((int3)((uint)_C >> 8),bVar4);
        param_2 = param_2 + 1;
        bVar3 = *param_1;
        _C_00 = CONCAT31((int3)((uint)_C_00 >> 8),bVar3);
        param_1 = param_1 + 1;
      } while (bVar4 == bVar3);
      _C_00 = FID_conflict__tolower(_C_00);
      _C = FID_conflict__tolower(_C);
    } while ((byte)_C_00 == (byte)_C);
    uVar1 = (uint)((byte)_C_00 < (byte)_C);
    _C = (1 - uVar1) - (uint)(uVar1 != 0);
LAB_004f2a0f:
    FUN_004f61c0(0x15);
  }
  return _C;
}


// ==== FUN_00420500 @ 00420500

void __thiscall FUN_00420500(void *this,undefined4 param_2)

{
  FUN_004204a0(this,&PTR_s_archer_005168e0,param_2);
  return;
}


// ==== FUN_004f2a30 @ 004f2a30

int __cdecl FUN_004f2a30(int param_1)

{
  if (DAT_005473e8 == 0) {
    if ((0x40 < param_1) && (param_1 < 0x5b)) {
      return param_1 + 0x20;
    }
  }
  else {
    __lock(0x15);
    param_1 = FID_conflict__tolower(param_1);
    FUN_004f61c0(0x15);
  }
  return param_1;
}


// ==== FUN_004448c0 @ 004448c0

float10 __thiscall FUN_004448c0(void *this,int param_2)

{
  return (float10)*(float *)((int)this + param_2 * 4 + 0x54);
}


// ==== FUN_004448d0 @ 004448d0

float10 __thiscall FUN_004448d0(void *this,int param_2)

{
  return (float10)*(float *)((int)this + param_2 * 4 + 0x60);
}


// ==== FUN_004342f0 @ 004342f0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004342f0(void *this)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  uint uVar6;
  float10 fVar7;
  char *pcStack_78;
  uint uStack_74;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined8 local_50;
  int local_1c;
  undefined4 local_18;
  undefined4 local_14;
  char local_10 [16];
  
  uStack_74 = 0x4342fe;
  iVar1 = FUN_0048f870();
  uStack_74 = 0x434307;
  iVar2 = FUN_004ac420(iVar1);
  if (iVar2 != *(int *)((int)this + -8)) {
    uStack_74 = 0x66;
    pcStack_78 = s__ns_o_GunProcess_cpp_0051a334;
    FUN_00490040(s_getGroup______parent_getGroup___0051a34c);
  }
  if ((*(byte *)(iVar1 + 0x23) & 1) != 0) {
    uStack_74 = 0x67;
    pcStack_78 = s__ns_o_GunProcess_cpp_0051a36c;
    FUN_00490040(s__parent_isAbstract___0051a384);
  }
  if ((*(byte *)(iVar1 + 0x23) & 8) != 0) {
    uStack_74 = 0x68;
    pcStack_78 = s__ns_o_GunProcess_cpp_0051a39c;
    FUN_00490040(s__parent_isAstral___0051a3b4);
  }
  if ((((*(byte *)(iVar1 + 0x23) & 0x20) == 0) && (DAT_00518904 == 0)) && (DAT_00511c8c == 0)) {
    uStack_74 = 0x434380;
    FUN_004348f0();
    uVar6 = (uint)*(ushort *)((int)this + -0x10) * 0x24 + DAT_005395dc;
    if ((DAT_00540a2c != 0) &&
       (((uVar6 < DAT_005395dc + 0x24U || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar6)) ||
        (((*(byte *)(uVar6 + 0xb) & 3) != 0 ||
         (((&DAT_0050f6e0)[(uint)*(byte *)(iVar1 + 0x20) * 9 + (uint)*(byte *)(uVar6 + 0x20)] != 0
          && (DAT_00537ea0 == 0)))))))) {
      uStack_74 = 0;
      pcStack_78 = (char *)0x4343ea;
      FUN_00434720((int)this + -0x28);
    }
    uVar6 = (uint)*(byte *)(iVar1 + 10);
    if ((*(double *)((int)this + -0x18) <= _DAT_005484a8) && ((*(byte *)(iVar1 + 0x23) & 1) == 0)) {
      uVar4 = *(undefined4 *)((int)this + -0xc);
      switch(uVar4) {
      case 0:
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
        *(undefined4 *)((int)this + -0xc) = 1;
        return;
      case 1:
        if ((DAT_00540a28 != 0) && ((DAT_00540a1c == 0 || (DAT_00537ea0 != 0)))) {
          local_50 = *(double *)(iVar1 + 0xe);
          uStack_74 = (uint)*(byte *)(iVar1 + 0x20);
          pcStack_78 = (char *)0x4344aa;
          pcStack_78 = (char *)FUN_00443110(*(int *)((int)this + 8) * 0x24 + DAT_005395dc);
          iVar2 = FUN_00435e10((undefined4)local_50,local_50._4_4_,uVar6);
          if (iVar2 == 0) {
            local_50 = (double)CONCAT44(local_50._4_4_,&pcStack_78);
            FUN_00436e50(&pcStack_78,0);
            FUN_00436e90((int)this + -0x28);
            uStack_74 = 0x3e4ccccd;
            pcStack_78 = (char *)0x0;
            fVar7 = (float10)FUN_0048cc50();
            local_50 = (double)(fVar7 + (float10)_DAT_005010a8 + (float10)_DAT_005484a8);
            *(double *)((int)this + -0x18) = local_50;
          }
          else {
            pcStack_78 = (char *)0x4344cb;
            uStack_74 = iVar2;
            FUN_00434290((int)this + -0x28);
          }
        }
        if (DAT_00531454 != 0) {
          pcStack_78 = local_10;
          uStack_74 = 0;
          puVar3 = (undefined4 *)FUN_004aeec0(iVar1);
          local_60 = *puVar3;
          local_5c = puVar3[1];
          local_58 = puVar3[2];
          local_54 = puVar3[3];
          local_50 = (double)CONCAT44(local_50._4_4_,&PTR_FUN_005003d0);
          local_18 = 0;
          local_14 = 0;
          uStack_74 = 0x434560;
          FUN_00440a50(&local_60);
          uStack_74 = 0x434569;
          FUN_00440a80(&local_58);
          uStack_74 = 0x434572;
          uStack_74 = __ftol();
          pcStack_78 = (char *)0x43457c;
          pcStack_78 = (char *)__ftol();
          uVar4 = __ftol();
          uVar5 = __ftol(uVar4);
          FUN_004eae20(&local_50,uVar5,uVar4);
          if (local_1c != 0) {
            do {
              uStack_74 = 0x4345ad;
              FUN_004eafe0(&local_50);
            } while (local_1c != 0);
            return;
          }
        }
        break;
      case 2:
        if (*(short *)((int)this + -0x10) == 0) {
          *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
          *(undefined4 *)((int)this + -0xc) = 1;
          return;
        }
        uStack_74 = 0x4345dc;
        iVar1 = FUN_004ac420(iVar1);
        if ((((iVar1 != 0) || (uVar6 == DAT_0051cb74)) && (uVar6 != DAT_0051cb50)) &&
           (uVar6 != DAT_0051cb48)) {
          pcStack_78 = *(char **)((int)this + 4);
          uStack_74 = (uint)*(ushort *)((int)this + -0x10);
          FUN_004a18f0(*(undefined4 *)((int)this + -0xc));
          *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
          *(undefined4 *)((int)this + -0xc) = 3;
          return;
        }
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
        *(undefined4 *)((int)this + -0xc) = 4;
        return;
      case 3:
      case 6:
      case 9:
        break;
      case 4:
        if (*(short *)((int)this + -0x10) != 0) {
          *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
          *(undefined4 *)((int)this + -0xc) = 5;
          return;
        }
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
        *(undefined4 *)((int)this + -0xc) = 1;
        return;
      case 5:
        if (*(ushort *)((int)this + -0x10) != 0) {
          uStack_74 = (uint)*(ushort *)((int)this + -0x10);
          pcStack_78 = *(char **)((int)this + 4);
          FUN_004a18f0(uVar4);
          *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
          *(undefined4 *)((int)this + -0xc) = 6;
          return;
        }
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
        *(undefined4 *)((int)this + -0xc) = 1;
        return;
      case 7:
        uStack_74 = 0x4346ab;
        (**(code **)(*(int *)((int)this + -0x28) + 4))();
        return;
      case 8:
        uStack_74 = 0;
        pcStack_78 = *(char **)((int)this + 4);
        FUN_004a18f0(uVar4);
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
        *(undefined4 *)((int)this + -0xc) = 9;
        return;
      case 10:
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
        *(undefined4 *)((int)this + -0xc) = 1;
        break;
      default:
        uStack_74 = 0xed;
        pcStack_78 = s__ns_o_GunProcess_cpp_0051a3c8;
        FUN_00490040(s_false_0051a3e0);
        return;
      }
    }
  }
  return;
}


