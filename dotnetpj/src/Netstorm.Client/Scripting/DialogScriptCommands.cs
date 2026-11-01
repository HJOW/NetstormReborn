using Netstorm.Client.Mission;

namespace Netstorm.Client.Scripting; // 전체 HTML과 메타가 공유하는 원본 명령 이름 표다.

/// <summary>cpppj UberGump의 HTML 명령 이름을 실제 입력 큐 코드로 조회한다.</summary>
public static class DialogScriptCommands
{
    /// <summary>ASCII 대소문자를 무시하고 원본 명령을 조회한다. 미등록 이름은 0xffffffff를 반환한다.</summary>
    /// <param name="name">HTML에서 읽은 원시 명령 이름.</param>
    public static uint Lookup(string name)
    {
        ArgumentNullException.ThrowIfNull(name);
        char[] key = name.ToCharArray();
        // 원본 stricmp의 ASCII 문자만 소문자로 바꾼다.
        for (int i = 0; i < key.Length; i++) if (key[i] is >= 'A' and <= 'Z') key[i] = (char)(key[i] + 32);
        uint command = new string(key) switch
        {
            "tell" => 0x02000095, "donothing" => 0x02000089, "startdemo" => 0x020000c0,
            "config" => 0x0200006d, "warning" => 0x02000102, "viewspot" => 0x020000d9,
            "setowner" => 0x020000cc, "salvage" => 0x0200005d, "sendallbucks" => 0x02000118,
            "askeditfort" => 0x020000c9, "readfort" => 0x020000ca,
            "showtechnology" => 0x020000a0, "cheatalltechnology" => 0x02000028,
            "quitapp" => 0x02000001, "requestquitapp" => 0x02000094, "menujustclosed" => 0x020000be,
            "reloadtheams" => 0x02000123, "pause" => 0x020000cf, "techhelp" => 0x02000049,
            "getfulldeck" => 0x020000cd, "toggleminimap" => 0x02000037, "togglebottomgump" => 0x02000088,
            "addisland" => 0x02000022, "savegomain" => 0x020000cb, "telltip" => 0x020000eb,
            "newfort" => 0x02000079, "screenmodeabschange" => 0x02000030, "screenmodechange" => 0x02000002,
            "acceptscreenmodechange" => 0x020000e4, _ => 0xffffffff
        };
        return command != 0xffffffff ? command : MissionGuideInput.ControlScriptCode(name);
    }
}
