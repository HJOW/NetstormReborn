using System.Buffers.Binary;
using System.Globalization;
using System.Runtime.Versioning;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원문 Runtime4800행과 실제 Win32 worker의 합류·재시작을 창·음향 장치 없이 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>원문 커널/COM/파일 입력만 공급하고 두 채널의 수명·갱신·정리를 실제 본체에서 얻는다.</summary>
public sealed class MusicRuntimeTests
{
    /// <summary>원본 실행기에 공급한 음악 버퍼 토큰이다.</summary>
    private const uint BufferToken = 0x16001010;
    /// <summary>원본 실행기에 공급한 열린 파일 토큰이다.</summary>
    private const uint FileToken = 0x17001000;
    /// <summary>원본 실행기에 공급한 종료 이벤트 핸들이다.</summary>
    private static readonly nint EventToken = 0x18001000;
    /// <summary>원본 실행기에 공급한 음악 스레드 핸들이다.</summary>
    private static readonly nint ThreadToken = 0x18002000;
    /// <summary>COM 실패 입력의 실제 signed E_FAIL이다.</summary>
    private const int Failure = unchecked((int)0x80004005);
    /// <summary>원문 DWORD 비트를 부호 없는 값으로 읽는다.</summary>
    private static uint Number(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>원문 BYTE 관찰의 소문자HEX 표기를 만든다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>이번 계층에서 선택하지 않는 효과음은 실제 호출 시 실패하게 둔다.</summary>
    private static SoundPlayer Effects(OriginalEdition edition, SoundState sound) => new(new SoundList(edition), sound, new()
    {
        Status = _ => throw new InvalidOperationException("예상하지 않은 효과음 조회"),
        Play = (_, _) => throw new InvalidOperationException("예상하지 않은 효과음 재생"),
        SetPosition = (_, _) => throw new InvalidOperationException("예상하지 않은 효과음 위치"),
        SetVolume = (_, _) => throw new InvalidOperationException("예상하지 않은 효과음 음량"),
        SetPan = (_, _) => throw new InvalidOperationException("예상하지 않은 효과음 좌우"),
        Stop = _ => throw new InvalidOperationException("예상하지 않은 효과음 정지"),
        Duplicate = (uint _, ref uint _) => throw new InvalidOperationException("예상하지 않은 효과음 복제"),
        Load = _ => throw new InvalidOperationException("예상하지 않은 효과음 적재"),
    });

    /// <summary>원문 Runtime fixture의 명시 IO/COM 입력과 감시 출력이다. 채널 상태 전이는 계산하지 않는다.</summary>
    private sealed class Boundary
    {
        /// <summary>원본 실행기와 같은21BYTE 독립 표본 입력이다.</summary>
        public readonly byte[] File = new byte[21];
        /// <summary>실제 FillBuffer의 두 직접 쓰기 구간과 나머지 감시 바이트다.</summary>
        public readonly byte[] Output = Enumerable.Repeat((byte)0xa7, 128).ToArray();
        /// <summary>원문 순서를 보존하는 IO/COM/커널 사건 목록이다.</summary>
        public readonly List<string> Events = [];
        /// <summary>명시 파일의 현재 절대 위치다.</summary>
        public int Position = 5;
        /// <summary>명시 파일 토큰이 아직 열려 있는지다.</summary>
        public bool Opened = true;
        /// <summary>COM의 직접 쓰기 구간을 아직 반납하지 않았는지다.</summary>
        public bool Locked;
        /// <summary>원문 재진입 잠금 관측의 현재 깊이다.</summary>
        public int LockDepth;
        /// <summary>원본에 공급한 실제 COM 상태 비트다.</summary>
        public uint Status;
        /// <summary>원문 실패 입력의 선택된 COM 위치다.</summary>
        public int Failed;
        /// <summary>추가 종료 검사에서 채널 Stop의 실제 파일 반납 예외를 입력한다.</summary>
        public bool CloseThrows;

        /// <summary>출력이 아닌 독립 입력 파일의 바이트를 배치한다.</summary>
        public Boundary()
        {
            // 원본 실행기와 같은 표본 패턴만 공급한다.
            for (int index = 0; index < File.Length; index++) File[index] = unchecked((byte)(index * 7 + 11));
        }

        /// <summary>원문 채널/개행 문장의 기록을 순서 있는 사건으로 만든다.</summary>
        public void Log(uint channel, string text) => Events.Add($"G:{channel}:{text.TrimEnd('\n')}");

        /// <summary>실제 Stop/Read/SetVolume이 소비할 파일/COM 응답만 공급한다.</summary>
        public MusicChannelHooks ChannelHooks() => new()
        {
            SetVolume = (token, volume) => { Assert.Equal(BufferToken, token); Events.Add($"V:#1:{volume}"); return Failed == 3 ? Failure : 0; },
            Stop = token => { Assert.Equal(BufferToken, token); Events.Add("X:#1"); },
            Release = token => { Assert.Equal(BufferToken, token); Events.Add("B:#1"); return 0; },
            Seek = (token, absolute) =>
            {
                int result = Opened && token == FileToken ? unchecked((int)absolute) : -1;
                Events.Add($"S:{token}:{absolute}:{result}");
                if (result != -1) Position = result;
                return result;
            },
            Read = (token, target) =>
            {
                int before = Position, result = -1;
                if (Opened && token == FileToken)
                {
                    result = Math.Min(target.Length, Position > File.Length ? 0 : File.Length - Position);
                    File.AsSpan(Position, result).CopyTo(target);
                    Position += result;
                }
                Events.Add($"R:{token}:{before}:{target.Length}:{result}");
                return result;
            },
            Close = token =>
            {
                Events.Add($"C:{token}");
                if (token == FileToken) Opened = false;
                if (CloseThrows) throw new IOException("종료 파일 반납 예외 입력");
            },
            Log = Log,
            LockEvent = entering => { LockDepth += entering ? 1 : -1; Assert.True(LockDepth >= 0); Events.Add(entering ? "E" : "L"); },
        };

        /// <summary>실제 FillBuffer에 원문 COM 반환과 두 직접 쓰기 구간만 공급한다.</summary>
        public MusicBufferHooks BufferHooks() => new()
        {
            Create = (ReadOnlySpan<byte> _, uint _, uint _, ref uint _) => throw new InvalidOperationException("Runtime4800 입력에서 선택하지 않은 버퍼 생성"),
            Status = (uint token, out uint value) =>
            {
                Assert.Equal(BufferToken, token); value = Status;
                int result = Failed == 1 ? Failure : 0;
                Events.Add($"T:#1:{value}:{result}");
                return result;
            },
            Restore = token => { Assert.Equal(BufferToken, token); int result = Failed == 2 ? Failure : 0; Events.Add($"A:#1:{result}"); return result; },
            Cursor = (uint token, out uint play, out uint write) =>
            {
                Assert.Equal(BufferToken, token); play = 0; write = 13;
                int result = Failed == 4 ? Failure : 0;
                Events.Add($"Q:#1:0:13:{result}");
                return result;
            },
            Lock = (uint token, uint offset, uint count, ref MusicBufferRegions regions) =>
            {
                Assert.Equal(BufferToken, token); Assert.False(Locked);
                int result = Failed == 5 ? Failure : 0;
                uint first = result < 0 ? 0 : count;
                if (result >= 0) { regions = new(Output.AsSpan(0, (int)first), Output.AsSpan(64, 0)); Locked = true; }
                Events.Add($"K:#1:{offset}:{count}:{first}:0:{result}");
                return result;
            },
            Unlock = (token, regions) =>
            {
                Assert.Equal(BufferToken, token); Assert.True(Locked); Locked = false;
                int result = Failed == 6 ? Failure : 0;
                Events.Add($"U:#1:{regions.First.Length}:{regions.Second.Length}:{result}");
                return result;
            },
            Play = (token, flags) => { Assert.Equal(BufferToken, token); int result = Failed == 7 ? Failure : 0; Events.Add($"P:#1:{flags}:{result}"); return result; },
        };
    }

    /// <summary>명시 대기 반환만 소비하며 초기화/반복/종료 정책은 구현하지 않는 커널 입력 경계다.</summary>
    private sealed class Kernel
    {
        /// <summary>채널과 같은 실제 사건 목록을 빌린다.</summary>
        private readonly Boundary _io;
        /// <summary>이벤트 생성의 성공 입력이다.</summary>
        public bool EventResult = true;
        /// <summary>스레드 생성의 성공 입력이다.</summary>
        public bool ThreadResult = true;
        /// <summary>종료 신호의 성공 입력이다.</summary>
        public bool SignalResult = true;
        /// <summary>원본에 공급한 순서 그대로의 대기 응답이다.</summary>
        public readonly Queue<uint> Waits = new();
        /// <summary>생성 경계에 넘긴 callback을 보관만 하며 동기 실행하지 않는다.</summary>
        public Func<uint>? Worker;
        /// <summary>같은 IO 사건 소유자만 보관한다.</summary>
        public Kernel(Boundary io) { _io = io; }
        /// <summary>원문 고정 핸들을 관찰과 같은 짧은 이름으로 만든다.</summary>
        private static string Handle(nint value) => value == EventToken ? "E" : value == ThreadToken ? "T" : ((long)value).ToString(CultureInfo.InvariantCulture);
        /// <summary>실제 커널 대신 명시 입력 반환만 공급한다.</summary>
        public MusicRuntimeHooks Hooks() => new()
        {
            Section = (index, initialize) => _io.Events.Add((initialize ? "I:" : "D:") + index),
            CreateEvent = () => { _io.Events.Add("A:" + (EventResult ? 1 : 0)); return EventResult ? EventToken : 0; },
            CreateThread = (Func<uint> worker, ref uint id) => { Worker = worker; id = 77; _io.Events.Add("T:77:" + (ThreadResult ? 1 : 0)); return ThreadResult ? ThreadToken : 0; },
            Wait = (handle, ms) =>
            {
                uint result = Waits.Count == 0 ? throw new InvalidOperationException("명시 대기 응답 소진") : Waits.Dequeue();
                _io.Events.Add($"W:{Handle(handle)}:{ms}:{result}");
                return result;
            },
            Signal = handle => { _io.Events.Add($"Z:{Handle(handle)}:{(SignalResult ? 1 : 0)}"); return SignalResult; },
            Terminate = handle => _io.Events.Add("J:" + Handle(handle)),
            Close = handle => { _io.Events.Add("O:" + Handle(handle)); if (handle == ThreadToken) Worker = null; },
            Log = _io.Log,
            Report = (_, line) => _io.Events.Add("!" + line),
        };
    }

    /// <summary>독립 입력 두 채널과 실제 공유 제어를 한 Runtime으로 조립한다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 전역이며 fixture 입력이 필요한 필드만 덮는다.</summary>
        public readonly SoundState Sound = new() { Initialized = true, Device = true, MusicVolume = -777 };
        /// <summary>실제 파일/COM 응답과 출력이다.</summary>
        public readonly Boundary Io = new();
        /// <summary>기본 채널의 실제96BYTE다.</summary>
        public readonly MusicChannelState A = new();
        /// <summary>두 번째 채널의 실제96BYTE다.</summary>
        public readonly MusicChannelState B = new();
        /// <summary>원문 핸들/번호의 실제 저장소다.</summary>
        public readonly MusicRuntimeState State = new() { ThreadId = 91 };
        /// <summary>같은 실제 상태의 효과음 부모다.</summary>
        public readonly SoundPlayer Effects;
        /// <summary>실제 기본 채널의 읽기/정지/갱신 본체다.</summary>
        public readonly MusicChannel Primary;
        /// <summary>초기화와 삭제에만 참여하는 두 번째 채널이다.</summary>
        public readonly MusicChannel Secondary;
        /// <summary>명시 기본 채널과 실제 명령 잠금이다.</summary>
        public readonly SoundMusicControl Music;
        /// <summary>결과를 계산하지 않는 명시 커널 응답이다.</summary>
        public readonly Kernel Kernel;

        /// <summary>원본 실행기와 같은 입력을 배치하고 실제 부모만 조립한다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            Span<byte> raw = A.Raw().Span;
            // 쓰지 않는 패딩과 원문 잠금 공간의 보존을 전수 관찰한다.
            for (int index = 0; index < raw.Length; index++) raw[index] = unchecked((byte)(index * 17 + 3));
            A.SetField(MusicField.Buffer, BufferToken); A.SetField(MusicField.BufferBytes, 16); A.SetField(MusicField.Flags, 3);
            A.SetField(MusicField.File, FileToken); A.SetField(MusicField.Length, 16); A.SetField(MusicField.ReadOffset, 0);
            A.SetField(MusicField.WriteOffset, 9); A.SetField(MusicField.DataOffset, 5); A.SetField(MusicField.StopDepth, 0);
            Span<byte> other = B.Raw().Span;
            // 두 번째 채널은 다른 감시 패턴을 사용해 별도 초기화를 확인한다.
            for (int index = 0; index < other.Length; index++) other[index] = unchecked((byte)(index * 11 + 5));
            Effects = MusicRuntimeTests.Effects(edition, Sound);
            Primary = new(edition, A, Io.ChannelHooks()); Secondary = new(edition, B, Io.ChannelHooks());
            Music = SoundMusicControl.ForPrimary(edition, Sound, Effects, Primary);
            Kernel = new(Io);
        }

        /// <summary>같은 실제 전역/채널/제어에 선택한 커널 응답을 결합한다.</summary>
        public MusicRuntime Runtime(MusicRuntimeHooks? hooks = null) => new(Sound, State, Primary, Secondary, Music, Io.BufferHooks(), hooks ?? Kernel.Hooks());
    }

    /// <summary>세 원본PE의 초기화·종료·worker·반복수명4800행을 사건/raw192/출력128/파일/핸들 전체로 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void RuntimeMatchesAllNativeObservations(string name, OriginalEdition edition)
    {
        string[][] all = X86Fixture.Read("musicruntime");
        Assert.Equal(4800, all.Length);
        string[][] rows = all.Where(row => row[0] == name).ToArray();
        Assert.Equal(1600, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(12, row.Length);
            string[] input = row[1].Split(','); Assert.Equal(13, input.Length);
            var scene = new Scene(edition);
            scene.Sound.MusicInitialized = Number(input[1]) != 0; scene.Sound.Initialized = Number(input[2]) != 0;
            scene.A.SetField(MusicField.Flags, Number(input[3])); scene.A.SetField(MusicField.Buffer, Number(input[4]) != 0 ? BufferToken : 0);
            scene.State.Shutdown = Number(input[5]) != 0 ? EventToken : 0; scene.State.Thread = Number(input[6]) != 0 ? ThreadToken : 0;
            scene.Kernel.EventResult = Number(input[7]) != 0; scene.Kernel.ThreadResult = Number(input[8]) != 0; scene.Kernel.SignalResult = Number(input[9]) != 0;
            // 외부 대기 응답만 배치하고 원문 재시도/worker 반복 수는 계산하지 않는다.
            foreach (string value in input[10].Split('|')) scene.Kernel.Waits.Enqueue(Number(value));
            scene.Io.Status = Number(input[11]); scene.Io.Failed = unchecked((int)Number(input[12]));
            using MusicRuntime runtime = scene.Runtime();
            string result = "-";
            if (input[0] == "I") runtime.Initialize();
            else if (input[0] == "D") runtime.Shutdown();
            else if (input[0] == "W") result = runtime.RunWorker().ToString(CultureInfo.InvariantCulture);
            else { runtime.Initialize(); runtime.Initialize(); runtime.Shutdown(); runtime.Shutdown(); }
            Assert.Equal(0, scene.Io.LockDepth); Assert.False(scene.Io.Locked); Assert.Null(runtime.WorkerFailure);
            bool same = (scene.Io.Events.Count == 0 ? "-" : string.Join(';', scene.Io.Events)) == row[2]
                && Hex(scene.A.Raw().Span) + Hex(scene.B.Raw().Span) == row[3] && Hex(scene.Io.Output) == row[4]
                && scene.Sound.MusicInitialized == (Number(row[5]) != 0)
                && scene.State.Thread == (nint)Number(row[6]) && scene.State.Shutdown == (nint)Number(row[7])
                && scene.State.ThreadId == Number(row[8]) && scene.Io.Position == Number(row[9])
                && scene.Io.Opened == (Number(row[10]) != 0) && result == row[11];
            if (!same) Assert.Fail($"Runtime {row[0]} {row[1]}\n사건 기대 {row[2]}\n사건 실제 {string.Join(';', scene.Io.Events)}\nraw 기대 {row[3]}\nraw 실제 {Hex(scene.A.Raw().Span)}{Hex(scene.B.Raw().Span)}");
            // 직접 worker 입력은 실제 생성한 스레드 수명을 소유하지 않는다.
            if (input[0] == "W") scene.Sound.MusicInitialized = false;
            return same;
        });
    }

    /// <summary>기존 callback 생성자와 다른 전역/채널의 혼용을 실제 Runtime이 거부한다.</summary>
    [Fact]
    public void RuntimeRejectsMissingOrForeignPrimaryAndKernelBindings()
    {
        var scene = new Scene();
        var legacy = new SoundMusicControl(OriginalEdition.Patch1078, scene.Sound, scene.Effects, scene.Primary.SetVolume);
        Assert.Throws<ArgumentException>(() => new MusicRuntime(scene.Sound, scene.State, scene.Primary, scene.Secondary, legacy, scene.Io.BufferHooks(), scene.Kernel.Hooks()));
        Assert.Throws<ArgumentException>(() => new MusicRuntime(scene.Sound, scene.State, scene.Primary, scene.Primary, scene.Music, scene.Io.BufferHooks(), scene.Kernel.Hooks()));
        Assert.Throws<ArgumentException>(() => new MusicRuntime(new SoundState(), scene.State, scene.Primary, scene.Secondary, scene.Music, scene.Io.BufferHooks(), scene.Kernel.Hooks()));
        Assert.Throws<ArgumentException>(() => new MusicRuntime(scene.Sound, scene.State, scene.Secondary, scene.Primary, scene.Music, scene.Io.BufferHooks(), scene.Kernel.Hooks()));
        Assert.Throws<ArgumentException>(() => scene.Runtime(new()));
    }

    /// <summary>초기화 assert가 예외를 던져도 이미 생성한 핸들을 같은 종료 본체로 반납한다.</summary>
    [Fact]
    public void InitializationDiagnosticExceptionCleansCreatedHandles()
    {
        var scene = new Scene(); scene.Kernel.EventResult = false; scene.Kernel.Waits.Enqueue(0);
        MusicRuntimeHooks hooks = scene.Kernel.Hooks(); hooks.Report = (_, _) => throw new IOException("초기화 진단 예외 입력");
        using MusicRuntime runtime = scene.Runtime(hooks);
        Assert.Throws<IOException>(runtime.Initialize);
        Assert.False(scene.Sound.MusicInitialized); Assert.Equal((nint)0, scene.State.Thread); Assert.Equal((nint)0, scene.State.Shutdown);
        Assert.Equal(0, scene.Io.LockDepth); Assert.Contains("O:T", scene.Io.Events);
    }

    /// <summary>기본 채널 정지의 실제 예외를 핸들/준비 반납 이후에 전달한다.</summary>
    [Fact]
    public void StopFailureIsRethrownAfterCompleteRuntimeCleanup()
    {
        var scene = new Scene(); scene.Kernel.Waits.Enqueue(0);
        using MusicRuntime runtime = scene.Runtime(); runtime.Initialize(); scene.Io.CloseThrows = true;
        Assert.Throws<IOException>(runtime.Shutdown);
        Assert.False(scene.Sound.MusicInitialized); Assert.Equal((nint)0, scene.State.Thread); Assert.Equal((nint)0, scene.State.Shutdown);
        Assert.Equal(0, scene.Io.LockDepth);
        Assert.True(scene.Io.Events.IndexOf("O:T") < scene.Io.Events.IndexOf("O:E"));
    }

    /// <summary>종료 대기 중 새 재생은 거부하되 같은 명령 잠금을 다른 호출자와 worker에게 반납한다.</summary>
    [Fact]
    public async Task ShutdownWaitReleasesCommandGateAndRejectsNewPlay()
    {
        var scene = new Scene();
        using var waiting = new ManualResetEventSlim(); using var finish = new ManualResetEventSlim();
        MusicRuntimeHooks hooks = scene.Kernel.Hooks();
        hooks.Wait = (_, milliseconds) =>
        {
            if (milliseconds == 0) return MusicRuntime.WaitTimeout;
            waiting.Set();
            if (!finish.Wait(TimeSpan.FromSeconds(5))) throw new InvalidOperationException("종료 대기 검사 제한 시간");
            return 0;
        };
        using MusicRuntime runtime = scene.Runtime(hooks); runtime.Initialize();
        Task stopping = Task.Run(runtime.Shutdown, TestContext.Current.CancellationToken);
        try
        {
            Assert.True(waiting.Wait(TimeSpan.FromSeconds(3), TestContext.Current.CancellationToken));
            Task<bool> command = Task.Run(() =>
            {
                bool playing = scene.Music.Play("track.mus", 1, new(), _ => throw new InvalidOperationException("종료 중 파일 조회"), null);
                scene.Music.SetVolume(-1500);
                return playing;
            }, TestContext.Current.CancellationToken);
            Assert.False(await command.WaitAsync(TimeSpan.FromSeconds(3), TestContext.Current.CancellationToken));
        }
        finally { finish.Set(); await stopping.WaitAsync(TimeSpan.FromSeconds(3), TestContext.Current.CancellationToken); }
        Assert.False(scene.Sound.MusicInitialized); Assert.Equal((nint)0, scene.State.Thread); Assert.Equal((nint)0, scene.State.Shutdown);
    }

    /// <summary>실제 worker 검사만 소유하는 새 임시 폴더와 긴 독립 WAVE 입력이다.</summary>
    private sealed class TemporaryWave : IDisposable
    {
        /// <summary>이 검사만 생성한 절대 임시 경로다.</summary>
        public string Root { get; } = Directory.CreateTempSubdirectory("NetstormDnMusicRuntime-").FullName;
        /// <summary>실제 Winmm이 읽을 한 곡의 경로다.</summary>
        public string Path { get; }
        /// <summary>독립 FileStream 결과와 대조할 실제 파일 전체 입력이다.</summary>
        public byte[] Bytes { get; } = new byte[46 + (int)MusicChannel.BufferBytes + 32];
        /// <summary>원문 검사와 같은 stereo16/22050Hz·18BYTE fmt WAVE를 작성한다.</summary>
        public TemporaryWave()
        {
            Path = System.IO.Path.Combine(Root, "track.mus");
            "RIFF"u8.CopyTo(Bytes); BinaryPrimitives.WriteUInt32LittleEndian(Bytes.AsSpan(4), (uint)Bytes.Length - 8); "WAVEfmt "u8.CopyTo(Bytes.AsSpan(8));
            BinaryPrimitives.WriteUInt32LittleEndian(Bytes.AsSpan(16), 18); BinaryPrimitives.WriteUInt16LittleEndian(Bytes.AsSpan(20), 1);
            BinaryPrimitives.WriteUInt16LittleEndian(Bytes.AsSpan(22), 2); BinaryPrimitives.WriteUInt32LittleEndian(Bytes.AsSpan(24), 22050);
            BinaryPrimitives.WriteUInt32LittleEndian(Bytes.AsSpan(28), 88200); BinaryPrimitives.WriteUInt16LittleEndian(Bytes.AsSpan(32), 4);
            BinaryPrimitives.WriteUInt16LittleEndian(Bytes.AsSpan(34), 16); "data"u8.CopyTo(Bytes.AsSpan(38));
            BinaryPrimitives.WriteUInt32LittleEndian(Bytes.AsSpan(42), MusicChannel.BufferBytes + 32);
            // worker 결과가 아닌 독립 파일 입력의 표본만 채운다.
            for (int index = 46; index < Bytes.Length; index++) Bytes[index] = unchecked((byte)((index - 46) * 7 + 11));
            File.WriteAllBytes(Path, Bytes);
        }
        /// <summary>이 검사에서 생성한 폴더만 반납한다. 실제 파일 owner는 먼저 종료되어야 한다.</summary>
        public void Dispose() => Directory.Delete(Root, true);
    }

    /// <summary>실제 Winmm 파일·Win32 worker를 사용하고 COM만 메모리 버퍼 응답으로 제공한다.</summary>
    [SupportedOSPlatform("windows")]
    private sealed class LiveScene : IDisposable
    {
        /// <summary>Runtime·음악 제어·효과음이 공유하는 현재 실제 전역이다.</summary>
        public readonly SoundState Sound = new() { Initialized = true, Device = true };
        /// <summary>다음 State에서도 다시 쓸 실제 음악 핸들 저장소다.</summary>
        public readonly MusicRuntimeState State = new();
        /// <summary>실제 열린 음악 파일을 worker 종료 뒤까지 소유한다.</summary>
        public readonly WindowsMusicFileStore Files = new();
        /// <summary>실제 읽기 대상이며 COM Lock이 직접 빌리는 메모리다.</summary>
        public readonly byte[] Buffer = new byte[(int)MusicChannel.BufferBytes];
        /// <summary>실제 기본 채널 상태다.</summary>
        public readonly MusicChannelState A = new();
        /// <summary>초기화/삭제에만 쓰는 두 번째 채널 상태다.</summary>
        public readonly MusicChannelState B = new();
        /// <summary>실제 기본 채널의 파일/갱신/정지 본체다.</summary>
        public readonly MusicChannel Primary;
        /// <summary>동일 제어 명령과 Runtime이 공유하는 음악 소유자다.</summary>
        public readonly SoundMusicControl Music;
        /// <summary>같은 실제 선택 이름 저장소다.</summary>
        public readonly MusicSelection Selection;
        /// <summary>실제 별도 Win32 worker의 수명 본체다.</summary>
        public readonly MusicRuntime Runtime;
        /// <summary>실제 worker의 첫 Play 횟수다. 주 스레드는 Volatile로 읽는다.</summary>
        public int Plays;
        /// <summary>기본 채널이 버퍼 참조를 반납한 횟수다.</summary>
        public int Releases;
        /// <summary>반납 전 직접 쓰기 구간이 남아 있는지다. 합류 뒤만 검사한다.</summary>
        public bool Locked;
        /// <summary>worker에서 실제 예외 경로를 선택하는 입력이다.</summary>
        public bool ThrowStatus;
        /// <summary>같은 음악 명령 잠금 안에서만 접근하는 실제 메모리 버퍼 상태다.</summary>
        private uint _status;

        /// <summary>실제 파일과 커널을 결합하되 장치/게임 창을 열지 않는다.</summary>
        public LiveScene(TemporaryWave input)
        {
            MusicChannelHooks channels = Files.FileHooks(new()
            {
                SetVolume = (_, _) => 0,
                Stop = _ => _status = 0,
                Release = _ => { Interlocked.Increment(ref Releases); return 0; },
            });
            Primary = new(OriginalEdition.Patch1078, A, channels);
            var secondary = new MusicChannel(OriginalEdition.Patch1078, B, channels);
            Music = SoundMusicControl.ForPrimary(OriginalEdition.Patch1078, Sound, Effects(OriginalEdition.Patch1078, Sound), Primary);
            MusicBufferHooks buffers = BufferHooks();
            Runtime = new(Sound, State, Primary, secondary, Music, buffers, WindowsMusicRuntime.CreateHooks());
            MusicOpenHooks open = Files.OpenHooks((query, _) => query.EndsWith("\\track.mus", StringComparison.Ordinal) ? input.Path : null);
            Selection = new(new(), Music, new(input.Root, null), open.Find!, Primary.BindPlaybackDevices(open, buffers));
        }

        /// <summary>실제 갱신이 채우기/재생을 수행하게 COM의 응답과 직접 쓰기 구간만 공급한다.</summary>
        private MusicBufferHooks BufferHooks() => new()
        {
            Create = (ReadOnlySpan<byte> _, uint _, uint _, ref uint token) => { _status = 0; token = BufferToken; return 0; },
            Status = (uint _, out uint status) => { if (ThrowStatus) throw new IOException("worker 상태 조회 예외 입력"); status = _status; return 0; },
            Restore = _ => 0,
            Cursor = (uint _, out uint play, out uint write) => { play = write = 0; return 0; },
            Lock = (uint _, uint offset, uint count, ref MusicBufferRegions regions) =>
            {
                if (Locked || offset != 0 || count != Buffer.Length) throw new InvalidOperationException("worker 실제 쓰기 구간 입력 오류");
                Locked = true; regions = new(Buffer, []); return 0;
            },
            Unlock = (_, _) => { if (!Locked) throw new InvalidOperationException("worker 쓰기 구간 반납 입력 오류"); Locked = false; return 0; },
            Play = (_, flags) => { if (flags != 1) throw new InvalidOperationException("worker 링 재생 flags 입력 오류"); _status = 1; Interlocked.Increment(ref Plays); return 0; },
        };

        /// <summary>실제 worker를 먼저 합류하고 그 뒤 실제 파일 저장소를 반납한다.</summary>
        public void Dispose() { Runtime.Dispose(); Files.Dispose(); }
    }

    /// <summary>같은 두 채널/제어/실제 커널 owner를 세 번 초기화·선택·갱신·종료하고 실제 파일 표본을 대조한다.</summary>
    [Fact]
    [SupportedOSPlatform("windows")]
    public void RealWin32WorkerStreamsFileAndReusesSameOwners()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var input = new TemporaryWave(); using var scene = new LiveScene(input);
        // 새 전역을 조립하지 않고 같은 실제 소유자 수명을 반복한다.
        for (int cycle = 1; cycle <= 3; cycle++)
        {
            scene.Runtime.Initialize(); Assert.True(scene.Sound.MusicInitialized);
            Assert.NotEqual((nint)0, scene.State.Thread); Assert.NotEqual((nint)0, scene.State.Shutdown);
            scene.Selection.Select("track.mus"); uint file = scene.A.Field(MusicField.File);
            Assert.True(SpinWait.SpinUntil(() => Volatile.Read(ref scene.Plays) >= cycle, TimeSpan.FromSeconds(3)));
            // 공개 음악 명령도 같은 실제 잠금과 worker를 사용한다.
            for (int volume = -1000; volume < -900; volume++) scene.Music.SetVolume(volume);
            scene.Runtime.Shutdown();
            Assert.Null(scene.Runtime.WorkerFailure); Assert.False(scene.Sound.MusicInitialized);
            Assert.Equal((nint)0, scene.State.Thread); Assert.Equal((nint)0, scene.State.Shutdown); Assert.NotEqual(0u, scene.State.ThreadId);
            Assert.False(scene.Primary.Active()); Assert.Equal(-1, scene.Files.Position(file)); Assert.False(scene.Locked);
            Assert.Equal(input.Bytes.AsSpan(46, scene.Buffer.Length).ToArray(), scene.Buffer);
            Assert.Equal(0u, scene.A.Field(MusicField.Buffer)); Assert.Equal(0u, scene.B.Field(MusicField.Buffer)); Assert.Equal(cycle, scene.Releases);
        }
    }

    /// <summary>실제 worker 예외를 진단으로 남기고 합류/파일 반납 뒤 다음 초기화에서만 지운다.</summary>
    [Fact]
    [SupportedOSPlatform("windows")]
    public void RealWorkerFailureSurvivesShutdownAndClearsOnNextInitialize()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var input = new TemporaryWave(); using var scene = new LiveScene(input) { ThrowStatus = true };
        scene.Runtime.Initialize(); scene.Selection.Select("track.mus"); uint file = scene.A.Field(MusicField.File);
        Assert.True(SpinWait.SpinUntil(() => scene.Runtime.WorkerFailure is not null, TimeSpan.FromSeconds(3)));
        scene.Runtime.Shutdown(); Assert.IsType<IOException>(scene.Runtime.WorkerFailure);
        Assert.Equal(-1, scene.Files.Position(file)); Assert.False(scene.Locked);
        Assert.Equal((nint)0, scene.State.Thread); Assert.Equal((nint)0, scene.State.Shutdown);
        scene.ThrowStatus = false; scene.Runtime.Initialize(); Assert.Null(scene.Runtime.WorkerFailure); scene.Runtime.Shutdown();
    }
}
