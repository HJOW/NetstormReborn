// ==== FUN_0049ebb0 @ 0049ebb0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_0049ebb0(void)

{
  float fVar1;
  float fVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  float *pfVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  undefined **ppuVar9;
  char local_100 [256];
  
  uVar3 = FUN_00459c60(0xfffffffe,PTR_DAT_00540cec,0xfffffffe,PTR_DAT_00540cf8,0xfffffffd,
                       "_shapes.shp",0,0);
  iVar4 = FUN_0041ad10(uVar3);
  if (iVar4 == 0) {
    FUN_004e0620("filename","\\Ns\\O\\Rifttype.cpp",0xa8);
  }
  DAT_0059ab24 = FUN_004dd580(iVar4);
  if (DAT_0059ab24 == 0) {
    FUN_004e0620("masterShapeDatabasePtr","\\Ns\\O\\Rifttype.cpp",0xaa);
  }
  iVar4 = DAT_00541348;
  DAT_0059ab20 = operator_new(DAT_00541348 * 500);
  if (DAT_0059ab20 == (void *)0x0) {
    DAT_0059ab20 = (void *)0x0;
  }
  else if (-1 < iVar4 + -1) {
    puVar7 = (undefined4 *)((int)DAT_0059ab20 + 0x9c);
    do {
      puVar8 = puVar7 + -0x27;
      for (iVar5 = 0x7d; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      puVar7[-1] = 5;
      *puVar7 = 10;
      puVar7[0x21] = 0xffffffff;
      puVar7[0x23] = 0xffffffff;
      puVar7[0x54] = 0xffffffff;
      puVar7 = puVar7 + 0x7d;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  FUN_0049c2e0();
  *(code **)(DAT_00541078 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004ae3d0;
  *(code **)(DAT_00541074 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004ae760;
  *(code **)(DAT_00541088 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004aea40;
  *(code **)(DAT_0054107c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004ae9f0;
  iVar4 = 10;
  if (10 < DAT_0054116c) {
    iVar5 = 5000;
    ppuVar9 = &PTR_s_evilRaySystemProcessType_00540fa0;
    do {
      _strncpy((char *)(iVar5 + 4 + (int)DAT_0059ab20),*ppuVar9,0x14);
      iVar4 = iVar4 + 1;
      ppuVar9 = ppuVar9 + 1;
      iVar5 = iVar5 + 500;
    } while (iVar4 < DAT_0054116c);
  }
  *(code **)(DAT_005410cc * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048be40;
  *(code **)(DAT_005410d0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048bf30;
  *(code **)(DAT_00541108 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048bed0;
  *(code **)(DAT_0054110c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048bea0;
  *(code **)(DAT_00541110 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048bf90;
  *(code **)(DAT_00541114 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048bf00;
  *(code **)(DAT_00541118 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048be70;
  *(code **)(DAT_00541148 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048bf60;
  *(code **)(DAT_00541154 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420ad0;
  *(code **)(DAT_00541170 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00451450;
  *(code **)(DAT_00541174 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0046a930;
  *(code **)(DAT_00541178 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045b580;
  *(code **)(DAT_0054117c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045fbd0;
  *(code **)(DAT_00541184 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0041c300;
  *(code **)(DAT_00541188 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0041c300;
  *(code **)(DAT_0054118c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0041c300;
  *(code **)(DAT_00541190 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048ef00;
  *(code **)(DAT_00541194 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048f060;
  *(code **)(_DAT_005411a0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00422ba0;
  *(code **)(DAT_005411a4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0046a6c0;
  *(code **)(DAT_005411ac * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00454db0;
  *(code **)(DAT_005411b0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00454db0;
  *(code **)(DAT_005411b4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00454db0;
  *(code **)(DAT_005411b8 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00454db0;
  *(code **)(DAT_005411c0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00455af0;
  *(code **)(DAT_005411d0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00443b70;
  *(code **)(DAT_005411d4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00443b90;
  *(code **)(DAT_005411e4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00486820;
  *(code **)(DAT_005411e8 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00462610;
  *(code **)(DAT_00541338 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00462610;
  *(code **)(DAT_0054133c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00462610;
  *(code **)(DAT_005411ec * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00462610;
  *(code **)(DAT_005411f4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004737c0;
  *(code **)(DAT_005411fc * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045acd0;
  *(code **)(DAT_00541214 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0042b010;
  *(code **)(DAT_00541218 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004625f0;
  *(code **)(DAT_0054121c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420e40;
  *(code **)(DAT_00541220 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420e40;
  *(code **)(DAT_00541224 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420e40;
  *(code **)(DAT_00541228 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420e60;
  *(code **)(DAT_0054122c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420e80;
  *(code **)(DAT_00541230 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00421040;
  *(code **)(DAT_00541340 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004dd0d0;
  *(code **)(DAT_00541344 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0042aff0;
  *(code **)(DAT_00541234 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00421060;
  *(code **)(DAT_00541238 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00421080;
  *(code **)(DAT_0054123c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00462630;
  *(code **)(DAT_00541240 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004857a0;
  *(code **)(DAT_00541244 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004dd0a0;
  *(code **)(DAT_00541248 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004dced0;
  *(code **)(DAT_0054124c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004dd280;
  *(code **)(DAT_00541250 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00454de0;
  *(code **)(DAT_00541258 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0046b810;
  *(code **)(DAT_0054125c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0046b810;
  *(code **)(DAT_00541264 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0046ae50;
  *(code **)(DAT_00541268 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0046a6c0;
  *(code **)(DAT_00541270 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00443fc0;
  *(code **)(DAT_00541274 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0042afb0;
  *(code **)(DAT_00541278 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00457e80;
  *(code **)(DAT_0054127c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0042af90;
  *(code **)(DAT_00541280 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045fbb0;
  *(code **)(DAT_00541284 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045c260;
  *(code **)(DAT_00541288 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0046add0;
  *(code **)(DAT_0054128c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0042b010;
  *(code **)(DAT_00541290 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048f080;
  *(code **)(DAT_00541294 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045c240;
  *(code **)(DAT_00541298 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045fbf0;
  *(code **)(DAT_0054129c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048eab0;
  *(code **)(DAT_005412a0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0042b030;
  *(code **)(DAT_005412a4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048ef00;
  *(code **)(DAT_005412a8 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0046adf0;
  *(code **)(DAT_005412ac * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048ea90;
  *(code **)(DAT_005412b0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048ed80;
  *(code **)(DAT_005412bc * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004429e0;
  *(code **)(DAT_005412c0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00486870;
  *(code **)(DAT_005412c8 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0042afd0;
  *(code **)(DAT_005412cc * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00443bb0;
  *(code **)(DAT_005412d0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004952a0;
  *(code **)(DAT_005412d4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004429e0;
  *(code **)(DAT_005412d8 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045ae00;
  *(code **)(DAT_005412dc * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004794e0;
  *(code **)(DAT_005412e4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045d2a0;
  *(code **)(DAT_00541310 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420fa0;
  *(code **)(DAT_00541318 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420fc0;
  *(code **)(DAT_0054131c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420fe0;
  *(code **)(DAT_005412e8 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0044aae0;
  *(code **)(DAT_005412ec * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0044b200;
  *(code **)(DAT_005412f0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0044a460;
  *(code **)(DAT_005412f4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0044ab00;
  *(code **)(DAT_005412f8 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420e40;
  *(undefined1 **)(DAT_005412fc * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0044a420;
  *(code **)(DAT_00541300 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004573b0;
  *(code **)(DAT_00541304 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420ea0;
  *(code **)(DAT_00541308 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00421000;
  *(code **)(DAT_0054130c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00421020;
  *(code **)(DAT_00541314 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420f20;
  *(code **)(DAT_00541320 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420f80;
  *(code **)(DAT_00541324 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420ec0;
  *(code **)(DAT_00541328 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420ee0;
  *(code **)(DAT_0054132c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420f00;
  *(code **)(DAT_00541330 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420f60;
  *(code **)(DAT_00541334 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420f40;
  *(code **)(DAT_00541098 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004536a0;
  *(code **)(DAT_0054109c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00458700;
  *(code **)(DAT_005410a0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00458760;
  *(undefined1 **)(DAT_005410a4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00470b00;
  *(undefined1 **)(DAT_005410c8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00487ef0;
  *(code **)(DAT_00541138 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004741d0;
  *(code **)(DAT_0054113c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00474900;
  *(code **)(DAT_00541150 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00458700;
  *(undefined1 **)(DAT_005411bc * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0049a810;
  *(code **)(DAT_00541200 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0045ade0;
  *(code **)(DAT_00541208 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00479180;
  *(code **)(DAT_0054120c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00479180;
  *(code **)(DAT_00541210 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048f0b0;
  *(code **)(DAT_0054126c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_0048bfc0;
  *(code **)(DAT_005412b8 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00491660;
  *(code **)(DAT_005412c4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00422720;
  iVar4 = 0;
  _DAT_0059ab18 = 0.0;
  _DAT_0059ab1c = 0.0;
  if (3 < DAT_00541348) {
    iVar5 = (DAT_00541348 - 4U >> 2) + 1;
    pfVar6 = (float *)((int)DAT_0059ab20 + 0x238);
    iVar4 = iVar5 * 4;
    do {
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[-0x67] == 1.4013e-45) {
        fVar1 = pfVar6[-0x7d] + _DAT_00500fc8;
        if (pfVar6[-0x7d] + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fae1:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[-0x67] == 0.0) &&
              (fVar2 = pfVar6[-0x7d] + _DAT_00501908, pfVar6[-0x7d] + _DAT_00501908 <= _DAT_0059ab1c
              )) goto LAB_0049fae1;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[0x16] == 1.4013e-45) {
        fVar1 = *pfVar6 + _DAT_00500fc8;
        if (*pfVar6 + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fb29:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[0x16] == 0.0) &&
              (fVar2 = *pfVar6 + _DAT_00501908, *pfVar6 + _DAT_00501908 <= _DAT_0059ab1c))
      goto LAB_0049fb29;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[0x93] == 1.4013e-45) {
        fVar1 = pfVar6[0x7d] + _DAT_00500fc8;
        if (pfVar6[0x7d] + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fb7c:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[0x93] == 0.0) &&
              (fVar2 = pfVar6[0x7d] + _DAT_00501908, pfVar6[0x7d] + _DAT_00501908 <= _DAT_0059ab1c))
      goto LAB_0049fb7c;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[0x110] == 1.4013e-45) {
        fVar1 = pfVar6[0xfa] + _DAT_00500fc8;
        if (pfVar6[0xfa] + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fbcf:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[0x110] == 0.0) &&
              (fVar2 = pfVar6[0xfa] + _DAT_00501908, pfVar6[0xfa] + _DAT_00501908 <= _DAT_0059ab1c))
      goto LAB_0049fbcf;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      pfVar6 = pfVar6 + 500;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
  }
  if (iVar4 < DAT_00541348) {
    pfVar6 = (float *)(iVar4 * 500 + 0x44 + (int)DAT_0059ab20);
    iVar4 = DAT_00541348 - iVar4;
    do {
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[0x16] == 1.4013e-45) {
        fVar1 = *pfVar6 + _DAT_00500fc8;
        if (*pfVar6 + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fc36:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[0x16] == 0.0) &&
              (fVar2 = *pfVar6 + _DAT_00501908, *pfVar6 + _DAT_00501908 <= _DAT_0059ab1c))
      goto LAB_0049fc36;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      pfVar6 = pfVar6 + 0x7d;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  uVar3 = FUN_004e49c0();
  _sprintf(local_100,"global.crossbowCost=%d",uVar3);
  FUN_00441580(local_100);
  return;
}


// ==== FUN_0049c2e0 @ 0049c2e0

void __cdecl FUN_0049c2e0(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  FUN_0049c230();
  DAT_00544cc4 = (FILE *)0x0;
  DAT_0059ab38 = 0;
  FUN_0049c160(PTR_DAT_00540dd0);
  FUN_0040cef0();
  _fclose(DAT_00544cc4);
  if (0 < DAT_00541348) {
    iVar3 = 0;
    iVar4 = 0;
    iVar2 = DAT_0059ab20;
    do {
      FUN_0049b0d0(iVar3 + iVar2);
      iVar2 = DAT_0059ab20;
      uVar1 = FUN_004e49c0();
      (&DAT_0059ab50)[iVar4] = uVar1;
      iVar4 = iVar4 + 1;
      iVar3 = iVar3 + 500;
    } while (iVar4 < DAT_00541348);
  }
  if (DAT_0059ab48 == (void *)0x0) {
    FUN_004e0620("budArray","\\Ns\\O\\Rifttype.cpp",0x548);
  }
  FUN_0049c270();
  _free(DAT_0059ab48);
  DAT_0059ab48 = (void *)0x0;
  return;
}


// ==== FUN_0049c3b0 @ 0049c3b0

void __cdecl FUN_0049c3b0(undefined4 param_1,int *param_2,int *param_3,int *param_4)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  char *pcVar4;
  long lVar5;
  undefined4 *puVar6;
  char *pcVar7;
  int *piVar8;
  int *piVar9;
  int iVar10;
  uint uVar11;
  int *piVar12;
  undefined4 uVar13;
  int *local_40;
  undefined1 local_38 [12];
  undefined1 local_2c [4];
  int *local_28;
  int local_24;
  int local_20;
  int *local_18;
  int local_14;
  int local_10;
  void *local_c;
  code *pcStack_8;
  uint local_4;
  
  iVar3 = DAT_0059ab38;
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fb3b0;
  local_c = ExceptionList;
  piVar9 = (int *)((DAT_0059ab38 + 0x45) * 500 + DAT_0059ab20);
  ExceptionList = &local_c;
  piVar9[0x75] = 1;
  piVar9[0x76] = 1;
  pcVar7 = *(char **)(&DAT_00540dcc + iVar3 * 4);
  piVar8 = piVar9 + 1;
  do {
    cVar2 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    *(char *)piVar8 = cVar2;
    piVar8 = (int *)((int)piVar8 + 1);
  } while (cVar2 != '\0');
  piVar9[0x47] = -1;
  piVar9[0x3a] = 0;
  piVar9[0x3b] = 0;
  if (param_2 != (int *)0x0) {
    if ((*param_2 != 1) && (param_2[1] != 1)) {
      FUN_004e0620("flag->btype == BT_IDENTIFIER || flag->utype == UT_TYPEFLAG_LIST",
                   "\\Ns\\O\\Rifttype.cpp",0x673);
    }
    local_28 = param_2;
    local_24 = param_2[1];
    local_20 = -1;
    while (piVar8 = local_28, local_28 != (int *)0x0) {
      local_20 = local_20 + 1;
      if ((*local_28 == 0) && (local_28[1] == local_24)) {
        piVar8 = (int *)local_28[2];
        local_28 = (int *)local_28[3];
      }
      else {
        local_28 = (int *)0x0;
      }
      if (*piVar8 != 1) {
        FUN_004e0620("flag->btype == BT_IDENTIFIER","\\Ns\\O\\Rifttype.cpp",0x678);
      }
      iVar3 = __stricmp((char *)piVar8[2],"dontdrawdefault");
      if (iVar3 == 0) {
        FUN_004e0620("false","\\Ns\\O\\Rifttype.cpp",0x67e);
      }
      iVar3 = __stricmp((char *)piVar8[2],"carribleInVehicle");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x10000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"container");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x20000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"default_hotspot");
      if ((iVar3 == 0) || (iVar3 = __stricmp((char *)piVar8[2],"defaulthotspot"), iVar3 == 0)) {
        piVar9[0x3a] = piVar9[0x3a] | 1;
      }
      iVar3 = __stricmp((char *)piVar8[2],"shadow");
      if (iVar3 == 0) {
        if ((piVar9[0x3a] & 0x400000U) != 0) {
          FUN_004e0620("!(ts->typeFlags & TYPE_FLAG_FLYER_SHADOW)","\\Ns\\O\\Rifttype.cpp",0x68f);
        }
        piVar9[0x3a] = piVar9[0x3a] | 0x40000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"flyershadow");
      if (iVar3 == 0) {
        if ((piVar9[0x3a] & 0x40000U) != 0) {
          FUN_004e0620("!(ts->typeFlags & TYPE_FLAG_SHADOW)","\\Ns\\O\\Rifttype.cpp",0x694);
        }
        piVar9[0x3a] = piVar9[0x3a] | 0x400000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"not_selectable");
      if ((iVar3 == 0) || (iVar3 = __stricmp((char *)piVar8[2],"notselectable"), iVar3 == 0)) {
        piVar9[0x3a] = piVar9[0x3a] | 0x8000000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"not_real");
      if ((iVar3 == 0) || (iVar3 = __stricmp((char *)piVar8[2],"notreal"), iVar3 == 0)) {
        piVar9[0x3b] = piVar9[0x3b] | 0x2000000;
        piVar9[0x3a] = piVar9[0x3a] | 0x28000000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"createsisland");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x400;
      }
      iVar3 = __stricmp((char *)piVar8[2],"mayDropOnRim");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 4;
      }
      iVar3 = __stricmp((char *)piVar8[2],"mayDropOnIsle");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 2;
      }
      iVar3 = __stricmp((char *)piVar8[2],"surface");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x800;
      }
      iVar3 = __stricmp((char *)piVar8[2],"randframe");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x100000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"matchframe");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x200000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"opaqueCollide");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x800000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"dontSave");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x20000000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"saveFrame");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x20;
      }
      iVar3 = __stricmp((char *)piVar8[2],"saveQA");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x1000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"saveQB");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x2000;
      }
      else {
        iVar3 = __stricmp((char *)piVar8[2],"dropblocking");
        if (iVar3 == 0) {
          piVar9[0x3b] = piVar9[0x3b] | 0x10;
        }
        else {
          iVar3 = __stricmp((char *)piVar8[2],"blocking");
          if (iVar3 == 0) {
            piVar9[0x3b] = piVar9[0x3b] | 0x18;
          }
          else {
            iVar3 = __stricmp((char *)piVar8[2],"shotblocking");
            if (iVar3 == 0) {
              piVar9[0x3b] = piVar9[0x3b] | 0x20;
            }
            else {
              iVar3 = __stricmp((char *)piVar8[2],"fencePost");
              if (iVar3 == 0) {
                piVar9[0x3b] = piVar9[0x3b] | 1;
              }
              else {
                iVar3 = __stricmp((char *)piVar8[2],"fence");
                if (iVar3 == 0) {
                  piVar9[0x3b] = piVar9[0x3b] | 0x20000000;
                }
                else {
                  iVar3 = __stricmp((char *)piVar8[2],"buried");
                  if (iVar3 == 0) {
                    pcVar7 = (char *)piVar8[2];
                    pcVar4 = pcVar7;
                    do {
                      cVar2 = *pcVar4;
                      pcVar4 = pcVar4 + 1;
                    } while (cVar2 != '\0');
                    if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                      pcVar4 = pcVar7;
                      do {
                        cVar2 = *pcVar4;
                        pcVar4 = pcVar4 + 1;
                      } while (cVar2 != '\0');
                      pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                    }
                    else {
                      pcVar4 = (char *)0x8;
                    }
                    _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                    piVar9[0x3b] = piVar9[0x3b] | 0x2000;
                  }
                  else {
                    iVar3 = __stricmp((char *)piVar8[2],"nugget");
                    if (iVar3 == 0) {
                      pcVar7 = (char *)piVar8[2];
                      pcVar4 = pcVar7;
                      do {
                        cVar2 = *pcVar4;
                        pcVar4 = pcVar4 + 1;
                      } while (cVar2 != '\0');
                      if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                        pcVar4 = pcVar7;
                        do {
                          cVar2 = *pcVar4;
                          pcVar4 = pcVar4 + 1;
                        } while (cVar2 != '\0');
                        pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                      }
                      else {
                        pcVar4 = (char *)0x8;
                      }
                      _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                      piVar9[0x3b] = piVar9[0x3b] | 0x8000;
                    }
                    else {
                      iVar3 = __stricmp((char *)piVar8[2],"bomb");
                      if (iVar3 == 0) {
                        pcVar7 = (char *)piVar8[2];
                        pcVar4 = pcVar7;
                        do {
                          cVar2 = *pcVar4;
                          pcVar4 = pcVar4 + 1;
                        } while (cVar2 != '\0');
                        if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                          pcVar4 = pcVar7;
                          do {
                            cVar2 = *pcVar4;
                            pcVar4 = pcVar4 + 1;
                          } while (cVar2 != '\0');
                          pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                        }
                        else {
                          pcVar4 = (char *)0x8;
                        }
                        _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                        piVar9[0x3b] = piVar9[0x3b] | 0x100;
                      }
                      else {
                        iVar3 = __stricmp((char *)piVar8[2],"emplacement");
                        if (iVar3 == 0) {
                          piVar9[0x3b] = piVar9[0x3b] | 0x40000;
                        }
                        else {
                          iVar3 = __stricmp((char *)piVar8[2],"bridge");
                          if (iVar3 == 0) {
                            pcVar7 = (char *)piVar8[2];
                            pcVar4 = pcVar7;
                            do {
                              cVar2 = *pcVar4;
                              pcVar4 = pcVar4 + 1;
                            } while (cVar2 != '\0');
                            if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                              pcVar4 = pcVar7;
                              do {
                                cVar2 = *pcVar4;
                                pcVar4 = pcVar4 + 1;
                              } while (cVar2 != '\0');
                              pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                            }
                            else {
                              pcVar4 = (char *)0x8;
                            }
                            _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                            piVar9[0x3b] = piVar9[0x3b] | 4;
                          }
                          else {
                            iVar3 = __stricmp((char *)piVar8[2],"walker");
                            if (iVar3 == 0) {
                              pcVar7 = (char *)piVar8[2];
                              pcVar4 = pcVar7;
                              do {
                                cVar2 = *pcVar4;
                                pcVar4 = pcVar4 + 1;
                              } while (cVar2 != '\0');
                              if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                pcVar4 = pcVar7;
                                do {
                                  cVar2 = *pcVar4;
                                  pcVar4 = pcVar4 + 1;
                                } while (cVar2 != '\0');
                                pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                              }
                              else {
                                pcVar4 = (char *)0x8;
                              }
                              _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                              piVar9[0x3b] = piVar9[0x3b] | 0x10000;
                            }
                            else {
                              iVar3 = __stricmp((char *)piVar8[2],"balloon");
                              if (iVar3 == 0) {
                                pcVar7 = (char *)piVar8[2];
                                pcVar4 = pcVar7;
                                do {
                                  cVar2 = *pcVar4;
                                  pcVar4 = pcVar4 + 1;
                                } while (cVar2 != '\0');
                                if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                  pcVar4 = pcVar7;
                                  do {
                                    cVar2 = *pcVar4;
                                    pcVar4 = pcVar4 + 1;
                                  } while (cVar2 != '\0');
                                  pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                }
                                else {
                                  pcVar4 = (char *)0x8;
                                }
                                _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                piVar9[0x3b] = piVar9[0x3b] | 0x20000;
                              }
                              else {
                                iVar3 = __stricmp((char *)piVar8[2],"island");
                                if (iVar3 == 0) {
                                  pcVar7 = (char *)piVar8[2];
                                  pcVar4 = pcVar7;
                                  do {
                                    cVar2 = *pcVar4;
                                    pcVar4 = pcVar4 + 1;
                                  } while (cVar2 != '\0');
                                  if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                    pcVar4 = pcVar7;
                                    do {
                                      cVar2 = *pcVar4;
                                      pcVar4 = pcVar4 + 1;
                                    } while (cVar2 != '\0');
                                    pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                  }
                                  else {
                                    pcVar4 = (char *)0x8;
                                  }
                                  _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                  piVar9[0x3b] = piVar9[0x3b] | 2;
                                }
                                else {
                                  iVar3 = __stricmp((char *)piVar8[2],"islandThreeByThree");
                                  if (iVar3 == 0) {
                                    piVar9[0x3b] = piVar9[0x3b] | 0x1000000;
                                  }
                                  else {
                                    iVar3 = __stricmp((char *)piVar8[2],"factory");
                                    if (iVar3 == 0) {
                                      pcVar7 = (char *)piVar8[2];
                                      pcVar4 = pcVar7;
                                      do {
                                        cVar2 = *pcVar4;
                                        pcVar4 = pcVar4 + 1;
                                      } while (cVar2 != '\0');
                                      if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                        pcVar4 = pcVar7;
                                        do {
                                          cVar2 = *pcVar4;
                                          pcVar4 = pcVar4 + 1;
                                        } while (cVar2 != '\0');
                                        pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                      }
                                      else {
                                        pcVar4 = (char *)0x8;
                                      }
                                      _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                      piVar9[0x3b] = piVar9[0x3b] | 0x4000;
                                    }
                                    else {
                                      iVar3 = __stricmp((char *)piVar8[2],"vortex");
                                      if (iVar3 == 0) {
                                        pcVar7 = (char *)piVar8[2];
                                        pcVar4 = pcVar7;
                                        do {
                                          cVar2 = *pcVar4;
                                          pcVar4 = pcVar4 + 1;
                                        } while (cVar2 != '\0');
                                        if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                          pcVar4 = pcVar7;
                                          do {
                                            cVar2 = *pcVar4;
                                            pcVar4 = pcVar4 + 1;
                                          } while (cVar2 != '\0');
                                          pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                        }
                                        else {
                                          pcVar4 = (char *)0x8;
                                        }
                                        _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                        piVar9[0x3b] = piVar9[0x3b] | 0x200;
                                      }
                                      else {
                                        iVar3 = __stricmp((char *)piVar8[2],"edgefarm");
                                        if (iVar3 == 0) {
                                          pcVar7 = (char *)piVar8[2];
                                          pcVar4 = pcVar7;
                                          do {
                                            cVar2 = *pcVar4;
                                            pcVar4 = pcVar4 + 1;
                                          } while (cVar2 != '\0');
                                          if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                            pcVar4 = pcVar7;
                                            do {
                                              cVar2 = *pcVar4;
                                              pcVar4 = pcVar4 + 1;
                                            } while (cVar2 != '\0');
                                            pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                          }
                                          else {
                                            pcVar4 = (char *)0x8;
                                          }
                                          _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                          piVar9[0x3b] = piVar9[0x3b] | 0x80000;
                                        }
                                        else {
                                          iVar3 = __stricmp((char *)piVar8[2],"focus");
                                          if (iVar3 == 0) {
                                            pcVar7 = (char *)piVar8[2];
                                            pcVar4 = pcVar7;
                                            do {
                                              cVar2 = *pcVar4;
                                              pcVar4 = pcVar4 + 1;
                                            } while (cVar2 != '\0');
                                            if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                              pcVar4 = pcVar7;
                                              do {
                                                cVar2 = *pcVar4;
                                                pcVar4 = pcVar4 + 1;
                                              } while (cVar2 != '\0');
                                              pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                            }
                                            else {
                                              pcVar4 = (char *)0x8;
                                            }
                                            _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                          }
                                          else {
                                            iVar3 = __stricmp((char *)piVar8[2],"guy");
                                            if (iVar3 == 0) {
                                              pcVar7 = (char *)piVar8[2];
                                              pcVar4 = pcVar7;
                                              do {
                                                cVar2 = *pcVar4;
                                                pcVar4 = pcVar4 + 1;
                                              } while (cVar2 != '\0');
                                              if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                                pcVar4 = pcVar7;
                                                do {
                                                  cVar2 = *pcVar4;
                                                  pcVar4 = pcVar4 + 1;
                                                } while (cVar2 != '\0');
                                                pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                              }
                                              else {
                                                pcVar4 = (char *)0x8;
                                              }
                                              _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4
                                                      );
                                              piVar9[0x3b] = piVar9[0x3b] | 0x400;
                                            }
                                            else {
                                              iVar3 = __stricmp((char *)piVar8[2],"geyser");
                                              if (iVar3 == 0) {
                                                pcVar7 = (char *)piVar8[2];
                                                pcVar4 = pcVar7;
                                                do {
                                                  cVar2 = *pcVar4;
                                                  pcVar4 = pcVar4 + 1;
                                                } while (cVar2 != '\0');
                                                if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8)
                                                {
                                                  pcVar4 = pcVar7;
                                                  do {
                                                    cVar2 = *pcVar4;
                                                    pcVar4 = pcVar4 + 1;
                                                  } while (cVar2 != '\0');
                                                  pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                                }
                                                else {
                                                  pcVar4 = (char *)0x8;
                                                }
                                                _strncpy((char *)(piVar9 + 0x3c),pcVar7,
                                                         (size_t)pcVar4);
                                                piVar9[0x3b] = piVar9[0x3b] | 0x10000000;
                                              }
                                              else {
                                                iVar3 = __stricmp((char *)piVar8[2],"flyer");
                                                if (iVar3 == 0) {
                                                  piVar9[0x3b] = piVar9[0x3b] | 0x100000;
                                                }
                                                else {
                                                  iVar3 = __stricmp((char *)piVar8[2],"residence");
                                                  if (iVar3 == 0) {
                                                    pcVar7 = (char *)piVar8[2];
                                                    pcVar4 = pcVar7;
                                                    do {
                                                      cVar2 = *pcVar4;
                                                      pcVar4 = pcVar4 + 1;
                                                    } while (cVar2 != '\0');
                                                    if (pcVar4 + (1 - (int)(pcVar7 + 1)) <
                                                        (char *)0x8) {
                                                      pcVar4 = pcVar7;
                                                      do {
                                                        cVar2 = *pcVar4;
                                                        pcVar4 = pcVar4 + 1;
                                                      } while (cVar2 != '\0');
                                                      pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                                    }
                                                    else {
                                                      pcVar4 = (char *)0x8;
                                                    }
                                                    _strncpy((char *)(piVar9 + 0x3c),pcVar7,
                                                             (size_t)pcVar4);
                                                    piVar9[0x3b] = piVar9[0x3b] | 0x40000000;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp((char *)piVar8[2],"fringe");
                                                    if (iVar3 == 0) {
                                                      piVar9[0x3b] = piVar9[0x3b] | 0x1000;
                                                    }
                                                    else {
                                                      iVar3 = __stricmp((char *)piVar8[2],"priest");
                                                      if (iVar3 == 0) {
                                                        piVar9[0x3b] = piVar9[0x3b] | 0x200000;
                                                      }
                                                      else {
                                                        iVar3 = __stricmp((char *)piVar8[2],"dais");
                                                        if (iVar3 == 0) {
                                                          piVar9[0x3b] = piVar9[0x3b] | 0x400000;
                                                        }
                                                        else {
                                                          iVar3 = __stricmp((char *)piVar8[2],
                                                                            "altar");
                                                          if (iVar3 == 0) {
                                                            piVar9[0x3b] = piVar9[0x3b] | 0x80000000
                                                            ;
                                                          }
                                                          else {
                                                            iVar3 = __stricmp((char *)piVar8[2],
                                                                              "predictable");
                                                            if (iVar3 == 0) {
                                                              piVar9[0x7a] = piVar9[0x7a] | 99;
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
  if (param_3 != (int *)0x0) {
    if ((*param_3 != 9) && (param_3[1] != 7)) {
      FUN_004e0620("attributeValue->btype == BT_ATTRIBUTE_VALUE || attributeValue->utype == UT_ATTRIBUTE_VALUE_LIST"
                   ,"\\Ns\\O\\Rifttype.cpp",0x757);
    }
    local_14 = param_3[1];
    local_10 = -1;
LAB_0049d090:
    piVar8 = param_3;
    if (piVar8 != (int *)0x0) {
      local_10 = local_10 + 1;
      if ((*piVar8 == 0) && (piVar8[1] == local_14)) {
        param_3 = (int *)piVar8[3];
        piVar8 = (int *)piVar8[2];
      }
      else {
        param_3 = (int *)0x0;
      }
      local_18 = param_3;
      if (*piVar8 != 9) {
        FUN_004e0620("attributeValue->btype == BT_ATTRIBUTE_VALUE","\\Ns\\O\\Rifttype.cpp",0x75c);
      }
      iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxHitPoints");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x10;
        if (*(int *)piVar8[3] != 2) {
          FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",0x762);
        }
        iVar3 = FUN_004e49c0();
        if ((*(byte *)(piVar9 + 0x3a) & 0x10) == 0) {
          FUN_004e0620("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0","\\Ns\\O\\Rifttype.cpp",0x1ef);
        }
        if (*piVar9 != 0) {
          FUN_004e0620("maxHitPoints == 0","\\Ns\\O\\Rifttype.cpp",0x1f0);
        }
        *piVar9 = iVar3;
      }
      else {
        iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rangeConst");
        if (iVar3 != 0) {
          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"helpText");
          if (iVar3 == 0) {
            if (*(int *)piVar8[3] != 10) {
              FUN_004e0620("attributeValue->p2->btype == BT_STRING_CONSTANT","\\Ns\\O\\Rifttype.cpp"
                           ,0x76c);
            }
            pcVar7 = __strdup(*(char **)(piVar8[3] + 8));
            piVar9[0x10] = (int)pcVar7;
            for (pcVar7 = _strstr(pcVar7,"\\n"); pcVar7 != (char *)0x0;
                pcVar7 = _strstr(pcVar7,"\\n")) {
              *pcVar7 = ' ';
              pcVar7[1] = '\n';
            }
            goto LAB_0049d090;
          }
          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rangeVar");
          if (iVar3 == 0) {
            uVar13 = 0x779;
          }
          else {
            iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"attackValueConst");
            if (iVar3 == 0) {
              uVar13 = 0x77d;
            }
            else {
              iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"attackValueVar");
              if (iVar3 == 0) {
                uVar13 = 0x781;
              }
              else {
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"minUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x785);
                  }
                  FUN_0049ad00(piVar9,0,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x78a);
                  }
                  FUN_0049ad60(piVar9,0,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxRainBattleUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x78f);
                  }
                  FUN_0049ad60(piVar9,0,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxThunderBattleUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x794);
                  }
                  FUN_0049ad60(piVar9,1,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxWindBattleUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x799);
                  }
                  FUN_0049ad60(piVar9,2,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"zorder");
                if ((iVar3 != 0) &&
                   (iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"cursorzorder"), iVar3 != 0)) {
                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"foot_x");
                  if ((iVar3 == 0) ||
                     (iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"footx"), iVar3 == 0)) {
                    if (*(int *)piVar8[3] != 2) {
                      FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                   0x7d5);
                    }
                    iVar3 = FUN_004e49c0();
                    piVar9[0x75] = iVar3;
                    if (DAT_0054dbe8 < iVar3) {
                      FUN_004e0620("ts->footX <= maxFootpadInTilesX","\\Ns\\O\\Rifttype.cpp",0x7d7);
                    }
                  }
                  else {
                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"foot_y");
                    if ((iVar3 == 0) ||
                       (iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"footy"), iVar3 == 0)) {
                      if (*(int *)piVar8[3] != 2) {
                        FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                     "\\Ns\\O\\Rifttype.cpp",0x7dc);
                      }
                      iVar3 = FUN_004e49c0();
                      piVar9[0x76] = iVar3;
                      if (DAT_0054dbec < iVar3) {
                        FUN_004e0620("ts->footY <= maxFootpadInTilesY","\\Ns\\O\\Rifttype.cpp",0x7de
                                    );
                      }
                    }
                    else {
                      iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"height");
                      if (iVar3 != 0) {
                        iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hpPerSec");
                        if (iVar3 == 0) {
                          if (*(int *)piVar8[3] != 2) {
                            FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                         "\\Ns\\O\\Rifttype.cpp",0x7e6);
                          }
                          piVar9[0x12] = *(int *)(piVar8[3] + 8);
                          goto LAB_0049d090;
                        }
                        iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rangeVar");
                        if ((iVar3 == 0) ||
                           (iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rangeConst"), iVar3 == 0))
                        {
                          uVar13 = 0x7ec;
                        }
                        else {
                          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"range");
                          if (iVar3 == 0) {
                            piVar9[0x11] = *(int *)(piVar8[3] + 8);
                            goto LAB_0049d090;
                          }
                          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"delayBetweenShots");
                          if (iVar3 == 0) {
                            piVar9[0x13] = *(int *)(piVar8[3] + 8);
                            goto LAB_0049d090;
                          }
                          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"attackValueConst");
                          if (iVar3 == 0) {
                            uVar13 = 0x7f8;
                          }
                          else {
                            iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"attackValueVar");
                            if (iVar3 == 0) {
                              uVar13 = 0x7fc;
                            }
                            else {
                              iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rateConst");
                              if (iVar3 == 0) {
                                uVar13 = 0x800;
                              }
                              else {
                                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rateVar");
                                if (iVar3 == 0) {
                                  uVar13 = 0x804;
                                }
                                else {
                                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hotFootRatioX");
                                  if (iVar3 == 0) {
                                    if (*(int *)piVar8[3] != 2) {
                                      FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                   "\\Ns\\O\\Rifttype.cpp",0x808);
                                    }
                                    iVar3 = FUN_004e49c0();
                                    piVar9[0x77] = iVar3;
                                    goto LAB_0049d090;
                                  }
                                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hotFootRatioY");
                                  if (iVar3 == 0) {
                                    if (*(int *)piVar8[3] != 2) {
                                      FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                   "\\Ns\\O\\Rifttype.cpp",0x80d);
                                    }
                                    iVar3 = FUN_004e49c0();
                                    piVar9[0x78] = iVar3;
                                    goto LAB_0049d090;
                                  }
                                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hotFootX");
                                  if (iVar3 == 0) {
                                    uVar13 = 0x812;
                                  }
                                  else {
                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hotFootY");
                                    if (iVar3 != 0) {
                                      iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"priority");
                                      if (iVar3 == 0) {
                                        if (*(int *)piVar8[3] != 2) {
                                          FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                       "\\Ns\\O\\Rifttype.cpp",0x81a);
                                        }
                                        iVar3 = FUN_004e49c0();
                                        piVar9[0x36] = iVar3;
                                      }
                                      else {
                                        iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                          "constructionRate");
                                        if (iVar3 == 0) {
                                          if (*(int *)piVar8[3] != 2) {
                                            FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                         "\\Ns\\O\\Rifttype.cpp",0x81f);
                                          }
                                          piVar9[0x14] = *(int *)(piVar8[3] + 8);
                                        }
                                        else {
                                          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"orientations"
                                                           );
                                          if (iVar3 == 0) {
                                            if (*(int *)piVar8[3] != 2) {
                                              FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                           "\\Ns\\O\\Rifttype.cpp",0x824);
                                            }
                                            iVar3 = FUN_004e49c0();
                                            piVar9[0x44] = iVar3;
                                          }
                                          else {
                                            iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                              "researchDepend");
                                            if (iVar3 == 0) {
                                              if (*(int *)piVar8[3] != 10) {
                                                FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x82a);
                                              }
                                              FUN_0049b000(piVar9,*(undefined4 *)(piVar8[3] + 8));
                                            }
                                            else {
                                              iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                "activeSound");
                                              if (iVar3 == 0) {
                                                if (*(int *)piVar8[3] != 10) {
                                                  FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x831);
                                                }
                                                iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                            (piVar8[3] + 8));
                                                piVar9[0x1b] = iVar3;
                                              }
                                              else {
                                                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                  "fireSound");
                                                if (iVar3 == 0) {
                                                  if (*(int *)piVar8[3] != 10) {
                                                    FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x839);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x1c] = iVar3;
                                                }
                                                else {
                                                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                    "impactSound");
                                                  if (iVar3 == 0) {
                                                    if (*(int *)piVar8[3] != 10) {
                                                      FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x840);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x1d] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "moveSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x846);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x1e] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "pickupSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x84d);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x1f] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "dropSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x854);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x20] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "fallSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x85b);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x21] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "miscSound0");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x862);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x22] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "miscSound1");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x869);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x22] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "buildDoneSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x870);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x24] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "damageEffect");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x877);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x35] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "speed");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x87c);
                                                  }
                                                  piVar9[0x38] = *(int *)(piVar8[3] + 8);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "turningSpeed");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x881);
                                                  }
                                                  piVar9[0x39] = *(int *)(piVar8[3] + 8);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "theme");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x886);
                                                  }
                                                  piVar9[0x26] = (&DAT_00592990)
                                                                 [**(char **)(piVar8[3] + 8)];
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "level");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x88b);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x25] = iVar3 + -1;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "useairdamage");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x890);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x6c] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "airdamage");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x894);
                                                  }
                                                  piVar9[0x6d] = *(int *)(piVar8[3] + 8);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "casttime");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x898);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x6e] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "threat");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x89c);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x70] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "praytime");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8a0);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x6f] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "manacost");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8a4);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x71] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "spawns");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8a8);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x72] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "effecttime");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8ac);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x73] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "cost");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8b0);
                                                  }
                                                  piVar9[0x31] = *(int *)(piVar8[3] + 8);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "group");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8b5);
                                                  }
                                                  iVar3 = FUN_004a8960(*(undefined4 *)
                                                                        (piVar8[3] + 8));
                                                  piVar9[0x27] = iVar3;
                                                  if (iVar3 == 10) {
                                                    FUN_004e0620("ts->group != NO_GROUP",
                                                                 "\\Ns\\O\\Rifttype.cpp",0x8b7);
                                                  }
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "mana");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8bb);
                                                  }
                                                  pcVar4 = *(char **)(piVar8[3] + 8);
                                                  pcVar7 = pcVar4 + 1;
                                                  do {
                                                    cVar2 = *pcVar4;
                                                    pcVar4 = pcVar4 + 1;
                                                  } while (cVar2 != '\0');
                                                  if (9 < (uint)((int)pcVar4 - (int)pcVar7)) {
                                                    FUN_004e0620(
                                                  "strlen(attributeValue->p2->string) <= 9",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8bc);
                                                  }
                                                  pcVar7 = *(char **)(piVar8[3] + 8);
                                                  piVar8 = piVar9 + 0x28;
                                                  do {
                                                    cVar2 = *pcVar7;
                                                    pcVar7 = pcVar7 + 1;
                                                    *(char *)piVar8 = cVar2;
                                                    piVar8 = (int *)((int)piVar8 + 1);
                                                  } while (cVar2 != '\0');
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "class");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8c1);
                                                  }
                                                  pcVar4 = *(char **)(piVar8[3] + 8);
                                                  pcVar7 = pcVar4 + 1;
                                                  do {
                                                    cVar2 = *pcVar4;
                                                    pcVar4 = pcVar4 + 1;
                                                  } while (cVar2 != '\0');
                                                  if (0x17 < (uint)((int)pcVar4 - (int)pcVar7)) {
                                                    FUN_004e0620(
                                                  "strlen(attributeValue->p2->string) <= (RT_CLASSNAME_LEN-1)"
                                                  ,"\\Ns\\O\\Rifttype.cpp",0x8c2);
                                                  }
                                                  pcVar4 = *(char **)(piVar8[3] + 8);
                                                  pcVar7 = (char *)((int)piVar9 + 0xaa);
                                                  do {
                                                    cVar2 = *pcVar4;
                                                    pcVar4 = pcVar4 + 1;
                                                    *pcVar7 = cVar2;
                                                    pcVar7 = pcVar7 + 1;
                                                  } while (cVar2 != '\0');
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "artifactChanceClass");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8c7);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x33] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "artifactFrame");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8cc);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x34] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "techBit");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8d1);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x7b] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "description");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8d6);
                                                  }
                                                  _strncpy((char *)(piVar9 + 6),
                                                           *(char **)(piVar8[3] + 8),0x28);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "crew");
                                                    if ((((iVar3 != 0) &&
                                                         (iVar3 = __stricmp(*(char **)(piVar8[2] + 8
                                                                                      ),"basecrew"),
                                                         iVar3 != 0)) &&
                                                        (iVar3 = __stricmp(*(char **)(piVar8[2] + 8)
                                                                           ,"joyfactor"), iVar3 != 0
                                                        )) && (iVar3 = __stricmp(*(char **)(piVar8[2
                                                  ] + 8),"foodproduced"), iVar3 != 0)) {
                                                    uVar13 = 0x8e4;
                                                    goto LAB_0049e539;
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
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                      goto LAB_0049d090;
                                    }
                                    uVar13 = 0x816;
                                  }
                                }
                              }
                            }
                          }
                        }
                        FUN_004e0620("false","\\Ns\\O\\Rifttype.cpp",uVar13);
                        goto LAB_0049d090;
                      }
                      iVar3 = FUN_004e49c0();
                      piVar9[0x79] = iVar3;
                    }
                  }
                  goto LAB_0049d090;
                }
                piVar12 = piVar9 + 0x41;
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"cursorzorder");
                if (iVar3 == 0) {
                  piVar9[0x42] = 1;
                  piVar12 = piVar9 + 0x43;
                }
                if (*(int *)piVar8[3] == 2) {
                  iVar3 = FUN_004e49c0();
                  *piVar12 = iVar3;
                  goto LAB_0049d090;
                }
                if (*(int *)piVar8[3] == 10) {
                  pcVar7 = *(char **)(piVar8[3] + 8);
                  FUN_00496c90(local_2c,"([A-Za-z0-9_]+)[ \\t]*");
                  local_4 = 0;
                  iVar3 = FUN_004964e0(local_2c,*(undefined4 *)(piVar8[3] + 8));
                  if (iVar3 != 0) {
                    pcVar7 = (char *)FUN_00495a10(local_2c,1,0,0x50,0);
                  }
                  uVar11 = 0;
                  do {
                    pcVar4 = _strstr((&PTR_s_zoNONE_00540d10)[uVar11 * 2],pcVar7);
                    if (pcVar4 != (char *)0x0) {
                      FUN_00496c90(local_38,"[ \\t]*([A-Za-z0-9_]+)[ \\t]*(\\+|-)[ \\t]*([0-9]+)");
                      local_4 = CONCAT31(local_4._1_3_,1);
                      iVar3 = FUN_004964e0(local_38,*(undefined4 *)(piVar8[3] + 8));
                      if (iVar3 == 0) {
                        *piVar12 = (&DAT_00540d14)[uVar11 * 2];
                      }
                      else {
                        pcVar7 = (char *)FUN_00495a10(local_38,2,0,0x50,0);
                        if (*pcVar7 == '+') {
                          pcVar7 = (char *)FUN_00495a10(local_38,3,0,0x50,0);
                          lVar5 = _atol(pcVar7);
                          *piVar12 = lVar5 + (&DAT_00540d14)[uVar11 * 2];
                        }
                        else {
                          pcVar7 = (char *)FUN_00495a10(local_38,2,0,0x50,0);
                          if (*pcVar7 == '-') {
                            pcVar7 = (char *)FUN_00495a10(local_38,3,0,0x50,0);
                            lVar5 = _atol(pcVar7);
                            *piVar12 = (&DAT_00540d14)[uVar11 * 2] - lVar5;
                          }
                        }
                      }
                      local_4 = local_4 & 0xffffff00;
                      FUN_00495a00(local_38);
                      if (uVar11 < 0x17) goto LAB_0049e7b3;
                      break;
                    }
                    uVar11 = uVar11 + 1;
                  } while (uVar11 < 0x17);
                  FUN_004e0620("zo<COUNTOF(zOrderDefs)","\\Ns\\O\\Rifttype.cpp",0x7c6);
LAB_0049e7b3:
                  local_4 = 0xffffffff;
                  FUN_00495a00(local_2c);
                  param_3 = local_18;
                  goto LAB_0049d090;
                }
                uVar13 = 0x7c9;
              }
            }
          }
LAB_0049e539:
          FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Rifttype.cpp",uVar13);
          goto LAB_0049d090;
        }
        FUN_004e0620("false","\\Ns\\O\\Rifttype.cpp",0x768);
      }
      goto LAB_0049d090;
    }
  }
  if ((param_4[1] != 2) && (*param_4 != 3)) {
    FUN_004e0620("frameBud->utype == UT_FRAME_LIST || frameBud->btype == BT_FRAME",
                 "\\Ns\\O\\Rifttype.cpp",0x8ed);
  }
  local_14 = param_4[1];
  iVar10 = -1;
  local_18 = param_4;
  iVar3 = 0;
  do {
    if ((*param_4 != 0) || (param_4[1] != local_14)) break;
    param_4 = (int *)param_4[3];
    iVar3 = iVar3 + 1;
  } while (param_4 != (int *)0x0);
  iVar3 = (uint)(param_4 != (int *)0x0) + iVar3;
  piVar9[0x45] = iVar3;
  puVar6 = operator_new(iVar3 * 4);
  piVar9[0x49] = (int)puVar6;
  for (uVar11 = piVar9[0x45] & 0x3fffffff; uVar11 != 0; uVar11 = uVar11 - 1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  bVar1 = false;
  for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
    *(undefined1 *)puVar6 = 0;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  while (piVar8 = (int *)0x0, local_18 != (int *)0x0) {
    local_40 = local_18;
    iVar10 = iVar10 + 1;
    if ((*local_18 == 0) && (local_18[1] == local_14)) {
      piVar8 = (int *)local_18[3];
      local_40 = (int *)local_18[2];
    }
    pcVar7 = (char *)(piVar9[0x49] + iVar10 * 4);
    local_18 = piVar8;
    if (*local_40 != 3) {
      FUN_004e0620("frameBud->btype == BT_FRAME","\\Ns\\O\\Rifttype.cpp",0x8f6);
    }
    piVar8 = (int *)local_40[2];
    if (*piVar8 != 4) {
      FUN_004e0620("orientVariant->btype == BT_ORIENT_VARIANT","\\Ns\\O\\Rifttype.cpp",0x8fa);
    }
    cVar2 = **(char **)(piVar8[2] + 8);
    *pcVar7 = cVar2;
    if ((cVar2 < 'A') || ('Z' < cVar2)) {
      FUN_004e0620("\'A\' <= frame->sideOrientation && frame->sideOrientation <= \'Z\'",
                   "\\Ns\\O\\Rifttype.cpp",0x8fd);
    }
    cVar2 = *(char *)(*(int *)(piVar8[2] + 8) + 1);
    if (cVar2 == '\0') {
      pcVar7[1] = 'P';
    }
    else {
      pcVar7[1] = cVar2;
    }
    cVar2 = FUN_004e49c0();
    pcVar7[2] = cVar2;
    pcVar7[3] = '\0';
    piVar8 = (int *)local_40[3];
    if (piVar8 != (int *)0x0) {
      local_24 = piVar8[1];
      while (piVar8 != (int *)0x0) {
        if ((*piVar8 == 0) && (piVar8[1] == local_24)) {
          piVar12 = (int *)piVar8[3];
          piVar8 = (int *)piVar8[2];
        }
        else {
          piVar12 = (int *)0x0;
        }
        if (*piVar8 != 1) {
          FUN_004e0620("frameFlagBud->btype == BT_IDENTIFIER","\\Ns\\O\\Rifttype.cpp",0x90d);
        }
        iVar3 = __stricmp((char *)piVar8[2],"fringe");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 1;
        }
        iVar3 = __stricmp((char *)piVar8[2],"suck");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 2;
        }
        iVar3 = __stricmp((char *)piVar8[2],"rim");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 4;
        }
        iVar3 = __stricmp((char *)piVar8[2],"lit");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 8;
        }
        iVar3 = __stricmp((char *)piVar8[2],"unlit");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 0x10;
        }
        iVar3 = __stricmp((char *)piVar8[2],"cracked");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 0x20;
        }
        iVar3 = __stricmp((char *)piVar8[2],"hard");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 0x40;
        }
        iVar3 = __stricmp((char *)piVar8[2],"baseframe");
        if (iVar3 == 0) {
          piVar9[0x4a] = iVar10;
        }
        iVar3 = __stricmp((char *)piVar8[2],"gumpframe");
        if (iVar3 == 0) {
          piVar9[0x47] = iVar10;
        }
        iVar3 = __stricmp((char *)piVar8[2],"default");
        if ((iVar3 == 0) && (piVar9[0x46] = iVar10, !bVar1)) {
          piVar9[0x48] = iVar10;
        }
        iVar3 = __stricmp((char *)piVar8[2],"help");
        piVar8 = piVar12;
        if (iVar3 == 0) {
          piVar9[0x48] = iVar10;
          bVar1 = true;
        }
      }
    }
  }
  if (DAT_0059af78 == (int *)0x0) {
    DAT_0059af78 = DAT_0059ab24;
  }
  if (*DAT_0059af78 != 0x30312e31) {
    do {
      if (999 < (int)piVar8) goto LAB_0049eb6b;
      piVar12 = DAT_0059af78 + 1;
      DAT_0059af78 = DAT_0059af78 + 1;
      piVar8 = (int *)((int)piVar8 + 1);
    } while (*piVar12 != 0x30312e31);
    if (999 < (int)piVar8) {
LAB_0049eb6b:
      FUN_004e0620("i < 1000","\\Ns\\O\\Rifttype.cpp",0x944);
    }
  }
  piVar9[0x37] = (int)DAT_0059af78;
  DAT_0059af78 = DAT_0059af78 + 1;
  FUN_0049c270();
  ExceptionList = local_c;
  return;
}


// ==== FUN_0049c270 @ 0049c270

void __cdecl FUN_0049c270(void)

{
  int *piVar1;
  
  piVar1 = DAT_0059ab48;
  if (DAT_0059ab48 < DAT_0059ab4c) {
    do {
      if (((*piVar1 == 10) || (*piVar1 == 1)) && ((void *)piVar1[2] != (void *)0x0)) {
        _free((void *)piVar1[2]);
      }
      piVar1 = piVar1 + 4;
    } while (piVar1 < DAT_0059ab4c);
    DAT_0059ab4c = DAT_0059ab48;
    return;
  }
  DAT_0059ab4c = DAT_0059ab48;
  return;
}


// ==== FUN_0049b0d0 @ 0049b0d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0049b0d0(void *this)

{
  char *pcVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  float fVar5;
  undefined1 *puVar6;
  int iVar7;
  uint uVar8;
  char *pcVar9;
  
  iVar4 = ((int)this - DAT_0059ab20) / 500;
  if (iVar4 == DAT_00541074) {
    *(undefined4 *)((int)this + 4) = s_DependForm_005102f8._0_4_;
    *(undefined4 *)((int)this + 8) = s_DependForm_005102f8._4_4_;
    *(undefined2 *)((int)this + 0xc) = s_DependForm_005102f8._8_2_;
    *(char *)((int)this + 0xe) = s_DependForm_005102f8[10];
  }
  if (iVar4 == DAT_00541078) {
    *(undefined4 *)((int)this + 4) = s_ProcessForm_005102ec._0_4_;
    *(undefined4 *)((int)this + 8) = s_ProcessForm_005102ec._4_4_;
    *(undefined4 *)((int)this + 0xc) = s_ProcessForm_005102ec._8_4_;
  }
  if (iVar4 == DAT_0054107c) {
    *(undefined4 *)((int)this + 4) = s_GumpForm_005102e0._0_4_;
    *(undefined4 *)((int)this + 8) = s_GumpForm_005102e0._4_4_;
    *(char *)((int)this + 0xc) = s_GumpForm_005102e0[8];
  }
  if (iVar4 == DAT_00541080) {
    *(undefined4 *)((int)this + 4) = s_PlayerSquid_005102d4._0_4_;
    *(undefined4 *)((int)this + 8) = s_PlayerSquid_005102d4._4_4_;
    *(undefined4 *)((int)this + 0xc) = s_PlayerSquid_005102d4._8_4_;
  }
  if (iVar4 == DAT_00541088) {
    *(undefined4 *)((int)this + 4) = s_ContentForm_005102c8._0_4_;
    *(undefined4 *)((int)this + 8) = s_ContentForm_005102c8._4_4_;
    *(undefined4 *)((int)this + 0xc) = s_ContentForm_005102c8._8_4_;
  }
  if (iVar4 == DAT_00541240) {
    _DAT_00540bdc = FUN_004e49c0();
  }
  iVar7 = *(int *)((int)this + 0x9c);
  if (iVar7 == 5) {
    *(uint *)((int)this + 0xec) = *(uint *)((int)this + 0xec) | 0x800;
  }
  if ((iVar7 == 1) || (iVar7 == 0)) {
    *(uint *)((int)this + 0xec) = *(uint *)((int)this + 0xec) | 0x8000000;
  }
  if (iVar7 == 2) {
    *(uint *)((int)this + 0xec) = *(uint *)((int)this + 0xec) | 0x4000000;
  }
  uVar3 = *(uint *)((int)this + 0xec);
  if ((uVar3 & 0x34200) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x20000;
    if ((uVar3 & 0x30000) == 0) {
      uVar8 = *(uint *)((int)this + 0xfc) | 0x10;
    }
    else {
      uVar8 = *(uint *)((int)this + 0xfc) | 0x20;
    }
    *(uint *)((int)this + 0xfc) = uVar8;
  }
  if ((uVar3 & 0x100) != 0) {
    *(uint *)((int)this + 0x100) = *(uint *)((int)this + 0x100) | 1;
  }
  if ((uVar3 & 0x2000) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x1000;
  }
  if ((uVar3 & 0xa00) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x10000000;
  }
  if ((uVar3 & 0x4000) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x4000000;
  }
  if ((uVar3 & 0x200) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x84000;
  }
  if ((*(float *)((int)this + 0x54) != _DAT_00500fc4) ||
     (*(float *)((int)this + 0x60) != _DAT_00500fc4)) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x4000;
  }
  if ((*(float *)((int)this + 0x58) != _DAT_00500fc4) ||
     (*(float *)((int)this + 100) != _DAT_00500fc4)) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x4000;
  }
  if ((*(float *)((int)this + 0x5c) != _DAT_00500fc4) ||
     (*(float *)((int)this + 0x68) != _DAT_00500fc4)) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x4000;
  }
  if (((*(uint *)((int)this + 0xe8) & 0x1000) != 0) && ((*(uint *)((int)this + 0xe8) & 0x2000) != 0)
     ) {
    FUN_004e0620("(typeFlags & TYPE_FLAG_SAVE_Q16) == 0","\\Ns\\O\\Rifttype.cpp",0x2e1);
  }
  if ((*(uint *)((int)this + 0xe8) & 4) == 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 2;
  }
  if ((*(uint *)((int)this + 0xec) & 0x40000) != 0) {
    *(uint *)((int)this + 0xec) = *(uint *)((int)this + 0xec) | 0x10;
  }
  uVar3 = *(uint *)((int)this + 0xec);
  if (((((uVar3 & 0x40000) != 0) && ((uVar3 & 0x4200) == 0)) && (*(int *)((int)this + 0x1d4) == 3))
     && (*(int *)((int)this + 0x1d8) == 3)) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 6;
  }
  if ((uVar3 & 0x408100) != 0) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 2;
  }
  if (((uVar3 & 0x70000) != 0) && ((uVar3 & 0x4000) == 0)) {
    *(uint *)((int)this + 0xe8) = *(uint *)((int)this + 0xe8) | 0x8000;
  }
  if (*(float *)((int)this + 0x50) == _DAT_00500fc4) {
    *(undefined4 *)((int)this + 0x50) = 0x41200000;
  }
  if (*(int *)((int)this + 0x9c) != 10) {
    if (*(float *)((int)this + 0xc4) == _DAT_00500fc4) {
      if ((uVar3 & 0x30000) == 0) {
        fVar5 = (float)(*(int *)((int)this + 0x94) + 1) * _DAT_00510298;
      }
      else {
        fVar5 = (float)(*(int *)((int)this + 0x94) + 1) * _DAT_0051029c;
      }
      *(float *)((int)this + 0xc4) = fVar5;
    }
    if (((uVar3 & 0x200000) == 0) &&
       (puVar6 = (undefined1 *)((int)this + 0xa0), *(char *)((int)this + 0xa0) == '\0')) {
      if (-1 < *(int *)((int)this + 0x94)) {
        *puVar6 = PTR_DAT_00540ad8[*(int *)((int)this + 0x98)];
        puVar6 = (undefined1 *)((int)this + 0xa1);
      }
      if (*(int *)((int)this + 0x94) == 1) {
        *puVar6 = PTR_DAT_00540ad8[3];
        puVar6 = puVar6 + 1;
      }
      if (*(int *)((int)this + 0x94) == 2) {
        *puVar6 = PTR_DAT_00540ad8[*(int *)((int)this + 0x98)];
        puVar6[1] = PTR_DAT_00540ad8[3];
        puVar6 = puVar6 + 2;
      }
      *puVar6 = 0;
    }
  }
  pcVar9 = (char *)((int)this + 0xa0);
  cVar2 = *(char *)((int)this + 0xa0);
  while (cVar2 != '\0') {
    iVar7 = _tolower((int)*pcVar9);
    *pcVar9 = (char)iVar7;
    pcVar1 = pcVar9 + 1;
    pcVar9 = pcVar9 + 1;
    cVar2 = *pcVar1;
  }
  FUN_0049b060(this);
  if (*(int *)((int)this + 0x1d8) == 6) {
    *(undefined4 *)((int)this + 0x1d8) = 8;
  }
  if (*(int *)((int)this + 0x108) == 0) {
    *(undefined4 *)((int)this + 0x10c) = *(undefined4 *)((int)this + 0x104);
  }
  if (iVar4 == DAT_005412d0) {
    *(undefined4 *)((int)this + 0xc4) = 0;
  }
  if ((*(uint *)((int)this + 0xec) & 0x400) != 0) {
    *(undefined4 *)((int)this + 0xc4) = 0;
  }
  return;
}


// ==== FUN_0049b060 @ 0049b060

void __thiscall FUN_0049b060(void *this)

{
  int *piVar1;
  int iVar2;
  char *pcVar3;
  int iVar4;
  
  iVar4 = 0;
  *(undefined4 *)((int)this + 300) = 0;
  piVar1 = (int *)((int)this + 0x170);
  do {
    piVar1[-0x10] = -1;
    *piVar1 = 0;
    pcVar3 = *(char **)((int)this + 0x124);
    iVar2 = 0;
    if (0 < *(int *)((int)this + 0x114)) {
      do {
        if ((int)*pcVar3 == iVar4 + 0x41) {
          if (piVar1[-0x10] == -1) {
            *(int *)((int)this + 300) = *(int *)((int)this + 300) + 1;
            piVar1[-0x10] = iVar2;
          }
          *piVar1 = *piVar1 + 1;
        }
        iVar2 = iVar2 + 1;
        pcVar3 = pcVar3 + 4;
      } while (iVar2 < *(int *)((int)this + 0x114));
    }
    iVar4 = iVar4 + 1;
    piVar1 = piVar1 + 1;
  } while (iVar4 < 0x10);
  return;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong __cdecl FUN_004e49c0(void)

{
  ulonglong uVar1;
  uint uVar2;
  float fVar3;
  float10 in_ST0;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  uVar1 = (ulonglong)ROUND(in_ST0);
  local_20 = (uint)uVar1;
  uStack_1c = (float)(uVar1 >> 0x20);
  fVar3 = (float)in_ST0;
  if ((local_20 != 0) || (fVar3 = uStack_1c, (uVar1 & 0x7fffffff00000000) != 0)) {
    if ((int)fVar3 < 0) {
      uVar1 = uVar1 + (0x80000000 < (uint)-(float)(in_ST0 - (float10)(longlong)uVar1));
    }
    else {
      uVar2 = (uint)(0x80000000 < (uint)(float)(in_ST0 - (float10)(longlong)uVar1));
      uVar1 = CONCAT44((int)uStack_1c - (uint)(local_20 < uVar2),local_20 - uVar2);
    }
  }
  return uVar1;
}


// ==== __stricmp @ 004e65ed

/* Library Function - Single Match
    __stricmp
   
   Library: Visual Studio 2003 Release */

int __cdecl __stricmp(char *_Str1,char *_Str2)

{
  _ptiddata p_Var1;
  pthreadlocinfo ptVar2;
  int iVar3;
  int iVar4;
  
  p_Var1 = __getptd();
  ptVar2 = (pthreadlocinfo)p_Var1->_tfpecode;
  if (ptVar2 != (pthreadlocinfo)PTR_DAT_00543efc) {
    ptVar2 = ___updatetlocinfo();
  }
  if (ptVar2->lc_category[0].wlocale == (wchar_t *)0x0) {
    iVar3 = ___ascii_stricmp(_Str1,_Str2);
  }
  else {
    do {
      iVar3 = ___tolower_mt(ptVar2,*_Str1);
      _Str1 = _Str1 + 1;
      iVar4 = ___tolower_mt(ptVar2,*_Str2);
      _Str2 = _Str2 + 1;
      if (iVar3 == 0) break;
    } while (iVar3 == iVar4);
    iVar3 = iVar3 - iVar4;
  }
  return iVar3;
}


// ==== ___ascii_stricmp @ 004ef510

/* Library Function - Single Match
    ___ascii_stricmp
   
   Library: Visual Studio 2003 Release */

int __cdecl ___ascii_stricmp(char *_Str1,char *_Str2)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  char cVar4;
  
  bVar3 = 0xff;
  do {
    do {
      cVar4 = '\0';
      if (bVar3 == 0) goto LAB_004ef556;
      bVar3 = *_Str2;
      _Str2 = _Str2 + 1;
      bVar2 = *_Str1;
      _Str1 = _Str1 + 1;
    } while (bVar2 == bVar3);
    bVar1 = bVar3 + 0xbf + (-((byte)(bVar3 + 0xbf) < 0x1a) & 0x20U) + 0x41;
    bVar2 = bVar2 + 0xbf;
    bVar3 = bVar2 + (-(bVar2 < 0x1a) & 0x20U) + 0x41;
  } while (bVar3 == bVar1);
  cVar4 = (bVar3 < bVar1) * -2 + '\x01';
LAB_004ef556:
  return (int)cVar4;
}


// ==== FUN_004a8960 @ 004a8960

void __cdecl FUN_004a8960(void)

{
  FUN_004a87c0();
  return;
}


// ==== _tolower @ 004e53dc

/* Library Function - Single Match
    _tolower
   
   Library: Visual Studio 2003 Release */

int __cdecl _tolower(int _C)

{
  _ptiddata p_Var1;
  pthreadlocinfo ptVar2;
  int iVar3;
  
  p_Var1 = __getptd();
  ptVar2 = (pthreadlocinfo)p_Var1->_tfpecode;
  if (ptVar2 != (pthreadlocinfo)PTR_DAT_00543efc) {
    ptVar2 = ___updatetlocinfo();
  }
  iVar3 = ___tolower_mt(ptVar2,_C);
  return iVar3;
}


// ==== ___tolower_mt @ 004e5314

/* Library Function - Single Match
    ___tolower_mt
   
   Library: Visual Studio 2003 Release */

uint __cdecl ___tolower_mt(int param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  LPCSTR _LpSrcStr;
  int iVar4;
  BOOL unaff_EDI;
  undefined4 local_8;
  
  uVar2 = param_2;
  iVar4 = param_1;
  if ((*(int *)(param_1 + 0x14) == 0) || ((*(int *)(param_1 + 0x24) != 0 && (param_2 < 0x80)))) {
    if ((0x40 < (int)param_2) && ((int)param_2 < 0x5b)) {
      return param_2 + 0x20;
    }
  }
  else {
    if (param_2 < 0x100) {
      if (*(int *)(param_1 + 0x28) < 2) {
        uVar3 = *(byte *)(*(int *)(param_1 + 0x48) + param_2 * 2) & 1;
      }
      else {
        uVar3 = ___isctype_mt(param_1,param_2,1);
      }
      if (uVar3 == 0) {
        return uVar2;
      }
    }
    iVar1 = param_1;
    if ((*(byte *)(*(int *)(iVar4 + 0x48) + 1 + ((int)uVar2 >> 8 & 0xffU) * 2) & 0x80) == 0) {
      param_1._0_2_ = (ushort)(byte)uVar2;
      _LpSrcStr = (LPCSTR)0x1;
    }
    else {
      param_1._0_2_ = CONCAT11((byte)uVar2,(char)(uVar2 >> 8));
      param_1._3_1_ = SUB41(iVar1,3);
      param_1._0_3_ = (uint3)(ushort)param_1;
      _LpSrcStr = (LPCSTR)0x2;
    }
    iVar4 = ___crtLCMapStringA(*(_locale_t *)(iVar4 + 0x14),(LPCWSTR)0x100,(DWORD)&param_1,_LpSrcStr
                               ,(int)&local_8,(LPSTR)0x3,*(int *)(iVar4 + 4),1,unaff_EDI);
    if (iVar4 != 0) {
      if (iVar4 != 1) {
        return (uint)CONCAT11((undefined1)local_8,local_8._1_1_);
      }
      return local_8 & 0xff;
    }
  }
  return uVar2;
}


// ==== FUN_0046b830 @ 0046b830

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0046b830(void *this)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  undefined4 *puVar7;
  uint uVar8;
  float10 fVar9;
  undefined4 local_58;
  undefined4 uStack_54;
  undefined1 local_50 [16];
  undefined1 local_40 [52];
  int local_c;
  
  iVar3 = FUN_0041bfb0();
  iVar4 = FUN_004ac950(iVar3);
  if (*(int *)((int)this + -8) != iVar4) {
    FUN_004e0620("getGroup() == parent.getGroup()","\\Ns\\O\\Gunprocess.cpp",0x68);
  }
  if ((*(byte *)(iVar3 + 0x28) & 1) != 0) {
    FUN_004e0620("!parent.isAbstract()","\\Ns\\O\\Gunprocess.cpp",0x69);
  }
  if ((*(byte *)(iVar3 + 0x28) & 8) != 0) {
    FUN_004e0620("!parent.isAstral()","\\Ns\\O\\Gunprocess.cpp",0x6a);
  }
  if ((*(byte *)(iVar3 + 0x28) & 0x20) != 0) {
    return;
  }
  if (DAT_005c85a4 != 0) {
    return;
  }
  if (DAT_005ca8ec != 0) {
    return;
  }
  iVar4 = (uint)*(ushort *)((int)this + -0x10) * 0x32 + DAT_005c8464;
  if ((DAT_00540bc4 != 0) &&
     (((iVar5 = FUN_0040ea40(iVar4), iVar5 == 0 || ((*(byte *)(iVar4 + 0xb) & 3) != 0)) ||
      ((*(int *)(&DAT_00595200 +
                ((uint)*(byte *)(iVar3 + 0x22) * 9 + (uint)*(byte *)(iVar4 + 0x22)) * 4) != 0 &&
       (DAT_0055a490 == 0)))))) {
    FUN_0046a290((int)this + -0x28,0);
  }
  uVar8 = (uint)*(byte *)(iVar3 + 10);
  if (_DAT_0055b4d0 < *(double *)((int)this + -0x18)) {
    return;
  }
  if ((*(byte *)(iVar3 + 0x28) & 1) != 0) {
    return;
  }
  switch(*(undefined4 *)((int)this + -0xc)) {
  case 0:
    if (DAT_005e4794 != 0) {
      iVar3 = FUN_00469070((int)this + -0x28);
      if (iVar3 != 0) {
        FUN_004e0620("getAnimFormSid() == 0","\\Ns\\O\\Gunprocess.cpp",0x83);
      }
      if ((DAT_005e4794 != 0) && (*(short *)((int)this + -0x10) != 0)) {
        FUN_004e0620("getTargetSid() == 0","\\Ns\\O\\Gunprocess.cpp",0x84);
      }
    }
    break;
  case 1:
    if ((DAT_005e4794 != 0) && (*(short *)((int)this + -0x10) != 0)) {
      FUN_004e0620("getTargetSid() == 0","\\Ns\\O\\Gunprocess.cpp",0x97);
    }
    if ((DAT_00540bc0 != 0) && ((DAT_00594fb8 == 0 || (DAT_0055a490 != 0)))) {
      uVar1 = *(undefined4 *)(iVar3 + 0xe);
      uVar2 = *(undefined4 *)(iVar3 + 0x12);
      uVar6 = FUN_004d4d00(*(int *)((int)this + 8) * 0x32 + DAT_005c8464,
                           *(undefined1 *)(iVar3 + 0x22));
      iVar4 = FUN_0046b6f0(uVar1,uVar2,uVar8,uVar6);
      if (iVar4 == 0) {
        local_58 = SUB84(_DAT_0055b4d0,0);
        uStack_54 = (undefined4)((ulonglong)_DAT_0055b4d0 >> 0x20);
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 2;
        *(undefined4 *)((int)this + -0x18) = local_58;
        *(undefined4 *)((int)this + -0x14) = uStack_54;
        fVar9 = (float10)FUN_00455930(0,0x3e4ccccd);
        *(double *)((int)this + -0x18) =
             (double)(fVar9 + (float10)_DAT_0050bc1c + (float10)_DAT_0055b4d0);
      }
      else {
        FUN_0046a960((int)this + -0x28,iVar4);
      }
    }
    if (DAT_005e4794 == 0) {
      return;
    }
    puVar7 = (undefined4 *)FUN_004ade70(iVar3,local_50,0);
    FUN_0041def0(local_40,*puVar7,puVar7[1],puVar7[2],puVar7[3],0);
    if (local_c == 0) {
      return;
    }
    do {
      if (((*(byte *)(local_c * 0x32 + 10 + DAT_005c8464) == DAT_005412cc) && (DAT_005e4794 != 0))
         && (*(char *)(local_c * 0x32 + DAT_005c8464 + 0x22) != *(char *)(iVar3 + 0x22))) {
        FUN_004e0620("badNoIslandFinder.s().getPlayerId() == parent.getPlayerId()",
                     "\\Ns\\O\\Gunprocess.cpp",0xad);
      }
      FUN_004b1810(local_40);
    } while (local_c != 0);
    return;
  case 2:
    if ((DAT_005e4794 != 0) && (iVar4 = FUN_00469070((int)this + -0x28), iVar4 != 0)) {
      FUN_004e0620("getAnimFormSid() == 0","\\Ns\\O\\Gunprocess.cpp",0xbc);
    }
    if (*(short *)((int)this + -0x10) != 0) {
      iVar3 = FUN_004ac950(iVar3);
      if ((((iVar3 != 0) || (uVar8 == DAT_00541288)) && (uVar8 != DAT_00541264)) &&
         (uVar8 != DAT_0054125c)) {
        FUN_004689c0(*(undefined4 *)((int)this + -0xc),*(undefined4 *)((int)this + 4),
                     *(undefined2 *)((int)this + -0x10));
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
        *(undefined4 *)((int)this + -0xc) = 3;
        return;
      }
      *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
      *(undefined4 *)((int)this + -0xc) = 4;
      return;
    }
    break;
  case 3:
  case 6:
  case 9:
    return;
  case 4:
    if ((DAT_005e4794 != 0) && (iVar3 = FUN_00469070((int)this + -0x28), iVar3 != 0)) {
      FUN_004e0620("getAnimFormSid() == 0","\\Ns\\O\\Gunprocess.cpp",0xd4);
    }
    *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
    if (*(short *)((int)this + -0x10) != 0) {
      *(undefined4 *)((int)this + -0xc) = 5;
      return;
    }
    goto LAB_0046b9a1;
  case 5:
    if (*(short *)((int)this + -0x10) != 0) {
      if ((DAT_005e4794 != 0) && (iVar3 = FUN_00469070((int)this + -0x28), iVar3 != 0)) {
        FUN_004e0620("getAnimFormSid() == 0","\\Ns\\O\\Gunprocess.cpp",0xe3);
      }
      FUN_004689c0(*(undefined4 *)((int)this + -0xc),*(undefined4 *)((int)this + 4),
                   *(undefined2 *)((int)this + -0x10));
      *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
      *(undefined4 *)((int)this + -0xc) = 6;
      return;
    }
    break;
  case 7:
    if ((DAT_005e4794 != 0) && (iVar3 = FUN_00469070((int)this + -0x28), iVar3 != 0)) {
      FUN_004e0620("getAnimFormSid() == 0","\\Ns\\O\\Gunprocess.cpp",0xea);
    }
    (**(code **)(*(int *)((int)this + -0x28) + 4))();
    return;
  case 8:
    FUN_004689c0(8,*(undefined4 *)((int)this + 4),0);
    *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
    *(undefined4 *)((int)this + -0xc) = 9;
    return;
  case 10:
    break;
  default:
    FUN_004e0620("false","\\Ns\\O\\Gunprocess.cpp",0xef);
    return;
  }
  *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
LAB_0046b9a1:
  *(undefined4 *)((int)this + -0xc) = 1;
  return;
}


