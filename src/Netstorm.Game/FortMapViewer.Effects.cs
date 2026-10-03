using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 원본 녹화에서 측정한 화면 연출(규칙과 무관): 생산 창 Storm Power 숫자의 따라가기·깜빡임, 떨어지는 유닛·다리 조각,
/// 건설·룬 진행음(jimbuild.wav)과 기절 보호막 소리(priestForceField.wav)의 반복, 지도 위 짧은 알림 문구.
/// 근거는 docs/videos/record-play-final-20261002.md (2026-09-30~10-01 녹화 세 개의 최종 판독).
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>Storm Power 숫자를 한 단계 따라가는 초당 횟수 (녹화 측정 약 60~67회/초, 원본은 화면 프레임마다)</summary>
    private const double StormPowerStepsPerSecond = 60;

    /// <summary>Storm Power 숫자 깜빡임 한 번의 길이(초) (exe FUN_0043db10·0x506858 = 0.15)</summary>
    private const double StormPowerBlinkSeconds = 0.15;

    /// <summary>Storm Power 숫자 깜빡임의 켜짐/꺼짐 전환 횟수 (exe FUN_0043db10 이 +0xb0 에 넣는 10)</summary>
    private const int StormPowerBlinkToggles = 10;

    /// <summary>떨어지는 물체의 처음 아래쪽 속도(논리 픽셀/초). 2026-10-01 녹화 13:22 골렘 낙하 궤적 맞춤값.</summary>
    private const float FallStartSpeed = 30f;

    /// <summary>떨어지는 물체의 아래쪽 가속도(논리 픽셀/초²). 같은 궤적 맞춤값(1.6초에 약 190px).</summary>
    private const float FallAcceleration = 109f;

    /// <summary>발판을 잃은 지상 유닛이 떨어지다 사라질 때까지(초). 녹화 13:21.6 붕괴 → 13:23.7 소멸.</summary>
    private const double UnitFallSeconds = 1.8;

    /// <summary>무너진 다리 조각이 떨어지다 사라질 때까지(초). 녹화 14:02.4 붕괴 → 14:03.4 무렵 소멸.</summary>
    private const double BridgeFallSeconds = 1.0;

    /// <summary>떨어지는 물체가 사라지기 전에 흐려지는 시간(초)</summary>
    private const double FallFadeSeconds = 0.3;

    /// <summary>jimbuild.wav 반복 간격(초) = 파일 길이. 녹음에서 건설 중·룬 마크 동안 1.14초마다 울렸다.</summary>
    private const double JimBuildSeconds = 1.14;

    /// <summary>priestForceField.wav 반복 간격(초) = 파일 길이 1.238초. 녹음에서 기절 사제가 있는 동안 이어서 울렸다.</summary>
    private const double ForceFieldSeconds = 1.238;

    /// <summary>내 오브젝트를 잃었을 때의 알림음 (exe 0x4b0b89, 오브젝트 제거 함수에서 내 것이 파괴되면 재생)</summary>
    private const string UnitLostSound = "unitLost.wav";

    /// <summary>
    /// 직전 갱신의 내 오브젝트 위치·종류. 파괴 사건은 이미 지워진 오브젝트 번호만 알려 주므로, 잃은 것이 내 것인지와 위치를 여기서 찾는다.
    /// </summary>
    private readonly Dictionary<int, (int X, int Y, ObjectKind Kind)> _myEntities = [];

    /// <summary>마지막으로 잃은 내 오브젝트의 칸 (U 키로 이동, 원본 _DAT_005c848c/90)</summary>
    private (int X, int Y)? _lastLostCell;

    /// <summary>지도 위 알림 문구를 보여 주는 시간(초)</summary>
    private const double NoticeSeconds = 4.0;

    /// <summary>알림 문구가 흐려지기 시작하는 남은 시간(초)</summary>
    private const double NoticeFadeSeconds = 1.0;

    /// <summary>건설 진행·룬 진행음</summary>
    private const string JimBuildSound = "jimbuild.wav";

    /// <summary>기절 사제의 보호막 소리</summary>
    private const string ForceFieldSound = "priestForceField.wav";

    /// <summary>생산 창에 표시 중인 Storm Power (처음 그릴 때 실제 값으로 맞춘다)</summary>
    private int? _shownStormPower;

    /// <summary>따라가기 단계를 쌓아 두는 시간(초)</summary>
    private double _stormPowerStepClock;

    /// <summary>남은 깜빡임 전환 수 (홀수인 동안 숫자를 숨긴다, 원본 FUN_0043da10 의 +0xb0 &amp; 1)</summary>
    private int _stormPowerBlinks;

    /// <summary>다음 깜빡임 전환까지 남은 시간(초)</summary>
    private double _stormPowerBlinkClock;

    /// <summary>떨어지고 있는 유닛·다리 조각 그림</summary>
    private readonly List<FallingSprite> _falling = [];

    /// <summary>다음 jimbuild.wav 까지 남은 시간(초, 0 이면 조건이 생기자마자 튼다)</summary>
    private double _jimBuildClock;

    /// <summary>다음 priestForceField.wav 까지 남은 시간(초)</summary>
    private double _forceFieldClock;

    /// <summary>마지막으로 그린 논리 화면 크기 (화면 안 판정용)</summary>
    private Point _viewSize;

    /// <summary>지금 지도 위에 보여 주는 알림 문구</summary>
    private string _shownNotice = "";

    /// <summary>알림 문구를 보여 준 지 지난 시간(초)</summary>
    private double _noticeAge = double.MaxValue;

    /// <summary>떨어지는 그림 하나</summary>
    /// <param name="TypeIndex">셰이프 블록 번호</param>
    /// <param name="Frame">프레임 번호</param>
    /// <param name="Color">소유자 색 번호 (0 = 원래 색)</param>
    /// <param name="World">떨어지기 시작한 기준점의 월드 픽셀 좌표</param>
    /// <param name="Duration">사라질 때까지의 시간(초)</param>
    private sealed record FallingSprite(int TypeIndex, int Frame, int Color, Vector2 World, double Duration)
    {
        /// <summary>떨어지기 시작한 뒤 지난 시간(초)</summary>
        public double Age { get; set; }
    }

    /// <summary>
    /// 매 갱신의 화면 연출을 진행한다: 세션의 낙하 기록을 그림으로 옮기고, Storm Power 숫자를 따라가게 하며, 반복 효과음을 낸다.
    /// 게임 시간이 멈춘 동안(일시정지·안내 창)은 원본처럼 숫자를 곧바로 실제 값으로 맞추고 낙하·반복음을 멈춘다.
    /// </summary>
    /// <param name="seconds">지난 갱신 이후 경과 시간(초)</param>
    private void UpdateEffects(double seconds)
    {
        bool running = SimulationRunning && !TutorialDialogOpen;
        // 세션이 알린 낙하를 떨어지는 그림으로 바꾼다
        foreach (FallenObject fallen in _session.DrainFallen()) AddFalling(fallen);
        if (running)
        {
            // 떨어지는 그림의 나이를 늘리고 다 떨어진 것은 지운다
            foreach (FallingSprite sprite in _falling) sprite.Age += seconds;
            _falling.RemoveAll(sprite => sprite.Age >= sprite.Duration);
        }
        UpdateStormPowerDisplay(seconds, running);
        // 다음 갱신의 파괴 사건에서 찾을 수 있도록 지금 내 오브젝트를 기억한다
        _myEntities.Clear();
        foreach (GameEntity entity in _session.Entities.Where(e => e.Owner == TestPlayer))
            _myEntities[entity.Id] = (entity.Footprint.AnchorX, entity.Footprint.AnchorY, entity.Kind);
        _noticeAge = _noticeAge >= double.MaxValue / 2 ? _noticeAge : _noticeAge + seconds;
        if (!running || _mission == null)
        {
            _jimBuildClock = 0; _forceFieldClock = 0;
            return;
        }
        // 화면 안에 건설 중인 오브젝트가 있거나, 화면 안 제단의 룬 마크가 타는 동안 jimbuild.wav 를 이어서 튼다
        bool building = _session.Entities.Any(e => !e.IsComplete && OnScreen(e.Footprint.AnchorX, e.Footprint.AnchorY))
            || _session.Rituals.Any(r => r.RuneMarked && _session.Entity(r.AltarId) is { } altar && OnScreen(altar.Footprint.AnchorX, altar.Footprint.AnchorY));
        _jimBuildClock = RepeatSound(building, _jimBuildClock, seconds, JimBuildSeconds, JimBuildSound);
        // 화면 안에 기절한 사제가 있으면 보호막 소리를 이어서 튼다
        bool shielded = _session.Entities.Any(e => e.Kind == ObjectKind.Priest && e.IsStunned && OnScreen(e.Footprint.AnchorX, e.Footprint.AnchorY));
        _forceFieldClock = RepeatSound(shielded, _forceFieldClock, seconds, ForceFieldSeconds, ForceFieldSound);
    }

    /// <summary>조건이 이어지는 동안 효과음을 일정 간격으로 반복한다. 조건이 끊기면 다음에는 곧바로 튼다.</summary>
    /// <param name="active">반복할 조건</param>
    /// <param name="clock">다음 재생까지 남은 시간(초)</param>
    /// <param name="seconds">이번 갱신의 경과 시간(초)</param>
    /// <param name="period">반복 간격(초)</param>
    /// <param name="sound">효과음 파일 이름</param>
    /// <returns>갱신된 남은 시간</returns>
    private double RepeatSound(bool active, double clock, double seconds, double period, string sound)
    {
        if (!active) return 0;
        clock -= seconds;
        if (clock > 0) return clock;
        QueueSound(sound);
        return Math.Max(clock + period, 0);
    }

    /// <summary>
    /// 전투로 파괴된 오브젝트가 내 것이면 unitLost.wav 를 내고 위치를 U 키용으로 기억한다.
    /// exe(0x4b0a71~0x4b0ba0)는 내 오브젝트가 제거될 때 일부 종류·제거 이유를 빼고 재생한다. 클론은 판매·낙하(별도 사건)와
    /// 공짜로 다시 생기는 비행 공격체를 뺀다 — 2026-10-01 캠페인 1-2 녹화 20분 동안 3번(09:30.5·13:17.5·13:27.5)만 울렸다.
    /// </summary>
    /// <param name="entityId">파괴된 오브젝트 번호</param>
    private void OnEntityDestroyed(int entityId)
    {
        if (!_myEntities.TryGetValue(entityId, out (int X, int Y, ObjectKind Kind) lost) || lost.Kind == ObjectKind.Flyer) return;
        QueueSound(UnitLostSound);
        _lastLostCell = (lost.X, lost.Y);
    }

    /// <summary>낙하 기록 하나를 지금 그림(유닛 기본 프레임 또는 무너진 다리 칸 프레임)으로 떨어뜨린다.</summary>
    private void AddFalling(FallenObject fallen)
    {
        if (fallen.Entity is { } entity)
        {
            int frame = entity.Type.Definition.Frames.DefaultFrame;
            _falling.Add(new FallingSprite(entity.Type.LoadIndex, frame, _playerColors.GetValueOrDefault(entity.Owner),
                WorldPixels(entity.Footprint.AnchorX, entity.Footprint.AnchorY) + HotFootShift(entity.Type), UnitFallSeconds));
        }
        else if (fallen.Cell is { } cell)
        {
            int frame = BridgeFrames.Find(_bridgeType.Definition.Frames, cell.Cell, cell.Condition);
            _falling.Add(new FallingSprite(_bridgeType.LoadIndex, frame, 0, WorldPixels(cell.X, cell.Y), BridgeFallSeconds));
        }
    }

    /// <summary>떨어지는 그림을 등가속 낙하 위치에 그리고 사라지기 직전에 흐리게 한다.</summary>
    private void DrawFalling(SpriteBatch batch, Vector2 center)
    {
        // 먼저 떨어진 것부터 그린다
        foreach (FallingSprite sprite in _falling)
        {
            float t = (float)sprite.Age;
            float drop = FallStartSpeed * t + 0.5f * FallAcceleration * t * t;
            float alpha = (float)Math.Clamp((sprite.Duration - sprite.Age) / FallFadeSeconds, 0, 1);
            DrawSprite(batch, sprite.TypeIndex, sprite.Frame, Screen(sprite.World + new Vector2(0, drop), center), sprite.Color, alpha: alpha);
        }
    }

    /// <summary>
    /// 생산 창 Storm Power 숫자를 원본처럼 실제 값으로 천천히 따라가게 한다(<see cref="StormPower.StepDisplay"/>).
    /// 게임 시간이 멈춘 동안은 곧바로 실제 값이 된다 — 2026-10-01 녹화에서 성공 창(게임 정지)과 같은 프레임에 +5,000 이 한 번에 올랐다.
    /// </summary>
    /// <param name="seconds">경과 시간(초)</param>
    /// <param name="running">게임 시간이 흐르는지</param>
    private void UpdateStormPowerDisplay(double seconds, bool running)
    {
        int actual = _session.Player(TestPlayer).StormPower;
        if (_shownStormPower is not int shown || !running)
        {
            _shownStormPower = actual;
            _stormPowerStepClock = 0;
        }
        else
        {
            _stormPowerStepClock += seconds;
            // 쌓인 시간만큼 한 단계씩 따라간다
            while (_stormPowerStepClock >= 1 / StormPowerStepsPerSecond && shown != actual)
            {
                _stormPowerStepClock -= 1 / StormPowerStepsPerSecond;
                shown = StormPower.StepDisplay(shown, actual);
            }
            if (shown == actual) _stormPowerStepClock = 0;
            _shownStormPower = shown;
        }
        if (_stormPowerBlinks <= 0) return;
        _stormPowerBlinkClock -= seconds;
        // 0.15초마다 켜짐/꺼짐을 바꾼다
        while (_stormPowerBlinkClock <= 0 && _stormPowerBlinks > 0)
        {
            _stormPowerBlinks--;
            _stormPowerBlinkClock += StormPowerBlinkSeconds;
        }
    }

    /// <summary>Storm Power 가 모자라 생산 창 유닛을 집지 못했을 때 숫자를 깜빡인다 (exe 0x43ecc4 → FUN_0043db10).</summary>
    private void BlinkStormPower()
    {
        _stormPowerBlinks = StormPowerBlinkToggles;
        _stormPowerBlinkClock = StormPowerBlinkSeconds;
    }

    /// <summary>깜빡임 중 숫자를 숨기는 차례인지</summary>
    private bool StormPowerHidden => _stormPowerBlinks % 2 == 1;

    /// <summary>칸 좌표의 기준점이 마지막으로 그린 지도 화면 안에 있는지 (사이드바 제외)</summary>
    private bool OnScreen(int x, int y)
    {
        if (_viewSize == Point.Zero) return false;
        Vector2 point = Screen(WorldPixels(x, y), _lastCenter);
        return point.X >= (_playUi ? PlaySidebarWidth : 0) && point.X < _viewSize.X && point.Y >= 0 && point.Y < _viewSize.Y;
    }

    /// <summary>
    /// 공개 캠페인 지도 아래쪽 가운데에 최근 알림을 그림자 글자로 잠깐 띄운다. 원본은 상태줄이 없으므로 돌 막대 없이 그린다.
    /// </summary>
    private void DrawNotice(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        if (_notice != _shownNotice)
        {
            _shownNotice = _notice;
            _noticeAge = 0;
        }
        if (_shownNotice.Length == 0 || _noticeAge >= NoticeSeconds) return;
        float alpha = (float)Math.Clamp((NoticeSeconds - _noticeAge) / NoticeFadeSeconds, 0, 1);
        Vector2 size = font.MeasureString(_shownNotice);
        float left = PlaySidebarWidth + (width - PlaySidebarWidth - size.X) / 2;
        var position = new Vector2(MathF.Round(Math.Max(PlaySidebarWidth + 4, left)), MathF.Round(height - size.Y - 24));
        batch.DrawString(font, _shownNotice, position + Vector2.One, Color.Black * (0.9f * alpha));
        batch.DrawString(font, _shownNotice, position, Color.Wheat * alpha);
    }
}
