using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Assets;

// 원본 10.78의 AI 설정 저장소와 처리기를 제공한다.
namespace Netstorm.Core.Original;

/// <summary>AI 설정 기본값에 쓰는 실제 정수 전역과 능력 진단 모드다.</summary>
public sealed class MissionAiSettingsState
{
    /// <summary>구조물 갱신 간격의 정수 전역이다.</summary>
    public int StuffRefreshRate { get; set; } = 3;
    /// <summary>초기 SP의 정수 전역이다.</summary>
    public int StartingSp { get; set; } = 2500;
    /// <summary>미등록 능력을 로그 전에 assert로 진단할지 나타낸다.</summary>
    public bool CheckingAsserts { get; set; }
}

/// <summary>현재 미션 설정과 CRT 부동소수점 변환 및 진단 효과다.</summary>
public sealed class MissionAiSettingsHooks
{
    /// <summary>현재 미션의 설정 키를 조회한다.</summary>
    public Func<string, string>? Get { get; init; }
    /// <summary>비어 있지 않은 비율 문자열을 원본 CRT 방식으로 변환한다.</summary>
    public Func<string, double>? ParseFloat { get; init; }
    /// <summary>미등록 능력의 assert 진단을 실행한다.</summary>
    public Action<string>? UnknownAbilityAssertion { get; init; }
    /// <summary>미등록 능력의 로그 진단을 실행한다.</summary>
    public Action<string>? UnknownAbilityLog { get; init; }
}

/// <summary>10.78 미션 AI가 사용하는 600바이트 설정의 기본값과 공통·소유자 덮어쓰기다.
/// 근거: cpppj/src/o/RawMissionAiSettings.h/.cpp, 원본 0040ef80·0040f030·004c2d90.</summary>
public sealed class RawMissionAiSettings
{
    /// <summary>원본 AI 설정 저장소 크기다.</summary>
    public const int Size = 600;
    /// <summary>기본 기술 목록의 원본 바이트 문자열이다.</summary>
    private const string DefaultTech = "sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher";
    /// <summary>원본 능력 비트 위치에 대응하는 이름 표다.</summary>
    private static readonly string[] Abilities = ["FOCUS_ON_THINGS_THAT_ATTACK_BUILDINGS", "BUILD_BLOCKERS_WHEN_BUILDINGS_HARMED", "ATTACH_TO_ENEMY_ISLAND", "STOP_COLLECTING_FROM_DANGEROUS_GEYSERS", "COLLECT_STORM_POWER", "ENCIRCLE_ENEMY_ISLAND", "CONCENTRATE_FIRE", "REPEAT_SUCCESSFUL_DROPS", "CAPTURE_ENEMY_PRIEST", "BUILD_DAIS", "SACRIFICE_ENEMY_PRIEST", "USE_PRIEST_TO_COLLECT", "BUILD_BRIDGES"];
    /// <summary>원본 색 번호에 대응하는 이름 표다.</summary>
    private static readonly string[] Colors = ["none", "blue", "red", "white", "green", "purple", "yellow", "lightblue", "orange"];
    /// <summary>원본 atof 적용 순서와 필드 위치다.</summary>
    private static readonly (string Key, int Offset)[] Rates = [("BridgeDrawRate", 0x220), ("StuffRefreshRate", 0x224), ("TimeBetweenMoves", 0x228)];
    /// <summary>원본 atol 적용 순서와 필드 위치다.</summary>
    private static readonly (string Key, int Offset)[] Integers = [("StartMoney", 0x22c), ("GeyserAttachments", 0x230), ("MaxInHand", 0x238), ("Collectors", 0x234), ("Enemy", 0x240), ("NoTemple", 0x244), ("Off", 0x254)];
    /// <summary>현재 기본 전역과 진단 상태다.</summary>
    private readonly MissionAiSettingsState _state;
    /// <summary>현재 설정과 변환·진단 효과다.</summary>
    private readonly MissionAiSettingsHooks _hooks;

    /// <summary>10.78 설정 처리기를 만들고 현재 전역과 실제 조회를 빌린다.</summary>
    /// <param name="edition">원본 판본.</param><param name="state">현재 전역.</param><param name="hooks">설정·변환·진단 효과.</param>
    public RawMissionAiSettings(OriginalEdition edition, MissionAiSettingsState state, MissionAiSettingsHooks hooks)
    {
        if (edition != OriginalEdition.Patch1078 || hooks.Get == null) throw new ArgumentException("MissionAI는 10.78과 현재 설정 조회가 필요하다");
        _state = state; _hooks = hooks;
    }

    /// <summary>실제 600바이트 계약을 확인한다.</summary>
    /// <param name="raw">설정 저장소.</param>
    private static void Check(byte[] raw) { if (raw.Length != Size) throw new ArgumentException("AI 설정은 600바이트여야 한다", nameof(raw)); }
    /// <summary>원본 little-endian DWORD를 읽는다.</summary>
    /// <param name="raw">저장소.</param><param name="offset">필드 위치.</param>
    private static uint Read(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));
    /// <summary>원본 little-endian DWORD만 쓰고 주변 바이트는 보존한다.</summary>
    /// <param name="raw">저장소.</param><param name="offset">필드 위치.</param><param name="value">쓸 DWORD.</param>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset, 4), value);
    /// <summary>첫 NUL까지의 원본 바이트와 종료 NUL을 쓰며 저장소 범위 이탈을 거부한다.</summary>
    /// <param name="raw">저장소.</param><param name="offset">문자열 위치.</param><param name="value">원본 바이트 문자열.</param>
    private static void Copy(byte[] raw, int offset, string value)
    {
        int end = value.IndexOf('\0');
        if (end < 0) end = value.Length;
        if (end >= raw.Length - offset) throw new ArgumentOutOfRangeException(nameof(value), "AI 설정 문자열이 저장소를 벗어난다");
        // 원본 바이트 문자열을 한 바이트씩 쓰고 뒤의 기존 바이트는 지우지 않는다.
        for (int index = 0; index < end; index++) raw[offset + index] = unchecked((byte)value[index]);
        raw[offset + end] = 0;
    }
    /// <summary>ASCII 영문 대소문자만 무시하여 원본 이름을 비교한다.</summary>
    /// <param name="left">첫 문자열.</param><param name="right">둘째 문자열.</param>
    private static bool Equal(string left, string right)
    {
        if (left.Length != right.Length) return false;
        // 첫 불일치에서 중단하며 비ASCII 바이트는 그대로 비교한다.
        for (int i = 0; i < left.Length; i++)
        {
            char a = left[i], b = right[i];
            if (a is >= 'A' and <= 'Z') a = (char)(a + 32);
            if (b is >= 'A' and <= 'Z') b = (char)(b + 32);
            if (a != b) return false;
        }
        return true;
    }

    /// <summary>원본 기본값이 지정하는 바이트만 덮어쓰고 나머지는 그대로 둔다.</summary>
    /// <param name="raw">호출자가 보유한 실제 설정 저장소.</param>
    public void Initialize(byte[] raw)
    {
        Check(raw); raw[0] = 0; Copy(raw, 0x1e, DefaultTech);
        Put(raw, 0x220, BitConverter.SingleToUInt32Bits(6f));
        Put(raw, 0x224, BitConverter.SingleToUInt32Bits((float)_state.StuffRefreshRate));
        Put(raw, 0x228, BitConverter.SingleToUInt32Bits(3f));
        Put(raw, 0x22c, unchecked((uint)((long)_state.StartingSp + 4000)));
        Put(raw, 0x230, 3); Put(raw, 0x234, 3); Put(raw, 0x238, 4); Put(raw, 0x23c, 0);
        Put(raw, 0x240, 0); Put(raw, 0x244, 0); Put(raw, 0x24c, 0); Put(raw, 0x250, 0); Put(raw, 0x254, 0); Put(raw, 0x248, 0xffff);
    }

    /// <summary>능력 이름의 비트를 변경한다. 미등록 이름도 진단 후 13번 비트를 변경한다.</summary>
    /// <param name="raw">설정 저장소.</param><param name="name">능력 이름.</param><param name="enabled">비트를 켤지.</param>
    public void SetAbility(byte[] raw, string name, bool enabled)
    {
        Check(raw);
        int nul = name.IndexOf('\0'); if (nul >= 0) name = name[..nul];
        if (name.Length == 0) return;
        int index = 0;
        // 이름이 일치하거나 원본 표의 끝에 도달할 때까지 찾는다.
        while (index < Abilities.Length && !Equal(name, Abilities[index])) index++;
        if (index == Abilities.Length)
        {
            if (_state.CheckingAsserts) (_hooks.UnknownAbilityAssertion ?? throw new InvalidOperationException("AI 능력 assert 연결 누락"))(name);
            (_hooks.UnknownAbilityLog ?? throw new InvalidOperationException("AI 능력 로그 연결 누락"))(name);
        }
        uint bit = 1u << (index & 31), current = Read(raw, 0x248);
        Put(raw, 0x248, enabled ? current | bit : current & ~bit);
    }

    /// <summary>현재 미션 설정의 공통 또는 소유자별 필드를 원본 조회 순서로 누적 적용한다.</summary>
    /// <param name="raw">설정 저장소.</param><param name="owner">양수면 해당 소유자, 그 외에는 공통 설정.</param>
    public void Apply(byte[] raw, int owner)
    {
        Check(raw);
        string prefix = "ai" + (owner > 0 ? owner.ToString(CultureInfo.InvariantCulture) : "");
        string Get(string key) => _hooks.Get!(prefix + key);
        string value = Get("Name"); if (value.Length != 0) Copy(raw, 0, value);
        value = Get("Tech"); if (value.Length != 0) Copy(raw, 0x1e, value);
        // 세 비율은 값이 있을 때만 CRT 변환을 호출하고 float 비트로 저장한다.
        foreach (var (key, offset) in Rates)
        {
            value = Get(key);
            if (value.Length != 0) Put(raw, offset, BitConverter.SingleToUInt32Bits((float)(_hooks.ParseFloat ?? throw new InvalidOperationException("AI 설정 atof 연결 누락"))(value)));
        }
        // 일곱 정수 설정을 원본 순서대로 적용한다.
        foreach (var (key, offset) in Integers)
        {
            value = Get(key);
            if (value.Length != 0) Put(raw, offset, unchecked((uint)ConfigNumbers.ParseLong(value)));
        }
        value = Get("Color");
        // none과 미등록 색은 기존 색을 유지한다.
        for (int color = 1; color < Colors.Length; color++) if (Equal(value, Colors[color])) { Put(raw, 0x250, (uint)color); break; }
        // 목록은 조회를 항목마다 반복하고 양수 동맹 비트를 누적한다.
        for (int index = 0; index < 10; index++)
        {
            int ally = ConfigNumbers.ParseLong(ConfigNumbers.ListItem(Get("AllyList"), index));
            if (ally > 0) Put(raw, 0x23c, Read(raw, 0x23c) | (1u << (ally & 31)));
        }
        // 능력 목록의 선두 느낌표 하나만 제거하고 비트 해제로 처리한다.
        for (int index = 0; index < 10; index++)
        {
            value = ConfigNumbers.ListItem(Get("Ability"), index);
            bool enabled = value.Length == 0 || value[0] != '!';
            SetAbility(raw, enabled ? value : value[1..], enabled);
        }
    }

    /// <summary>같은 저장소에 기본값, 공통 설정, 소유자 설정을 차례로 누적한다.</summary>
    /// <param name="raw">설정 저장소.</param><param name="owner">소유자 번호.</param>
    public void Prepare(byte[] raw, int owner) { Initialize(raw); Apply(raw, -1); Apply(raw, owner); }
}
