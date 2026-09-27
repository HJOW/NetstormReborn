# 오브젝트 수치표 (자동 생성)

> `python tools/typefile.py table` 로 생성. 직접 수정하지 말 것.
> 출처: `netstorm.tarc` 의 `.type` 파일 (패치판 10.7x 기준). 속성 의미는 [type.md](../formats/type.md) 참고.

| 타입 | 설명 | class | theme | level | cost | HP | range | hpPerSec | delayBetweenShots | constructionRate | speed | foot | manacost | casttime | threat | 플래그 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| fireVortex | Fire Temple | Energy | Fire |  | 5000 | 5000 |  |  |  | 10 |  | 8×6 |  |  | 20 | vortex yuckWalk dropBlocking shotblocking mayDropOnRim mayDropOnIsle |
| fireWalker | Fire Ant | Ground Transport | Fire | 1 |  | 50 |  |  |  |  | 2.0 | 1×1 |  |  |  | walker |
| rainFlyer | Man o'War | Aerial Attack | rain |  |  | 150 | 28 | 16 |  | 5 | 2.1 | 1×1 |  |  |  | flyer dontSave flyershadow |
| rainBalloon | Cloud Floater | Aerial Transport | rain | 3 | 1000 | 200 |  |  |  |  | 2.0 | 2×2 |  |  | 15 | balloon flyershadow |
| rainaviary | Man o'War Pool | Air Attack Base | rain | 3 | 600 | 200 | 28 |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| rainBlocker | Ice Tower | Defense | rain | 2 | 800 | 1060 |  |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| rainFence | Acid Barricade | Defense | rain | 2 | 400 | 700 | 45 |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow fence fencePost |
| rainVortex | Rain Temple | Energy | rain |  | 5000 | 5000 |  |  |  | 10 |  | 8×6 |  |  | 20 | vortex yuckWalk dropBlocking shotblocking mayDropOnRim mayDropOnIsle shadow |
| rainwalker | Crystal Crab | Ground Transport | rain | 1 | 500 | 200 |  |  |  |  | 2.4 | 1×1 |  |  | 15 | walker shadow |
| rainFactory | Rain Workshop | Production | rain |  | 1000 | 4000 |  |  |  |  |  | 8×6 |  |  | 20 | factory yuckWalk dropBlocking shotblocking shadow |
| raincannon | Ice Cannon | Shooter | rain | 2 | 600 | 600 | 30 | 20 |  |  |  | 3×3 |  |  | 5 | createsisland emplacement saveFrame shadow |
| rainBattery | Rain Generator | Source of Energy | rain | 1 | 400 | 700 |  |  |  |  |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| sunFlyer | Whirligig | Aerial Attack | sun |  |  | 50 | 30 | 10 |  | 5 | 2 | 1×1 |  |  |  | flyer dontSave flyershadow |
| sunBalloon | Balloon | Aerial Transport | sun | 2 | 600 | 100 |  |  |  |  | 1.9 | 1×1 |  |  | 15 | balloon flyershadow |
| sunaviary | Whirlibase | Air Attack Base | sun | 2 |  | 200 | 30 |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| efarm | Edge Farm | Defense | sun | 1 |  |  |  |  |  |  |  | 1×1 |  |  |  | edgefarm matchframe dropBlocking walkBlocking mayDropOnRim |
| sunBlocker | Stone Tower | Defense | sun | 1 | 400 | 2000 |  |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| sunFence | Sun Barricade | Defense | sun | 1 | 300 | 700 | 50 |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement fence shadow |
| sunwalker | Golem | Ground Transport | sun | 1 |  | 50 |  |  |  |  | 2.0 | 1×1 |  |  | 10 | walker shadow |
| outpost | Outpost | Production | sun |  | 600 | 2000 |  |  |  | 10 |  | 5×4 |  |  | 20 | factory yuckWalk dropBlocking shotblocking mayDropOnIsle mayDropOnRim shadow |
| sunFactory | Sun Workshop | Production | sun |  | 800 | 3000 |  |  |  |  |  | 7×6 |  |  | 10 | factory yuckWalk dropBlocking shotblocking shadow |
| sunArcher | Sun Disc Thrower | Shooter | sun | 1 | 300 | 300 | 9 | 15 |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement |
| suncannon | Sun Cannon | Shooter | sun | 1 | 400 | 600 | 20 | 16 | 5.0 | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| emptyGeyser | Empty Storm Geyser | Source of Storm Power | sun |  | 2000 |  |  |  |  |  |  | 3×3 |  |  |  | dropBlocking shadow createsisland |
| geyser | Storm Geyser | Source of Storm Power | sun |  | 2000 |  |  |  |  |  |  | 3×3 |  |  |  | geyser dropBlocking shadow createsisland |
| Bird | Bird | World Event | sun |  |  | 10 | 30 |  |  | 5 | 2 | 1×1 |  |  |  | dontSave |
| thunderFence | Arc Spire | Defense | thunder | 1 | 400 | 1000 | 45 | 45 |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement fence fencePost |
| thunderBlocker | Bulwark | Defense | thunder | 2 | 800 | 3900 |  |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement container shadow |
| thunderVortex | Thunder Temple | Energy | thunder |  | 5000 | 5000 |  |  |  | 10 |  | 8×6 |  |  | 20 | vortex yuckWalk dropBlocking shotblocking mayDropOnRim mayDropOnIsle shadow |
| bulf | Bulf | Ground Transport | thunder | 1 | 500 | 800 |  |  |  |  | 2.0 | 1×1 |  |  | 15 | walker shadow |
| thunderFactory | Thunder Workshop | Production | thunder |  | 1000 | 4000 |  |  |  |  |  | 8×6 |  |  | 10 | factory yuckWalk dropBlocking shotblocking shadow |
| thundercannon | Thunder Cannon | Shooter | thunder | 2 | 1200 | 1000 | 42 | 40 |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement saveFrame |
| thunderArcher | Vander Tower | Shooter | thunder | 3 | 600 | 600 | 15 | 35 | 1.0 | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| thunderBattery | Thunder Generator | Source of Energy | thunder | 1 | 400 | 700 |  |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| windFlyer | Dust Devil | Aerial Attack | wind |  |  |  | 30 | 12 |  | 5 | 3.3 | 1×1 |  |  |  | flyer dontSave flyershadow |
| windBalloon | Air Ship | Aerial Transport | wind | 3 | 1200 | 655 |  |  |  |  | 2.5 | 4×4 |  |  | 15 | balloon flyershadow |
| windaviary | Devil Maker | Air Attack Base | wind | 3 | 800 | 400 | 30 |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| windBlocker | Wind Tower | Defense | wind | 2 | 800 | 600 |  |  |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow saveFrame |
| windVortex | Wind Temple | Energy | wind |  | 5000 | 5000 |  |  |  | 10 |  | 8×6 |  |  | 20 | vortex yuckWalk dropBlocking shotblocking mayDropOnRim mayDropOnIsle shadow |
| windwalker | Sail Skater | Ground Transport | wind | 2 | 600 | 100 |  |  |  |  | 3.4 | 1×1 |  |  | 15 | walker shadow |
| windFactory | Wind Workshop | Production | wind |  | 1000 | 4000 |  |  |  |  |  | 8×6 |  |  | 20 | factory yuckWalk dropBlocking shotblocking shadow |
| windArcher | Crossbow | Shooter | wind | 2 | 550 | 490 | 16 | 25 |  | 10 |  | 3×3 |  |  | 5 | createsisland emplacement shadow saveFrame |
| windBattery | Wind Generator | Source of Energy | wind | 1 | 400 | 700 |  |  |  |  |  | 3×3 |  |  | 5 | createsisland emplacement shadow |
| Altar | Altar | Altar |  |  | 500 | 1500 |  |  |  |  |  | 7×7 |  |  |  | dontSave altar |
| Dais | Altar | Altar |  |  | 500 |  |  |  |  |  |  | 7×7 |  |  |  | dais saveFrame mayDropOnRim |
| bridge | Bridge | Bridge |  |  |  |  |  |  |  |  |  | 1×1 |  |  | 1 | surface bridge |
| fenceShield | Force Field | Defense |  |  |  |  |  |  |  |  |  | 1×1 |  |  |  | not_real not_selectable dontSave |
| bombInvisible | Invisibility | Defensive Spell |  | 1 | 1000 |  | 7 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombHardener | Bridge Harden | Defensive Spell |  | 2 | 150 |  | 7 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombHeal | Heal | Defensive Spell |  | 2 | 200 |  | 9 |  |  |  |  | 1×1 | 10 | 2 |  | default_hotspot bomb mayDropOnRim |
| bombParalyze | Paralysis | Defensive Spell |  | 2 | 1000 |  | 7 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| Priest | High Priest | High Priest |  |  |  | 100 | 30 |  |  |  | 1.8 | 1×1 |  |  | 25 | walker priest shadow saveQa |
| bombExplodeLarge | Decimation | Offensive Spell |  | 1 | 800 |  | 7 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombExplodeSmall | Point Blast | Offensive Spell |  | 1 | 300 |  | 1 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombLightingwave | Thunderstorm | Offensive Spell |  | 1 | 2500 |  | 15 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombLightingZap | Thunder Strike | Offensive Spell |  | 2 | 1800 |  | 15 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombmeteor | Bombardment | Offensive Spell |  | 2 | 1100 |  | 15 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombExplodeMedium | Devastation | Offensive Spell |  | 3 | 400 |  | 3 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombgraviton | Graviton | Offensive Spell |  | 3 | 500 |  | 5 |  |  |  |  | 1×1 | 10 | 1 |  | default_hotspot bomb mayDropOnRim |
| bombSpecialOne | Devastation | Offensive Spell |  | 99 | 100 |  | 3 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombTreason | Treason | Offsensive Spell |  | 1 | 2000 |  | 7 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| Residence | Residence | Residence |  |  |  |  |  |  |  |  |  | 4×4 |  |  |  | residence randframe yuckWalk dropBlocking |
| nugget | Storm Crystal | Source of Storm Power |  |  | 200 |  |  |  |  |  |  | 1×1 |  |  |  | nugget shadow |
| Buried | Obelisk | Spell |  |  |  |  |  |  |  |  |  | 1×1 |  |  |  | buried shadow |
| bombIIIMano | Hydra Flood | Summons Spell |  | 1 | 1500 |  | 28 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombIIITwister | Vortex | Summons Spell |  | 1 | 1650 |  | 30 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombIIMano | Hydra Wave | Summons Spell |  | 2 | 1000 |  | 28 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombIITwister | Twister | Summons Spell |  | 2 | 1100 |  | 30 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombIMano | Hydra | Summons Spell |  | 3 | 500 |  | 28 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
| bombTwister | Whirlwind | Summons Spell |  | 3 | 550 |  | 30 |  |  |  |  | 1×1 | 10 | 5 |  | default_hotspot bomb mayDropOnRim |
