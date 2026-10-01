# 패치판 내장 도움말 전체 본문

> 2026-10-02. 원본 10.78과 네 번째 녹화 세션의 아카이브에서 같은 `d/help.english`를 확인했다.
> 이 문서는 영문 원문을 보존한 자료이며 게임 규칙을 새로 확정하거나 전체 본문을 번역한 문서는 아니다.

[녹화에서 방문한 페이지의 한국어 정리](../gameplay/help-text-record-play-20261001.md),
[옵션 동작 관찰](../screens/options-menu.md), [태그·제어 코드 포함 UTF-8 원문](in-game-help-source.txt).

- 원본 도움말: 120,888바이트, 2,727행, SHA-256 `faf16cdb210def79619ab5245688463c6fbd700b746069a59a44d7c7c9a31746`.
- 전체 138앵커 정의를 113본문 묶음으로 보존했다. 빈 연속 앵커는 별칭으로 묶었다.
- 중복 앵커의 첫/뒤 본문을 모두 싣고, 오탈자·수치·문장을 임의 수정하지 않았다.
- 표시 서식만 걷어낸 읽기용 본문이다. 그림은 `〔그림: 자산〕`, 조건부 문장은 조건 표시로 남겼다.
- `<info>`는 게임이 그림과 타입별 능력치 머리를 덧붙인다. 이 문서의 정적 본문에 그 실행 시점 값은 포함되지 않는다.
  녹화에 나온 High Priest의 전체 머리 글자는 한국어 정리 문서에 따로 기록했다.
- 파란 링크의 주소는 절 아래에 별도로 보존했다. 외부 주소·명령·오탈자를 가진 주소는 그대로 기록하며 실행하지 않았다.
- `~` 서식의 문자 인용은 표시 형태로 풀었다. 서식 코드 자체의 정확한 재현은 UTF-8 원문을 사용한다.
- 녹화에서 열지 않은 주제도 원본 자료로 포함했다. 해당 절에 녹화 확인을 주장하지 않는다.

## 본문 목록

| 번호 | 제목 / 앵커 | 녹화에서 방문한 주제 |
|---|---|---|
| 1 | [How To Capture and Sacrifice](#topic-sacrificeoutline) · `sacrificeOutline` | 예 |
| 2 | [How To Immobilize the Priest](#topic-immobilehelp) · `immobileHelp` | 원본 자료 보완 |
| 3 | [How To Bring the Enemy Priest to your Altar](#topic-pickuphelp) · `pickupHelp` | 원본 자료 보완 |
| 4 | [How To Send Your Own Priest to the Altar](#topic-moveyourpriesthelp) · `moveyourpriestHelp` | 원본 자료 보완 |
| 5 | [How to Perform the Sacrifice](#topic-performsacrificehelp) · `performsacrificeHelp` | 원본 자료 보완 |
| 6 | [Nimbus](#topic-spherehelp) · `sphereHelp` | 예 |
| 7 | [map](#topic-map) · `map` | 원본 자료 보완 |
| 8 | [Deusphere](#topic-deuspherehelp) · `deusphereHelp` | 원본 자료 보완 |
| 9 | [Serenisphere](#topic-serenispherehelp) · `serenisphereHelp` | 예 |
| 10 | [Pyrosphere](#topic-pyrospherehelp) · `pyrosphereHelp` | 원본 자료 보완 |
| 11 | [Neutral Islands](#topic-bountyislandhelp) · `bountyislandHelp` | 원본 자료 보완 |
| 12 | [NetStorm Instructions](#topic-f1help) · `F1Help` | 예 |
| 13 | [NetStorm User Interface](#topic-interfacehelp) · `interfaceHelp` | 예 |
| 14 | [Chat Window](#topic-chatviewhelp) · `chatViewHelp` | 원본 자료 보완 |
| 15 | [Storm Power Available Window](#topic-moneygumphelp) · `moneyGumpHelp` | 원본 자료 보완 |
| 16 | [Bridge](#topic-bridgetype) · `bridgeType` | 예 |
| 17 | [Production Window](#topic-teleportviewhelp) · `teleportViewHelp` | 원본 자료 보완 |
| 18 | [Sky Overview](#topic-minimapgumphelp) · `minimapGumpHelp` | 원본 자료 보완 |
| 19 | [.Format Command](#topic-formatcommandhelp) · `formatCommandHelp` | 원본 자료 보완 |
| 20 | [Chat Helper](#topic-chathelperhelp) · `chathelperHelp` | 원본 자료 보완 |
| 21 | [Tactics in NetStorm](#topic-tacticshelp) · `tacticHelp / tacticsHelp` | 원본 자료 보완 |
| 22 | [Salvaging](#topic-salvagehelp) · `salvageHelp` | 원본 자료 보완 |
| 23 | [Unit Information](#topic-statshelp) · `statsHelp` | 원본 자료 보완 |
| 24 | [altarType](#topic-altartype) · `daisType / runeType / altarType` | 원본 자료 보완 |
| 25 | [priestType](#topic-priesttype) · `priestType` | 예 |
| 26 | [The Priest's Powers of Construction](#topic-vesselpriesthelp) · `vesselPriestHelp` | 예 |
| 27 | [The Three Furies of Nimbus](#topic-themehelp) · `themeHelp` | 예 |
| 28 | [Battle Units](#topic-unithelp) · `unitHelp` | 예 |
| 29 | [vortexHelp](#topic-vortexhelp) · `rainVortexType / windVortexType / thunderVortexType / vortexHelp` | 원본 자료 보완 |
| 30 | [The Temple:
Acquiring Storm Power](#topic-spvortexhelp) · `spVortexHelp` | 원본 자료 보완 |
| 31 | [The Temple:
Making Golems](#topic-golemvortexhelp) · `golemVortexHelp` | 원본 자료 보완 |
| 32 | [The Temple:
Generating Energy](#topic-influencevortexhelp) · `influenceVortexHelp` | 원본 자료 보완 |
| 33 | [NetStorm Servers](#topic-serverhelp) · `serverHelp` | 원본 자료 보완 |
| 34 | [edgeFarmType](#topic-edgefarmtype) · `edgeFarmType` | 원본 자료 보완 |
| 35 | [Spells](#topic-bombhelp) · `obeliskType / buriedType / bombHelp` | 원본 자료 보완 |
| 36 | [bombLightingZapType](#topic-bomblightingzaptype) · `bombLightingZapType` | 원본 자료 보완 |
| 37 | [bombLightingwaveType](#topic-bomblightingwavetype) · `bombLightingwaveType` | 원본 자료 보완 |
| 38 | [bombTwisterType](#topic-bombtwistertype) · `bombTwisterType` | 원본 자료 보완 |
| 39 | [bombIITwisterType](#topic-bombiitwistertype) · `bombIITwisterType` | 원본 자료 보완 |
| 40 | [bombIIITwisterType](#topic-bombiiitwistertype) · `bombIIITwisterType` | 원본 자료 보완 |
| 41 | [bombIManoType](#topic-bombimanotype) · `bombIManoType` | 원본 자료 보완 |
| 42 | [bombIIManoType](#topic-bombiimanotype) · `bombIIManoType` | 원본 자료 보완 |
| 43 | [bombIIIManoType](#topic-bombiiimanotype) · `bombIIIManoType` | 원본 자료 보완 |
| 44 | [bombGravitonType](#topic-bombgravitontype) · `bombGravitonType` | 원본 자료 보완 |
| 45 | [bombMeteorType](#topic-bombmeteortype) · `bombMeteorType` | 원본 자료 보완 |
| 46 | [bombExplodeSmallType](#topic-bombexplodesmalltype) · `bombExplodeSmallType` | 원본 자료 보완 |
| 47 | [bombSpecialOneType](#topic-bombspecialonetype) · `bombExplodeMediumType / bombSpecialOneType` | 원본 자료 보완 |
| 48 | [bombExplodeLargeType](#topic-bombexplodelargetype) · `bombExplodeLargeType` | 원본 자료 보완 |
| 49 | [bombHardenerType](#topic-bombhardenertype) · `bombHardenerType` | 원본 자료 보완 |
| 50 | [bombHealType](#topic-bombhealtype) · `bombHealType` | 원본 자료 보완 |
| 51 | [bombInvisibleType](#topic-bombinvisibletype) · `bombInvisibleType` | 원본 자료 보완 |
| 52 | [bombParalyzeType](#topic-bombparalyzetype) · `bombParalyzeType` | 원본 자료 보완 |
| 53 | [bombTreasonType](#topic-bombtreasontype) · `bombTreasonType` | 원본 자료 보완 |
| 54 | [residenceType](#topic-residencetype) · `residenceType` | 원본 자료 보완 |
| 55 | [Energy](#topic-influencehelp) · `influenceHelp` | 원본 자료 보완 |
| 56 | [thunderFactoryType](#topic-thunderfactorytype) · `factoryHelp / sunFactoryType / windFactoryType / rainFactoryType / thunderFactoryType` | 원본 자료 보완 |
| 57 | [Upgrading Workshops](#topic-upgradehelp) · `upgradeHelp` | 원본 자료 보완 |
| 58 | [Knowledge](#topic-technologyhelp) · `researchHelp / technologyHelp` | 원본 자료 보완 |
| 59 | [emptyGeyserType](#topic-emptygeysertype) · `geyserType / emptyGeyserType` | 원본 자료 보완 |
| 60 | [nuggetType](#topic-nuggettype) · `nuggetType` | 원본 자료 보완 |
| 61 | [sunArcherType](#topic-sunarchertype) · `sunArcherType` | 원본 자료 보완 |
| 62 | [sunAviaryType](#topic-sunaviarytype) · `sunAviaryType` | 원본 자료 보완 |
| 63 | [sunFlyerType](#topic-sunflyertype) · `sunFlyerType` | 원본 자료 보완 |
| 64 | [sunBlockerType](#topic-sunblockertype) · `sunBlockerType` | 원본 자료 보완 |
| 65 | [sunCannonType](#topic-suncannontype) · `sunCannonType` | 원본 자료 보완 |
| 66 | [sunBalloonType](#topic-sunballoontype) · `sunBalloonType` | 원본 자료 보완 |
| 67 | [sunFenceType](#topic-sunfencetype) · `sunFenceType` | 원본 자료 보완 |
| 68 | [sunWalkerType](#topic-sunwalkertype) · `sunWalkerType` | 원본 자료 보완 |
| 69 | [windArcherType](#topic-windarchertype) · `windArcherType` | 원본 자료 보완 |
| 70 | [windAviaryType](#topic-windaviarytype) · `windAviaryType` | 원본 자료 보완 |
| 71 | [windFlyerType](#topic-windflyertype) · `windFlyerType` | 원본 자료 보완 |
| 72 | [windBatteryType](#topic-windbatterytype) · `windBatteryType` | 원본 자료 보완 |
| 73 | [windBlockerType](#topic-windblockertype) · `windBlockerType` | 원본 자료 보완 |
| 74 | [windBalloonType](#topic-windballoontype) · `windBalloonType` | 원본 자료 보완 |
| 75 | [windWalkerType](#topic-windwalkertype) · `windWalkerType` | 원본 자료 보완 |
| 76 | [thunderArcherType](#topic-thunderarchertype) · `thunderArcherType` | 원본 자료 보완 |
| 77 | [thunderBatteryType](#topic-thunderbatterytype) · `thunderBatteryType` | 원본 자료 보완 |
| 78 | [thunderBlockerType](#topic-thunderblockertype) · `thunderBlockerType` | 원본 자료 보완 |
| 79 | [thunderCannonType](#topic-thundercannontype) · `thunderCannonType` | 원본 자료 보완 |
| 80 | [thunderFenceType](#topic-thunderfencetype) · `thunderFenceType` | 원본 자료 보완 |
| 81 | [bulfType](#topic-bulftype) · `thunderWalkerType / bulfType` | 원본 자료 보완 |
| 82 | [rainAviaryType](#topic-rainaviarytype) · `rainAviaryType` | 원본 자료 보완 |
| 83 | [rainFlyerType](#topic-rainflyertype) · `rainFlyerType` | 원본 자료 보완 |
| 84 | [rainBatteryType](#topic-rainbatterytype) · `rainBatteryType` | 원본 자료 보완 |
| 85 | [rainBlockerType](#topic-rainblockertype) · `rainBlockerType` | 원본 자료 보완 |
| 86 | [growingrainBlockerType](#topic-growingrainblockertype) · `growingrainBlockerType` | 원본 자료 보완 |
| 87 | [rainCannonType](#topic-raincannontype) · `rainCannonType` | 원본 자료 보완 |
| 88 | [rainBalloonType](#topic-rainballoontype) · `rainBalloonType` | 원본 자료 보완 |
| 89 | [rainFenceType](#topic-rainfencetype) · `rainFenceType` | 원본 자료 보완 |
| 90 | [rainWalkerType](#topic-rainwalkertype) · `rainWalkerType` | 원본 자료 보완 |
| 91 | [Transports](#topic-transporthelp) · `transportHelp` | 원본 자료 보완 |
| 92 | [Shooters](#topic-shooterhelp) · `shooterHelp` | 원본 자료 보완 |
| 93 | [The Three Furies of Nimbus](#topic-themehelp-2) · `themeHelp` | 예 |
| 94 | [Battle Units](#topic-unithelp-2) · `unitHelp` | 예 |
| 95 | [vortexHelp](#topic-vortexhelp-2) · `rainVortexType / windVortexType / thunderVortexType / vortexHelp` | 원본 자료 보완 |
| 96 | [The Temple:
Acquiring Storm Power](#topic-spvortexhelp-2) · `spVortexHelp` | 원본 자료 보완 |
| 97 | [The Temple:
Making Golems](#topic-golemvortexhelp-2) · `golemVortexHelp` | 원본 자료 보완 |
| 98 | [The Temple:
Generating Energy](#topic-influencevortexhelp-2) · `influenceVortexHelp` | 원본 자료 보완 |
| 99 | [edgeFarmType](#topic-edgefarmtype-2) · `edgeFarmType` | 원본 자료 보완 |
| 100 | [Storm Power](#topic-stormpowerhelp) · `moneyHelp / stormPowerHelp` | 원본 자료 보완 |
| 101 | [outpostHelp](#topic-outposthelp) · `outpostType / outpostHelp` | 원본 자료 보완 |
| 102 | [Campaign vs. Multiplayer Mode](#topic-multiplayerhelp) · `campaignHelp / multiplayerHelp` | 예 |
| 103 | [Multiplayer Quick Start](#topic-multiquickhelp) · `multiQuickHelp` | 원본 자료 보완 |
| 104 | [Rank](#topic-rankhelp) · `rankHelp` | 원본 자료 보완 |
| 105 | [Multiplayer Survival Guide](#topic-multisafehelp) · `multiSafeHelp` | 원본 자료 보완 |
| 106 | [Reliability Ratings](#topic-reliabilityhelp) · `reliabilityHelp` | 원본 자료 보완 |
| 107 | [Battle Master](#topic-battlemasterhelp) · `battlemasterHelp` | 원본 자료 보완 |
| 108 | [Challenge Rings](#topic-battleislandtype) · `battleIslandType` | 원본 자료 보완 |
| 109 | [Zones](#topic-challengeislandtype) · `zoneHelp / challengeIslandType` | 원본 자료 보완 |
| 110 | [Player List Window](#topic-playerlistviewhelp) · `playerListView / playerListViewHelp` | 원본 자료 보완 |
| 111 | [Battle Status Window](#topic-battlestatusviewhelp) · `battleStatusView / battleStatusViewHelp` | 원본 자료 보완 |
| 112 | [Player Settings](#topic-playeroptionshelp) · `playerOptionsHelp` | 원본 자료 보완 |
| 113 | [Battle Options](#topic-battleoptionshelp) · `battleOptionsHelp` | 원본 자료 보완 |

<a id="topic-sacrificeoutline"></a>

## 1. How To Capture and Sacrifice

원본 앵커: `sacrificeOutline` · 본문 시작: 원문 1행.

```text
How To Capture and Sacrifice

A fevered fear.
A whetted bone.
A drop of blood.
A scream.
A moan.

Capture the High Priest in Battle with one of your Transports, and slaughter him on your Altar in exchange for new Knowledge (in multiplayer mode) or to win a mission (in the Campaign).

The whole process:

1. Immobilize the Priest.
2. Bring the Enemy Priest to your Altar.
3. Send your own Priest to the Altar.
4. Perform the Sacrifice.

Remember: If your Priest is incapacitated or killed he will not be able to perform the Sacrifice!

Learn more about The Priest's Powers of Construction.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#altarType` · [본문 이동](#topic-altartype)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#multiplayerHelp` · [본문 이동](#topic-multiplayerhelp)
- `#immobileHelp` · [본문 이동](#topic-immobilehelp)
- `#pickupHelp` · [본문 이동](#topic-pickuphelp)
- `#moveyourpriestHelp` · [본문 이동](#topic-moveyourpriesthelp)
- `#performsacrificeHelp` · [본문 이동](#topic-performsacrificehelp)
- `#vesselPriestHelp` · [본문 이동](#topic-vesselpriesthelp)

<a id="topic-immobilehelp"></a>

## 2. How To Immobilize the Priest

원본 앵커: `immobileHelp` · 본문 시작: 원문 27행.

```text
How To Immobilize the Priest

Pulverize the enemy Priest with your battle units. When you've reduced him to half of his Hit Points, he'll stop moving, drop anything he's carrying, and a thin force field will form around him, keeping him alive.

A Priest will not be able to cast a Spell while immobilized nor will he be able to construct buildings.

Notice that the Priest is constantly regenerating Hit Points, so be sure to keep up a steady barrage or he may regenerate himself enough to flee.

Next: Use a Transport to pick him up and carry him to your Altar.
```

원본 연결 주소:

- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#statsHelp` · [본문 이동](#topic-statshelp)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#vesselPriestHelp` · [본문 이동](#topic-vesselpriesthelp)
- `#pickupHelp` · [본문 이동](#topic-pickuphelp)

<a id="topic-pickuphelp"></a>

## 3. How To Bring the Enemy Priest to your Altar

원본 앵커: `pickupHelp` · 본문 시작: 원문 44행.

```text
How To Bring the Enemy Priest to your Altar

Once immobilized, you can send a Transport to pick up the Priest. (Exception: a Priest cannot pick up another Priest.)

The Priest must be damaged to at least fifty percent, otherwise he will resist being captured.

When your Transport returns to the Altar, the enemy Priest will be chained in the Sacrificial Circle.

Note that the Priest draws new life force from whatever Transport is carrying him, and therefore the Priest will be fully healed and un-paralyzed when released by the Transport.

Next: Send your own Priest to the Altar.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#moveyourpriestHelp` · [본문 이동](#topic-moveyourpriesthelp)

<a id="topic-moveyourpriesthelp"></a>

## 4. How To Send Your Own Priest to the Altar

원본 앵커: `moveyourpriestHelp` · 본문 시작: 원문 61행.

```text
How To Send Your Own Priest to the Altar

You need a Priest to slaughter a Priest.

At any time, you may move your Priest to your Altar. If he is ever there at a time when you have an enemy Priest chained in the Sacrificial Circle, the Sacrifice will commence. To move the Priest to the Altar, left-click on him and then left click on the Altar. If an Enemy Priest is bound on the Altar, notice that your cursor appears as a dagger when positioned over the Altar.

In single-player mode, you Sacrifice a Priest to win. In multiplayer mode, you Sacrifice a Priest to gain new Knowledge for your next battle, or to Upgrade your Altar for a chance at even better Knowledge

In multiplayer mode, at the moment the Sacrifice begins, you must choose a gift from the Furies.

Your gift can be new Knowledge which the Furies will grant you immediately upon completion of the Sacrifice.

A different gift allows you to Upgrade your Altar. Better Altars grant better Knowledge from the Furies. Note that you will only be able to Upgrade an Altar on your home island, because it is the only island that will persist from battle to battle.

Next: Perform the Sacrifice.
```

원본 연결 주소:

- `#altarType` · [본문 이동](#topic-altartype)
- `#multiQuickHelp` · [본문 이동](#topic-multiquickhelp)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#performsacrificeHelp` · [본문 이동](#topic-performsacrificehelp)

<a id="topic-performsacrificehelp"></a>

## 5. How to Perform the Sacrifice

원본 앵커: `performsacrificeHelp` · 본문 시작: 원문 89행.

```text
How to Perform the Sacrifice

Your Priest must ward five runes to complete the Sacrifice. If he is rendered immobile during the Sacrifice the Spell will be broken and the enemy Priest will escape. Likewise, if the Altar takes too much damage the enemy Priest will also escape.

When the Sacrifice is complete you will be granted your gift: New Knowledge or an Upgraded Altar, as appropriate. When new Knowledge is granted the Altar is consumed. You will need to build a new one to Sacrifice again.

If you opt for an Upgraded Altar but do not have the chance to use it in this battle, have no fear: this Upgraded Altar will remain on your home island for use in your next on-line encounter. Note that you will only be able to Upgrade an Altar on your home island, because it is the only island that will persist from battle to battle.

Note that when you Sacrifice for Knowledge, it becomes instantly available to you, ready to be put into production by the proper Workshop. Press F6 to Review your new Knowledge.

If you gain this Knowledge in Multiplayer mode, it is yours permanently, and will be available in future multiplayer battles. But in future battles, it will not appear instantly in your Production Window: you will need to put this Knowledge into production in a Workshop in order to make it available.

Return to How To Capture and Sacrifice
```

원본 연결 주소:

- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)
- `#factoryHelp` · [본문 이동](#topic-thunderfactorytype)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)

<a id="topic-spherehelp"></a>

## 6. Nimbus

원본 앵커: `sphereHelp` · 본문 시작: 원문 119행.

```text
Nimbus

The world of Nimbus is composed of three spheres: the Serenisphere, the Pyrosphere and the Deusphere.

* In the Serenisphere you'll match up with your on-line opponents.
* In the Pyrosphere you'll do battle to capture enemy High Priests.
* In the Deusphere the Furies themselves do eternal battle, constantly hurling Storm Geysers into the Pyrosphere to fuel your battles.

Learn more about the Serenisphere...
```

원본 연결 주소:

- `#serenisphereHelp` · [본문 이동](#topic-serenispherehelp)
- `#pyrosphereHelp` · [본문 이동](#topic-pyrospherehelp)
- `#deusphereHelp` · [본문 이동](#topic-deuspherehelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#geyserType` · [본문 이동](#topic-emptygeysertype)

<a id="topic-map"></a>

## 7. map

원본 앵커: `map` · 본문 시작: 원문 137행.

```text
〔그림: buried.2〕
```

<a id="topic-deuspherehelp"></a>

## 8. Deusphere

원본 앵커: `deusphereHelp` · 본문 시작: 원문 142행.

```text
Deusphere

In the Deusphere the Furies themselves do eternal battle.

The Deusphere is the innermost sphere of Nimbus: where the Furies rage against one another in a constant war, hurling land up from the planet below. Pray that you never fall down into their fight: it would mean an instant, painful death.

Learn more about the Furies...
```

원본 연결 주소:

- `#sphereHelp` · [본문 이동](#topic-spherehelp)
- `#themeHelp` · [본문 이동](#topic-themehelp)

<a id="topic-serenispherehelp"></a>

## 9. Serenisphere

원본 앵커: `serenisphereHelp` · 본문 시작: 원문 156행.

```text
Serenisphere

The Serenisphere is the outermost sphere of Nimbus: a peaceful place to match up with fellow players for battle in the Pyrosphere below.

The Serenisphere contains Challenge Rings. Right-click on a ring to join the battle. You'll join at whatever location you selected.

Below you will see Zones. You'll start at a Zone that roughly matches your Knowledge level. However, if you wish to visit other places to begin battles, you may right-click on any Zone to travel there.

You may also sail your island through the sky by left-clicking elsewhere.

If you're the first to join a battle, you'll be the BattleMaster. The BattleMaster decides the rules of a battle and can also kick others off of the ring. Anybody kicked off of a Challenge Ring cannot rejoin until the BattleMaster leaves.

When everyone else has joined and is ready to go, the BattleMaster must hit 'Start Battle' to descend to the Pyrosphere for full-on battle.

Before a battle can commence, all players must click on the colored boxes near their names. Your color in battle will be the same as your box color.

Learn more about the Pyrosphere...
```

원본 연결 주소:

- `#sphereHelp` · [본문 이동](#topic-spherehelp)
- `#battleIslandType` · [본문 이동](#topic-battleislandtype)
- `zoneHelp` · [본문 이동](#topic-challengeislandtype)
- `#battleMasterHelp` · [본문 이동](#topic-battlemasterhelp)
- `#pyrosphereHelp` · [본문 이동](#topic-pyrospherehelp)

<a id="topic-pyrospherehelp"></a>

## 10. Pyrosphere

원본 앵커: `pyrosphereHelp` · 본문 시작: 원문 185행.

```text
Pyrosphere

The Pyrosphere is the central sphere of Nimbus. It is here that you'll square off against your opponents.

Your goal is to capture High Priests from your opponents and Sacrifice to the Furies.

As you do battle in the Pyrosphere Obelisks and Storm Geysers will appear on small floating islands, hurled up from the Deusphere below, and ripe for pillage.

The more fierce the battle, the more resources appear.

Learn more about the Deusphere...
```

원본 연결 주소:

- `#sphereHelp` · [본문 이동](#topic-spherehelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#geyserType` · [본문 이동](#topic-emptygeysertype)
- `#deusphereHelp` · [본문 이동](#topic-deuspherehelp)

<a id="topic-bountyislandhelp"></a>

## 11. Neutral Islands

원본 앵커: `bountyislandHelp` · 본문 시작: 원문 204행.

```text
Neutral Islands

Neutral islands appear in multiplayer battle only. Board them for additional Storm Power or to endow your Transports with the ability to cast powerful Spells. Spells appear on neutral islands in the form of Obelisks. Neutral Islands can also be claimed as your own with an Outpost.
```

원본 연결 주소:

- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)
- `#stormpowerHelp` · [본문 이동](#topic-stormpowerhelp)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#outpostType` · [본문 이동](#topic-outposthelp)

<a id="topic-f1help"></a>

## 12. NetStorm Instructions

원본 앵커: `F1Help` · 본문 시작: 원문 214행.

```text
NetStorm Instructions

Click on blue text to get more information. Click and drag to scroll a help window. 〔조건 시작: {global.inMission}〕

You may hit F8 to Review Mission Objectives.

〔조건 끝〕

GAME HELP
* Campaign vs. Multiplayer Mode
* User Interface
* The World of Nimbus
* The Three Furies
* High Priests
* Unit Overview
* The Priest's Powers of Construction
* How to Capture and Sacrifice

GAME SUPPORT
* Customer Service / Technical Support
* Netstorm:HQ's Web Site (Updates and patches)
* Ticonderoga Entertainment's Web Site (The creator of the latest patches)

GAME INFORMATION
* Activision's Web Site (Producer of NetStorm, Offers limited support)
Titanic were the creators of NetStorm, sadly they are no longer around.
```

원본 연결 주소:

- `#campaignHelp` · [본문 이동](#topic-multiplayerhelp)
- `#interfaceHelp` · [본문 이동](#topic-interfacehelp)
- `#sphereHelp` · [본문 이동](#topic-spherehelp)
- `#themeHelp` · [본문 이동](#topic-themehelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#vesselpriestHelp` · [본문 이동](#topic-vesselpriesthelp)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `cmd:Tell,TechSupport` · 외부/명령/본문 대상 미확인
- `http://www.netstormhq.com` · 외부/명령/본문 대상 미확인
- `http://te.netstormhq.com` · 외부/명령/본문 대상 미확인
- `http://www.activision.com` · 외부/명령/본문 대상 미확인

<a id="topic-interfacehelp"></a>

## 13. NetStorm User Interface

원본 앵커: `interfaceHelp` · 본문 시작: 원문 256행.

```text
NetStorm User Interface

Left Click on a unit to select it.
Right Click on a window or unit to get a menu of commands.
Hold down ALT to scroll the screen towards the cursor. To scroll in full screen mode (or in Window mode if your desktop is set to 640 x 480 resolution) simply move the cursor to the edge of the screen. (Note: with MicroSoft's IntellimouseTM, holding the center "Wheel" button is equivalent to holding the ALT key.)
Escape brings up an options menu at the top of the screen.

F1 General Help.
F2 Hide buildings so you can see behind them.
F3 Hide and show the Chat Window.
F4 or H Jump to your Home Temple.
F5 Jump to your Priest.
F6 Show the Knowledge you control.
F7 Toggle island colors.
F8 Review mission objectives.
F9 Show who is online (in Multiplayer).
Shift-F3 Hide and show the Island Themes.
Shift-F7 Take a screen Shot.
Shift-F9 or the Pause button Pause the game.
TAB Cycle through the last five units you've placed.
Ctrl-F5 or N Cycle through all of your Transports, selecting each in turn.
T Hide and show the game timer.
P or R Selects your priest - press it twice to go to and select your priest.
D Allows you to grab the last unit you selected on the Production Window.
C Changes the way your units and bridges roate.
U Jump to the position of your most recently lost unit.
E Selects the last available bridge slot.

Q W
A S Use these keys to grab bridge pieces directly into your cursor.
(Z X) (Z & X are only relevant for games with 6 available bridge pieces.)

Another feature: If you hold the Shift key and type a number 0 through 9, NetStorm will remember the screen location. To hop directly back to that screen location, simply press the number key.

Note: many of the hot-keys above require you to have the Chat Window hidden.

For reference, here are the symbols used in the game to represent Energy:
〔그림: mana.8〕 - Wind
〔그림: mana.9〕 - Rain
〔그림: mana.10〕 - Thunder
〔그림: mana.11〕 - Sun

To learn what any window or unit does just right-click it and select "About" from the menu. If you are still learning how to play the help may seem confusing, so please watch the demo and play the Early Missions.
```

원본 연결 주소:

- `#chatViewHelp` · [본문 이동](#topic-chatviewhelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)
- `#teleportViewHelp` · [본문 이동](#topic-teleportviewhelp)
- `#campaignHelp` · [본문 이동](#topic-multiplayerhelp)

<a id="topic-chatviewhelp"></a>

## 14. Chat Window

원본 앵커: `chatViewHelp` · 본문 시작: 원문 334행.

```text
Chat Window

You can talk to other players when you activate the Chat Window by hitting F3. Type your message and hit ENTER to send it to the Chat Windows of other players!

If you'd rather not see what a certain player is typing, right-click on that player's island or units and select Ignore Chat.

Normally, everyone nearby sees your chat messages. You may restrict who gets your messages by typing their name followed by a colon (:). You can also type battle: to send messages to everyone in your Challenge Rings, allies: to send to allies only, or if you're an observer, you can type Observer: and it will be sent to observers only.

There are ways to tell if someone is whispering to you, to you and others, or to a group of people. You can see who they are talking to by looking at the icon before their name in the chat box:
Whispering to more than one person will have 〔표시 코드: I93.113〕
Whispering to one person will have 〔표시 코드: I93.111〕
Whispering to Observers will have 〔표시 코드: I93.112〕
Whispering to the Battle will have 〔표시 코드: I93.109〕
Whispering to Allies will have 〔표시 코드: I93.110〕

If you hit the TAB key the game will automatically search for a name that is closest to what you already typed. For example, if I wanted to send a message to a player called "OzTheGreatAndPowerful" I definitely don't want to type his whole name! So I just type in "oz" and hit TAB and his whole name appears automatically.

You can make your chat messages more distinctive by using the .format command to change your message color and style.

You can read more about some chat options in the Chat Helper section.

You may also read a general interface discussion.
```

원본 연결 주소:

- `#battleIslandType` · [본문 이동](#topic-battleislandtype)
- `#formatCommandHelp` · [본문 이동](#topic-formatcommandhelp)
- `#chathelperHelp` · [본문 이동](#topic-chathelperhelp)
- `#interfaceHelp` · [본문 이동](#topic-interfacehelp)

<a id="topic-moneygumphelp"></a>

## 15. Storm Power Available Window

원본 앵커: `moneyGumpHelp` · 본문 시작: 원문 373행.

```text
Storm Power Available Window

"Storm Power is the Essence! Creation and Destruction become one!" -Tenth Circle Teachings

This number shows your available Storm Power, used to build units and Buildings.

if you have less than 2000〔그림: fortgump.3〕 remaining, the number shows in yellow (for example, 1900〔그림: fortgump.3〕). If you have less than 1000〔그림: fortgump.3〕, it shows in red (for example, 850〔그림: fortgump.3〕).

The icon 〔그림: fortgump.3〕 is used to indicate amounts of Storm Power. If you are trying to build something and its Storm Power requirements are drawn in red (for example "Cannon 1000〔그림: fortgump.3〕") then you do not have enough Storm Power to build it.

In the Production Window units will blink if you attempt to build them but lack the Storm Power.

You may also read a general interface discussion.
```

원본 연결 주소:

- `#stormPowerHelp` · [본문 이동](#topic-stormpowerhelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#teleportViewHelp` · [본문 이동](#topic-teleportviewhelp)
- `#interfaceHelp` · [본문 이동](#topic-interfacehelp)

<a id="topic-bridgetype"></a>

## 16. Bridge

원본 앵커: `bridgeType` · 본문 시작: 원문 395행.

```text
Bridge

"We don't build bridges to make peace." -General Jan Masaryk

In the Pyrosphere (i.e. in battle), you will build bridges out from the edges of your island.

When you build a Temple, bridge pieces will appear in the Production Window at the upper-left of your screen. When a bridge piece first appears in the Production Window it is cracked, but if the piece remains in the Window for a few seconds it becomes more solid. The implications and subtleties of using cracked bridge pieces versus solid ones will become clear as you experiment with each in battle.

Rather than selecting each bridge separately on the Production Window, simply use the Q, W, A, S, Z, X, and E keys. Each key corresponds to an adjacent bridge. Pressing the E key allows the user to select any available bridge unit. To rotate a bridge 90 degrees simply right-click with the mouse and to activate reverse bridge rotation, press the C key. A bridge piece will go red if it's not legal to lay it down in a particular place.

Use bridges to connect to resources, to assault enemy islands, and to create territory where you can lay your units. Remember that stationary units are placed at the end of bridges, never directly on them!

Rules to remember: There must be a continuous attachment from the Temple on your island to the bridge you are laying down. Also, bridges cannot be attached to an island if an Edge Farm stands in the way.

In Multiplayer mode you can cause Generators to Meltdown to slightly damage enemy units or break through bridges by right-clicking the unit and selecting Meltdown.
```

원본 연결 주소:

- `#pyrosphereHelp` · [본문 이동](#topic-pyrospherehelp)
- `#teleportViewHelp` · [본문 이동](#topic-teleportviewhelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)
- `#edgeFarmType` · [본문 이동](#topic-edgefarmtype)
- `generatorHelp` · 외부/명령/본문 대상 미확인

<a id="topic-teleportviewhelp"></a>

## 17. Production Window

원본 앵커: `teleportViewHelp` · 본문 시작: 원문 429행.

```text
Production Window

Pick up units from the Production Window by left-clicking, and left-click again to lay them down on an island or bridge spur.

Units will show red in this window if you need more Storm Power to build them.

Any Knowledge that you have put into production in a Workshop or acquired by Sacrifice will appear in this window.

Right-click on any unit in this window to get more information about it.

You may also read a general interface discussion.
```

원본 연결 주소:

- `unitHelp` · [본문 이동](#topic-unithelp)
- `#stormPowerHelp` · [본문 이동](#topic-stormpowerhelp)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#factoryHelp` · [본문 이동](#topic-thunderfactorytype)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `#interfaceHelp` · [본문 이동](#topic-interfacehelp)

<a id="topic-minimapgumphelp"></a>

## 18. Sky Overview

원본 앵커: `minimapGumpHelp` · 본문 시작: 원문 448행.

```text
Sky Overview

In the Pyrosphere, the Sky Overview shows you the Neutral Islands and the islands of your on-line opponents.

The Sky Overview Window displays islands and bridges in their corresponding colors Geysers also stand out as brown dots.

Left-click in the Sky Overview to see a particular part of the sky. You may also hold down the left mouse button to drag the view around. This can be handy during battles when you want to get a good aerial view of the strategic situation!
```

원본 연결 주소:

- `#pyrosphereHelp` · [본문 이동](#topic-pyrospherehelp)
- `#bountyislandHelp` · [본문 이동](#topic-bountyislandhelp)
- `#bridgeType` · [본문 이동](#topic-bridgetype)

<a id="topic-formatcommandhelp"></a>

## 19. .Format Command

원본 앵커: `formatCommandHelp` · 본문 시작: 원문 464행.

```text
.Format Command

When you type .format in the Chat Window and follow it with text, that text will appear before any chat message you send from then on. For example, in the Chat Window type
.format ~y~E
and hit ENTER. All of your chat messages will now be yellow and embossed! You can use other lowercase letters for colors as well, such as
~b Blue
~l Light Blue
~d Teal
~r Red
~g Green
~u Light Green
~k Black
~m Magenta
~s Dark Magenta
~o Orange
~a Light Orange
~x Brown
~q Dark Tan
~j Light Tan
~p Pink
~y Yellow
~i Light Yellow
~t Silver
~. Resets the color. (That is Tilde + Period)

Some uppercase letters have meaning as well, for example, if you type
.format ~E~y
your chat messages will all be yellow and embossed. Other styles include
~I Italicizes text.
~B Bolds text.
~E Embosses all of your chat text.
~S Strikeouts all of your chat text.
~U Underlines all of your chat text.
~N Sets all your chat text to normal again.

For more colors and other command codes you can check the Chat Helper and for more help on formats you can check Netstorm:HQ.

Note: You can use .back instead of .format as it will do the same thing. If you want to change something in the front of your name you can use .front just like .format.
```

원본 연결 주소:

- `#chathelperHelp` · [본문 이동](#topic-chathelperhelp)
- `http://www.netstormhq.com` · 외부/명령/본문 대상 미확인

<a id="topic-chathelperhelp"></a>

## 20. Chat Helper

원본 앵커: `chathelperHelp` · 본문 시작: 원문 538행.

```text
Chat Helper

What is the Chat Helper?

The Chat Helper is the small button to the left of the Chat Window. Clicking on it brings up a menu of functions and options to let you decide what you are able to see in the Chat Window and to help you create effects for your own text.

Format

The Format menu gives you two options: Edit Back Format, and Edit Front Format. Edit Back Format brings up a text box in which you can put the text and codes you want to appear before your name in the Chat Window. Edit Front Format also brings up a text box, but instead allows you to input the text and codes you want to appear in front of your name in the Chat Window.

This is an alternative way to using the .back, .format and .front commands.

Chat Effects

The Chat Effects menu allows you to decide which effects will be displayed or not and whether sounds will be played or not when you receive text from other users featuring these codes. There are four options:

Allow Chat Sound: Turning this off will prevent sounds sent by other users from being played.

Allow Background Colors: Turning this off will stop background colors received from being displayed, 〔표시 코드: B.3〕such as this.

Allow Foreground Colors: Turning this off will stop foreground colors received from being displayed which use the ~[C.xxx] code, 〔표시 코드: C.52〕such 〔표시 코드: C.156〕as 〔표시 코드: C.223〕these, but it leaves the regular colors, such as ~y and ~r.

Allow Embossed Offset: Turning this off will prevent any adjusted Emboss effects from being displayed (those which use the ~[E.X#xY#] command)

Chat Controls

These options are only available inside a game, and they allow you to decide which channels you can receive text from, and which actions should be performed when a new message is received.

Channel Controls

Show General Channel: Disabling this means you do not receive text which is sent to everybody.

Show Allies Channel: Disabling this means that you do not receive text which is sent through the ALLIES: Channel.

Show Observer Channel: Disabling this means that you do not receive text which is sent through the OBSERVER: Channel.

Show Whisper Channel: Disabling this means that you do not receive text from anyone whispering to you.

Player Controls

Show Allies Chat: Disabling this means that you do not receive text from your Allies inside a game.

Show Enemies Chat: Disabling this means that you do not receive text from your Enemies inside a game.

Show Observers Chat: Disabling this means that you do not receive text from the Observers inside a game.

Other Controls

Open Window on Message: Disabling this means that the Chat Window does not open when a message is received.

Play sound on Message: Disabling this means that sounds aren't played when a message is Received.

Insert

This menu allows you to insert certain codes into the Chat Window which change the colors of, or which give certain effects to, your text. You can learn how to use these in the .format section. These are the menus:

Text Colors: This menu allows you to insert colors from the 255 available ones.

Background Colors: This menu allows you to insert background colors which will be displayed behind the text when used in conjunction with the ~E command (embossed text).

Icons: This menu allows you to insert icons from the many hundreds.

Keywords: These give you some command templates to use: Mimicing, Sounds, Icons and Emboss Offset (the positioning of the background color when used with the ~E command (embossed text).

Styles: This menu gives you the codes of the formats which alter the style, not the color, of your text.

Prefabricated Colors: This menu gives you a list of the basic colors used in Netstorm which are not blocked by disabling the "Allow Foreground Colors" option. They also have simple codes, e.g. ~y, or ~b.

Commands: This menu gives you the commands which can be used to alter what is displayed before and after your name in the chat window.

Copy

Clicking this copies text which you have put at the bottom of the Chat Window to the clipboard. If no text is highlighted, all of the text is copied; if some of the text is highlighted, only that text is copied to the clipboard.

Paste

Clicking this pastes any copied text into the text line of Chat Window, where the cursor is.

Note: For more help on formats you can check Netstorm:HQ.
```

원본 연결 주소:

- `#chatViewHelp` · [본문 이동](#topic-chatviewhelp)
- `#formatCommandHelp` · [본문 이동](#topic-formatcommandhelp)
- `http://www.netstormhq.com` · 외부/명령/본문 대상 미확인

<a id="topic-tacticshelp"></a>

## 21. Tactics in NetStorm

원본 앵커: `tacticHelp / tacticsHelp` · 본문 시작: 원문 631행.

```text
Tactics in NetStorm

It is crucial to understand how your units will fight in battle. Your strategy will depend upon these facts.

All units will always choose the closest available target. Once locked, they will keep at it until destroyed. Certain defensive units, like Towers, can be interposed to force the attacker to choose a new target. The closest target will be chosen even if it is invulnerable to the attacker!

When a unit explodes cracked bridge nearby will also crumble. Normal bridge that was adjacent to the unit will crack, or become a jagged end.

When shooting units explode they also damage every other unit nearby! They may explode from that damage in a chain reaction. Temples also explode in this way.

Salvaging a unit does not cause such an explosion, but bridge is affected in the same way, as if the unit had exploded.

You can build units 1) on your home island, 2) on any un-owned island which is connected by bridges to your home island, or 3) off of any bridge that you own.

When you destroy an enemy unit you get 25% of its Storm Power value. Remember that Whirligigs and Man o' War cost no additional Storm Power to build, and thus return none to you when destroyed. Only their bases will do so.

When you are targeting an enemy, remember that you must shoot the area where it meets the ground. If your shots are hitting its upper areas, then you're not really shooting it at all.
```

원본 연결 주소:

- `unitHelp` · [본문 이동](#topic-unithelp)
- `salvageHelp` · [본문 이동](#topic-salvagehelp)

<a id="topic-salvagehelp"></a>

## 22. Salvaging

원본 앵커: `salvageHelp` · 본문 시작: 원문 661행.

```text
Salvaging

You may Salvage any unit and recoup 25% of the Storm Power you spent to build it.

If the unit has been damaged, you will recoup less Storm Power. For example, if you build a unit for 400〔그림: fortgump.3〕 you can Salvage it for 100〔그림: fortgump.3〕. However, if one third of its hit points are gone, you'll only get 66〔그림: fortgump.3〕.

One last thing to know: If the damage goes beyond half of the unit's hit points you will recoup no Storm Power at all.

Note: Salvaging a unit next to a bridge will make solid bridges crack, and cracked bridges fall.
```

원본 연결 주소:

- `unitHelp` · [본문 이동](#topic-unithelp)

<a id="topic-statshelp"></a>

## 23. Unit Information

원본 앵커: `statsHelp` · 본문 시작: 원문 676행.

```text
Unit Information

Alignment: Each unit in the game is aligned with one of the Furies or with Sun. The unit's alignment determines what Fury grants Knowledge for it, what Workshop can produce it, and what sort of Energy is required to build it.

Class: Describes what the unit does and how it can be used. A list of classes appears at the bottom of this page.

Hits: Tells how much damage the unit can absorb before being destroyed.

Range: If the unit shoots or spreads an effect, this number tells how many 'tiles' the effect will reach. For reference, the size of a Sun Disc Thrower is 3x3 'tiles' and the screen is forty tiles high.

Damage: Describes how much damage the unit will inflict, on average, every second. Remember, the rating isn't a per-shot rating, but an absolute scale telling you the unit's effectiveness over time.

Cost in Storm Power: Tells how much Storm Power you must expend in order to build the unit.

Energy to Build: Shows you how much Energy is required to create the unit and exactly which types of Energy are needed.

Units may be Salvaged to recoup some of their Storm Power.

CLASSES:

Production:
Wind Temple
Rain Temple
Thunder Temple
Sun Workshop
Wind Workshop
Rain Workshop
Thunder Workshop
Outpost
Holy Place:
Altar
High Priest:
High Priest
Source of Energy:
Wind Generator
Rain Generator
Thunder Generator
Source of Storm Power:
Storm Geyser
Storm Crystal
Bridge:
Bridge
Shooter:
Sun Cannon
Ice Cannon
Thunder Cannon
Sun Disc Thrower
Crossbow
Vander Tower
Aerial Attack:
Whirligig
Dust Devil
Man o' War
Air Attack Base:
Whirlibase
Devil Maker
Man o' War Pool
Defense:
Stone Tower
Wind Tower
Ice Tower
Bulwark
Barricade
Acid Barricade
Arc Spire
Edge Farm
Ground Transport:
Golem
Sail Skater
Crystal Crab
Bulf
Aerial Transport:
Balloon
Air Ship
Cloud Floater
Offensive Spell:
Point Blast
Devastation
Decimation
Treason
Bombardment
Graviton
ThunderStorm
Thunder Strike
Defensive Spell:
Bridge Harden
Heal
Invisibility
Paralysis
Summons Spell:
Whirlwind
Twister
Vortex
Hydra
Hydra Wave
Hydra Flood
Residence:
Residence
```

원본 연결 주소:

- `unitHelp` · [본문 이동](#topic-unithelp)
- `#themeHelp` · [본문 이동](#topic-themehelp)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#factoryHelp` · [본문 이동](#topic-thunderfactorytype)
- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `salvageHelp` · [본문 이동](#topic-salvagehelp)
- `#windVortexType` · [본문 이동](#topic-vortexhelp)
- `#rainVortexType` · [본문 이동](#topic-vortexhelp)
- `#thunderVortexType` · [본문 이동](#topic-vortexhelp)
- `#sunFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#windFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#rainFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#thunderFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#outpostType` · [본문 이동](#topic-outposthelp)
- `#altarType` · [본문 이동](#topic-altartype)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#windbatteryType` · [본문 이동](#topic-windbatterytype)
- `#rainbatteryType` · [본문 이동](#topic-rainbatterytype)
- `#thunderbatteryType` · [본문 이동](#topic-thunderbatterytype)
- `#geyserType` · [본문 이동](#topic-emptygeysertype)
- `#nuggetType` · [본문 이동](#topic-nuggettype)
- `#bridgeType` · [본문 이동](#topic-bridgetype)
- `#suncannonType` · [본문 이동](#topic-suncannontype)
- `#raincannonType` · [본문 이동](#topic-raincannontype)
- `#thundercannonType` · [본문 이동](#topic-thundercannontype)
- `#sunarcherType` · [본문 이동](#topic-sunarchertype)
- `#windarcherType` · [본문 이동](#topic-windarchertype)
- `#thunderarcherType` · [본문 이동](#topic-thunderarchertype)
- `#sunflyerType` · [본문 이동](#topic-sunflyertype)
- `#windflyerType` · [본문 이동](#topic-windflyertype)
- `#rainflyerType` · [본문 이동](#topic-rainflyertype)
- `#sunaviaryType` · [본문 이동](#topic-sunaviarytype)
- `#windaviaryType` · [본문 이동](#topic-windaviarytype)
- `#rainaviaryType` · [본문 이동](#topic-rainaviarytype)
- `#sunblockerType` · [본문 이동](#topic-sunblockertype)
- `#windblockerType` · [본문 이동](#topic-windblockertype)
- `#rainblockerType` · [본문 이동](#topic-rainblockertype)
- `#thunderblockerType` · [본문 이동](#topic-thunderblockertype)
- `#sunfenceType` · [본문 이동](#topic-sunfencetype)
- `#rainfenceType` · [본문 이동](#topic-rainfencetype)
- `#thunderfenceType` · [본문 이동](#topic-thunderfencetype)
- `#edgefarmType` · [본문 이동](#topic-edgefarmtype)
- `#sunwalkerType` · [본문 이동](#topic-sunwalkertype)
- `#windwalkerType` · [본문 이동](#topic-windwalkertype)
- `#rainwalkerType` · [본문 이동](#topic-rainwalkertype)
- `#bulfType` · [본문 이동](#topic-bulftype)
- `#sunballoonType` · [본문 이동](#topic-sunballoontype)
- `#windballoonType` · [본문 이동](#topic-windballoontype)
- `#rainballoonType` · [본문 이동](#topic-rainballoontype)
- `#bombExplodeSmallType` · [본문 이동](#topic-bombexplodesmalltype)
- `#bombExplodeMediumType` · [본문 이동](#topic-bombspecialonetype)
- `#bombExplodeLargeType` · [본문 이동](#topic-bombexplodelargetype)
- `#bombTreasonType` · [본문 이동](#topic-bombtreasontype)
- `#bombMeteorType` · [본문 이동](#topic-bombmeteortype)
- `#bombGravitonType` · [본문 이동](#topic-bombgravitontype)
- `#bombLightingwaveType` · [본문 이동](#topic-bomblightingwavetype)
- `#bombLightingZapType` · [본문 이동](#topic-bomblightingzaptype)
- `#bombHardenerType` · [본문 이동](#topic-bombhardenertype)
- `#bombHealType` · [본문 이동](#topic-bombhealtype)
- `#bombInvisibleType` · [본문 이동](#topic-bombinvisibletype)
- `#bombParalyzeType` · [본문 이동](#topic-bombparalyzetype)
- `#bombTwisterType` · [본문 이동](#topic-bombtwistertype)
- `#bombIITwisterType` · [본문 이동](#topic-bombiitwistertype)
- `#bombIIITwisterType` · [본문 이동](#topic-bombiiitwistertype)
- `#bombIManoType` · [본문 이동](#topic-bombimanotype)
- `#bombIIManoType` · [본문 이동](#topic-bombiimanotype)
- `#bombIIIManoType` · [본문 이동](#topic-bombiiimanotype)
- `#residenceType` · [본문 이동](#topic-residencetype)

<a id="topic-altartype"></a>

## 24. altarType

원본 앵커: `daisType / runeType / altarType` · 본문 시작: 원문 797행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"It is a window onto the greatest of all glory,
and into the deepest of all oblivion." -Askeld Urvonnus, High Priest of Nimbus

Use your Altar to Sacrifice captured Priests to the Furies.

Your Priest can build an Altar. Each Altar has five runes - Sun, Wind, Rain, Thunder and Storm - which must be filled in order to complete a Sacrifice. As each rune is filled, one of the supporting bridges leaves the center and when all five are gone, the enemy Priest plunges into the abyss.

In the Campaign when you Sacrifice all the enemy Priests, the mission will usually end successfully.

In Multiplayer mode Sacrificing has a different meaning. You Sacrifice to get Knowledge from the Furies. In order to get higher level units, you must Upgrade your Altar (by Sacrificing). The Upgraded Altar (there is a level two and a level three Altar) will stay with your island from game to game. (Note that you will not be able to Upgrade an Altar that is not on your home island.) Your Altar's level must match the level of the unit you're trying to get from the Furies. Here is a table of every unit and its level.

Note that when an Altar is destroyed, a captive Priest will go free.

Click here for to learn How To Capture and Sacrifice a Priest.
```

원본 연결 주소:

- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `#themeHelp` · [본문 이동](#topic-themehelp)
- `#campaignHelp` · [본문 이동](#topic-multiplayerhelp)
- `multiplayerHelp` · [본문 이동](#topic-multiplayerhelp)
- `unitHelp` · [본문 이동](#topic-unithelp)

<a id="topic-priesttype"></a>

## 25. priestType

원본 앵커: `priestType` · 본문 시작: 원문 826행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Who among you will face the Fury?
Who will know of holiness and fear?
And who will be destroyed?" -Book of Nimbus

Your High Priest is the ultimate vessel of Knowledge and power from the Furies. Your Priest can build special buildings and Altars, and also performs Sacrifices of other Priests to the Furies.

A Priest can only be destroyed on an Altar. He cannot be killed by battle units, only incapacitated. Furthermore, if the Priest has a Temple his wounds will gradually heal during battle.

Priests are also able to "Pray" in order to acquire the Devastation Spell which they are then able to cast for 100〔그림: fortgump.3〕. Right-click your Priest and choose "Pray" to see how this works. (Multiplayer Only)

To view your High Priest simply press F5. If you wish to select your High Priest, press P or R once, to view and select it press it twice.

The Priest's Powers of Construction
Sacrificing a Priest for new Knowledge
```

원본 연결 주소:

- `#themeHelp` · [본문 이동](#topic-themehelp)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)
- `#bombExplodeMediumType` · [본문 이동](#topic-bombspecialonetype)
- `#vesselPriestHelp` · [본문 이동](#topic-vesselpriesthelp)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)

<a id="topic-vesselpriesthelp"></a>

## 26. The Priest's Powers of Construction

원본 앵커: `vesselPriestHelp` · 본문 시작: 원문 851행.

```text
The Priest's Powers of Construction

Use your High Priest to build Temples, Workshops, and Altars, and (in multiplayer mode) Outposts on any large island--including your own first island. Right-click on him and select the appropriate option. The building will go to your cursor, ready to be placed. (Note that your Priest must be able to walk to the build location.) Once you've placed the building, the Priest will walk to it and begin construction.

Note that a Priest cannot construct buildings while immobilized.

Like the Golem, your Priest also serves as a Transport. You can use him to acquire and cast Spells or to transport Storm Crystals. Additionally, the Priest will try to move to a safer location if the bridge underneath him becomes dangerous.

Learn more about Sacrificing a Priest for new Knowledge
```

원본 연결 주소:

- `#vortexHelp` · [본문 이동](#topic-vortexhelp)
- `#factoryHelp` · [본문 이동](#topic-thunderfactorytype)
- `#altarType` · [본문 이동](#topic-altartype)
- `#multiplayerHelp` · [본문 이동](#topic-multiplayerhelp)
- `#outpostType` · [본문 이동](#topic-outposthelp)
- `#immobileHelp` · [본문 이동](#topic-immobilehelp)
- `#sunWalkerType` · [본문 이동](#topic-sunwalkertype)
- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#nuggetType` · [본문 이동](#topic-nuggettype)
- `#bridgeType` · [본문 이동](#topic-bridgetype)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)

<a id="topic-themehelp"></a>

## 27. The Three Furies of Nimbus

원본 앵커: `themeHelp` · 본문 시작: 원문 871행.

같은 앵커가 원본에서 여러 번 정의된다: `themeHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
The Three Furies of Nimbus

At the heart of Nimbus a constant storm rages. There the three Furies, Wind, Rain and Thunder, strive in never-ending conflict for control of the world.

This conflict tears huge chunks of the Hidden Planet up into the sky. Upon these newborn islands live the Nimbians, your people.

Death and destruction please the Furies. Thus, when you destroy an enemy unit, you are granted one quarter of the Storm Power value of that enemy unit--except for individual Aerial Attackers which do not actually cost any Storm Power to build.

Even more pleasing to the Furies is the Sacrifice of a Priest. Such Sacrifices cause them to grant the gift of battle Knowledge.

Each Fury has a symbol for its Energy:
〔그림: mana.8〕 - Wind
〔그림: mana.9〕 - Rain
〔그림: mana.10〕 - Thunder

There is also common Knowledge called "Sun," symbolized by 〔그림: mana.11〕. Sun units can use any type of Energy but are relatively weak.

Beyond Sun Knowledge each Fury can supply specialized Knowledge. This Knowledge tends to require the Energy of the particular Fury who bestowed it.

Associated with the Wind Fury are:

1〔그림: windVortex.*〕 2〔그림: windfactory.*〕 3〔그림: windbattery.*〕

1) The Temple where Wind is worshipped.
2) The Workshop producing Wind-aligned units.
3) The Wind Generator that generates Wind Energy.
4) All Wind-aligned units.

Associated with the Rain Fury are:

1〔그림: RainVortex.*〕 2〔그림: Rainfactory.*〕 3〔그림: Rainbattery.*〕

1) The Temple where Rain is worshipped.
2) The Workshop producing Rain-aligned units.
3) The Rain Generator that generates Rain Energy.
4) All Rain-aligned units.

Associated with the Thunder Fury are:

1〔그림: ThunderVortex.*〕 2〔그림: Thunderfactory.*〕 3〔그림: Thunderbattery.*〕

1) The Temple where Thunder is worshipped.
2) The Workshop producing Thunder-aligned units.
3) The Thunder Generator that generates Thunder Energy.
4) All Thunder-aligned units.

Sun, which has no Fury associated with it, is nevertheless supplied by all Furies. Thus it has:
1〔그림: sunFactory.*〕

1) A Workshop producing unaligned (Sun) units.
2) All Sun units.

Learn more about Sacrificing a Priest for new Knowledge...
```

원본 연결 주소:

- `#sphereHelp` · [본문 이동](#topic-spherehelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#windVortexType` · [본문 이동](#topic-vortexhelp)
- `#windFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#windBatteryType` · [본문 이동](#topic-windbatterytype)
- `#RainVortexType` · [본문 이동](#topic-vortexhelp)
- `#RainFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#RainBatteryType` · [본문 이동](#topic-rainbatterytype)
- `#ThunderVortexType` · [본문 이동](#topic-vortexhelp)
- `#ThunderFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#ThunderBatteryType` · [본문 이동](#topic-thunderbatterytype)
- `#SunFactoryType` · [본문 이동](#topic-thunderfactorytype)

<a id="topic-unithelp"></a>

## 28. Battle Units

원본 앵커: `unitHelp` · 본문 시작: 원문 937행.

같은 앵커가 원본에서 여러 번 정의된다: `unitHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
Battle Units

Battle units are divided into four varieties: Sun, Wind, Rain and Thunder. Units may be Salvaged if necessary. There are basic Tactics that all units use for attack.

Sun Units

Level One
* Golem / Ground Transport
* Sun Disc Thrower / Shooter
* Sun Cannon / Shooter
* Stone Tower / Defense
* Sun Barricade / Defense
Level Two
* Whirligig / Aerial Attack
* Whirlibase / Aerial Attack Base
* Balloon / Aerial Transport

Wind Units

Level One
* Wind Generator / Source of Energy
Level Two
* Sail Skater / Ground Transport
* Crossbow / Shooter
* Wind Tower / Defense
Level Three
* Dust Devil / Aerial Attack
* Devil Maker / Aerial Attack Base
* Air Ship / Aerial Transport

Rain Units

Level One
* Rain Generator / Source of Energy
* Crystal Crab / Ground Transport
Level Two
* Ice Cannon / Shooter
* Ice Tower / Defense
* Acid barricade / Defense
Level Three
* Man o' War / Aerial Attack
* Man o' War Pool / Aerial Attack Base
* Cloud Floater / Aerial Transport

Thunder Units

Level One
* Thunder Generator / Source of Energy
* Bulf / Ground Transport
* Arc Spire / Defense
Level Two
* Thunder Cannon / Shooter
* Bulwark / Defense
Level Three
* Vander Tower / Shooter

How to read unit information.
Other units and their Alignments.
```

원본 연결 주소:

- `salvageHelp` · [본문 이동](#topic-salvagehelp)
- `tacticsHelp` · [본문 이동](#topic-tacticshelp)
- `#sunWalkerType` · [본문 이동](#topic-sunwalkertype)
- `#sunArcherType` · [본문 이동](#topic-sunarchertype)
- `#sunCannonType` · [본문 이동](#topic-suncannontype)
- `#sunBlockerType` · [본문 이동](#topic-sunblockertype)
- `#sunFenceType` · [본문 이동](#topic-sunfencetype)
- `#sunFlyerType` · [본문 이동](#topic-sunflyertype)
- `#sunAviaryType` · [본문 이동](#topic-sunaviarytype)
- `#sunBalloonType` · [본문 이동](#topic-sunballoontype)
- `#windBatteryType` · [본문 이동](#topic-windbatterytype)
- `#windWalkerType` · [본문 이동](#topic-windwalkertype)
- `#windArcherType` · [본문 이동](#topic-windarchertype)
- `#windBlocker` · 외부/명령/본문 대상 미확인
- `#windFlyerType` · [본문 이동](#topic-windflyertype)
- `#windAviaryType` · [본문 이동](#topic-windaviarytype)
- `#windBalloonType` · [본문 이동](#topic-windballoontype)
- `#rainBatteryType` · [본문 이동](#topic-rainbatterytype)
- `#rainWalkerType` · [본문 이동](#topic-rainwalkertype)
- `#rainCannonType` · [본문 이동](#topic-raincannontype)
- `#rainBlockerType` · [본문 이동](#topic-rainblockertype)
- `#rainFenceType` · [본문 이동](#topic-rainfencetype)
- `#rainFlyerType` · [본문 이동](#topic-rainflyertype)
- `#rainAviaryType` · [본문 이동](#topic-rainaviarytype)
- `#rainBalloonType` · [본문 이동](#topic-rainballoontype)
- `#thunderBatteryType` · [본문 이동](#topic-thunderbatterytype)
- `#bulfType` · [본문 이동](#topic-bulftype)
- `#thunderFence` · 외부/명령/본문 대상 미확인
- `#thunderCannonType` · [본문 이동](#topic-thundercannontype)
- `#thunderBlockerType` · [본문 이동](#topic-thunderblockertype)
- `#thunderArcher` · 외부/명령/본문 대상 미확인
- `#statsHelp` · [본문 이동](#topic-statshelp)
- `#themeHelp` · [본문 이동](#topic-themehelp)

<a id="topic-vortexhelp"></a>

## 29. vortexHelp

원본 앵커: `rainVortexType / windVortexType / thunderVortexType / vortexHelp` · 본문 시작: 원문 1002행.

같은 앵커가 원본에서 여러 번 정의된다: `rainVortexType / windVortexType / thunderVortexType / vortexHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The blood of holy men is the elixir of destiny.

Rain Temple
Wind Temple
Thunder Temple

The Temple is the ultimate center of Energy on each island. Temples take one of three forms, as listed above.

As long as your Temple is standing, your Priest will heal during battle. They also confer the following benefits:

1. You may build bridges off of the island containing the Temple.
2. The island is warded against enemy construction. No enemy may build on it.
3. Transports may return Storm Crystals to the Temple to turn them to Storm Power.
4. The island will change to reflect the theme of which ever temple you build.

These benefits are nearly identical to the Outpost, a key unit in multiplayer play.

Hit F4 to jump to your Temple during game play.

Find out about using the Temple for:
Acquiring Storm Power
Making Golems
Generating Energy
```

원본 연결 주소:

- `#rainVortexType` · [본문 이동](#topic-vortexhelp)
- `#windVortexType` · [본문 이동](#topic-vortexhelp)
- `#thunderVortexType` · [본문 이동](#topic-vortexhelp)
- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#bridgeType` · [본문 이동](#topic-bridgetype)
- `#nuggetType` · [본문 이동](#topic-nuggettype)
- `#stormpowerHelp` · [본문 이동](#topic-stormpowerhelp)
- `outpostType` · [본문 이동](#topic-outposthelp)
- `multiplayerHelp` · [본문 이동](#topic-multiplayerhelp)
- `#spVortexHelp` · [본문 이동](#topic-spvortexhelp)
- `#golemVortexHelp` · [본문 이동](#topic-golemvortexhelp)
- `#influenceVortexHelp` · [본문 이동](#topic-influencevortexhelp)

<a id="topic-spvortexhelp"></a>

## 30. The Temple:
Acquiring Storm Power

원본 앵커: `spVortexHelp` · 본문 시작: 원문 1038행.

같은 앵커가 원본에서 여러 번 정의된다: `spVortexHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
The Temple:
Acquiring Storm Power

In battle, the Temple converts Storm Power into a usable resource. Without a Temple you can't collect or process Storm Power.

Storm Power is collected from Storm Geysers in the form of Storm Crystals.

Learn more about making Golems...
```

원본 연결 주소:

- `#moneyHelp` · [본문 이동](#topic-stormpowerhelp)
- `#geyserType` · [본문 이동](#topic-emptygeysertype)
- `#nuggetType` · [본문 이동](#topic-nuggettype)
- `#golemVortexHelp` · [본문 이동](#topic-golemvortexhelp)

<a id="topic-golemvortexhelp"></a>

## 31. The Temple:
Making Golems

원본 앵커: `golemVortexHelp` · 본문 시작: 원문 1051행.

같은 앵커가 원본에서 여러 번 정의된다: `golemVortexHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
The Temple:
Making Golems

Temples allow you to create Golems that serve as Transports on your new island.

A Golem will show up in your Production Window when you first descend to the Pyrosphere for battle. You can make as many of these servants as you wish, provided you have enough Storm Power.

Learn more about how the Vortex generates Energy...
```

원본 연결 주소:

- `#sunWalkerType` · [본문 이동](#topic-sunwalkertype)
- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#teleportViewHelp` · [본문 이동](#topic-teleportviewhelp)
- `#pyrosphereHelp` · [본문 이동](#topic-pyrospherehelp)
- `#moneyHelp` · [본문 이동](#topic-stormpowerhelp)
- `#influenceVortexHelp` · [본문 이동](#topic-influencevortexhelp)

<a id="topic-influencevortexhelp"></a>

## 32. The Temple:
Generating Energy

원본 앵커: `influenceVortexHelp` · 본문 시작: 원문 1066행.

같은 앵커가 원본에서 여러 번 정의된다: `influenceVortexHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
The Temple:
Generating Energy

To build anything in Nimbus, you must have the adequate Energy. Your Temple is one of the two sources for this Energy. As you prepare to build anything on your home island, whether it's a Golem, a Workshop, or units for battle, your Temple will supply the Energy you need.

Left-click on your Temple to see the range of this Energy.

Return to Temple Help.
```

원본 연결 주소:

- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)

<a id="topic-serverhelp"></a>

## 33. NetStorm Servers

원본 앵커: `serverHelp` · 본문 시작: 원문 1078행.

```text
NetStorm Servers

[[ DO NOT TRANSLATE YET ]] Here is what a NetStorm server is: Here is how a local server works: Here is how an internet server works: If you have trouble use the troubleshooting guide. [[CONTINUE TRANSLATING HERE.]]
```

<a id="topic-edgefarmtype"></a>

## 34. edgeFarmType

원본 앵커: `edgeFarmType` · 본문 시작: 원문 1096행.

같은 앵커가 원본에서 여러 번 정의된다: `edgeFarmType`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"In other cultures, the plowing is not so dangerous." -Pavel Milyukov, Nimbian farmer

No bridge may be attached where an Edge Farm grows, so enemy boarding attempts are foiled there --but this may also foil your attempts to build bridges out from your own island.

As a Temple is built, the alignment of that Temple grows into the Edge Farm itself.

The inhabitants of the islands grow their food around the island's perimeter, leaving interior space open for development.
```

원본 연결 주소:

- `#bridgeType` · [본문 이동](#topic-bridgetype)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)

<a id="topic-bombhelp"></a>

## 35. Spells

원본 앵커: `obeliskType / buriedType / bombHelp` · 본문 시작: 원문 1113행.

```text
Spells

"They add a little spice to the carnage." -Oltor Hammershold, Nimbian battle-cook.

Transports can acquire and cast Spells that have special effects in the world.

Spells manifest in the world as Obelisks. Left-click your Transport and then position the cursor over an Obelisk. Left-click again when the hand-cursor appears, and now your Transport will travel to the Obelisk and acquire a Spell. Each transport can possess one Spell. An icon for that Spell will then appear overlaid on the Transport.

Once a Transport has acquired a Spell, it can cast the Spell an unlimited number of times, provided the player has sufficient Storm Power. The Transport keeps that Spell until destroyed, or until he acquires another Spell--displacing the previous one. Left-click on the Transport to see the range of the Spell. Right-click on the Transport to cast the Spell.

Once you cast the Spell, your Transport will halt in place until the Spell is cast. This will take a couple of seconds.

Spells are more common in multiplayer mode than in the Campaign.

Spells do not affect the caster.

The various kinds of Spells:
〔그림: bombExplodeSmall.1〕Point Blast
〔그림: bombExplodeMedium.1〕Devastation
〔그림: bombExplodeLarge.1〕Decimation
〔그림: bombHardener.1〕Bridge Harden
〔그림: bombHeal.1〕Heal
〔그림: bombInvisible.1〕Invisibility
〔그림: bombParalyze.1〕Paralysis
〔그림: bombTreason.1〕Treason
〔그림: bombMeteor.1〕Bombardment
〔그림: bombGraviton.1〕Graviton
〔그림: bombLightingwave.1〕ThunderStorm
〔그림: bombLightingZap.1〕Thunder Strike
〔그림: bombTwister.1〕Whirlwind
〔그림: bombIITwister.1〕Twister
〔그림: bombIIITwister.1〕Vortex
〔그림: bombIMano.1〕Hydra
〔그림: bombIIMano.1〕Hydra Wave
〔그림: bombIIIMano.1〕Hydra Flood
```

원본 연결 주소:

- `#multiplayerHelp` · [본문 이동](#topic-multiplayerhelp)
- `#bombExplodeSmallType` · [본문 이동](#topic-bombexplodesmalltype)
- `#bombExplodeMediumType` · [본문 이동](#topic-bombspecialonetype)
- `#bombExplodeLargeType` · [본문 이동](#topic-bombexplodelargetype)
- `#bombHardenerType` · [본문 이동](#topic-bombhardenertype)
- `#bombHealType` · [본문 이동](#topic-bombhealtype)
- `#bombInvisibleType` · [본문 이동](#topic-bombinvisibletype)
- `#bombParalyzeType` · [본문 이동](#topic-bombparalyzetype)
- `#bombTreasonType` · [본문 이동](#topic-bombtreasontype)
- `#bombMeteorType` · [본문 이동](#topic-bombmeteortype)
- `#bombGravitonType` · [본문 이동](#topic-bombgravitontype)
- `#bombLightingwaveType` · [본문 이동](#topic-bomblightingwavetype)
- `#bombLightingZapType` · [본문 이동](#topic-bomblightingzaptype)
- `#bombTwisterType` · [본문 이동](#topic-bombtwistertype)
- `#bombIITwisterType` · [본문 이동](#topic-bombiitwistertype)
- `#bombIIITwisterType` · [본문 이동](#topic-bombiiitwistertype)
- `#bombIManoType` · [본문 이동](#topic-bombimanotype)
- `#bombIIManoType` · [본문 이동](#topic-bombiimanotype)
- `#bombIIIManoType` · [본문 이동](#topic-bombiiimanotype)

<a id="topic-bomblightingzaptype"></a>

## 36. bombLightingZapType

원본 앵커: `bombLightingZapType` · 본문 시작: 원문 1162행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"The skies of nimbus take on a bleak blackness as every hair upon your body begins to rise. For a brief moment a bittersweet tranquility emerges… then the sky bellows out a bone shattering roar unmatched by a thousand thunder cannons while bolts from above rain down reeking havoc upon all that stands in there path…" -Fleet Admiral of Nimbus

〔그림: bombLightingZap.1〕
A Spell that deals a random amount of damage to everything in its range.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-bomblightingwavetype"></a>

## 37. bombLightingwaveType

원본 앵커: `bombLightingwaveType` · 본문 시작: 원문 1175행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Forged by the thunder furies for those pure of heart.
Comes the power to summon lightning and make it reach its mark.
Many will run and more will hide as the clouds within grow gray.
Yet amps of current shall flood there bodies and soon dead they will lay." -Cantata, 4 of the 3rd Thunder Core

〔그림: bombLightingwave.1〕
A Spell that destroys everything destructible in its range.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-bombtwistertype"></a>

## 38. bombTwisterType

원본 앵커: `bombTwisterType` · 본문 시작: 원문 1187행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Fallen Devil from above, wreaking havoc until he’s done." -DeaCon, Godly Council

〔그림: bombTwister.1〕
A Spell that summons two Dust Devils to attack enemy units.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#windFlyerType` · [본문 이동](#topic-windflyertype)

<a id="topic-bombiitwistertype"></a>

## 39. bombIITwisterType

원본 앵커: `bombIITwisterType` · 본문 시작: 원문 1196행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"In the lands of lightning and thunderous roars,
The winds of death began to soar,
And when it died back to a calm,
Not a single thing had found no harm." -DWMI, Nimbian Poet

〔그림: bombIITwister.1〕
A Spell that summons four Dust Devils to attack enemy units.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#windFlyerType` · [본문 이동](#topic-windflyertype)

<a id="topic-bombiiitwistertype"></a>

## 40. bombIIITwisterType

원본 앵커: `bombIIITwisterType` · 본문 시작: 원문 1208행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"It's not the wind that hurts... Have you ever been hit by a flying bridge? -Papers, Nimbian Safety Inspector"

〔그림: bombIIITwister.1〕
A Spell that summons six Dust Devils to attack enemy units.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#windFlyerType` · [본문 이동](#topic-windflyertype)

<a id="topic-bombimanotype"></a>

## 41. bombIManoType

원본 앵커: `bombIManoType` · 본문 시작: 원문 1217행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Often it is neccessary to purge the poison from the mind by embodying it in the form of ones' own fear." -Lord Dwia, High Priest of the XIIIBY

〔그림: bombIMano.1〕
A Spell that summons a Man o' War to attack enemy units.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#rainFlyerType` · [본문 이동](#topic-rainflyertype)

<a id="topic-bombiimanotype"></a>

## 42. bombIIManoType

원본 앵커: `bombIIManoType` · 본문 시작: 원문 1227행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Just when you think you can relax, along come the bringers of destruction." -Kenzo, Veteran Archer

〔그림: bombIIMano.1〕
A Spell that summons two Man o' War to attack enemy units
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#rainFlyerType` · [본문 이동](#topic-rainflyertype)

<a id="topic-bombiiimanotype"></a>

## 43. bombIIIManoType

원본 앵커: `bombIIIManoType` · 본문 시작: 원문 1236행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Where one is good, three are better." -Dwindlin, Nimbian Mathematician

〔그림: bombIIIMano.1〕
A Spell that summons three Man o' War to attack enemy units.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#rainFlyerType` · [본문 이동](#topic-rainflyertype)

<a id="topic-bombgravitontype"></a>

## 44. bombGravitonType

원본 앵커: `bombGravitonType` · 본문 시작: 원문 1245행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Apparatuses with flying capabilities were designed to work under a specific gravitational constant; even a slight change in this constant will result in a catastrophic failure." -Ventusmori, Nimbian Engineer

〔그림: bombGraviton.1〕
A Spell that destroys all air units in its range.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-bombmeteortype"></a>

## 45. bombMeteorType

원본 앵커: `bombMeteorType` · 본문 시작: 원문 1256행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Rain from the sky has no allies." -UnSpokenOne, Wise Nimbian

〔그림: bombMeteor.1〕
A Spell that deals a random amount of damage to everything in its range.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-bombexplodesmalltype"></a>

## 46. bombExplodeSmallType

원본 앵커: `bombExplodeSmallType` · 본문 시작: 원문 1265행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"I don't think 'cute' is the word you're looking for." -Ervin Grots, Apprentice

〔그림: bombExplodeSmall.1〕
A Spell that destroys everything destructible in its range.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-bombspecialonetype"></a>

## 47. bombSpecialOneType

원본 앵커: `bombExplodeMediumType / bombSpecialOneType` · 본문 시작: 원문 1275행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Ideal for day-to-day pandemonium." -Telsen Rott,

〔그림: bombExplodeMedium.1〕
A Spell that destroys anything destructible in its range.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-bombexplodelargetype"></a>

## 48. bombExplodeLargeType

원본 앵커: `bombExplodeLargeType` · 본문 시작: 원문 1285행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Run. Faster." -Captain Knut Hamsun, Nimbian casualty.

〔그림: bombExplodeLarge.1〕
A Spell that destroys anything destructible in its range.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-bombhardenertype"></a>

## 49. bombHardenerType

원본 앵커: `bombHardenerType` · 본문 시작: 원문 1294행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"The mortar itself takes on a hellish willfulness." -Hule Gundersun, Master Mason

〔그림: bombHardener.1〕
A Spell that makes bridges indestructible in its range.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#bridgeType` · [본문 이동](#topic-bridgetype)

<a id="topic-bombhealtype"></a>

## 50. bombHealType

원본 앵커: `bombHealType` · 본문 시작: 원문 1303행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"And your illness was illusion.
And your fever was a dream." -Book of Nimbus

〔그림: bombHeal.1〕
A Spell that that restores all units in its range with some of their missing hit points. It also heals Paralysis, and, because of the iridescence of its healing magic, it negates Invisibility.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#bombParalyzeType` · [본문 이동](#topic-bombparalyzetype)
- `#bombInvisibleType` · [본문 이동](#topic-bombinvisibletype)

<a id="topic-bombinvisibletype"></a>

## 51. bombInvisibleType

원본 앵커: `bombInvisibleType` · 본문 시작: 원문 1315행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"It's not that you do not see, but merely that your eyes do not perceive." -Luffor Haginit, Nimbian Logician

〔그림: bombInvisible.1〕
A Spell that temporarily turns all units invisible in its range. Note that nothing can be targeted or picked up while it is invisible. Invisible blockers will still block, but anything else previously targeted becomes un-targeted when it becomes invisible.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-bombparalyzetype"></a>

## 52. bombParalyzeType

원본 앵커: `bombParalyzeType` · 본문 시작: 원문 1327행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"A stalemate with the ether itself." -Dag Haverlink, Nimbian Gamesman

〔그림: bombParalyze.1〕
A Spell that stops all movement and shooting in its range. Any affected Transports will not be able to cast their Spells. Paralyzed units can be distinguished by the Spell icon next to them.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#TransportHelp` · [본문 이동](#topic-transporthelp)

<a id="topic-bombtreasontype"></a>

## 53. bombTreasonType

원본 앵커: `bombTreasonType` · 본문 시작: 원문 1338행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"The most delightful victory over any foe is to persuade his right hand to hack off his left." -General Greig Hamlinken

〔그림: bombTreason.1〕
A Spell that gives you the ownership of all units in its range, including bridges and all non-Priest Transports.
```

원본 연결 주소:

- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#bridgeType` · [본문 이동](#topic-bridgetype)
- `#PriestType` · [본문 이동](#topic-priesttype)
- `#transportHelp` · [본문 이동](#topic-transporthelp)

<a id="topic-residencetype"></a>

## 54. residenceType

원본 앵커: `residenceType` · 본문 시작: 원문 1348행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"I wouldn't mind living above ground, but battle crossfire tends to burn your house down." -Damon Runyon, regular guy

As a Temple is built, the alignment of that Temple grows into the Residence.

Nimbians live in submerged houses below the chaos of battle. You cannot move or destroy a residence. And you can't build on top of it.
```

원본 연결 주소:

- `#vortexHelp` · [본문 이동](#topic-vortexhelp)

<a id="topic-influencehelp"></a>

## 55. Energy

원본 앵커: `influenceHelp` · 본문 시작: 원문 1360행.

```text
Energy

Energy is what helps you build units in battle. There are three types of Energy provided by Temples and "Generators":
〔그림: mana.8〕 - Wind Energy from 〔그림: windvortex.*〕 and 〔그림: windbattery.*〕
〔그림: mana.9〕 - Rain Energy from 〔그림: rainvortex.*〕 and 〔그림: rainbattery.*〕
〔그림: mana.10〕 - Thunder Energy from 〔그림: thundervortex.*〕 and 〔그림: thunderbattery.*〕

Building units using Energy

Icons next to each thing you build tell you what Energy is needed to complete the structure. For example,
〔그림: mana.2〕〔그림: thunderCannon.0〕

tells you that this Thunder Cannon will need one Thunder Energy - that is, a Thunder Temple or a Thunder Generator must be nearby to satisfy the Cannon's Energy requirement.

When you see that a unit requires Sun Energy - 〔그림: mana.3〕 - it means that any sort of Energy will satisfy its requirements. For example,
〔그림: mana.3〕〔그림: sunCannon.0〕

tells you that this Sun Cannon will accept one of any Energy, be it Rain, Wind, or Thunder.

Left-click on Temples and Generators to see the Energy they provide.
```

원본 연결 주소:

- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)
- `#thunderbatteryType` · [본문 이동](#topic-thunderbatterytype)

<a id="topic-thunderfactorytype"></a>

## 56. thunderFactoryType

원본 앵커: `factoryHelp / sunFactoryType / windFactoryType / rainFactoryType / thunderFactoryType` · 본문 시작: 원문 1395행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"The Nimbian aptitude for complex mechanics is simply unsurpassed." - Nimbian Mechanic

Workshops build your units for battle.

You'll build Workshops in battle to construct combat units based on the Knowledge that you've gained from Sacrificing High Priests. This Knowledge carries over from battle to battle so that your diversity and power of units can increase over time.

Each Workshop can build only a few units so choose carefully!

On a new, Level I Workshop you'll have two "Production Slots." When you choose a new Knowledge to build, you are filling up one of the slots. Once you've chosen what a Workshop will build, you can't change it.

But, you get one additional slot each time you Upgrade a Workshop. In order to Upgrade you must pay a certain amount of Storm Power.

When a Workshop creates a unit, a Storm Power Stream streaks from the Workshop to the build site. When it combines with the proper Energy the unit will be created on the spot. However, if the Workshop is destroyed you will lose the ability to build anything that Workshop was making until you build another Workshop to replace it.
```

원본 연결 주소:

- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#upgradeHelp` · [본문 이동](#topic-upgradehelp)
- `#moneyHelp` · [본문 이동](#topic-stormpowerhelp)
- `#influenceHelp` · [본문 이동](#topic-influencehelp)

<a id="topic-upgradehelp"></a>

## 57. Upgrading Workshops

원본 앵커: `upgradeHelp` · 본문 시작: 원문 1421행.

```text
Upgrading Workshops

"More power, more speed. Absolute power, speed of light!" - Spen Lodel, Sun Disciple

Upgrade a Workshop to build new types of units for battle.

To get more Production Slots in a Workshop you may Upgrade it by paying Storm Power. Right-click on the Workshop to learn how much Storm Power it requires for the Upgrade.

You may only Upgrade each Workshop three times.
```

원본 연결 주소:

- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#moneyHelp` · [본문 이동](#topic-stormpowerhelp)
- `#factoryHelp` · [본문 이동](#topic-thunderfactorytype)

<a id="topic-technologyhelp"></a>

## 58. Knowledge

원본 앵커: `researchHelp / technologyHelp` · 본문 시작: 원문 1437행.

```text
Knowledge

"You can read their bleeding guts like a draftsman's blueprint."
- Haldor Hurlmen, Nimbian High Priest and Mechanical Engineer

Knowledge gives you the ability to build units in Battle. In Multiplayer mode, your accumulated Knowledge persists from each on-line battle to the next.

Once you've acquired Knowledge by Sacrificing High Priests, you can put that Knowledge into production in any Workshop of the appropriate Fury alignment.

From the outset, you are granted the Knowledge for the Disc Throwers and Sun Cannons which you may then put into production with your first Workshop. Everything else will cost you the Sacrifice of some number of Priests.

Once you've killed some Priests in exchange for the Knowledge you want, that Knowledge will be available to you when you build the appropriate Workshop.

For instance, I kill three Priests in exchange for the ability to build Thunder Cannons. Now when I build a Thunder Workshop, I can choose to fill one of my slots with Thunder Cannons.

But if I build a Wind Workshop, I won't be able to build my Thunder Cannons there. Each Workshop can only build the units corresponding to its alignment: Sun units in Sun Workshops, Thunder units in Thunder Workshops, and so on.

If you have exhausted all of the available slots for a Workshop, it's time to Upgrade, which will increase the number of slots available for additional Knowledge or time to build an additional Workshop.

Learn more about Workshops...

Learn more about Sacrificing Priests for new Knowledge...
```

원본 연결 주소:

- `#unitHelp` · [본문 이동](#topic-unithelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `#factoryHelp` · [본문 이동](#topic-thunderfactorytype)
- `#themeHelp` · [본문 이동](#topic-themehelp)
- `#sunarcherType` · [본문 이동](#topic-sunarchertype)
- `#suncannontype` · [본문 이동](#topic-suncannontype)
- `#upgradeHelp` · [본문 이동](#topic-upgradehelp)

<a id="topic-emptygeysertype"></a>

## 59. emptyGeyserType

원본 앵커: `geyserType / emptyGeyserType` · 본문 시작: 원문 1476행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The coagulation of the Geyser into Crystal form is like watching white fire turn to ice.

Storm Geysers provide you with Storm Power in the form of Storm Crystals which your Transports can collect for you. Right-clicking on a Storm Geyser will show the available Storm Power.

Several Geyser islands appear at the opening of each battle, and more will be generated throughout the battle as Storm Power continues to be hurled up from the Fury battle below. Every time something is destroyed, part of its energy falls into this battle of the Furies, and reappears as Geyser islands.

If you send a Ground or Aerial Transport out to a Geyser, the Transport will set itself into a loop collecting Storm Crystals until the Geyser is depleted. As a Storm Geyser begins to deplete, the steam spewing will shorten.
```

원본 연결 주소:

- `#stormpowerHelp` · [본문 이동](#topic-stormpowerhelp)
- `#nuggetType` · [본문 이동](#topic-nuggettype)
- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#themeHelp` · [본문 이동](#topic-themehelp)
- `#TransportHelp` · [본문 이동](#topic-transporthelp)

<a id="topic-nuggettype"></a>

## 60. nuggetType

원본 앵커: `nuggetType` · 본문 시작: 원문 1495행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The coagulation of the Geyser into Crystal form is like watching white fire turn to ice.

Storm Crystals are condensed Storm Power hurled up by Storm Geysers.

If you send a Ground or Aerial Transport out to a Geyser, it will set itself into a loop collecting Storm Crystals until the Geyser is depleted.
```

원본 연결 주소:

- `#stormpowerHelp` · [본문 이동](#topic-stormpowerhelp)
- `#geyserType` · [본문 이동](#topic-emptygeysertype)
- `#TransportHelp` · [본문 이동](#topic-transporthelp)

<a id="topic-sunarchertype"></a>

## 61. sunArcherType

원본 앵커: `sunArcherType` · 본문 시작: 원문 1506행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Sun Disc Throwers decapitate the clumsy." -Major Tomas Rultoff.

The Sun Disc Thrower fires in any direction at short range. It is crucial for defense against air attack.

A massive rotor arm flings the exploding saucers with perfect precision.
```

<a id="topic-sunaviarytype"></a>

## 62. sunAviaryType

원본 앵커: `sunAviaryType` · 본문 시작: 원문 1515행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

Only a roof of gold could contain the blinding light of the Whirligig's creation.

The Whirlibase builds and refuels Whirligigs--airborne attackers.

If you destroy it, its Whirligig will have no place to refuel. Each time a Whirligig is destroyed, its home base will make a new one.

Note: Nothing can ever be targeted by more than three Aerial Attackers at once.
```

원본 연결 주소:

- `#sunFlyerType` · [본문 이동](#topic-sunflyertype)

<a id="topic-sunflyertype"></a>

## 63. sunFlyerType

원본 앵커: `sunFlyerType` · 본문 시작: 원문 1527행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"This device is lofted on its own impossibility, and so it destroys by the powers of negation." -Jakob Hammersholt, inventor of the Whirligig.

The Whirligigs fly into enemy territory, wreaking havoc on enemy units from above.

Generated by Whirlibases, they go from target to target until something knocks them out of the sky, but they need to refuel once every minute. A Whirligig cannot be used for cargo.

Rule to remember: A Whirligig will never target a Transport.

Note: Nothing can ever be targeted by more than three Aerial Attackers at once, and Ground Transports are only targeted by one Aerial Attacker at a time.
```

원본 연결 주소:

- `#sunAviaryType` · [본문 이동](#topic-sunaviarytype)
- `#transporthelp` · [본문 이동](#topic-transporthelp)

<a id="topic-sunblockertype"></a>

## 64. sunBlockerType

원본 앵커: `sunBlockerType` · 본문 시작: 원문 1543행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

Stone Towers are built from the outside in. Their builders give their strength and courage to the Tower, so much so that in every third layer of bricks is the body of a brave man.

The Stone Tower is a damage-absorber. Set it between a valuable unit and hostile fire to block incoming shots.
```

<a id="topic-suncannontype"></a>

## 65. sunCannonType

원본 앵커: `sunCannonType` · 본문 시작: 원문 1551행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Two things in life cannot be ignored: one is a cannon, the other is a cannon-ball." -Commander Petrik 'Bombard' Lombard

The Sun Cannon shoots straight north, south, east, or west.

It inflicts more damage per shot than the Sun Disc Thrower, and it can suffer more damage than a Sun Disc Thrower, too, but fires less often. The Cannon will rotate itself to target an enemy.
```

원본 연결 주소:

- `#shooterHelp` · [본문 이동](#topic-shooterhelp)
- `#sunArcherType` · [본문 이동](#topic-sunarchertype)

<a id="topic-sunballoontype"></a>

## 66. sunBalloonType

원본 앵커: `sunBalloonType` · 본문 시작: 원문 1564행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Step 1: Find the unpunctured hide of a young Air Whale...."
from Balloon Building by Halldor Laxness

The Balloon is an Aerial Transport ideal for gathering Storm Power, for capturing High Priests, and for collecting and casting Spells.

Because the Balloon is airborne, it doesn't need bridges to move around, but be careful: a few enemy hits will pop it like a soap bubble.

Remember: Transports cannot carry both a Priest and a Storm Crystal at the same time.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#stormpowerhelp` · [본문 이동](#topic-stormpowerhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#bridgeType` · [본문 이동](#topic-bridgetype)

<a id="topic-sunfencetype"></a>

## 67. sunFenceType

원본 앵커: `sunFenceType` · 본문 시작: 원문 1579행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Between Nimbus and the Beyond stands an infinite, burning barrier." -Book of Nimbus.

The Barricade keeps out enemy fire.

If you set up two Barricade posts along straight horizontal or vertical lines, you'll create a force field between them that keeps out troublesome enemy fire, but lets your own Shooters fire right through. The fence of the Sun Barricades will represent the color of your home island.

Your allies can connect their Sun Barricade with yours and vice-versa. Also, placing a Sun Barricade of your own in between an Enemy Sun Barricade's fence will deactivate them.
```

원본 연결 주소:

- `#shooterHelp` · [본문 이동](#topic-shooterhelp)

<a id="topic-sunwalkertype"></a>

## 68. sunWalkerType

원본 앵커: `sunWalkerType` · 본문 시작: 원문 1594행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Golems are dumb, clumsy, humorless, and they smell bad.
And in wartime, they are the best friends you'll ever have." -Major Tane Russo

The Golem is a Ground Transport which carries cargo. He's ideal for gathering Storm Power, for capturing High Priests, and for collecting and casting Spells.

A supernatural servant generated by the Temple for your aid. A personal gift from the Furies.

Be aware: Golem are dumb, and they will walk straight off a bridge if it is broken. All other Transports stop before falling to their deaths.

Remember: Transports cannot carry both a Priest and a Storm Crystal at the same time.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#stormpowerhelp` · [본문 이동](#topic-stormpowerhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#themeHelp` · [본문 이동](#topic-themehelp)
- `#sunWalkerType` · [본문 이동](#topic-sunwalkertype)

<a id="topic-windarchertype"></a>

## 69. windArcherType

원본 앵커: `windArcherType` · 본문 시작: 원문 1611행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Bolts should weigh a hundred pounds and be well-balanced. You will know your bowstring has perfect tension when you hear the Nimbian winds resonate it at F-sharp above Middle C." from The Sixth Archer's Handbook

The Crossbow is a self-aiming Shooter.

As you're preparing to build it, right-click to aim it in one of four directions. The Crossbow will target the closest unit in an arc. Crucial to fending off attacks by air, the Crossbow packs a precise wallop.
```

원본 연결 주소:

- `#shooterHelp` · [본문 이동](#topic-shooterhelp)

<a id="topic-windaviarytype"></a>

## 70. windAviaryType

원본 앵커: `windAviaryType` · 본문 시작: 원문 1624행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Another impasse in the Devil Maker experiments: today, it counter-spun the entire island like a top, flinging pets and small children over the edge and into the abyss. I must reconsider the design." -journal entry, Dr. Dere Dulkus.

The Devil Maker generates ornery Dust Devils--Aerial Attackers.

If you destroy it, the Dust Devils wind down. Each time a Dust Devil is destroyed, its home base will make a new one.

Note: Nothing can ever be targeted by more than three Aerial Attackers at once.
```

원본 연결 주소:

- `#windFlyerType` · [본문 이동](#topic-windflyertype)

<a id="topic-windflyertype"></a>

## 71. windFlyerType

원본 앵커: `windFlyerType` · 본문 시작: 원문 1638행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Amid the howling of a Dust Devil, a doomed man hears it scream his name." -Nimbian proverb

The Dust Devils fly into enemy territory, wreaking havoc from above.

It cracks bridge when it passes. The Dust Devil always lasts for twenty seconds, and regenerates from its Devil Maker every thirty-two seconds. It cannot be used for cargo.

Note: Nothing can ever be targeted by more than three Aerial Attackers at once, and Ground Transports are only targeted by one Aerial Attacker at a time.
```

원본 연결 주소:

- `#bridgeType` · [본문 이동](#topic-bridgetype)
- `#windAviaryType` · [본문 이동](#topic-windaviarytype)
- `#transportHelp` · [본문 이동](#topic-transporthelp)

<a id="topic-windbatterytype"></a>

## 72. windBatteryType

원본 앵커: `windBatteryType` · 본문 시작: 원문 1652행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"With a hundred Wind Generators, you could power a hurricane." -Def Ilderless, Nimbian sage.

A Wind Generator spreads Wind Energy in a radius around it.

The symbol for Wind Energy is 〔그림: mana.8〕.

Left-click to see the Energy it produces.

You'll need Wind Generators to build Wind-aligned units in battle.

In Multiplayer mode you can cause it to Meltdown to slightly damage enemy units or break through bridges by right-clicking the unit and selecting Meltdown.
```

원본 연결 주소:

- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)

<a id="topic-windblockertype"></a>

## 73. windBlockerType

원본 앵커: `windBlockerType` · 본문 시작: 원문 1669행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The universe is ninety-nine percent sky and one percent island.
In the sky, the Wind rules.

The Wind Tower is a damage absorber. Place it between a valuable unit and enemy fire. The attacking unit will retarget to attack the Wind Tower.

On three sides it takes normal damage, but on its curved back it is utterly invulnerable. It is still vulnerable to air assault.

As you're preparing to build it, right-click to face the invulnerable side north, south, east or west.
```

<a id="topic-windballoontype"></a>

## 74. windBalloonType

원본 앵커: `windBalloonType` · 본문 시작: 원문 1682행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"In Nimbus there is nothing more graceful or fine." -Gilden Fraj, Nimbian Poet

The Air Ship is an Aerial Transport ideal for gathering Storm Power, for capturing High Priests, and for collecting and casting Spells.

Compared to the Balloon, it is much sturdier, and faster.

Because the Air Ship is airborne, it doesn't need bridges to move around.

Remember: Transports cannot carry both a Priest and a Storm Crystal at the same time.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#stormpowerhelp` · [본문 이동](#topic-stormpowerhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#sunballoontype` · [본문 이동](#topic-sunballoontype)
- `#bridgeType` · [본문 이동](#topic-bridgetype)

<a id="topic-windwalkertype"></a>

## 75. windWalkerType

원본 앵커: `windWalkerType` · 본문 시작: 원문 1699행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The best pilots know the breezes by the names they call each other.

The Sail Skater is a Ground Transport ideal for gathering Storm Power, for capturing High Priests, and for collecting and casting Spells.

The Sail Skater is the speediest of the Ground Transports.

Remember: Transports cannot carry both a Priest and a Storm Crystal at the same time.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#stormpowerhelp` · [본문 이동](#topic-stormpowerhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-thunderarchertype"></a>

## 76. thunderArcherType

원본 앵커: `thunderArcherType` · 본문 시작: 원문 1712행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

If you throw a raw Bulf egg into the air in front of a Vander Tower, it'll hit the ground hard-boiled.

The Vander Tower is a medium range Shooter that only targets airborne attackers or airborne Transports.

The Vander Tower builds a charge and then releases it in a shock of violent energy. It fires in any direction.
```

원본 연결 주소:

- `#shooterHelp` · [본문 이동](#topic-shooterhelp)
- `#transportHelp` · [본문 이동](#topic-transporthelp)

<a id="topic-thunderbatterytype"></a>

## 77. thunderBatteryType

원본 앵커: `thunderBatteryType` · 본문 시작: 원문 1723행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"As night fell around us
We tended to our fallen cohorts
By the strobing Generator's glow."
-from The Anthem of Thunder Ridge

A Thunder Generator spreads Thunder Energy in a radius around it.

The symbol for Thunder Energy is 〔그림: mana.10〕.

Left-click to see the Energy it produces.

You'll need Thunder Generators to build Thunder-aligned units in battle.

In Multiplayer mode you can cause it to Meltdown to slightly damage enemy units or break through bridges by right-clicking the unit and selecting Meltdown.
```

원본 연결 주소:

- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)

<a id="topic-thunderblockertype"></a>

## 78. thunderBlockerType

원본 앵커: `thunderBlockerType` · 본문 시작: 원문 1743행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"How is a Bulwark like your mother?" -last words of Bernie Lovell, Nimbian comic.

The Bulwark is a damage-absorber. Set it between a valuable unit and hostile fire. The attacking unit will retarget to attack the Bulwark.

The Bulwark is the toughest building in Nimbus. It is utterly invulnerable to air assault, and very sturdy against ground fire. The Bulwark sucks up damage like a sponge, but it is not cheap.
```

<a id="topic-thundercannontype"></a>

## 79. thunderCannonType

원본 앵커: `thunderCannonType` · 본문 시작: 원문 1755행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Expect to go permanently deaf." - first line of The Tenth Thunder Artillery Handbook

The Thunder Cannon is the meanest Shooter in Nimbus: long-range, inflicts heavy damage, and can take a beating.

It doesn't fire as often as other Shooters, and it can only fire north, south, east, or west, and you must orient it in the direction you wish it to fire.

As you're preparing to build it, right-click to rotate the unit.
```

원본 연결 주소:

- `#shooterHelp` · [본문 이동](#topic-shooterhelp)

<a id="topic-thunderfencetype"></a>

## 80. thunderFenceType

원본 앵커: `thunderFenceType` · 본문 시작: 원문 1768행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"If a captain sees an Arc Spire ahead and doesn't turn his vessel around for home, it means that he is either very brave, very stupid, or has absolutely nothing to lose." -Air Admiral Heike Kamerlingh

The Arc Spires form barricades. Set up two Spires and a deadly pulse courses between them.

The Arc Spire's charge will be drawn across any enemy unit between its poles.
```

<a id="topic-bulftype"></a>

## 81. bulfType

원본 앵커: `thunderWalkerType / bulfType` · 본문 시작: 원문 1779행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"In its natural habitat the Bulf grows no larger than a cannonball, but on a strict diet of nickel and limestone, he explodes to a hundred times that size." -field journal, J. T. Kierken

The Bulf is a Ground Transport ideal for gathering Storm Power, for capturing High Priests, and for collecting and casting Spells.

It is the toughest Transport in Nimbus, but quite slow.

Remember: Transports cannot carry both a Priest and a Storm Crystal at the same time.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#stormpowerhelp` · [본문 이동](#topic-stormpowerhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-rainaviarytype"></a>

## 82. rainAviaryType

원본 앵커: `rainAviaryType` · 본문 시작: 원문 1794행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The recipe is a very ancient one, primarily requiring fermented rain and poison spores. No stirring is required.

The Man o' War Pool cranks out Man o' War--Aerial Attackers.

Each time a Man o' War is destroyed, its home Pool will make a new one.

Note: Nothing can ever be targeted by more than three Aerial Attackers at once.
```

원본 연결 주소:

- `#rainFlyerType` · [본문 이동](#topic-rainflyertype)

<a id="topic-rainflyertype"></a>

## 83. rainFlyerType

원본 앵커: `rainFlyerType` · 본문 시작: 원문 1806행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"I train my men to slaughter each other at the very sight of it." -Deisen Trow, Leader of the Band of Seerin

Generated by Man o' War Pool, the Man o' War flies into enemy territory, wreaking havoc from above.

Each one lives for one minute targeting any ground units. If it kills something other than a shooter the Storm Power released nourishes the Man o' War and will continue on with an additional fifteen seconds added to its remaining life span.

The Man o' War prefers to attack Ground Transports, but if none are available it will strike at the nearest unit.

Each Man o' War is about twice as tough as a Balloon. It cannot be used for cargo.

Note: Nothing can ever be targeted by more than three Aerial Attackers at once, and Ground Transports are only targeted by one Aerial Attacker at a time.

The life span can not exceed one minute, but each unit that is killed other than a shooter will add fifteen seconds untill the full minute is restored.
```

원본 연결 주소:

- `#rainAviaryType` · [본문 이동](#topic-rainaviarytype)
- `#shooterHelp` · [본문 이동](#topic-shooterhelp)
- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#sunballoontype` · [본문 이동](#topic-sunballoontype)

<a id="topic-rainbatterytype"></a>

## 84. rainBatteryType

원본 앵커: `rainBatteryType` · 본문 시작: 원문 1827행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"In the Beyond, there is one sound that is always present: the faraway music of falling water." -Book of Nimbus

A Rain Generator spreads Rain Energy in a radius around it.

The symbol for Rain Energy is 〔그림: mana.9〕.

Left-click to see the Energy it produces.

You'll need Rain Generators to build Rain-aligned units in battle.

In Multiplayer mode you can cause it to Meltdown to slightly damage enemy units or break through bridges by right-clicking the unit and selecting Meltdown.
```

원본 연결 주소:

- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)

<a id="topic-rainblockertype"></a>

## 85. rainBlockerType

원본 앵커: `rainBlockerType` · 본문 시작: 원문 1845행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The first step to getting water to flow uphill, is enticing ice to melt backwards.

The Ice Tower is a damage absorber. Set it between a valuable unit and hostile fire. The attacking unit will retarget to attack the Ice Tower.

The Ice Tower is weaker than other damage absorber, such as the Bulwark and Stone Tower, but it regenerates by itself if destroyed.

If built on a floating island, an Offensive Spell can destroy its island and kill the Ice Tower permanently.
```

원본 연결 주소:

- `#thunderBlockerType` · [본문 이동](#topic-thunderblockertype)
- `#sunBlockerType` · [본문 이동](#topic-sunblockertype)
- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-growingrainblockertype"></a>

## 86. growingrainBlockerType

원본 앵커: `growingrainBlockerType` · 본문 시작: 원문 1859행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The first step to getting water to flow uphill, is enticing ice to melt backwards.

The Ice Tower is weaker than other damage absorber, such as the Bulwark and Stone Tower, but it regenerates by itself if destroyed.

As a damage absorber, the Ice Tower is relatively weak, but it regenerates by itself if destroyed.

If built on a floating island, an Offensive Spell can destroy its island and kill the Ice Tower permanently.
```

원본 연결 주소:

- `#thunderBlockerType` · [본문 이동](#topic-thunderblockertype)
- `#sunBlockerType` · [본문 이동](#topic-sunblockertype)
- `#buriedType` · [본문 이동](#topic-bombhelp)

<a id="topic-raincannontype"></a>

## 87. rainCannonType

원본 앵커: `rainCannonType` · 본문 시작: 원문 1872행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

Like a friendly snow-ball fight, with razor-sharp icicles.

The Ice Cannon is a Shooter, ideal for targeting clustered enemy ground units.

It inflicts light damage. You must orient it in the direction you wish it to shoot. But on the upside, its artillery fire splinters into shrapnel on impact, damaging other nearby targets: strategize appropriately.

As you're preparing to build it, right-click to rotate the Ice Cannon north, south, east or west.
```

원본 연결 주소:

- `#shooterHelp` · [본문 이동](#topic-shooterhelp)

<a id="topic-rainballoontype"></a>

## 88. rainBalloonType

원본 앵커: `rainBalloonType` · 본문 시작: 원문 1885행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

Every year there is a grueling contest among the thickest rain clouds for the honor of powering the Floaters. Most dissipate. A handful survive.

The Cloud Floater is an Aerial Transport ideal for gathering Storm Power, for capturing High Priests, and for collecting and casting Spells.

The Floater is quite sturdy, but it is expensive. The Cloud Floater is very difficult to hit due to its insubstantial nature, and enemy shots seldom actually hit it.

Because it is airborne, it doesn't need bridges to move around.

Remember: Transports cannot carry both a Priest and a Storm Crystal at the same time.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#stormpowerhelp` · [본문 이동](#topic-stormpowerhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#bridgeType` · [본문 이동](#topic-bridgetype)

<a id="topic-rainfencetype"></a>

## 89. rainFenceType

원본 앵커: `rainFenceType` · 본문 시작: 원문 1903행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Even water can sometimes burn like fire." -Book of Nimbus

The Acid Barricade provides the tightest protection of any barricade.

If you line up two Acid Barricade posts along straight horizontal or vertical lines, an acidic barrier is created. The Barricade dissolves every enemy unit that comes between the posts.

The Acid Barricade does not protect against enemy fire.
```

<a id="topic-rainwalkertype"></a>

## 90. rainWalkerType

원본 앵커: `rainWalkerType` · 본문 시작: 원문 1916행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"Perhaps it is merely a machine, but it's a mean one." -Bertram Freilig, first Nimbian run over by a Crystal Crab

The Crystal Crab is a Ground Transport ideal for gathering Storm Power, for capturing High Priests, and for collecting and casting Spells.

It is sturdy, and quick. If it encounters another Transport other than a fellow Crab, it will stun it, and steal what it's carrying, whether Priest or Storm Crystal.

It is also smart enough to move to a safer location if the bridge underneath it becomes dangerous.

Remember: Transports cannot carry both a Priest and a Storm Crystal at the same time.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#stormpowerhelp` · [본문 이동](#topic-stormpowerhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#bridgeType` · [본문 이동](#topic-bridgetype)

<a id="topic-transporthelp"></a>

## 91. Transports

원본 앵커: `transportHelp` · 본문 시작: 원문 1933행.

```text
Transports

Any cargo-bearing unit, can be Aerial or Ground.

Moving the Transport

Left-click on a Transport and as you move the cursor around the screen you'll see a cross-hairs any place that's legal to move, and a no-circle any place that's illegal. Left-click someplace legal, and the Transport will move there.

Picking up Cargo

If you left-click your Transport and then move your cursor over an object that you can pick up, your cursor becomes a hand. If you left-click on that object, your Transport will go pick it up for you.

You can pick up Storm Crystals, High Priests, and Spells in the form of Obelisks.

Remember: Transports cannot carry both a Priest and a Storm Crystal at the same time. And a Priest cannot carry another Priest.

Dropping Cargo

Right-click a cargo-carrying Transport, and choose "Drop ...". This does not apply to Spells, which cannot be dropped.

Casting Spells

Once a Transport has acquired a Spell, it can cast the Spell an unlimited number of times, provided the player has sufficient Storm Power. The Transport keeps that Spell until the Transport is destroyed, or until it acquires another Spell, supplanting the previous one. Now when you right-click on your Transport, you'll be offered the option to cast the Spell for the indicated amount of Storm Power.

General

Transports can carry only one Spell at a time.

Ground Transports will only attract one Aerial Attacker at a time-- Buildings and other units attract three at a time.

Golem are dumb, and they will walk straight off a bridge if it is broken. All other Transports stop before falling to their deaths.

If a Priest or a Crystal Crab is standing on a dangerous bridge, it will try to move automatically to a safer position.

A Rule to Remember:
You can't carry your own High Priests in your own Transports.
```

원본 연결 주소:

- `unitHelp` · [본문 이동](#topic-unithelp)
- `#nuggetType` · [본문 이동](#topic-nuggettype)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#buriedType` · [본문 이동](#topic-bombhelp)
- `#sunWalkerType` · [본문 이동](#topic-sunwalkertype)
- `#rainWalkerType` · [본문 이동](#topic-rainwalkertype)

<a id="topic-shooterhelp"></a>

## 92. Shooters

원본 앵커: `shooterHelp` · 본문 시작: 원문 1986행.

```text
Shooters

Shooters in Nimbus automatically target the closest enemy unit so you never have to tell them what to attack.

Left-click on a Shooter to see its attack range.

Remember that Shooters that fire in straight lines will target Ground or Aerial Transports but often miss as the Transport passes by.
```

원본 연결 주소:

- `#transportHelp` · [본문 이동](#topic-transporthelp)

<a id="topic-themehelp-2"></a>

## 93. The Three Furies of Nimbus

원본 앵커: `themeHelp` · 본문 시작: 원문 1998행.

같은 앵커가 원본에서 여러 번 정의된다: `themeHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
The Three Furies of Nimbus

At the heart of Nimbus a constant storm rages. There the three Furies, Wind, Rain and Thunder, strive in never-ending conflict for control of the world.

This conflict tears huge chunks of the Hidden Planet up into the sky. Upon these newborn islands live the Nimbians, your people.

Death and destruction please the Furies. Thus, when you destroy an enemy unit, you are granted one quarter of the Storm Power value of that enemy unit--except for individual Aerial Attackers which do not actually cost any Storm Power to build.

Even more pleasing to the Furies is the Sacrifice of a Priest. Such Sacrifices cause them to grant the gift of battle Knowledge.

Each Fury has a symbol for its Energy:
〔그림: mana.8〕 - Wind
〔그림: mana.9〕 - Rain
〔그림: mana.10〕 - Thunder

There is also common Knowledge called "Sun," symbolized by 〔그림: mana.11〕. Sun units can use any type of Energy but are relatively weak.

Beyond Sun Knowledge each Fury can supply specialized Knowledge. This Knowledge tends to require the Energy of the particular Fury who bestowed it.

Associated with the Wind Fury are:

1〔그림: windVortex.*〕 2〔그림: windfactory.*〕 3〔그림: windbattery.*〕

1) The Temple where Wind is worshipped.
2) The Workshop producing Wind-aligned units.
3) The Wind Generator that generates Wind Energy.
4) All Wind-aligned units.

Associated with the Rain Fury are:

1〔그림: RainVortex.*〕 2〔그림: Rainfactory.*〕 3〔그림: Rainbattery.*〕

1) The Temple where Rain is worshipped.
2) The Workshop producing Rain-aligned units.
3) The Rain Generator that generates Rain Energy.
4) All Rain-aligned units.

Associated with the Thunder Fury are:

1〔그림: ThunderVortex.*〕 2〔그림: Thunderfactory.*〕 3〔그림: Thunderbattery.*〕

1) The Temple where Thunder is worshipped.
2) The Workshop producing Thunder-aligned units.
3) The Thunder Generator that generates Thunder Energy.
4) All Thunder-aligned units.

Sun, which has no Fury associated with it, is nevertheless supplied by all Furies. Thus it has:
1〔그림: sunFactory.*〕

1) A Workshop producing unaligned (Sun) units.
2) All Sun units.

Learn more about Sacrificing a Priest for new Knowledge...
```

원본 연결 주소:

- `#sphereHelp` · [본문 이동](#topic-spherehelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#windVortexType` · [본문 이동](#topic-vortexhelp)
- `#windFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#windBatteryType` · [본문 이동](#topic-windbatterytype)
- `#RainVortexType` · [본문 이동](#topic-vortexhelp)
- `#RainFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#RainBatteryType` · [본문 이동](#topic-rainbatterytype)
- `#ThunderVortexType` · [본문 이동](#topic-vortexhelp)
- `#ThunderFactoryType` · [본문 이동](#topic-thunderfactorytype)
- `#ThunderBatteryType` · [본문 이동](#topic-thunderbatterytype)
- `#SunFactoryType` · [본문 이동](#topic-thunderfactorytype)

<a id="topic-unithelp-2"></a>

## 94. Battle Units

원본 앵커: `unitHelp` · 본문 시작: 원문 2064행.

같은 앵커가 원본에서 여러 번 정의된다: `unitHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
Battle Units

Battle units are divided into four varieties: Sun, Wind, Rain and Thunder. Units may be Salvaged if necessary. There are basic Tactics that all units use for attack.

Sun Units

Level One
* Golem / Ground Transport
* Sun Disc Thrower / Shooter
* Sun Cannon / Shooter
* Stone Tower / Defense
Level Two
* Whirligig / Aerial Attack
* Balloon / Aerial Transport
Level Three
* Sun Barricade / Defense

Wind Units

Level One
* Wind Generator / Source of Energy
* Sail Skater / Ground Transport
Level Two
* Crossbow / Shooter
* Wind Tower / Defense
Level Three
* Dust Devil / Aerial Attack
* Air Ship / Aerial Transport

Rain Units

Level One
* Rain Generator / Source of Energy
* Acid barricade / Defense
Level Two
* Crystal Crab / Ground Transport
* Ice Tower / Defense
* Man o' War / Aerial Attack
Level Three
* Ice Cannon / Shooter
* Cloud Floater / Aerial Transport

Thunder Units

Level One
* Thunder Generator / Source of Energy
* Bulf / Ground Transport
Level Two
* Thunder Cannon / Shooter
* Arc Spire / Defense
* Bulwark / Defense
Level Three
* Vander Tower / Shooter

How to read unit information.
Other units and their Alignments.
```

원본 연결 주소:

- `salvageHelp` · [본문 이동](#topic-salvagehelp)
- `tacticsHelp` · [본문 이동](#topic-tacticshelp)
- `#sunWalkerType` · [본문 이동](#topic-sunwalkertype)
- `#sunArcherType` · [본문 이동](#topic-sunarchertype)
- `#sunCannonType` · [본문 이동](#topic-suncannontype)
- `#sunBlockerType` · [본문 이동](#topic-sunblockertype)
- `#sunFlyerType` · [본문 이동](#topic-sunflyertype)
- `#sunBalloonType` · [본문 이동](#topic-sunballoontype)
- `#sunFenceType` · [본문 이동](#topic-sunfencetype)
- `#windBatteryType` · [본문 이동](#topic-windbatterytype)
- `#windWalkerType` · [본문 이동](#topic-windwalkertype)
- `#windArcherType` · [본문 이동](#topic-windarchertype)
- `#windBlocker` · 외부/명령/본문 대상 미확인
- `#windFlyerType` · [본문 이동](#topic-windflyertype)
- `#windBalloonType` · [본문 이동](#topic-windballoontype)
- `#rainBatteryType` · [본문 이동](#topic-rainbatterytype)
- `#rainFenceType` · [본문 이동](#topic-rainfencetype)
- `#rainWalkerType` · [본문 이동](#topic-rainwalkertype)
- `#rainBlockerType` · [본문 이동](#topic-rainblockertype)
- `#rainFlyerType` · [본문 이동](#topic-rainflyertype)
- `#rainCannonType` · [본문 이동](#topic-raincannontype)
- `#rainBalloonType` · [본문 이동](#topic-rainballoontype)
- `#thunderBatteryType` · [본문 이동](#topic-thunderbatterytype)
- `#bulfType` · [본문 이동](#topic-bulftype)
- `#thunderCannonType` · [본문 이동](#topic-thundercannontype)
- `#thunderFence` · 외부/명령/본문 대상 미확인
- `#thunderBlockerType` · [본문 이동](#topic-thunderblockertype)
- `#thunderArcher` · 외부/명령/본문 대상 미확인
- `#statsHelp` · [본문 이동](#topic-statshelp)
- `#themeHelp` · [본문 이동](#topic-themehelp)

<a id="topic-vortexhelp-2"></a>

## 95. vortexHelp

원본 앵커: `rainVortexType / windVortexType / thunderVortexType / vortexHelp` · 본문 시작: 원문 2127행.

같은 앵커가 원본에서 여러 번 정의된다: `rainVortexType / windVortexType / thunderVortexType / vortexHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

The blood of holy men is the elixir of destiny.

Rain Temple
Wind Temple
Thunder Temple

The Temple is the ultimate center of Energy on each island. Temples take one of three forms, as listed above.

As long as your Temple is standing, your Priest will heal during battle.

Hit F4 to jump to your Temple during game play.

Find out about using the Temple for:
Acquiring Storm Power
Making Golems
Generating Energy
```

원본 연결 주소:

- `#rainVortexType` · [본문 이동](#topic-vortexhelp)
- `#windVortexType` · [본문 이동](#topic-vortexhelp)
- `#thunderVortexType` · [본문 이동](#topic-vortexhelp)
- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `#spVortexHelp` · [본문 이동](#topic-spvortexhelp)
- `#golemVortexHelp` · [본문 이동](#topic-golemvortexhelp)
- `#influenceVortexHelp` · [본문 이동](#topic-influencevortexhelp)

<a id="topic-spvortexhelp-2"></a>

## 96. The Temple:
Acquiring Storm Power

원본 앵커: `spVortexHelp` · 본문 시작: 원문 2152행.

같은 앵커가 원본에서 여러 번 정의된다: `spVortexHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
The Temple:
Acquiring Storm Power

In battle, the Temple converts Storm Power into a usable resource. Without a Temple you can't collect or process Storm Power.

Storm Power is collected from Storm Geysers in the form of Storm Crystals.

Learn more about making Golems...
```

원본 연결 주소:

- `#moneyHelp` · [본문 이동](#topic-stormpowerhelp)
- `#geyserType` · [본문 이동](#topic-emptygeysertype)
- `#nuggetType` · [본문 이동](#topic-nuggettype)
- `#golemVortexHelp` · [본문 이동](#topic-golemvortexhelp)

<a id="topic-golemvortexhelp-2"></a>

## 97. The Temple:
Making Golems

원본 앵커: `golemVortexHelp` · 본문 시작: 원문 2165행.

같은 앵커가 원본에서 여러 번 정의된다: `golemVortexHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
The Temple:
Making Golems

Temples allow you to create Golems that serve as Transports on your new island.

A Golem will show up in your Production Window when you first descend to the Pyrosphere for battle. You can make as many of these servants as you wish, provided you have enough Storm Power.

Learn more about how the Vortex generates Energy...
```

원본 연결 주소:

- `#sunWalkerType` · [본문 이동](#topic-sunwalkertype)
- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#teleportViewHelp` · [본문 이동](#topic-teleportviewhelp)
- `#pyrosphereHelp` · [본문 이동](#topic-pyrospherehelp)
- `#moneyHelp` · [본문 이동](#topic-stormpowerhelp)
- `#influenceVortexHelp` · [본문 이동](#topic-influencevortexhelp)

<a id="topic-influencevortexhelp-2"></a>

## 98. The Temple:
Generating Energy

원본 앵커: `influenceVortexHelp` · 본문 시작: 원문 2180행.

같은 앵커가 원본에서 여러 번 정의된다: `influenceVortexHelp`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
The Temple:
Generating Energy

To build anything in Nimbus, you must have the adequate Energy. Your Temple is one of the two sources for this Energy; the other source is Generators. As you prepare to build anything on your home island, whether it's a Golem, a Workshop, or units for battle, your Temple will supply the Energy you need.

Left-click on your Temple to see the range of this Energy.

Return to Temple Help.
```

원본 연결 주소:

- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)

<a id="topic-edgefarmtype-2"></a>

## 99. edgeFarmType

원본 앵커: `edgeFarmType` · 본문 시작: 원문 2192행.

같은 앵커가 원본에서 여러 번 정의된다: `edgeFarmType`. 게임의 적용 우선순위는 이 문서에서 확정하지 않는다.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

"In other cultures, the plowing is not so dangerous." -Pavel Milyukov, Nimbian farmer

No bridge may be attached where an Edge Farm grows, so enemy boarding attempts are foiled there --but this may also foil your attempts to build bridges out from your own island.

As a Temple is built, the alignment of that Temple grows into the Edge Farm itself.

The inhabitants of the islands grow their food around the island's perimeter, leaving interior space open for development.
```

원본 연결 주소:

- `#bridgeType` · [본문 이동](#topic-bridgetype)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)

<a id="topic-stormpowerhelp"></a>

## 100. Storm Power

원본 앵커: `moneyHelp / stormPowerHelp` · 본문 시작: 원문 2208행.

```text
Storm Power

Storm Power is like currency: you spend it for every unit you build--except bridges.

Your current total is displayed in the Storm Power Available Window in the upper left of the screen.

Storm Power is an unstable state of matter between Energy and Time. It is the essence of all that is created or destroyed in Nimbus.

You can use it to build nearly anything in the world of Nimbus. Remember, though, that everything also requires Energy to build.

Storm Power appears in the world as Storm Geysers: 〔그림: geyser.*〕
Left-click on one of your Transports and send it over to a Geyser. The Transport will now automatically circle between the Geyser and your Temple, mining the Storm Power in the form of Storm Crystals, until the Geyser is depleted. (Storm Crystal: 〔그림: nugget.*〕)
It will then, find the next, closest un-depleted Geyser and do the same. Watch your Storm Power Available Window crank as your Transports gather the goods!

In your left-hand Production Window, units will appear red if you do not have enough Storm Power to build them.

Destroying enemy units in battle will give you half of their Storm Power value! The other half falls into the war between the Furies below, and eventually it is lofted up again as Storm Geysers.
```

원본 연결 주소:

- `#unitHelp` · [본문 이동](#topic-unithelp)
- `#bridgeType` · [본문 이동](#topic-bridgetype)
- `#moneyGumpHelp` · [본문 이동](#topic-moneygumphelp)
- `#sphereHelp` · [본문 이동](#topic-spherehelp)
- `#influenceHelp` · [본문 이동](#topic-influencehelp)
- `#geyserType` · [본문 이동](#topic-emptygeysertype)
- `#transportHelp` · [본문 이동](#topic-transporthelp)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)
- `#nuggetType` · [본문 이동](#topic-nuggettype)
- `#teleportViewHelp` · [본문 이동](#topic-teleportviewhelp)
- `#themehelp` · [본문 이동](#topic-themehelp)

<a id="topic-outposthelp"></a>

## 101. outpostHelp

원본 앵커: `outpostType / outpostHelp` · 본문 시작: 원문 2243행.

```text
〔게임이 타입별 그림·능력치 머리를 덧붙이는 페이지〕

You can almost feel the power as it takes over the island! - Sar Quinset

Outposts give you control over an island in Multiplayer play.

Use your Priest to create Outposts on islands in multiplayer games and take them over. When an Outpost is created you gain four benefits:

1. You may build bridges off of the island containing the Outpost.
2. The island is warded against enemy construction. No enemy may build on it.
3. Transports may return Storm Crystals to the Outpost to turn them to Storm Power.
4. Units can be teleported directly from an Outpost, rather than having to travel the entire distance from your Workshops.

The rule for island domination is: The player with the most Outposts on an island dominates. If two players build simultaneously, the island may still remain unowned until one player creates another Outpost there.

Outposts are only available in multiplayer games.

Note: The island you build an outpost on will reflect the theme of your Temple.
```

원본 연결 주소:

- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)
- `#priestType` · [본문 이동](#topic-priesttype)
- `multiplayerHelp` · [본문 이동](#topic-multiplayerhelp)
- `#vortexHelp` · [본문 이동](#topic-vortexhelp)

<a id="topic-multiplayerhelp"></a>

## 102. Campaign vs. Multiplayer Mode

원본 앵커: `campaignHelp / multiplayerHelp` · 본문 시작: 원문 2272행.

```text
Campaign vs. Multiplayer Mode

NetStorm has a number of Campaign missions for you to play, as well as a Multiplayer mode which you can play indefinitely. If you play the "Early Missions" you will be instructed in the workings of the game.

Campaign

In the Campaign you begin each mission with a specific group of Knowledge. In order to win you must usually Sacrifice the enemy's High Priest to the Furies. This action vanquishes your enemy and allows you to advance to the next mission.

Multiplayer

In Multiplayer mode things are slightly different. Your Nimbians start with a simple Knowledge Profile. Each time you Sacrifice an enemy Priest you will be able to gain new Knowledge to improve your fighting ability.

Multiplayer maps are configured differently from those in the Campaign. You'll see that there are many small islands to control as you attempt to defeat your enemy. The Outpost (which does not appear in the Original Campaigns) is instrumental in winning Multiplayer games.

The Altar also functions differently and you should learn about Zones and being a Battle Master. If you want to chat with other players you might be interested in the .format command.
```

원본 연결 주소:

- `#priestType` · [본문 이동](#topic-priesttype)
- `themeHelp` · [본문 이동](#topic-themehelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#outpostType` · [본문 이동](#topic-outposthelp)
- `#altarType` · [본문 이동](#topic-altartype)
- `#challengeIslandType` · [본문 이동](#topic-challengeislandtype)
- `#battlemasterHelp` · [본문 이동](#topic-battlemasterhelp)
- `#chatViewHelp` · [본문 이동](#topic-chatviewhelp)
- `#formatCommandHelp` · [본문 이동](#topic-formatcommandhelp)

<a id="topic-multiquickhelp"></a>

## 103. Multiplayer Quick Start

원본 앵커: `multiQuickHelp` · 본문 시작: 원문 2306행.

```text
Multiplayer Quick Start

Want to make sure a super-experienced guy doesn't kick your butt? Check out the Multiplayer Survival Guide.

Other things you need to learn about for Multiplayer play:

* Chat lets you talk with other players.
* Reliability Ratings tell you whether your opponent will finish a game.
* Zones let you find players your own Level.
* Challenge Rings let you start battles.
* Battle Mastering is a skill you'll want to have.
* Battle Options let you change the rules of the game.
* Player Settings let you set each player's settings.
* Outposts are used to capture territory.
* Altars let you Sacrifice to gain Knowledge.
* Rank is achievement on a grand scale.

* Click here for a key to the information contained in:
〔그림: fortgump.L1〕〔그림: fortgump.L0〕1.43 devo12
```

원본 연결 주소:

- `#multiSafeHelp` · [본문 이동](#topic-multisafehelp)
- `#chatViewHelp` · [본문 이동](#topic-chatviewhelp)
- `#reliabilityHelp` · [본문 이동](#topic-reliabilityhelp)
- `#challengeIslandType` · [본문 이동](#topic-challengeislandtype)
- `#battleIslandType` · [본문 이동](#topic-battleislandtype)
- `#battlemasterHelp` · [본문 이동](#topic-battlemasterhelp)
- `#battleOptionsHelp` · [본문 이동](#topic-battleoptionshelp)
- `#playerOptionsHelp` · [본문 이동](#topic-playeroptionshelp)
- `#outpostType` · [본문 이동](#topic-outposthelp)
- `#altarType` · [본문 이동](#topic-altartype)
- `#sacrificeOutline` · [본문 이동](#topic-sacrificeoutline)
- `#technologyHelp` · [본문 이동](#topic-technologyhelp)
- `#rankHelp` · [본문 이동](#topic-rankhelp)
- `#playerlistView` · [본문 이동](#topic-playerlistviewhelp)
- `#PlayerListView` · [본문 이동](#topic-playerlistviewhelp)

<a id="topic-rankhelp"></a>

## 104. Rank

원본 앵커: `rankHelp` · 본문 시작: 원문 2338행.

```text
Rank

What divides the true veterans from the beginners? Rank does.

Having a higher Rank than another player does not give you any advantages in battle - it is merely a way of showing your skill and how many Priests you have sacrificed.

Higher Rank is achieved when you have Sacrificed many Priests and gained every single piece of Knowledge the Furies can bestow. At this moment, when you next Sacrifice you will get the option to

ADVANCE TO RANK X

The 'X' will be the number of your new Rank. If you choose this option, all of the technologies that you have gained whilst advancing to level 43 will be lost, and you will start over again with the Sun Disc Thrower, the Sun Cannon, and one Generator. Choosing this option will also increase your Rank by 1.

In previous versions of Netstorm, Ranks gave players an advantage in battle, by giving their units more health and stronger attacking power, but this was considered unfair. This system of stronger units is now used in the Handicap system, of which the advantages are decided by the Player Settings the Battle Master can set.

Back to Multiplayer Quick Start
```

원본 연결 주소:

- `playerOptionsHelp` · [본문 이동](#topic-playeroptionshelp)
- `#battlemasterHelp` · [본문 이동](#topic-battlemasterhelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-multisafehelp"></a>

## 105. Multiplayer Survival Guide

원본 앵커: `multiSafeHelp` · 본문 시작: 원문 2365행.

```text
Multiplayer Survival Guide

You need to know that players with a higher Level than yours have been playing longer and have won more battles. Be cautious if they join a game you're playing.

Even players with low Levels might be veterans. Check out their Rank by looking next to their name. A player with higher rank inflicts more damage the higher his Rank, and suffers less damage from lower Ranked players.

You can also examine how many games the player has played. To find out, right-click on the player's island and choose View Player Info to get details about them. This is probably the best way to tell how good a player is, as the players who are the best are those who have been playing for the longest.

If they have played many more games than you, they are probably much more experienced than you are. If you're not on a Challenge Ring then you don't need to be concerned.

If they are on the same Ring as you are, you have two options if you do not want to play with them. First, you can simply leave the Challenge Ring by left-clicking outside the ring. Second, if you are the Battle Master you can right-click their name in the left window and Kick them Off, however it is common courtesy to politely ask a player to leave before using this option.

It is also wise to check a player's Reliability Rating before playing.

Back to Multiplayer Quick Start
```

원본 연결 주소:

- `battleIslandType` · [본문 이동](#topic-battleislandtype)
- `#battlemasterHelp` · [본문 이동](#topic-battlemasterhelp)
- `reliabilityHelp` · [본문 이동](#topic-reliabilityhelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-reliabilityhelp"></a>

## 106. Reliability Ratings

원본 앵커: `reliabilityHelp` · 본문 시작: 원문 2399행.

```text
Reliability Ratings

Every player gets a Reliability Rating that indicates how often he finishes games.

At first this may seem unimportant - but consider how disappointing it can be when your enemy disconnects right at the end of a battle - just so you won't beat him.

That is where Reliability Ratings come in. Whenever somebody disconnects from an unfinished battle and never returns, their Reliability Rating goes down. Of course, it's impossible to tell whether they disconnected intentionally, or whether their Internet connection is poor. A disconnect is a disconnect. So a low Reliability Rating might just mean you have a flaky Internet connection - not that you're a sore loser.

You can view a player's Reliability Rating in the Challenge area by right-clicking on their island. A Reliability Rating of 100% is as good as it gets. A Rating of 0% means they have never finished any games!

If you find yourself with a low Reliability Rating you might consider switching Internet Service Providers.

Back to Multiplayer Quick Start
```

원본 연결 주소:

- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-battlemasterhelp"></a>

## 107. Battle Master

원본 앵커: `battlemasterHelp` · 본문 시작: 원문 2424행.

```text
Battle Master

The Battle Master can control the rules and Players Settings of a battle and kick unruly players off of the Challenge Ring before a battle begins.

The rules of a battle are shown at the bottom of the left hand window and the player settings are shown at the top left hand window. Click on a rule or setting to change it. When a rule or setting is changed you and all the other players in the battle will become "unchecked." This is a safety measure to prevent rules and settings changes from slipping past players undetected.

As Battle Master you have the responsibility of creating a good game for all the players involved. If one player is not appropriate to your game (perhaps his level is too high) you can Kick him Off the ring. Just right-click on his name in the window and select Yes. However, we ask you to be nice to the fellow players and try asking them to leave before kicking.

We urge you to consider the requests of other players on your ring regarding rules changes. Nobody likes to wait a long time for a battle to start, and heeding such requests can make everyone happier.

There is one other extremely important role for you as Battle Master. Your computer will be the one chosen to route information between the other players in the game. If you have a slow or poor connection, it is good etiquette to allow others to become a Battle Master instead.

Another consideration is your connection speed. If you're hosting a game of five or more players, you might ask around the ring to see who has the fastest connection. A fast Battle Master makes a more reliable game for everyone.

Review Player Settings
Review Battle Options

Back to Multiplayer Quick Start
```

원본 연결 주소:

- `battleOptionsHelp` · [본문 이동](#topic-battleoptionshelp)
- `playerOptionsHelp` · [본문 이동](#topic-playeroptionshelp)
- `battleIslandType` · [본문 이동](#topic-battleislandtype)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-battleislandtype"></a>

## 108. Challenge Rings

원본 앵커: `battleIslandType` · 본문 시작: 원문 2468행.

```text
Challenge Rings

Left-click on a Challenge Ring to join that ring.

If you're the first player on the Challenge Ring, you will become the Battle Master, as long as the Pass Server Diagnostic option located under Options then Game is selected. Pass Server Diagnostic allows you to serve, however, if you are behind a firewall or have a slow connection you may choose to deselect this option and disallows you to become a Battle Master. If you are not the first player on the ring you will simply become a member of that battle.

If you are behind a firewall but would like to be able to serve NetStorm games you will need to open/forward ports 6799 and 6800. There is more help regarding this subject at Netstorm:HQ.

Your location on the challenge ring is your location in the battle!

When you join the battle, your name will appear in the left window with a colored box next to it and other Player Settings. Your name will first be a gray color until you connect to the Battle Master. If your name stays gray color, then for some reason you can not connect to the Battle Master. Once the gray color goes away the color of the box and your name is the color you'll have in the battle. To indicate that you're ready to start, click the colored box next to your name.

Once you clicked the box at first a black circle will appear in the box: 〔그림: fortgump.L6〕

The circle should light up if your name is not gray. If it lights up this means your computer has confirmed that it can make a connection to the Battle Master: 〔그림: fortgump.L7〕

If the black circle never lights up and your name stays gray, you cannot make this connection for some reason. You will want to leave this battle and join another. In rare circumstances, leaving the Challenge Ring and rejoining will allow the connection to be established.

You don't have to join a battle if you don't want to. You can leave a battle ring at any time. The Battle Master has the power to 'kick', however it is common courtesy to politely ask a player to leave before using this option.

If a ring has yellow text on it, that means the battle is using a version that is different enough from yours that you may not play together.

Note: The Battle Master can add a description by right clicking the center of the ring and typing into the field.

Back to Multiplayer Quick Start
```

원본 연결 주소:

- `#battlemasterHelp` · [본문 이동](#topic-battlemasterhelp)
- `http://www.netstormhq.com` · 외부/명령/본문 대상 미확인
- `playerOptionsHelp` · [본문 이동](#topic-playeroptionshelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-challengeislandtype"></a>

## 109. Zones

원본 앵커: `zoneHelp / challengeIslandType` · 본문 시작: 원문 2513행.

```text
Zones

NetStorm has many Zones for you to play in. When you first enter the Pyrosphere you'll move automatically to a Zone that matches your Level. If the best Zone for you has no other players, you'll probably appear in a nearby Zone with somebody in it.

In each Zone you'll see Challenge Rings and more Zones.
〔그림: battleIsland.*〕 "Challenge Ring"
〔그림: challengeIsland.*〕 "Zone"

If you wish to visit another Zone, examine the words under the Zone. If it says, "Empty" then there are no other players in that Zone. However, if someone joined the zone recently you may not be able to tell they joined the zone. Pushing F9 will refresh the zones and allow you to see if anyone joined the zone recently. If there is someone in the zone it will say the number of players in the zone, and there average level. To travel to another Zone, just left-click the Zone.

While in a Zone, click on the player's name on the left window and your screen will zoom to that player. Left-click near that player and your island will fly to the new location you've chosen.

Back to Multiplayer Quick Start
```

원본 연결 주소:

- `#battleIslandType` · [본문 이동](#topic-battleislandtype)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-playerlistviewhelp"></a>

## 110. Player List Window

원본 앵커: `playerListView / playerListViewHelp` · 본문 시작: 원문 2543행.

```text
Player List Window

If you are floating free in the Pyrosphere you will see the names of every player present in your Zone. You can click on a player's name to see where that player is.

With each player handle you may see some or all of the following information, and might look like this: 〔그림: fortgump.L1〕〔그림: fortgump.L0〕1.43 devo12

〔그림: fortgump.L1〕 indicates that the player is marked as not being able to be the Battle Master i.e. the server in a multi-player game. In short, battles will be more robst the fewer players are fire-walled. If you are fire-walled yourself, look for battles where most of your opponents are not i.e. the server in a multi-player game.

If you see the 〔그림: fortgump.L1〕 icon next to your name, but don't think that you are actually fire-walled, you can get rid of the icon by pressing Esc, clicking Options, clicking Game and select the Pass Server Diagnostic option. This means that you can be a Battle Master at the start of a game, and that you can take over the role of Battle Master in the middle of a game if the current Battle Master disconnects.

If you do not see the 〔그림: fortgump.L1〕 icon next to your name, but you know that you are unable to serve well, then you can mark yourself as firewalled by unselecting the option mentioned in the previous paragraph. This will mean that you will never become Battle Master in a game.

〔그림: fortgump.L0〕 indicates that the player does not have the NetStorm CD currently in his or her CD-Rom drive. Note: On most servers you will not see this icon.

The numbers before the name, example 1.43 are the Rank and level. So therefore 1.43 would be rank 1 level 43. The higher the level the more Units the player has. The higher the rank is the longer the player has been playing.

devo12 is an example of a player's handle name

While inside a Challenge Ring you may notice icons such as 〔표시 코드: IfortGump.L9〕 〔표시 코드: IfortGump.L11〕 〔그림: icon.B0〕. You can review these icons in the Player Settings section.

See the Multiplayer Quick Start for additional information.
```

원본 연결 주소:

- `challengeIslandType` · [본문 이동](#topic-challengeislandtype)
- `battleMasterHelp` · [본문 이동](#topic-battlemasterhelp)
- `rankHelp` · [본문 이동](#topic-rankhelp)
- `unitHelp` · [본문 이동](#topic-unithelp)
- `battleIslandType` · [본문 이동](#topic-battleislandtype)
- `playerOptionsHelp` · [본문 이동](#topic-playeroptionshelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-battlestatusviewhelp"></a>

## 111. Battle Status Window

원본 앵커: `battleStatusView / battleStatusViewHelp` · 본문 시작: 원문 2587행.

```text
Battle Status Window

The top half of this window displays all the players in your battle and the current Player Settings.

The bottom half shows the current Battle Options.

Each player must click the box next to his or her name before the Battle Master can start the battle.

If somebody joins and their name stays gray it means they can not connect. When the gray named person clicks in their pip will stay dark meaning they cannot establish a connection with the Battle Master and cannot play in that game. Sometimes leaving the Challenge Ring and rejoining will allow the connection to be established.
〔그림: fortgump.L6〕 Dark Pip (can not connect)
〔그림: fortgump.L7〕 Light Pip (connected)

Click here for a key to the information contained in:
〔그림: fortgump.L1〕〔그림: fortgump.L0〕devo12

Back to Multiplayer Quick Start
```

원본 연결 주소:

- `#playeroptionshelp` · [본문 이동](#topic-playeroptionshelp)
- `#battleoptionshelp` · [본문 이동](#topic-battleoptionshelp)
- `battlemasterhelp` · [본문 이동](#topic-battlemasterhelp)
- `#battleIslandType` · [본문 이동](#topic-battleislandtype)
- `#playerlistView` · [본문 이동](#topic-playerlistviewhelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-playeroptionshelp"></a>

## 112. Player Settings

원본 앵커: `playerOptionsHelp` · 본문 시작: 원문 2611행.

```text
Player Settings

Player settings are settings the Battle Master sets for each player.

Observer - determines whether the indicated person is fighting or observing the battle. A 〔표시 코드: IfortGump.L9〕 represents that the person is playing, while a 〔표시 코드: IfortGump.L10〕 represents that the person is observing. An observer will not have an island, a priest, or Storm Power in the game. The Battle Master can adjust this setting by clicking on the icon.

Preset Teams - To create a preset team, the Battle Master can click on the colored shields until the players on the same team have matching shields. Right clicking a colored shield will make this setting go back.
The Shields are: 〔표시 코드: IfortGump.L11〕〔표시 코드: IfortGump.L12〕〔표시 코드: IfortGump.L13〕〔표시 코드: IfortGump.L14〕〔표시 코드: IfortGump.L15〕〔표시 코드: IfortGump.L16〕〔표시 코드: IfortGump.L17〕〔표시 코드: IfortGump.L18〕

Handicap - The Handicap option allows the Battle Master to strengthen the units of a certain player. Left and Right clicking the icon forces it to change.
The icons from one to ten are: 〔그림: icon.B0〕 〔그림: icon.B1〕 〔그림: icon.B2〕 〔그림: icon.B3〕 〔그림: icon.B4〕 〔그림: icon.B5〕 〔그림: icon.B6〕 〔그림: icon.B7〕 〔그림: icon.B8〕 〔그림: icon.B9〕
Once the rank gets high, it will have two icons and it works just like a number system. The higher the icon the more strength the player units are. You can check the handicap any time in battle by pressing F9 to bring up the player list. For more information on this you can check Netstorm:HQ.

Review Battle Options
Back to Multiplayer Quick Start
```

원본 연결 주소:

- `battlemasterhelp` · [본문 이동](#topic-battlemasterhelp)
- `http://www.netstormhq.com` · 외부/명령/본문 대상 미확인
- `battleOptionsHelp` · [본문 이동](#topic-battleoptionshelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)

<a id="topic-battleoptionshelp"></a>

## 113. Battle Options

원본 앵커: `battleOptionsHelp` · 본문 시작: 원문 2639행.

```text
Battle Options

Battle options change the initial conditions of a battle.

Game Options> Clicking this text will pop up a drop down menu that will list all the battle options that can be changed in drop down format.

(Sort By) you can click the sort by menu to easily toggle between the battle options viewing.

The Show All Options has the same appearance of older version of NS however clicking the white text will bring up a drop down box allowing you to change the value of the option easily while clicking the tan text will toggle the options by one as seen in prior version of Netstorm.

Show Only Changed Optionsthis option will show only the battle options that have been changed from there original default. This option allows you to add battle master option much the same as you would add excluded units. Clicking the white or tan text will reset the battle option to default and remove it from the current viewpoint.

Game Type - Netstorm as a game over the years has evolved into many branches and with every introduction of a new patch the game dynamics tend to change. Keeping this in mind the concept of Game Types was born. In general Game Types allows you to easily select between different generations of battle styles that have been created over the years. This option also organizes the battle menu into something that is easier to read and easier to configure and auto configures some of the battle options thus making setting up a game quicker and easier. For example

Netstorm Standard is classified as the most current style of game play. This selection features the newly implemented geyser distribution system and includes battle options that are traditionally associated with Modern Netstorm game play.

Netstorm Classic is a highbred between Netstorm 10.64 and Netstorm 10.37 and utilizes the random map geyser distribution system commonly found in older version of the game.

Netstorm Flexible unlocks 95% of the battle options that can be configured and is very similar to the layout of Netstorm 10.70

Netstorm Geyserless is a radical new concept born from the idea of the furies granting the player small amounts of money during the game rather than the player having to worry about collecting. This game style is very new, and can be quite fun.

Player Customizable allows the player to create there own game style and edit almost every aspect of the game. Who knows maybe you will create a game style that everyone will enjoy playing.

Bridge Slots - determines how many bridges will appear in your Production Window during the game.

Unit Rate - determines how quickly units refresh in your Production Window. For example, when you lay a unit down in the game its image in the Production Window will stay dimmed for a while. Increase the Unit Rate and it will light up more quickly.

Generator Range - describes how far a Generator will cast its energy. A longer range means it's easier to deploy high level units (but there is less strategy to the game).

Kill Reward - When you destroy an enemy unit you usually get one quarter of the Storm Power value of that unit. You can change the reward value with this option.

Breakable Alliances - Are players allowed to break alliances once they have made them? Normally you cannot break alliances. If you want greater freedom you can switch this to Yes.

Allow Send - Are players allowed to send Storm Power to other players? Normally you can send Storm Power to other players. If you want to disallow this you can switch this to No.

Money per Geyser - How much Storm Power will each Geyser produce?

Starting Cash - determines how much Storm Power you start with. This is normally set at 6500, however if you feel you need more or less at the start of game you can change this option.

Geyser Amount - determines how many geysers are in the game. You can change this to have more or less geysers in the game.

Spell - Do you want spells in the game? Normally spells are in the game. If you want to disallow spells you can switch this to No. This option does not affect the priest.

Map Mode - determines what type of map you want. This is normally set to the traditional multiplayer map Archipelago, but you can change this to a Single Island.

Map Size - Changing this option will result in a larger or smaller map size.

Map Density - determines the size and amount of islands in the game. Higher densities increase the odds of more and larger islands appearing, while lower densities decrease these odds.

Island Dynamics - this option if active causes random items such as trees, ruins and monuments to be created allover the map.

Geysers Placement - This option toggles between having no geyser on startup or respawning during the game, the old Netstorm geyser mode where geysers could spawn anywhere on the map and the new geyser mode that evenly distributes geysers across the map.

Geysers Respawns - this option sets how often the respawn geyser could.

Resource Injections - This option allows the furies to send you storm power at discreet intervals of time.

Injection Value - This option sets how much storm power the furies will send you per injection.

Excluded Units - If you don't want to allow a certain unit or building to be built during your battle, you can exclude them here. Also excluding spells will not allow a particular spell to appear on the map.

Review Player Settings
Back to Multiplayer Quick Start
```

원본 연결 주소:

- `playerOptionsHelp` · [본문 이동](#topic-playeroptionshelp)
- `multiQuickHelp` · [본문 이동](#topic-multiquickhelp)
