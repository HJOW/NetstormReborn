# 원본 소스 파일 → cpppj 경로 (자동 생성)

> `python tools/cpp_source_map.py` 로 생성. 직접 수정하지 말 것.
> 두 판본 exe 의 assert 문자열에 남은 원본 소스 경로를 모았다. assert 가 없는 소스 파일은 여기에 나오지 않는다.
> **cpppj 경로**의 파일 이름은 CD판 표기(원래 대소문자가 남아 있다)의 첫 글자를 대문자로 한 것이다.
> **함수 수**는 패치판에서 그 파일 이름을 참조하는 함수의 수다([모듈 맵](../docs/exe/modules.md)). 그 파일의 전체 함수 수가 아니다.
> **상태**는 cpppj 에 그 경로의 파일이 있는지만 본다. 파일이 있어도 일부 함수만 옮긴 것일 수 있다.

## `\Ns\O\` → `src/o/` (75개, 파일 있음 0개)

| cpppj 경로 | 패치판 표기 | CD판 표기 | 함수 수 | 상태 |
|---|---|---|---|---|
| `src/o/_p_baseflyerprocessdata.cpp` | _p_baseflyerprocessdata.cpp | _p_baseflyerprocessdata.cpp |  |  |
| `src/o/_p_constructionprocessdata.cpp` | _p_constructionprocessdata.cpp | _p_constructionprocessdata.cpp |  |  |
| `src/o/_p_daisprocessdata.cpp` | _p_daisprocessdata.cpp | _p_daisprocessdata.cpp |  |  |
| `src/o/_p_fireanimprocessdata.cpp` | _p_fireanimprocessdata.cpp | _p_fireanimprocessdata.cpp |  |  |
| `src/o/_p_gunprocessdata.cpp` | _p_gunprocessdata.cpp | _p_gunprocessdata.cpp |  |  |
| `src/o/_p_packets.cpp` | _p_packets.cpp | _p_packets.cpp |  |  |
| `src/o/_p_rainaviaryprocessdata.cpp` | _p_rainaviaryprocessdata.cpp | _p_rainaviaryprocessdata.cpp |  |  |
| `src/o/_p_rotateanimprocessdata.cpp` | _p_rotateanimprocessdata.cpp | _p_rotateanimprocessdata.cpp |  |  |
| `src/o/_p_sunaviaryprocessdata.cpp` | _p_sunaviaryprocessdata.cpp | _p_sunaviaryprocessdata.cpp |  |  |
| `src/o/_p_windaviaryprocessdata.cpp` | _p_windaviaryprocessdata.cpp | _p_windaviaryprocessdata.cpp |  |  |
| `src/o/AddRandom.cpp` | Addrandom.cpp | addRandom.cpp | 2 |  |
| `src/o/Ai.cpp` | Ai.cpp | ai.cpp | 15 |  |
| `src/o/Angle.h` | angle.h | angle.h | 1 |  |
| `src/o/BaseFile.cpp` | Basefile.cpp | BaseFile.cpp | 6 |  |
| `src/o/BaseProcess.cpp` | Baseprocess.cpp | BaseProcess.cpp | 5 |  |
| `src/o/BoardCoord.cpp` | Boardcoord.cpp | BoardCoord.cpp | 5 |  |
| `src/o/Bomb.cpp` | Bomb.cpp | bomb.cpp | 5 |  |
| `src/o/Bridge.cpp` | Bridge.cpp | bridge.cpp | 6 |  |
| `src/o/Bud.h` | bud.h | bud.h | 2 |  |
| `src/o/CanonDecoder.cpp` | Canondecoder.cpp | CanonDecoder.cpp | 2 |  |
| `src/o/Carrier.cpp` | Carrier.cpp | carrier.cpp | 13 |  |
| `src/o/Ch.cpp` | Ch.cpp | ch.cpp | 10 |  |
| `src/o/ChunkMap.cpp` | Chunkmap.cpp | ChunkMap.cpp | 4 |  |
| `src/o/Config.cpp` | Config.cpp | config.cpp | 6 |  |
| `src/o/ConfigInterface.cpp` | Configinterface.cpp | ConfigInterface.cpp | 12 |  |
| `src/o/Connection.cpp` | — | connection.cpp |  |  |
| `src/o/Construction.cpp` | Construction.cpp | construction.cpp | 3 |  |
| `src/o/Damageable.cpp` | Damageable.cpp | Damageable.cpp | 7 |  |
| `src/o/DataManager.cpp` | Datamanager.cpp | DataManager.cpp | 7 |  |
| `src/o/Deck.cpp` | Deck.cpp | deck.cpp | 5 |  |
| `src/o/Fence.cpp` | Fence.cpp | — | 2 |  |
| `src/o/Filefinder.cpp` | Filefinder.cpp | filefinder.cpp | 1 |  |
| `src/o/FileSpec.cpp` | Filespec.cpp | FileSpec.cpp | 1 |  |
| `src/o/Flyer.cpp` | Flyer.cpp | flyer.cpp | 13 |  |
| `src/o/Gem.cpp` | Gem.cpp | gem.cpp | 4 |  |
| `src/o/Geometry.cpp` | Geometry.cpp | geometry.cpp | 1 |  |
| `src/o/Graph.cpp` | Graph.cpp | graph.cpp | 8 |  |
| `src/o/GunAnim.cpp` | Gunanim.cpp | GunAnim.cpp | 10 |  |
| `src/o/GunProcess.cpp` | Gunprocess.cpp | GunProcess.cpp | 10 |  |
| `src/o/Islandbuilder.cpp` | Islandbuilder.cpp | islandbuilder.cpp | 13 |  |
| `src/o/IslandDropper.cpp` | Islanddropper.cpp | IslandDropper.cpp | 1 |  |
| `src/o/IslandList.cpp` | Islandlist.cpp | islandList.cpp | 8 |  |
| `src/o/IsleInfo.cpp` | Isleinfo.cpp | IsleInfo.cpp | 2 |  |
| `src/o/Kernel.cpp` | Kernel.cpp | Kernel.cpp | 2 |  |
| `src/o/Mana.cpp` | Mana.cpp | Mana.cpp | 4 |  |
| `src/o/Markstream.h` | markstream.h | markstream.h | 2 |  |
| `src/o/Memstream.h` | memstream.h | memstream.h | 22 |  |
| `src/o/Nugget.cpp` | Nugget.cpp | nugget.cpp | 2 |  |
| `src/o/Path.cpp` | Path.cpp | path.cpp | 3 |  |
| `src/o/Path.h` | path.h | path.h | 8 |  |
| `src/o/Pathprocess.cpp` | Pathprocess.cpp | pathprocess.cpp | 8 |  |
| `src/o/Periodicharmer.cpp` | Periodicharmer.cpp | — | 1 |  |
| `src/o/Player.cpp` | Player.cpp | Player.cpp | 18 |  |
| `src/o/Regexp.cpp` | Regexp.cpp | regexp.cpp | 8 |  |
| `src/o/Regular.cpp` | Regular.cpp | regular.cpp | 1 |  |
| `src/o/RiftType.cpp` | Rifttype.cpp | RiftType.cpp | 14 |  |
| `src/o/RiftType.h` | rifttype.h | — | 6 |  |
| `src/o/Serverboard.cpp` | Serverboard.cpp | serverboard.cpp | 3 |  |
| `src/o/Snart.cpp` | Snart.cpp | snart.cpp | 1 |  |
| `src/o/Spot.cpp` | Spot.cpp | spot.cpp | 2 |  |
| `src/o/Squid.cpp` | Squid.cpp | Squid.cpp | 42 |  |
| `src/o/Squid.h` | squid.h | squid.h | 10 |  |
| `src/o/SquidFinder.cpp` | Squidfinder.cpp | SquidFinder.cpp | 7 |  |
| `src/o/SquidHash.cpp` | Squidhash.cpp | SquidHash.cpp | 2 |  |
| `src/o/StaticString.cpp` | Staticstring.cpp | staticString.cpp | 1 |  |
| `src/o/Template.cpp` | Template.cpp | template.cpp | 37 |  |
| `src/o/Terr.cpp` | — | Terr.cpp |  |  |
| `src/o/TerrainBuilder.cpp` | Terrainbuilder.cpp | terrainBuilder.cpp | 4 |  |
| `src/o/Totalmade.cpp` | Totalmade.cpp | — | 5 |  |
| `src/o/Unit.cpp` | Unit.cpp | unit.cpp | 2 |  |
| `src/o/Unit.h` | unit.h | unit.h |  |  |
| `src/o/Vortex.cpp` | Vortex.cpp | vortex.cpp | 4 |  |
| `src/o/WinUtil.cpp` | Winutil.cpp | winUtil.cpp | 5 |  |
| `src/o/Xlat.cpp` | Xlat.cpp | xlat.cpp | 3 |  |
| `src/o/Ztrans.cpp` | Ztrans.cpp | ztrans.cpp | 12 |  |

## `.\` (클라이언트) → `src/client/` (60개, 파일 있음 1개)

| cpppj 경로 | 패치판 표기 | CD판 표기 | 함수 수 | 상태 |
|---|---|---|---|---|
| `src/client/Animator3d.cpp` | Animator3d.cpp | animator3d.cpp | 6 |  |
| `src/client/Build.cpp` | Build.cpp | build.cpp | 5 |  |
| `src/client/Burning.cpp` | Burning.cpp | Burning.cpp | 4 |  |
| `src/client/ButtonGump.cpp` | Buttongump.cpp | ButtonGump.cpp | 1 |  |
| `src/client/Chat.cpp` | Chat.cpp | Chat.cpp | 7 |  |
| `src/client/Chatgump.cpp` | Chatgump.cpp | chatgump.cpp | 3 |  |
| `src/client/ClientBoard.cpp` | Clientboard.cpp | ClientBoard.cpp | 6 |  |
| `src/client/ClientDebug.cpp` | Clientdebug.cpp | ClientDebug.cpp | 1 |  |
| `src/client/ClientMain.cpp` | Clientmain.cpp | ClientMain.cpp | 3 | 있음 |
| `src/client/Cloud.cpp` | — | cloud.cpp |  |  |
| `src/client/CloudSystem.cpp` | — | CloudSystem.cpp |  |  |
| `src/client/ColorSort.cpp` | Colorsort.cpp | ColorSort.cpp | 6 |  |
| `src/client/CombatGump.cpp` | Combatgump.cpp | CombatGump.cpp | 3 |  |
| `src/client/Contact.cpp` | Contact.cpp | contact.cpp | 1 |  |
| `src/client/Cursor.cpp` | Cursor.cpp | cursor.cpp | 5 |  |
| `src/client/Dais.cpp` | Dais.cpp | dais.cpp | 8 |  |
| `src/client/Dissolve.cpp` | Dissolve.cpp | — | 3 |  |
| `src/client/Dude.cpp` | Dude.cpp | Dude.cpp | 8 |  |
| `src/client/EditGump.cpp` | Editgump.cpp | EditGump.cpp | 4 |  |
| `src/client/EvilRay.cpp` | Evilray.cpp | EvilRay.cpp | 1 |  |
| `src/client/Factory.cpp` | Factory.cpp | — | 3 |  |
| `src/client/Fierytail.cpp` | Fierytail.cpp | — | 1 |  |
| `src/client/Flyingshrapnel.cpp` | Flyingshrapnel.cpp | Flyingshrapnel.cpp | 2 |  |
| `src/client/FortSpec.cpp` | Fortspec.cpp | fortSpec.cpp | 4 |  |
| `src/client/Fountain.cpp` | — | fountain.cpp |  |  |
| `src/client/Guide.cpp` | Guide.cpp | guide.cpp | 3 |  |
| `src/client/Gump.cpp` | Gump.cpp | Gump.cpp | 18 |  |
| `src/client/HtmlGump.cpp` | Htmlgump.cpp | HtmlGump.cpp | 1 |  |
| `src/client/Image.cpp` | Image.cpp | Image.cpp | 2 |  |
| `src/client/Kaboom.cpp` | Kaboom.cpp | Kaboom.cpp |  |  |
| `src/client/KenCloud.cpp` | Kencloud.cpp | kenCloud.cpp | 2 |  |
| `src/client/Lightning.cpp` | Lightning.cpp | lightning.cpp | 4 |  |
| `src/client/Look.cpp` | — | look.cpp |  |  |
| `src/client/ManaUser.cpp` | Manauser.cpp | ManaUser.cpp | 1 |  |
| `src/client/Markup.cpp` | — | Markup.cpp |  |  |
| `src/client/MenuGump.cpp` | Menugump.cpp | MenuGump.cpp | 16 |  |
| `src/client/MetaDisplay.cpp` | Metadisplay.cpp | metaDisplay.cpp | 14 |  |
| `src/client/Minimap.cpp` | Minimap.cpp | minimap.cpp | 1 |  |
| `src/client/Missile.cpp` | Missile.cpp | Missile.cpp | 2 |  |
| `src/client/Mission.cpp` | Mission.cpp | mission.cpp | 3 |  |
| `src/client/MovieGump.cpp` | Moviegump.cpp | MovieGump.cpp | 5 |  |
| `src/client/Normal.cpp` | Normal.cpp | normal.cpp | 2 |  |
| `src/client/Particle.cpp` | Particle.cpp | Particle.cpp | 3 |  |
| `src/client/Piecegump.cpp` | — | piecegump.cpp |  |  |
| `src/client/Priest.cpp` | Priest.cpp | — | 7 |  |
| `src/client/Recorder.cpp` | Recorder.cpp | recorder.cpp | 1 |  |
| `src/client/Renderer.cpp` | Renderer.cpp | Renderer.cpp | 5 |  |
| `src/client/Screen.cpp` | Screen.cpp | Screen.cpp | 14 |  |
| `src/client/ScrollGump.cpp` | Scrollgump.cpp | ScrollGump.cpp | 1 |  |
| `src/client/ShapeToBuffer.cpp` | Shapetobuffer.cpp | ShapeToBuffer.cpp | 1 |  |
| `src/client/Sound.cpp` | Sound.cpp | sound.cpp | 13 |  |
| `src/client/SoundProcess.cpp` | Soundprocess.cpp | soundProcess.cpp | 1 |  |
| `src/client/Splash.cpp` | — | Splash.cpp |  |  |
| `src/client/State.cpp` | State.cpp | state.cpp | 12 |  |
| `src/client/StyleText.cpp` | Styletext.cpp | StyleText.cpp | 15 |  |
| `src/client/TextGump.cpp` | — | TextGump.cpp |  |  |
| `src/client/Ubergump.cpp` | Ubergump.cpp | ubergump.cpp | 2 |  |
| `src/client/Ui.cpp` | Ui.cpp | ui.cpp | 4 |  |
| `src/client/UserInput.cpp` | Userinput.cpp | UserInput.cpp | 2 |  |
| `src/client/VFXDraw.cpp` | Vfxdraw.cpp | VFXDraw.cpp | 3 |  |

## `\Ns\Zacket\` → `src/zacket/` (1개, 파일 있음 0개)

| cpppj 경로 | 패치판 표기 | CD판 표기 | 함수 수 | 상태 |
|---|---|---|---|---|
| `src/zacket/Zacket.cpp` | Zacket.cpp | zacket.cpp | 1 |  |
