using System.Buffers.Binary;
// 원문 자식 주소 식별자의 호출 순서를 짧게 표현한다.
using E = Netstorm.Core.Original.StateWorldCleanupEffect;

// PATCH10.78의 전체 State 정리 부모와 고정 범위 초기화를 이식한다.
namespace Netstorm.Core.Original;

/// <summary>004b2df0의 현재값 재조회·부분 쓰기·선택 효과 순서를 보존한다. CD 부모로 가장하지 않는다.</summary>
public static class RawStateWorldCleanup
{
    /// <summary>실제 PlayerNoMake 경고의 첫255DWORD만0으로 쓴다.</summary>
    public static void ResetStatePlayerNoMakeWarnings(Span<uint> warnings)
    {
        if (warnings.Length < 255) throw new ArgumentException("State PlayerNoMake의실제255DWORD누락", nameof(warnings));
        warnings[..255].Clear();
    }
    /// <summary>현재 spot의 첫65536BYTE만0으로 쓰며 Pool/Hash/Graph나 이웃 버퍼는 바꾸지 않는다.</summary>
    public static void ResetStateSpotMap(Span<byte> spots)
    {
        if (spots.Length < 65536) throw new ArgumentException("State spot의실제65536BYTE누락", nameof(spots));
        spots[..65536].Clear();
    }
    /// <summary>각 원문 DWORD store마다 현재 Player base를 재조회하고 같은 필드만 쓴다.</summary>
    private static void ClearPlayerField(StateWorldCleanupGlobals globals, uint player, uint field)
    {
        Memory<byte> bytes = Need(globals.Players)(); int offset = checked((int)(player * 180 + field));
        if (offset > bytes.Length || 4 > bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(player), "State현재Player필드물리부족");
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Span.Slice(offset, 4), 0);
    }
    /// <summary>전송/정지→저장/Player→UI/Kernel→Core0→Pool→Player 재구성→모드/기한 순서로 실행한다.</summary>
    public static void CleanupStateWorld(uint reason, StateWorldCleanupGlobals globals, StateWorldCleanupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        // 각 원문 사용 위치에서 현재 실제 DWORD를 조회한다.
        uint Read(uint address) => Need(globals.Read32)(address);
        // 같은 실제 전역에 한 DWORD 효과를 즉시 반영한다.
        void Write(uint address, uint value) => Need(globals.Write32)(address, value);
        // 지정 double 필드의 원시64비트를 그대로 저장한다.
        void Wide(uint address, ulong bits = 0) => Need(globals.Write64Bits)(address, bits);
        // 현재 선택한 원문 자식만 그 위치에서 요구한다.
        void Effect(E selected) => Need(hooks.Effect)(selected);
        if (reason == 0) { Effect(E.CloseAllConnections); Effect(E.EndConnections); }
        else if (Read(0x5c85c0) == 0)
        {
            // 진입 client 모드는 한 번만 판정하며 Player1..8을 원문 순서로 해제한다.
            for (uint player = 1; player < 9; player++) Need(hooks.Disconnect)(player);
        }
        else Need(hooks.Disconnect)(9);
        Need(hooks.Disconnect)(10); Need(hooks.Disconnect)(11);
        uint fort = Read(0x594fb8); Write(0x5c85c0, 0); Write(0x54db14, 0);
        bool invalid = fort != 0 ? (Read(0x594fbc) != 0 || Read(0x594fc8) != 0) : (Read(0x594fbc) != 0 && Read(0x594fc8) != 0);
        if (invalid) Need(hooks.Assertion)(0x77a);
        // 실제 정지 깊이를 해제한 매 반복 뒤 다시 읽으며 게임/음향을 같은 순서로 푼다.
        while (Need(hooks.PauseDepth)() != 0) { Effect(E.ResumeGame); Effect(E.ResumeAudio); }
        Effect(E.Clear446e70); Effect(E.Clear4c1f20); Effect(E.ResetClock); Effect(E.Clear42b1b0);
        // 원문 rep stosd의 같은22DWORD만 순서대로0으로 만든다.
        for (uint word = 0; word < 22; word++) Write(0x5c84e8 + word * 4, 0);
        uint selected = Read(0x540cac); uint manager = Need(globals.TemplateManager)(selected);
        Write(0x5467b8, 0); Write(0x542544, 1); Write(0x5950ac, 0); Write(0x5950b0, 0); Write(0x595090, 0); Write(0x5c85e0, 0); Write(0x5c85cc, 0); Write(0x5c85c8, 0);
        if (manager != 0)
        {
            if (Read(0x594fb8) != 0 && Read(0x5c85a4) == 0) { Need(hooks.SaveFort)(selected); selected = Read(0x540cac); }
            if (Read(0x594fbc) != 0 && Read(0x594fa4) == 0 && Read(0x5c8758) == 14) Need(hooks.SaveBattle)(selected);
        }
        // Player0은 생략하고 각 실제 필드 store마다 현재 base를 다시 읽는다.
        for (uint player = 1; player < 9; player++)
        {
            // 전투 관련8DWORD만0으로 쓰며 다른 필드와 이웃 기록은 보존한다.
            for (uint field = 0x7c; field <= 0x98; field += 4) ClearPlayerField(globals, player, field);
            Need(hooks.ResetAi)(player); Need(hooks.ResetPriest)(player);
            if (Read(0x595344) == 0) Need(hooks.ResetOtherPlayer)(player, 0);
            else
            {
                uint owner = Read(0x540c70);
                if (player != owner) Need(hooks.ResetOtherPlayer)(player, 0);
                else { uint bits = Read(0x5c85a4) != 0 ? 0x483d9c80u : 0u; Need(hooks.SetSp)(owner, BitConverter.UInt32BitsToSingle(bits)); Effect(E.RefreshSp); }
            }
        }
        Effect(E.ClearTemplates); uint metamap = Read(0x594fc8); Write(0x54c8a0, 0);
        if (metamap != 0) Effect(E.LeaveMetamap);
        Effect(E.Clear445140); Effect(E.ClearCursor);
        float defaultSp = unchecked((int)Read(0x540bdc)); uint type = Read(0x541240);
        Write(0x5467bc, 0); Write(0x59508c, 0); Write(0x5424bc, 25); Need(globals.TypeDefault)(type).Cost = defaultSp; Wide(0x5950b8);
        Write(0x5453e4, 0); Wide(0x5950c8); Wide(0x5950c0); Wide(0x565e68); Wide(0x5c8618); Wide(0x595358);
        Effect(E.ClearPlayerLinks); Effect(E.ClearKernel); Effect(E.KillGumps); Effect(E.CleanupGumps); Effect(E.ReplaceUber); Effect(E.Clear43b540);
        Need(globals.Write16)(0x5c89b4, 0); Write(0x595344, 0); Need(hooks.SelectCore)(0, 1);
        Write(0x5ca8f0, 0); Write(0x5ca938, 0); Need(hooks.ResetPool)(0);
        Effect(E.Reset422490); Effect(E.Clear4c2450); Effect(E.MapMode); Effect(E.SceneMusic); Effect(E.Clear472830); Effect(E.Clear4b2b50); Effect(E.Clear4328a0); Effect(E.Clear4ab940);
        Effect(E.Reset568b60); Effect(E.Reset54c8a0); Write(0x54c8a0, 1); Write(0x5c8bb0, 0); Effect(E.ResetGraph); Write(0x59a8ac, 0); Write(0x59a8b0, 1);
        Effect(E.Clear4a9e50); Effect(E.InitializePlayers); Effect(E.ResetZone);
        uint editor = Read(0x5c85a4); Write(0x5c85a0, 0); Write(0x55a420, 0); Write(0x5c8490, 0xc61c4000); Write(0x5c848c, 0xc61c4000);
        if (Read(0x595344) != 0) Need(hooks.SetSp)(Read(0x540c70), BitConverter.UInt32BitsToSingle(editor != 0 ? 0x483d9c80u : 0u));
        Effect(E.RefreshSp); uint state = Read(0x5c8758); uint mission = Read(0x594fa0);
        Write(0x595088, 0); Write(0x5c85d4, 0); Write(0x594fa4, mission); Write(0x594fb8, 0); Write(0x594fbc, 0); Write(0x594fc8, 0); Write(0x5c85a4, 0);
        if (state != 29 && state != 33) Need(hooks.SelectCore)(1, 1);
        Effect(E.InterpretOptions); Write(0x5949d4, unchecked(Read(0x5949d4) + 1u)); Effect(E.MapMode);
        double now = Need(hooks.WallSeconds)(); Wide(0x5c8610, BitConverter.DoubleToUInt64Bits(now + 10.0));
        Write(0x5c7a74, 0); Wide(0x5c85f0); Write(0x5c85b4, 0); Write(0x540bc0, 0); Write(0x540bc4, 0); Wide(0x5c85f8);
        Write(0x542534, 0xffffffff); Write(0x542538, 0xffffffff); Write(0x54253c, 0xffffffff); Write(0x542540, 0xffffffff);
    }
    /// <summary>실제로 도달한 장치만 필수로 요구하며 선택하지 않은 경계를 선행 검사하지 않는다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("State정리의실제선택장치누락");
}
