// 원문 도움말의 오자·별칭·개행도 그대로 유지하는 고정 자료다.
namespace Netstorm.Client.Host;

/// <summary>전체 개발자 해석기가 같은 순서로 출력하는 Source 고정 문자열이다.</summary>
public sealed partial class HostSession
{
    /// <summary>원문 수준29에 순서대로 전달할32문자열이다.</summary>
    private static readonly string[] DeveloperHelp =
    [
        "setOwner(so) sid newowner - sets the owner of sid via server.\n",
        "setHackMove(shm) 1|0 - sets the hack move flag on client.\n",
        "dump(d) sid - dumps lots of info about a given sid.\n",
        "setNoDamage(snd) 1|0 - sets the \"nodamage\" flag on client and server.\n",
        "scatter(sct) type - generates some random pieces of type near bc=<0,0>.\n",
        "dumpTypes(dts) 1|0 - dumps the type info. 1=detailed, 0=abreviated.\n",
        "dumpType(dt) type - dumps info about a specific type.\n",
        "shootMyFriends(smf) 1|0 - attackers will target pieces of same owner.\n",
        "create(c) type frame - create type of frame at bc=<20,20>.\n",
        "setLocalHitPoints(slhp) sid - set the hitpoints field for sid only on this client.\n",
        "setAbstract(sa) sid 1|0 - set the abstract flag on sid to 1 or 0 (Only on client).\n",
        "fillMyTank(fmt) amount - set all graphs' energy to amount (Only on this client and the server)\n",
        "applyDamage(ad) sid amount - calls take damage on sid. (Client and server)\n",
        "generatorTimer(gt) 1|0 - make stuff appear in combat gump fast\n",
        "combatGumpColsRows(cr) cols rows - make more stuff appear in combat gump\n",
        "heaptrace(ht) 1|0 - walk the heap and print stats each frame\n",
        "destroy(de) sid - kill a sid\n",
        "dumpArtData(dad) type - dumps art data (hot spot, etc)\n",
        "dumpPlayers(dp) - dump the active players.\n",
        "dumpgraph(dg) graphNumber - dump the graph\n",
        "roundtotile(rtt) 1|0 - allow cursor to have fractional pos\n",
        "setally(sally) player ally - set ally for player\n",
        "clrally(cally) player ally - clr ally of player\n",
        "dumpally(dally) player - dump ally status\n",
        "createShipShit(css) - creates random crap in your ship\n",
        "setbridgekillperiod(sbkp) - sets period for bridge kill scan\n",
        "setsubid(ssi) - sets the subid and handle associated with this fort\n",
        "quickVortex(qv) - makes vortices build instantly\n",
        "quickBuild(qb) - makes everything build instantly\n",
        "queryDirectDraw(qdd) - shows DirectDraw driver info\n",
        "addLabel(al) - adds a label to the build record stream\n",
        "getPriest(gp) - sticks a enemy priest next to your priest\n",
    ];
}
