using System.Buffers.Binary;
using System.Text;

// 실제 AI 본체·Player기술·원소분류를 같은 원문 메모리에서 갱신한다.
namespace Netstorm.Core.Original;

/// <summary>AI초기갱신이 원문 위치마다 읽는 현재 미션·제외타입·워크샾 표다.</summary>
public sealed class AiBrainInitialUpdateGlobals
{
    /// <summary>00594fa4의현재 비제로 미션 표식이다. C++의bool참조도 같은 조회기로 표현한다.</summary>
    public required Func<uint> MissionMode { get; init; }
    /// <summary>0054126c의현재 분류 제외타입133이다.</summary>
    public required Func<uint> ExcludedType { get; init; }
    /// <summary>theme0/1/2/3의현재b0/b4/b8/ac워크샾4개다.</summary>
    public required uint[] Workshops { get; init; }
}

/// <summary>실제 언어 설정과TYPE+a0요구 에너지 문자열을 조회한다.</summary>
public sealed class AiBrainInitialUpdateHooks
{
    /// <summary>004410f0의현재언어 defaultAiName이다.</summary>
    public Func<string>? DefaultName { get; init; }
    /// <summary>TYPE+a0현재 문자열이다. 최대길이 갱신 분기에서는 다시 조회한다.</summary>
    public Func<uint, string>? RequiredEnergy { get; init; }
}

/// <summary>PATCH00414710의이름·기술·빌더·원소분류·최대비용을 실제AI본체에 적용한다.
/// 근거: cpppj/src/o/RawAiBrainInitialUpdate.h/.cpp. 지갑2d8과SP는 재지정하지 않는다.</summary>
public sealed class RawAiBrainInitialUpdate
{
    /// <summary>생성자와 같은 본체·목록 소유자다.</summary>
    private readonly RawAiBrainMemory _memory;
    /// <summary>현재9Player와기술부모 생성기다.</summary>
    private readonly RawPlayerWorld _player;
    /// <summary>실제 문자열→40개목록→Ensure/내용물 생성 사슬이다.</summary>
    private readonly RawPlayerTechnology _technology;
    /// <summary>현재 내용물타입6·권한·죽음 진단 전역이다.</summary>
    private readonly ContainedFinderState _finder;
    /// <summary>현재비용·flags2·theme의TYPE표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>복사본 없이 현재 장기 전역을 빌린다.</summary>
    private readonly AiBrainInitialUpdateGlobals _globals;
    /// <summary>실제이름·요구에너지 문자열 조회다.</summary>
    private readonly AiBrainInitialUpdateHooks _hooks;

    /// <summary>같은PATCH풀·기술부모·내용물·TYPE와현재워크샾4개를 연결한다.</summary>
    public RawAiBrainInitialUpdate(RawAiBrainMemory memory, RawPlayerWorld player, RawPlayerTechnology technology,
        ContainedFinderState finder, IReadOnlyList<OriginalTypeRecord> types,
        AiBrainInitialUpdateGlobals globals, AiBrainInitialUpdateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(player);
        ArgumentNullException.ThrowIfNull(technology);
        ArgumentNullException.ThrowIfNull(finder);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(player.Pool(), technology.Pool()) || player.Pool().Edition != OriginalEdition.Patch1078
            || types.Count == 0 || globals.Workshops is null || globals.Workshops.Length != 4
            || globals.MissionMode is null || globals.ExcludedType is null)
            throw new ArgumentException("AI초기갱신 같은PATCH세계/TYPE/워크샾4개 누락");
        _memory = memory; _player = player; _technology = technology; _finder = finder;
        _types = types; _globals = globals; _hooks = hooks;
    }

    /// <summary>실제 소유메모리 안의 정렬되지 않은DWORD를 읽는다.</summary>
    private static uint Read(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));
    /// <summary>주변원문을 바꾸지 않고 정확히4바이트만 쓴다.</summary>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset, 4), value);
    /// <summary>원본ftol의signed64절삭 하위DWORD다. FISTP무효결과 하위DWORD는0이다.</summary>
    private static uint Truncate(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0
        ? 0u : unchecked((uint)(long)Math.Truncate(value));
    /// <summary>같은 소유메모리에서 첫NUL까지 실제8비트 문자열을 읽는다.</summary>
    private static string Text(byte[] raw, int offset)
    {
        int end = Array.IndexOf(raw, (byte)0, offset);
        if (end < 0) throw new ArgumentOutOfRangeException(nameof(offset), "AI초기갱신NUL문자열 소유메모리 범위 오류");
        return Encoding.Latin1.GetString(raw, offset, end - offset);
    }
    /// <summary>현대문자열 뒤의NUL이후는 에너지C문자열 길이에 포함하지 않는다.</summary>
    private static int Length(string value)
    {
        int nul = value.IndexOf('\0');
        return nul < 0 ? value.Length : nul;
    }
    /// <summary>도달한 실제 조회가 없으면 명시 거부한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("AI초기갱신 실제 조회 미연결");
    /// <summary>타입0도 조회하되 현재실제표 밖의 원문메모리손상은 거부한다.</summary>
    private OriginalTypeRecord Type(uint number) => number < _types.Count ? _types[(int)number]
        : throw new ArgumentOutOfRangeException(nameof(number), "AI초기갱신 실제TYPE범위 오류");
    /// <summary>원본004709d0의theme0..3워크샾과theme4/5의0반환이다.</summary>
    private uint Workshop(int theme)
    {
        if (theme < 0 || theme > 5) throw new ArgumentOutOfRangeException(nameof(theme), "AI초기갱신 실제원소 범위 오류");
        return theme < 4 ? _globals.Workshops[theme] : 0;
    }

    /// <summary>600바이트재복사→선택이름→실제기술부모/목록→원소분류를실행한다. 기존누적개수는원문위치만초기화한다.</summary>
    public void Update(uint token, ReadOnlySpan<byte> settings)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount || settings.Length != RawMissionAiSettings.Size)
            throw new ArgumentException("AI초기갱신 실제본체/600바이트설정 누락");
        uint nameOwner = Read(raw, 0x260);
        if (nameOwner >= 9) throw new ArgumentOutOfRangeException(nameof(token), "AI초기갱신 실제이름owner범위 오류");
        settings.CopyTo(raw);
        if (_globals.MissionMode() != 0)
        {
            string name = raw[0] != 0 ? Text(raw, 0) : Need(_hooks.DefaultName)();
            name = name[..Length(name)];
            byte[] encoded = Encoding.Latin1.GetBytes(name), records = _player.Records();
            int offset = (int)nameOwner * 180 + 12;
            if (offset + encoded.Length + 1 > records.Length) throw new ArgumentOutOfRangeException(nameof(settings), "AI초기갱신 실제이름 저장소범위 오류");
            encoded.CopyTo(records, offset); records[offset + encoded.Length] = 0;
        }
        // 설정재복사뒤의 실제owner를 각 원문조회 위치에서 다시 읽는다.
        int Owner() => unchecked((int)Read(raw, 0x260));
        if (_player.Technology(Owner()) == 0) _player.Create(Owner());
        if (_player.Technology(Owner()) == 0) return;
        if (raw[0x1e] != 0) _technology.AddList(Owner(), Text(raw, 0x1e));
        var counts = new int[10];
        // 분류플래그5개와최대비용만초기화한다. 빌더/원소목록개수·394·390·ad8은유지한다.
        for (int index = 0; index < 5; index++) Put(raw, 0x3b4 + index * 4, 0);
        Put(raw, 0x290, 0);
        uint parent = _player.Technology(Owner());
        if (parent > ushort.MaxValue || parent >= _player.Pool().Capacity) throw new ArgumentOutOfRangeException(nameof(token), "AI초기갱신 실제기술부모 범위 오류");
        var cursor = new RawContainedFinder(_player.Pool(), _finder);
        // 다음SID를먼저저장하는실제커서로현재flags4/TYPE를방문시읽는다.
        for (int sid = cursor.Begin((int)parent); sid != 0; sid = cursor.Next())
        {
            ReadOnlySpan<byte> piece = _player.Pool().Slot(sid);
            uint type = BinaryPrimitives.ReadUInt16LittleEndian(piece.Slice(18, 2));
            OriginalTypeRecord entry = Type(type);
            if ((piece[20] & 4) == 0) continue;
            int builderCount = unchecked((int)Read(raw, 0x28c)), builderCapacity = unchecked((int)Read(raw, 0x288));
            if ((entry.Flags2 & 0x30000u) != 0 && builderCount < builderCapacity)
            {
                if (builderCount < 0) throw new ArgumentOutOfRangeException(nameof(token), "AI초기갱신 실제빌더count범위 오류");
                Put(_memory.Bytes(Read(raw, 0x284)), builderCount * 4, type);
                Put(raw, 0x28c, unchecked((uint)builderCount + 1));
            }
            int firstLength = Length(Need(_hooks.RequiredEnergy)(type));
            if (Read(raw, 0xad8) <= firstLength) Put(raw, 0xad8, (uint)Length(Need(_hooks.RequiredEnergy)(type)));
            double maximum = unchecked((int)Read(raw, 0x290)), cost = entry.Cost;
            Put(raw, 0x290, Truncate(maximum > cost ? maximum : cost));
            if (type == _globals.ExcludedType()) continue;
            int theme = entry.Theme;
            if (theme < 0 || theme > 5) throw new ArgumentOutOfRangeException(nameof(token), "AI초기갱신 원본분류stack범위 오류");
            // theme5와sun battery개수는원문처럼실제counts[5]하나를공유한다.
            counts[theme] = unchecked(counts[theme] + 1);
            if ((entry.Flags2 & 0x800u) != 0)
            {
                if (theme + 5 >= counts.Length) throw new ArgumentOutOfRangeException(nameof(token), "AI초기갱신battery분류stack범위 오류");
                counts[theme + 5] = unchecked(counts[theme + 5] + 1);
                if (Read(raw, 0x390) == 3) Put(raw, 0x390, (uint)theme);
            }
            if (counts[theme] % 5 == 1)
            {
                int count = unchecked((int)Read(raw, 0x3b0));
                if (count < 0) throw new ArgumentOutOfRangeException(nameof(token), "AI초기갱신 실제원소count범위 오류");
                Put(raw, 0x398 + count * 4, (uint)theme); Put(raw, 0x3b0, unchecked(Read(raw, 0x3b0) + 1));
                uint candidate = Workshop(theme), previous = Read(raw, 0x394);
                if (previous == 0 || unchecked((int)Truncate(Type(candidate).Cost)) > unchecked((int)Truncate(Type(previous).Cost))) Put(raw, 0x394, candidate);
            }
        }
        // 단독battery원소를중립3으로치환하거나한번제거하며기존목록전체를압축한다.
        for (int theme = 0; theme < 5; theme++)
        {
            if (counts[theme + 5] != 1 || counts[theme] != 1) continue;
            int removed = 0;
            // 제거후에도source는원래index로진행하고destination만한칸앞에쓴다.
            for (int index = 0; index < unchecked((int)Read(raw, 0x3b0)); index++)
            {
                uint value = Read(raw, 0x398 + index * 4);
                Put(raw, 0x398 + (index - removed) * 4, value);
                if (value != theme) continue;
                if (counts[3] % 5 == 0)
                { Put(raw, 0x398 + index * 4, 3); Put(raw, 0x3b4 + theme * 4, 2); counts[3]++; break; }
                if (removed == 0) { removed = 1; Put(raw, 0x3b4 + theme * 4, 1); counts[3]++; }
            }
            Put(raw, 0x3b0, unchecked(Read(raw, 0x3b0) - (uint)removed));
        }
    }
}
