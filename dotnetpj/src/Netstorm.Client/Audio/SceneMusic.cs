using System.Buffers.Binary;
using Netstorm.Core.Original;

// 원문 장면 음악의 판단/현재 전역은 장치 재생과 구분하여 같은 Client 수명에 보유한다.
namespace Netstorm.Client.Audio;

/// <summary>cpppj SoundSceneMusic의 실제 전역이다. Session/음향 장치 교체에서 재초기화하지 않는다.</summary>
public sealed class SceneMusicState
{
    /// <summary>원소 곡의 현재 색인이다. 원문 최초값은 메뉴/해인3이다.</summary>
    public int Index { get; set; } = 3;
    /// <summary>승리 곡의 실제 끝시각이다.</summary>
    public double FanfareEnd { get; set; }
    /// <summary>패배 곡의 실제 끝시각이다.</summary>
    public double DefeatEnd { get; set; }
    /// <summary>00565e68의 실제8BYTE를 State 정리와 음악 감독이 함께 빌린다.</summary>
    public DoubleBox SongEnd { get; } = new();
    /// <summary>승리 곡 허용 판정의 현재 결과DWORD다.</summary>
    public uint ResultState { get; set; }
    /// <summary>현재 조회가 없는 독립/CD 경로의 명시 전투DWORD다.</summary>
    public uint Battle { get; set; }
    /// <summary>Player 표가 준비됐는지 나타내는 현재DWORD다.</summary>
    public uint PlayersReady { get; set; }
    /// <summary>희생 판정에 전달할 현재 로컬Player번호다.</summary>
    public uint LocalPlayer { get; set; }
    /// <summary>설치되면 원문 소비 위치마다 실제 현재00594fbc를 다시 조회한다.</summary>
    public Func<uint>? CurrentBattle { get; set; }
    /// <summary>현재 날씨색이다. 원문 최초값은167이다.</summary>
    public uint Tint { get; set; } = 167;
    /// <summary>팔레트 초기화가 채우는 원소별 실제 색번호4개다.</summary>
    public uint[] Tints { get; } = new uint[4];
    /// <summary>날씨 팔레트 적용 뒤 세우는 화면 갱신DWORD다.</summary>
    public uint PaletteDirty { get; set; }
}

/// <summary>시계/곡 선택/실제 월드/설정/표시/효과음의 필수 장치다. 생성 시 모두 검증한다.</summary>
public sealed class SceneMusicHooks
{
    /// <summary>정지와 무관한 원문 벽시각이다.</summary>
    public Func<double>? WallSeconds { get; init; }
    /// <summary>현재곡의 첫NUL앞 좁은 이름이다.</summary>
    public Func<string>? CurrentName { get; init; }
    /// <summary>실제 곡 선택과 현재이름/채널 갱신이다.</summary>
    public Action<string>? Select { get; init; }
    /// <summary>현재 기본채널의 실제 곡길이 초다.</summary>
    public Func<double>? Duration { get; init; }
    /// <summary>현재Player의 제단 희생 단계 판정이다.</summary>
    public Func<uint, bool>? Sacrificing { get; init; }
    /// <summary>현재 멀티플레이 대기실 판정이다.</summary>
    public Func<bool>? WaitingRoom { get; init; }
    /// <summary>현재 화면 갱신 요청이다.</summary>
    public Action? Refresh { get; init; }
    /// <summary>현재 ascendancyPalette 옵션 판정이다.</summary>
    public Func<bool>? AscendancyPalette { get; init; }
    /// <summary>선택한 실제 날씨 팔레트를 적용한다.</summary>
    public Action<string>? LoadPalette { get; init; }
    /// <summary>실제 번개 효과를 시작한다.</summary>
    public Action? ThunderFlash { get; init; }
    /// <summary>실제 이름의 효과음을 모든 추가 인자0으로 한 번 재생한다.</summary>
    public Action<string>? WeatherSound { get; init; }
}

/// <summary>00469c80~00469fc0의 실제 음악 감독이다. 시각읽기 수/NaN/부작용 순서를 보존하며 재생장치는 소유하지 않는다.</summary>
public sealed class SceneMusic
{
    /// <summary>메뉴 곡의 원문 이름이다.</summary>
    public const string MenuMusic = "ser22.mus";
    /// <summary>희생 곡의 원문 이름이다.</summary>
    public const string SacrificeMusic = "sacrifice.mus";
    /// <summary>대기실 곡의 원문 이름이다.</summary>
    public const string AnticipationMusic = "anticipation.mus";
    /// <summary>승리 곡의 원문 이름이다.</summary>
    public const string FanfareMusic = "fanfare.mus";
    /// <summary>패배 곡의 원문 이름이다.</summary>
    public const string DefeatMusic = "defeat.mus";
    /// <summary>바람/비/천둥/해 순서의 원문 곡 표다.</summary>
    private static readonly string[] ElementMusic = ["wind22.mus", "rain22.mus", "thu22.mus", "sun22.mus"];
    /// <summary>같은 색인으로 조회하는 원문 팔레트 이름 표다.</summary>
    private static readonly string[] Palettes = ["windy.col", "rainy.col", "thundery.col", "sunny.col"];
    /// <summary>천둥에만 이름이 존재하는 원문 날씨 효과음 표다.</summary>
    private static readonly string[] WeatherSounds = ["", "", "thunderCrack.wav", ""];
    /// <summary>같은Client의 실제 장기 전역이다.</summary>
    private readonly SceneMusicState _state;
    /// <summary>구름/Session과 공유하는 실제 게임난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>검증한 실제 하위효과 장치다.</summary>
    private readonly SceneMusicHooks _hooks;

    /// <summary>필수 하위누락을 생성 시 거부하며 OS자원/곡/난수를 아직 선택하지 않는다.</summary>
    public SceneMusic(SceneMusicState state, GameRandom random, SceneMusicHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.WallSeconds is null || hooks.CurrentName is null || hooks.Select is null || hooks.Duration is null || hooks.Sacrificing is null || hooks.WaitingRoom is null || hooks.Refresh is null || hooks.AscendancyPalette is null || hooks.LoadPalette is null || hooks.ThunderFlash is null || hooks.WeatherSound is null)
            throw new ArgumentException("장면 음악의 필수 경계 누락", nameof(hooks));
        _state = state; _random = random; _hooks = hooks;
    }
    /// <summary>ASCII A~Z만 접어 원문 C locale의 이름 비교를 수행한다.</summary>
    private static bool Same(string left, string right)
    {
        if (left.Length != right.Length) return false;
        // Unicode 언어 규칙을 섞지 않고 각 현재 좁은 문자를 비교한다.
        for (int index = 0; index < left.Length; index++)
        {
            char a = left[index], b = right[index]; if (a is >= 'A' and <= 'Z') a = (char)(a + 32); if (b is >= 'A' and <= 'Z') b = (char)(b + 32);
            if (a != b) return false;
        }
        return true;
    }
    /// <summary>원문 표밖 미정읽기는 정상값으로 감추지 않고 실제 선택 위치에서 거부한다.</summary>
    private static T At<T>(T[] table, int index) => index >= 0 && index < table.Length ? table[index] : throw new ArgumentOutOfRangeException(nameof(index), "장면 음악 색인0~3밖 읽기");
    /// <summary>실제기본음악채널+0x18의8BYTE길이를리틀엔디언double로읽는다. 채널을열거나다른필드를변경하지않는다.</summary>
    public static double ChannelDuration(ReadOnlySpan<byte> primary)
    {
        if (primary.Length < 0x20) throw new ArgumentException("음악채널길이의실제8BYTE물리부족", nameof(primary));
        return BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(primary.Slice(0x18, 8)));
    }
    /// <summary>현재원문 전투조회가 있으면 저장한 fallback을 읽지 않는다.</summary>
    public uint CurrentBattle() => _state.CurrentBattle is { } read ? read() : _state.Battle;
    /// <summary>결과 곡의 잠금을 원문 재조회/시계순서로 판정한다. 비교불가도 잠긴상태다.</summary>
    public bool ResultMusicLocked()
    {
        if (Same(_hooks.CurrentName!(), FanfareMusic) && !(_hooks.WallSeconds!() >= _state.FanfareEnd)) return true;
        return Same(_hooks.CurrentName!(), DefeatMusic) && !(_hooks.WallSeconds!() >= _state.DefeatEnd);
    }
    /// <summary>같은곡/잠금/승리상태 관문 뒤 곡을 선택하고 원문 횟수대로 현재길이와 시계를 읽는다.</summary>
    public void Request(string name)
    {
        if (Same(_hooks.CurrentName!(), name)) return;
        if (ResultMusicLocked() && !Same(name, AnticipationMusic)) return;
        if (Same(name, FanfareMusic) && _state.ResultState != 1) return;
        _hooks.Select!(name);
        if (Same(name, FanfareMusic)) { double length = _hooks.Duration!(); _state.FanfareEnd = length + _hooks.WallSeconds!(); }
        else if (Same(name, DefeatMusic)) { double length = _hooks.Duration!(); _state.DefeatEnd = length + _hooks.WallSeconds!(); }
        if (30.0 < _hooks.Duration!()) { double length = _hooks.Duration!(); _state.SongEnd.Value = length + _hooks.WallSeconds!(); return; }
        _state.SongEnd.Value = _hooks.WallSeconds!() + 180.0;
    }
    /// <summary>색인을 먼저32비트로 올려 C나머지를 적용하고 희생/원소곡과 날씨 효과를 순서대로 선택한다.</summary>
    public void Next()
    {
        _state.Index = unchecked(_state.Index + 1) % 4;
        if (_state.PlayersReady != 0 && _hooks.Sacrificing!(_state.LocalPlayer)) { Request(SacrificeMusic); return; }
        Request(At(ElementMusic, _state.Index)); ApplyWeather();
    }
    /// <summary>시각이 만료되고 비교가능할때만 현재battle을 읽어 다음곡/대기실/메뉴를 선택한다.</summary>
    public void Frame()
    {
        if (!(_hooks.WallSeconds!() >= _state.SongEnd.Value)) return;
        if (CurrentBattle() != 0) { Next(); return; }
        if (_hooks.WaitingRoom!()) { Request(AnticipationMusic); return; }
        Request(MenuMusic);
    }
    /// <summary>현재battle로첫곡을고른뒤Frame은현재모드를다시읽을수있으며마지막에현재색인의tint를쓴다.</summary>
    public void Start()
    {
        if (CurrentBattle() == 0) { _state.Index = 3; Request(MenuMusic); }
        else { _state.Index = (int)(_random.Next(4000) / 1000); Next(); }
        Frame(); _state.Tint = At(_state.Tints, _state.Index);
    }
    /// <summary>화면갱신→옵션/색/팔레트/dirty→현재효과음/번개/재생 순서를 지킨다.</summary>
    public void ApplyWeather()
    {
        _hooks.Refresh!();
        if (_hooks.AscendancyPalette!()) { _state.Tint = At(_state.Tints, _state.Index); _hooks.LoadPalette!(At(Palettes, _state.Index)); _state.PaletteDirty = 1; }
        string sound = At(WeatherSounds, _state.Index); if (sound.Length == 0) return;
        if (_state.Index == 2) _hooks.ThunderFlash!(); _hooks.WeatherSound!(sound);
    }
}
