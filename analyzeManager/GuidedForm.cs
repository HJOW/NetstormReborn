using System.Diagnostics;
using System.Drawing;
using System.Drawing.Text;
using System.Text;
using System.Text.Json;

namespace Netstorm.AnalyzeManager;

/// <summary>게임 옆에 조작 순서를 보여주고 사용자가 녹화를 시작·중단할 수 있는 창.</summary>
public sealed class GuidedForm : Form
{
    /// <summary>안내 파일을 한 번에 지나치게 많이 읽지 않도록 제한한다.</summary>
    private const int StepsLimitBytes = 64_000;
    private readonly SessionStore _store;
    private readonly AnalysisSession _session;
    private readonly string _stepsPath;
    private readonly string _statePath;
    private readonly Label _heading;
    private readonly TextBox _instruction;
    private readonly Label _status;
    private readonly Button _previous;
    private readonly Button _next;
    private readonly Button _start;
    private readonly Button _stop;
    private readonly System.Windows.Forms.Timer _statusTimer;
    private readonly PrivateFontCollection? _privateFonts;
    private FileStream? _desktopLock;
    private GuidedRecorder? _recorder;
    private string[] _steps = [];
    private int _step;
    /// <summary>마지막으로 안내 창을 배치한 주 게임 창의 바깥 좌표.</summary>
    private Rectangle? _placedGameBounds;

    /// <summary>기존 관리 세션과 UTF-8 안내를 읽고 게임을 가리지 않는 위치에 창을 놓는다.</summary>
    public GuidedForm(SessionStore store, string sessionId, string? suppliedSteps)
    {
        _store = store;
        _session = store.Load(sessionId);
        using Process? process = store.OwnedProcess(_session);
        if (process == null) throw new InvalidOperationException("기존 세션의 게임이 실행 중이어야 안내 모드를 열 수 있습니다.");
        string directory = store.SessionDirectory(sessionId);
        _stepsPath = Path.Combine(directory, "guide-steps.txt");
        _statePath = Path.Combine(directory, "guide-state.json");
        if (suppliedSteps != null) CopySteps(suppliedSteps);
        LoadSteps();
        if (File.Exists(_statePath))
        {
            GuideState? state = JsonSerializer.Deserialize<GuideState>(File.ReadAllText(_statePath, Encoding.UTF8), SessionStore.Json);
            _step = Math.Clamp(state?.Step ?? 0, 0, _steps.Length - 1);
        }
        Text = "NetStorm 원본 분석 안내";
        FormBorderStyle = FormBorderStyle.FixedToolWindow;
        StartPosition = FormStartPosition.Manual;
        MaximizeBox = false;
        MinimizeBox = false;
        TopMost = true;
        ClientSize = new Size(320, 460);
        string? fontFile = Directory.Exists(Path.Combine(store.Repository, "fonts"))
            ? Directory.EnumerateFiles(Path.Combine(store.Repository, "fonts"), "D2Coding*.ttc").FirstOrDefault() : null;
        if (fontFile != null)
        {
            PrivateFontCollection? loaded = null;
            try
            {
                loaded = new PrivateFontCollection();
                loaded.AddFontFile(fontFile);
            }
            catch (ArgumentException) { loaded?.Dispose(); loaded = null; }
            _privateFonts = loaded;
        }
        Font = _privateFonts?.Families.Length > 0 ? new Font(_privateFonts.Families[0], 10) : new Font("Malgun Gothic", 10);
        _heading = new Label { Left = 12, Top = 12, Width = 296, Height = 28, Font = new Font(Font, FontStyle.Bold) };
        _instruction = new TextBox { Left = 12, Top = 50, Width = 296, Height = 230, BorderStyle = BorderStyle.FixedSingle,
            Multiline = true, ReadOnly = true, WordWrap = true, ScrollBars = ScrollBars.Vertical, TabStop = false };
        _previous = new Button { Text = "이전", Left = 12, Top = 290, Width = 90, Height = 36 };
        _next = new Button { Text = "다음 단계", Left = 110, Top = 290, Width = 198, Height = 36 };
        _start = new Button { Text = "녹화 시작 / 이어서", Left = 12, Top = 338, Width = 188, Height = 36 };
        _stop = new Button { Text = "녹화 중단", Left = 208, Top = 338, Width = 100, Height = 36, Enabled = false };
        var reload = new Button { Text = "안내 다시 읽기", Left = 12, Top = 384, Width = 296, Height = 30 };
        _status = new Label { Left = 12, Top = 422, Width = 296, Height = 30, Text = "녹화 대기" };
        Controls.AddRange([_heading, _instruction, _previous, _next, _start, _stop, reload, _status]);
        _previous.Click += (_, _) => ChangeStep(-1);
        _next.Click += (_, _) => ChangeStep(1);
        _start.Click += (_, _) => StartRecording();
        _stop.Click += (_, _) => StopRecording();
        reload.Click += (_, _) => { LoadSteps(); RestoreGameFocus(); };
        _statusTimer = new System.Windows.Forms.Timer { Interval = 1000 };
        _statusTimer.Tick += (_, _) => UpdateStatus();
        if (!PositionBesideGame(WindowsGame.OuterBounds(WindowsGame.FindWindow(process.Id, preferForeground: false))))
            throw new InvalidOperationException("게임 옆에 안내 창을 놓을 공간이 없습니다. 더 넓은 화면이나 두 번째 모니터를 사용하세요.");
        UpdateStep();
        _desktopLock = store.LockDesktop();
        _statusTimer.Start();
    }

    /// <summary>새 안내 텍스트를 세션 폴더에 복사해 CLI를 다시 열어도 같은 지시를 보여준다.</summary>
    private void CopySteps(string source)
    {
        SessionStore.RejectReparse(source);
        if (new FileInfo(source).Length > StepsLimitBytes) throw new ArgumentException("안내 파일은 64 KB 이하여야 합니다.");
        byte[] bytes = File.ReadAllBytes(source);
        SessionStore.WriteSmallFile(_stepsPath, bytes);
    }

    /// <summary>빈 줄을 제외한 각 줄을 하나의 조작 단계로 읽는다.</summary>
    private void LoadSteps()
    {
        if (!File.Exists(_stepsPath)) throw new ArgumentException("--steps-file로 UTF-8 안내 파일을 지정하세요.");
        if (new FileInfo(_stepsPath).Length > StepsLimitBytes) throw new ArgumentException("안내 파일은 64 KB 이하여야 합니다.");
        _steps = File.ReadAllLines(_stepsPath, new UTF8Encoding(false, true))
            .Select(line => line.Trim()).Where(line => line.Length != 0).ToArray();
        if (_steps.Length == 0) throw new ArgumentException("안내 파일에 조작 단계가 없습니다.");
        _step = Math.Clamp(_step, 0, _steps.Length - 1);
        if (_instruction != null) UpdateStep();
    }

    /// <summary>주 게임 창의 테두리까지 피해 안내 창을 오른쪽이나 왼쪽 작업 영역에 배치한다.</summary>
    private bool PositionBesideGame(Rectangle gameBounds)
    {
        Point? position = GuidePlacement.Beside(gameBounds, Size, Screen.AllScreens.Select(screen => screen.WorkingArea));
        if (position == null) return false;
        Location = position.Value;
        _placedGameBounds = gameBounds;
        return true;
    }

    /// <summary>게임 창의 크기·위치가 바뀌었거나 안내 창과 겹치면 새 위치를 찾는다.</summary>
    private bool KeepBesideGame(int processId)
    {
        GameWindow game = WindowsGame.FindWindow(processId, preferForeground: false);
        Rectangle bounds = WindowsGame.OuterBounds(game);
        return (_placedGameBounds == bounds && !bounds.IntersectsWith(Bounds)) || PositionBesideGame(bounds);
    }

    /// <summary>현재 지시와 단계 번호를 갱신한다.</summary>
    private void UpdateStep()
    {
        _heading.Text = $"단계 {_step + 1} / {_steps.Length}";
        _instruction.Text = _steps[_step];
        _previous.Enabled = _step > 0;
        _next.Enabled = _step < _steps.Length - 1;
    }

    /// <summary>사용자가 단계를 바꾸면 그 시각과 지시 내용을 세션 이벤트에 남긴다.</summary>
    private void ChangeStep(int direction)
    {
        int next = Math.Clamp(_step + direction, 0, _steps.Length - 1);
        if (next == _step) return;
        _step = next;
        SaveState();
        _store.Append(_session, "guided_step", new { index = _step + 1, total = _steps.Length, instruction = _steps[_step] });
        UpdateStep();
        RestoreGameFocus();
    }

    /// <summary>안내 창을 닫고 다시 열 때 같은 단계를 복구한다.</summary>
    private void SaveState() => SessionStore.WriteSmallFile(_statePath, JsonSerializer.SerializeToUtf8Bytes(new GuideState(_step), SessionStore.Json));

    /// <summary>녹화 전용 잠금을 잡은 뒤 세 기록기를 시작한다.</summary>
    private void StartRecording()
    {
        if (_recorder != null) return;
        try
        {
            using Process? process = _store.OwnedProcess(_session);
            if (process == null || !KeepBesideGame(process.Id))
                throw new InvalidOperationException("게임 옆에 안내 창을 놓을 공간이 없어 녹화를 시작할 수 없습니다.");
            _recorder = new GuidedRecorder(_store, _session);
            _recorder.Start();
            _store.Append(_session, "guided_step", new { index = _step + 1, total = _steps.Length, instruction = _steps[_step] });
            _start.Enabled = false;
            _stop.Enabled = true;
            RestoreGameFocus();
        }
        catch (Exception error)
        {
            _recorder?.Dispose();
            _recorder = null;
            MessageBox.Show(this, error.Message, "녹화 시작 실패", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    /// <summary>영상과 오디오 조각을 닫고 세션은 그대로 두어 이후 녹화를 재개할 수 있게 한다.</summary>
    private void StopRecording()
    {
        if (_recorder == null) return;
        Exception? error = _recorder.Error;
        _recorder.Dispose();
        _recorder = null;
        _start.Enabled = true;
        _stop.Enabled = false;
        _status.Text = error == null ? "녹화 중단됨 · 다시 시작 가능" : $"녹화 오류: {error.Message}";
        SaveState();
        RestoreGameFocus();
    }

    /// <summary>게임 창 변경에 맞춰 안내를 다시 배치하고 녹화 오류·프레임 수를 갱신한다.</summary>
    private void UpdateStatus()
    {
        if (_recorder?.Error != null)
        {
            StopRecording();
            return;
        }
        using Process? process = _store.OwnedProcess(_session);
        if (process == null)
        {
            if (_recorder != null) StopRecording();
            _status.Text = "게임 종료 · 녹화 저장됨";
            return;
        }
        try
        {
            if (!KeepBesideGame(process.Id))
            {
                if (_recorder != null) StopRecording();
                _status.Text = "게임 옆에 안내 창을 놓을 공간이 없습니다.";
                return;
            }
        }
        catch (Exception error)
        {
            _status.Text = "게임 창 위치 확인 실패: " + error.Message;
            return;
        }
        if (_recorder == null) return;
        _status.Text = $"녹화 중 · 영상 {_recorder.FrameCount}프레임";
    }

    /// <summary>버튼으로 안내를 조작한 직후 입력 초점을 게임으로 되돌린다.</summary>
    private async void RestoreGameFocus()
    {
        try
        {
            using Process? process = _store.OwnedProcess(_session);
            if (process != null) await WindowsGame.FocusAsync(process.Id, CancellationToken.None);
        }
        catch (Exception error) { _status.Text = "게임 포커스 복귀 실패: " + error.Message; }
    }

    /// <summary>안내 창을 닫아도 게임을 종료하지 않고 녹화 파일만 안전하게 닫는다.</summary>
    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        _statusTimer.Stop();
        try
        {
            StopRecording();
            SaveState();
        }
        finally
        {
            _desktopLock?.Dispose();
            _desktopLock = null;
        }
        base.OnFormClosing(e);
    }

    /// <summary>안내 창에서 사용한 D2Coding 개인 글꼴을 해제한다.</summary>
    protected override void Dispose(bool disposing)
    {
        if (disposing) _privateFonts?.Dispose();
        base.Dispose(disposing);
    }

    /// <summary>마지막으로 표시한 안내 단계의 세션 상태.</summary>
    private sealed record GuideState(int Step);
}
