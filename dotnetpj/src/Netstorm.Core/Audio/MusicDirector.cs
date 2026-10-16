namespace Netstorm.Core.Audio;

/// <summary>음악 감독이 재생 장치에 보내는 요청 종류.</summary>
public enum AudioCueKind
{
    /// <summary>배경음악을 이 곡으로 바꾼다 (한 번 재생, 끝나면 감독이 다음 곡을 정한다)</summary>
    Music,

    /// <summary>효과음을 한 번 재생한다</summary>
    Sound,
}

/// <summary>재생 요청 하나.</summary>
/// <param name="Kind">종류</param>
/// <param name="Name">원본 파일 이름 (예: "rain22.mus", "thunderCrack.wav")</param>
public sealed record AudioCue(AudioCueKind Kind, string Name);

/// <summary>
/// 원본 배경음악 선택 규칙 (docs/exe/music.md 2절, exe FUN_00469fc0·00469f00·00469f60·00469db0·00469c80·00494eb0).
/// <list type="bullet">
/// <item>전투 밖(메뉴)은 <see cref="MenuTrack"/>.</item>
/// <item>전투는 원소 곡 4개를 wind → rain → thunder → sun 순으로 한 곡씩 돌린다. 첫 곡은 난수 색인의 다음 곡이다. 천둥 곡은 thunderCrack.wav 를 함께 튼다.</item>
/// <item>내 제단에서 희생 의식이 시작되면 곧바로 <see cref="SacrificeTrack"/>. 곡이 끝났을 때 의식이 계속 중이면 다시 희생 음악, 아니면 다음 원소 곡이다
/// (의식이 끝나도 곡 끝까지는 희생 음악이 이어진다 — 2026-09-30 녹화와 일치).</item>
/// <item>곡 끝 시각은 실시간이다 (게임 정지와 무관). 30초 이하 곡은 180초 뒤에 다시 확인한다.</item>
/// <item>결과 음악(fanfare·defeat)이 재생 중이면 끝날 때까지 다른 요청을 무시한다.</item>
/// </list>
/// </summary>
public sealed class MusicDirector
{
    /// <summary>메뉴(전투 밖) 음악</summary>
    public const string MenuTrack = "ser22.mus";

    /// <summary>희생 의식 음악 (모든 미션 공통)</summary>
    public const string SacrificeTrack = "sacrifice.mus";

    /// <summary>승리 결과 음악</summary>
    public const string VictoryTrack = "fanfare.mus";

    /// <summary>패배 결과 음악</summary>
    public const string DefeatTrack = "defeat.mus";

    /// <summary>멀티플레이 대기실 음악 (결과 음악 잠금을 무시하는 유일한 곡)</summary>
    public const string LobbyTrack = "anticipation.mus";

    /// <summary>전투 원소 곡 표 (exe 0x5409e8, 색인 0~3)</summary>
    public static readonly IReadOnlyList<string> BattleTracks = ["wind22.mus", "rain22.mus", "thu22.mus", "sun22.mus"];

    /// <summary>천둥 곡과 함께 트는 효과음 (exe 0x5409d8[2])</summary>
    public const string ThunderCue = "thunderCrack.wav";

    /// <summary>천둥 곡의 색인</summary>
    private const int ThunderIndex = 2;

    /// <summary>메뉴 음악을 틀 때 두는 색인 (exe FUN_00469fc0 의 3)</summary>
    private const int MenuIndex = 3;

    /// <summary>첫 곡 난수의 범위 (exe FUN_004558f0(0, 4000))</summary>
    public const int StartRandomRange = 4000;

    /// <summary>첫 곡 난수를 색인으로 바꾸는 나눗수</summary>
    private const int StartRandomDivisor = 1000;

    /// <summary>이 길이(초) 이하인 곡은 곡 길이 대신 <see cref="ShortTrackRecheckSeconds"/> 뒤에 다음 곡을 정한다 (exe 0x501708 = 30.0)</summary>
    public const double ShortTrackSeconds = 30.0;

    /// <summary>짧은 곡의 다음 확인까지 시간(초) (exe 0x5019e0 = 180.0)</summary>
    public const double ShortTrackRecheckSeconds = 180.0;

    /// <summary>곡 이름 → 길이(초). 모르는 곡은 0 을 돌려준다.</summary>
    private readonly Func<string, double> _durationOf;

    /// <summary>아직 재생 장치로 보내지 않은 요청</summary>
    private readonly List<AudioCue> _pending = [];

    /// <summary>결과 음악이 끝나는 시각 (그 전에는 다른 곡 요청을 무시한다)</summary>
    private double _resultLockUntil = double.NegativeInfinity;

    /// <summary>지금 곡 이름 (없으면 null)</summary>
    public string? Current { get; private set; }

    /// <summary>지금 곡을 시작한 실시간(초)</summary>
    public double CurrentStartedAt { get; private set; }

    /// <summary>다음 곡을 정할 실시간(초)</summary>
    public double NextChangeAt { get; private set; } = double.PositiveInfinity;

    /// <summary>원소 곡 색인 (0~3)</summary>
    public int Index { get; private set; } = MenuIndex;

    /// <summary>전투 음악 순환 중인지</summary>
    public bool InBattle { get; private set; }

    /// <summary>곡 길이 조회 함수로 만든다</summary>
    /// <param name="durationOf">곡 파일 이름 → 길이(초)</param>
    public MusicDirector(Func<string, double> durationOf) => _durationOf = durationOf;

    /// <summary>메뉴 음악을 시작한다 (전투 밖 화면 진입)</summary>
    /// <param name="now">실시간(초)</param>
    public void StartMenu(double now)
    {
        InBattle = false;
        Index = MenuIndex;
        Request(MenuTrack, now);
    }

    /// <summary>전투 음악 순환을 시작한다 (미션 진입). 첫 곡은 난수 색인의 다음 곡이다.</summary>
    /// <param name="now">실시간(초)</param>
    /// <param name="random">0 이상 <see cref="StartRandomRange"/> 미만의 난수</param>
    public void StartBattle(double now, int random)
    {
        InBattle = true;
        Index = Math.Clamp(random, 0, StartRandomRange - 1) / StartRandomDivisor;
        Advance(now, sacrificing: false);
    }

    /// <summary>매 프레임 부른다. 곡 끝 시각이 지났으면 다음 곡을 정한다.</summary>
    /// <param name="now">실시간(초)</param>
    /// <param name="mySacrificeInProgress">내 제단의 희생 의식이 진행 중인지</param>
    public void Update(double now, bool mySacrificeInProgress)
    {
        if (now < NextChangeAt)
        {
            return;
        }
        if (InBattle)
        {
            Advance(now, mySacrificeInProgress);
        }
        else
        {
            Request(MenuTrack, now);
        }
        if (now >= NextChangeAt && Current != null)
        {
            // 같은 곡을 다시 고른 경우(곡 끝에서 의식이 계속되는 등): 원본은 요청을 무시하지만 클론은 곡을 처음부터 다시 튼다
            Play(Current, now);
        }
    }

    /// <summary>내 제단의 희생 의식이 시작됐다 (exe FUN_00494eb0: 제단 소유자가 나일 때만)</summary>
    /// <param name="now">실시간(초)</param>
    public void OnMySacrificeStarted(double now) => Request(SacrificeTrack, now);

    /// <summary>전투 결과 음악을 튼다 (결과 화면). 끝날 때까지 다른 요청을 막는다.</summary>
    /// <param name="now">실시간(초)</param>
    /// <param name="victory">승리이면 fanfare, 아니면 defeat</param>
    public void PlayResult(double now, bool victory)
    {
        string track = victory ? VictoryTrack : DefeatTrack;
        Request(track, now);
        if (Current == track)
        {
            _resultLockUntil = now + _durationOf(track);
        }
    }

    /// <summary>쌓인 재생 요청을 꺼내고 비운다</summary>
    public IReadOnlyList<AudioCue> TakeCues()
    {
        AudioCue[] cues = [.. _pending];
        _pending.Clear();
        return cues;
    }

    /// <summary>다음 원소 곡(또는 희생 음악)으로 넘어간다 (exe FUN_00469f00)</summary>
    private void Advance(double now, bool sacrificing)
    {
        Index = (Index + 1) % BattleTracks.Count;
        if (sacrificing)
        {
            Request(SacrificeTrack, now);
            return;
        }
        Request(BattleTracks[Index], now);
        if (Index == ThunderIndex)
        {
            _pending.Add(new AudioCue(AudioCueKind.Sound, ThunderCue));
        }
    }

    /// <summary>곡 요청 (exe FUN_00469db0): 같은 곡이면 무시, 결과 음악 재생 중이면 대기실 음악 외 무시</summary>
    private void Request(string track, double now)
    {
        if (string.Equals(track, Current, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }
        if (now < _resultLockUntil && !track.Equals(LobbyTrack, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }
        Play(track, now);
    }

    /// <summary>곡을 바꾸고 다음 곡을 정할 시각을 잡는다</summary>
    private void Play(string track, double now)
    {
        Current = track;
        CurrentStartedAt = now;
        double duration = _durationOf(track);
        NextChangeAt = now + (duration > ShortTrackSeconds ? duration : ShortTrackRecheckSeconds);
        _pending.Add(new AudioCue(AudioCueKind.Music, track));
    }
}
