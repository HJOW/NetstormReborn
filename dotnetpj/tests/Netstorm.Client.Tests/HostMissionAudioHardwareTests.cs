using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;

// 실제 숨긴 HWND·DirectSound·Winmm·Win32 worker만 사용하는 명시 음소거 검사다.
namespace Netstorm.Client.Tests;

/// <summary>이 명시 검사의 실제 장치/창 수명이 다른 검사와 겹치지 않게 한다.</summary>
[CollectionDefinition("실제 장기 음악 장치", DisableParallelization = true)]
public sealed class HostMissionAudioHardwareCollection
{
}

/// <summary>장치/커널 성공 응답이나 준비 플래그를 주입하지 않고 실제 장기 음악 owner의 재열기·최종 반납을 검사한다.</summary>
[Collection("실제 장기 음악 장치")]
[SupportedOSPlatform("windows")]
public sealed class HostMissionAudioHardwareTests
{
    /// <summary>실제 Win32 비신호 대기의 반환값이다.</summary>
    private const uint WaitTimeout = 0x102;
    /// <summary>실제 스레드 객체가 종료되어 신호 상태일 때0ms 대기가 반환하는 값이다.</summary>
    private const uint WaitObject = 0;
    /// <summary>원래 핸들이 실제로 닫힌 직후 Windows가 보고하는 오류 번호다.</summary>
    private const int ErrorInvalidHandle = 6;
    /// <summary>실제 worker 핸들과 동일 권한의 자기 관측용 복제 핸들을 요청한다.</summary>
    private const uint DuplicateSameAccess = 2;
    /// <summary>자기 숨긴 STATIC 창만 만든다. 포커스·가시성을 바꾸지 않는다.</summary>
    [DllImport("user32.dll", EntryPoint = "CreateWindowExW", CharSet = CharSet.Unicode)]
    private static extern nint CreateWindow(uint extended, string className, string name, uint style,
        int x, int y, int width, int height, nint parent, nint menu, nint instance, nint parameter);
    /// <summary>같은 테스트가 만든 HWND만 모든 음향 자원 반납 뒤 파괴한다.</summary>
    [DllImport("user32.dll", EntryPoint = "DestroyWindow", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool DestroyWindow(nint window);
    /// <summary>검사 HWND가 화면에 표시되지 않았는지 읽는다.</summary>
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool IsWindowVisible(nint window);
    /// <summary>보유한 값이 실제 열린 커널 핸들인지 Windows에서 확인한다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetHandleInformation(nint handle, out uint flags);
    /// <summary>실제 worker 핸들과 원문 DWORD threadId의 일치를 읽는다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint GetThreadId(nint thread);
    /// <summary>현재 worker 생존/수동 리셋 이벤트의 비신호 상태만0ms로 조회한다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint WaitForSingleObject(nint handle, uint milliseconds);
    /// <summary>핸들 복제에만 사용하는 현재 프로세스의 빌린 의사 핸들이다. 검사에서 닫지 않는다.</summary>
    [DllImport("kernel32.dll")]
    private static extern nint GetCurrentProcess();
    /// <summary>같은 실제 스레드 객체를 종료 뒤에도 관측할 자기 복제 핸들을 발급한다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool DuplicateHandle(nint sourceProcess, nint source, nint targetProcess,
        out nint target, uint access, [MarshalAs(UnmanagedType.Bool)] bool inherit, uint options);
    /// <summary>검사가 발급받은 복제 핸들만 반납한다. 제품의 원래 thread/event 핸들은 닫지 않는다.</summary>
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CloseHandle(nint handle);
    /// <summary>IDirectSoundBuffer::GetVolume의 실제 LONG 출력 ABI다. 실제 버퍼에서 음소거 적용을 다시 읽는다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int GetBufferVolume(nint buffer, out int volume);

    /// <summary>새 public 진단 API 없이 같은 실제 private owner의 identity를 읽는다. 값을 쓰거나 성공 플래그를 보정하지 않는다.</summary>
    private static T Field<T>(object owner, string name) =>
        (T)(owner.GetType().GetField(name, BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new MissingFieldException(owner.GetType().FullName, name)).GetValue(owner)!;

    /// <summary>같은 실제 음향 명령 잠금 아래 얻은 파일/버퍼/재생 진행의 관측값이다.</summary>
    /// <param name="File">현재 열린 Winmm 토큰이다.</param>
    /// <param name="Buffer">현재 실제 COM 버퍼 토큰이다.</param>
    /// <param name="DataOffset">같은 파일의 표본 시작 위치다.</param>
    /// <param name="ReadOffset">실제 채널이 정확히 읽은 표본 바이트 수다.</param>
    /// <param name="WriteOffset">실제 링 버퍼 다음 쓰기 위치다.</param>
    /// <param name="FilePosition">Winmm이 보고한 실제 파일 절대 위치다.</param>
    /// <param name="PlayCursor">DirectSound가 보고한 실제 재생 커서다.</param>
    /// <param name="Status">DirectSound의 재생/반복 상태다.</param>
    /// <param name="NativeVolume">같은 COM 버퍼에서 읽은 실제 음량이다.</param>
    private readonly record struct Progress(uint File, uint Buffer, uint DataOffset, uint ReadOffset,
        uint WriteOffset, int FilePosition, uint PlayCursor, uint Status, int NativeVolume);

    /// <summary>worker와 같은 명령 잠금으로 원시 필드·실제 Winmm 위치·COM 커서를 함께 읽는다. 잠금 대기도100ms로 제한한다.</summary>
    private static Progress ReadProgress(MusicChannel channel, SoundMusicControl control,
        WindowsMusicFileStore files, WindowsSoundDevice device)
    {
        bool held = false;
        try
        {
            Monitor.TryEnter(control.CommandGate, TimeSpan.FromMilliseconds(100), ref held);
            if (!held) return default;
            MusicChannelState state = channel.State;
            uint file = state.Field(MusicField.File), buffer = state.Field(MusicField.Buffer);
            if (file == 0 || buffer == 0 || !channel.Active()) return default;
            MusicBufferHooks buffers = device.MusicBuffers();
            Assert.Equal(0, buffers.Status!(buffer, out uint status));
            Assert.Equal(0, buffers.Cursor!(buffer, out uint play, out _));
            nint pointer = Field<Dictionary<uint, nint>>(device, "_buffers")[buffer];
            nint method = Marshal.ReadIntPtr(Marshal.ReadIntPtr(pointer), 6 * nint.Size);
            Assert.Equal(0, Marshal.GetDelegateForFunctionPointer<GetBufferVolume>(method)(pointer, out int volume));
            return new(file, buffer, state.Field(MusicField.DataOffset), state.Field(MusicField.ReadOffset),
                state.Field(MusicField.WriteOffset), files.Position(file), play, status, volume);
        }
        finally { if (held) Monitor.Exit(control.CommandGate); }
    }

    /// <summary>고정500ms 완료 시각을 강제하지 않고 실제 변화가 보일 때까지만 최대10초간50ms 간격으로 조회한다.</summary>
    private static Progress AwaitProgress(MusicChannel channel, SoundMusicControl control,
        WindowsMusicFileStore files, WindowsSoundDevice device, MusicRuntime runtime,
        Func<Progress, bool> ready, string reason)
    {
        var clock = Stopwatch.StartNew();
        Progress last = default;
        // 느린 하드웨어도 실제 진행이 있으면 즉시 통과하며 정확한 갱신 지연 시간은 검증값으로 쓰지 않는다.
        while (clock.Elapsed < TimeSpan.FromSeconds(10))
        {
            last = ReadProgress(channel, control, files, device);
            if (ready(last)) return last;
            if (runtime.WorkerFailure is not null) break;
            Thread.Sleep(50);
        }
        Assert.Fail(reason + ": " + last + ", worker=" + runtime.WorkerFailure);
        return default;
    }

    /// <summary>실제 worker/event 핸들을 조회해 별도 스레드·비신호 이벤트와 보관된 threadId를 확인한다.</summary>
    private static void AssertLiveKernel(MusicRuntimeState state)
    {
        Assert.NotEqual((nint)0, state.Thread); Assert.NotEqual((nint)0, state.Shutdown);
        Assert.True(GetHandleInformation(state.Thread, out _)); Assert.True(GetHandleInformation(state.Shutdown, out _));
        Assert.NotEqual(0u, state.ThreadId); Assert.Equal(state.ThreadId, GetThreadId(state.Thread));
        Assert.Equal(WaitTimeout, WaitForSingleObject(state.Thread, 0));
        Assert.Equal(WaitTimeout, WaitForSingleObject(state.Shutdown, 0));
    }

    /// <summary>정리 실패가 본문/앞선 정리의 최초 예외를 덮지 않게 그 예외의 Data에 보존한다.</summary>
    private static void CleanupPreservingFirst(Action cleanup, Exception? first, string key)
    {
        try { cleanup(); }
        catch (Exception error)
        {
            if (first is null) throw;
            try { first.Data[key] = error; }
            catch { /* 진단 부착 실패도 최초 예외의 전달을 막지 않는다. */ }
        }
    }

    /// <summary>다른 native 호출 전에 성공 여부와 오류 번호를 보관하여 원래 핸들의 실제 무효를 확인한다.</summary>
    private static void AssertInvalidHandle(nint handle)
    {
        bool valid = GetHandleInformation(handle, out _);
        int error = Marshal.GetLastPInvokeError();
        Assert.False(valid);
        Assert.Equal(ErrorInvalidHandle, error);
    }

    /// <summary>자기 복제 핸들의 반납 실패도 관측하되 원래 제품 핸들에는 CloseHandle을 호출하지 않는다.</summary>
    private static void CloseInspectionHandle(nint handle)
    {
        bool closed = CloseHandle(handle);
        int error = Marshal.GetLastPInvokeError();
        Assert.True(closed, $"검사 복제 스레드 핸들 반납 실패: {error}");
    }

    /// <summary>종료 직전 실제 thread/event를 보관하고 정상 복귀 직후 원래 핸들 반납과 같은 스레드 객체의 종료를 확인한다.</summary>
    private static void AssertNativeShutdown(MusicRuntimeState state, Action shutdown)
    {
        nint thread = state.Thread, signal = state.Shutdown, duplicate = 0;
        Exception? first = null;
        try
        {
            Assert.NotEqual((nint)0, thread); Assert.NotEqual((nint)0, signal);
            nint process = GetCurrentProcess();
            bool copied = DuplicateHandle(process, thread, process, out nint copiedThread, 0, false, DuplicateSameAccess);
            int error = Marshal.GetLastPInvokeError();
            if (copied) duplicate = copiedThread;
            Assert.True(copied, $"실제 worker 관측용 핸들 복제 실패: {error}");
            Assert.NotEqual((nint)0, duplicate);
            shutdown();
            // 이 짧은 직렬 검사에서도 외부 스레드의 원래 핸들 숫자 재사용까지 배제하지는 못한다.
            // 새 owner/장치를 만들기 전에 즉시 검사하고, 스레드 종료 자체는 별도 복제 핸들로 확인한다.
            AssertInvalidHandle(thread);
            AssertInvalidHandle(signal);
            Assert.Equal(WaitObject, WaitForSingleObject(duplicate, 0));
        }
        catch (Exception error) { first = error; throw; }
        finally
        {
            if (duplicate != 0)
                CleanupPreservingFirst(() => CloseInspectionHandle(duplicate), first, "복제 스레드 핸들 정리 실패");
        }
    }

    /// <summary>직접 만든 숨긴 창만 파괴하며 호출 직후 보관한 실제 오류로 실패를 보고한다.</summary>
    private static void DestroyInspectionWindow(nint window)
    {
        bool destroyed = DestroyWindow(window);
        int error = Marshal.GetLastPInvokeError();
        Assert.True(destroyed, $"검사 숨긴 HWND 반납 실패: {error}");
    }

    /// <summary>sound0/music1 최초 실제 열기·공통 음소거·표본/재생커서 진행·Close/재열기·최종 반납을 한 소유자로 검사한다.</summary>
    [Fact(Explicit = true)]
    public void RealMutedMusicOwnerReopensWithDisabledEffectsAndReleasesAllNativeResources()
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Windows 실제 장기 음악 장치 검사입니다");
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        nint window = CreateWindow(0, "STATIC", "Netstorm muted music owner inspection", 0x80000000,
            0, 0, 1, 1, 0, 0, 0, 0);
        Assert.NotEqual((nint)0, window);
        HostMissionAudio? audio = null;
        Exception? firstException = null;
        try
        {
            Assert.False(IsWindowVisible(window));
            var startup = new HostStartupScene(data);
            audio = new HostMissionAudio(startup, () => Stopwatch.GetTimestamp() / (double)Stopwatch.Frequency);
            var device = Field<WindowsSoundDevice>(audio, "_device");
            var files = Field<WindowsMusicFileStore>(audio, "_musicFiles");
            var primary = Field<MusicChannel>(audio, "_primaryMusic");
            var control = Field<SoundMusicControl>(audio, "_musicControl");
            var runtime = Field<MusicRuntime>(audio, "_musicRuntime");
            var handles = Field<MusicRuntimeState>(audio, "_musicRuntimeState");
            var state = Field<SoundState>(audio, "_state");
            var options = new ClientAudioOptions { Sound = false, Music = true, Quality = 3,
                SoundVolumeStep = 1, MusicVolumeStep = 1 };
            Assert.False(audio.DeviceReady); Assert.Empty(audio.CurrentMusicName); Assert.False(primary.Active());
            Assert.True(audio.Initialize(window, options), device.LastInitializationFailure);
            Assert.True(audio.DeviceReady); Assert.False(audio.SoundEnabled); Assert.True(state.MusicInitialized);
            Assert.Empty(audio.CurrentMusicName); Assert.False(primary.Active());
            AssertLiveKernel(handles);
            // 최초 준비는 파일 재생을 시작하지 않는다. 실제 음악 선택 전에 공통 음량을-10000으로 보류한다.
            audio.PushMute();
            Assert.Equal(1, state.VolumeHoldDepth);
            Assert.Equal(SoundConstants.Minimum, state.MasterVolume); Assert.Equal(SoundConstants.Minimum, state.MusicVolume);
            uint previousFile = 0;
            // 같은 owner를 두 번 사용하며 두 번째 초기화에서도 이미 적용한 실제 공통 음소거를 유지한다.
            for (int cycle = 0; cycle < 2; cycle++)
            {
                if (cycle != 0)
                {
                    Assert.True(audio.Initialize(window, options), device.LastInitializationFailure);
                    Assert.True(audio.DeviceReady); Assert.False(audio.SoundEnabled); Assert.True(state.MusicInitialized);
                    Assert.Equal(1, state.VolumeHoldDepth); Assert.Equal(SoundConstants.Minimum, state.MusicVolume);
                    AssertLiveKernel(handles);
                    Assert.Same(files, Field<WindowsMusicFileStore>(audio, "_musicFiles"));
                    Assert.Same(primary, Field<MusicChannel>(audio, "_primaryMusic"));
                    Assert.Same(control, Field<SoundMusicControl>(audio, "_musicControl"));
                }
                audio.SelectMusic("anticipation.mus");
                Assert.Equal("anticipation.mus", audio.CurrentMusicName);
                Assert.True(audio.CurrentMusicDuration > 0);
                Progress first = AwaitProgress(primary, control, files, device, runtime,
                    value => value.File != 0 && value.Buffer != 0 && value.ReadOffset > 0
                        && value.FilePosition > value.DataOffset && (value.Status & 5u) == 5u,
                    "실제 첫 표본 읽기/링 재생이 관측되지 않았습니다");
                Assert.True(first.File > previousFile); previousFile = first.File;
                Assert.Equal(SoundConstants.Minimum, first.NativeVolume);
                // 최초 전체 채움 다음에도 실제 하드웨어 커서와 Winmm 읽기가 함께 전진해야 한다.
                Progress later = AwaitProgress(primary, control, files, device, runtime,
                    value => value.File == first.File && value.Buffer == first.Buffer
                        && value.ReadOffset > first.ReadOffset && value.FilePosition > first.FilePosition
                        && value.PlayCursor != first.PlayCursor,
                    "실제 주기 worker의 다음 읽기/재생커서 진행이 관측되지 않았습니다");
                Assert.Equal(SoundConstants.Minimum, later.NativeVolume);
                Assert.Null(runtime.WorkerFailure);
                if (cycle == 0)
                {
                    AssertNativeShutdown(handles, audio.CloseDevice);
                    Assert.False(audio.DeviceReady); Assert.False(state.MusicInitialized);
                    Assert.Equal((nint)0, handles.Thread); Assert.Equal((nint)0, handles.Shutdown);
                    Assert.False(primary.Active()); Assert.Equal(0u, primary.State.Field(MusicField.File));
                    Assert.Equal(0u, primary.State.Field(MusicField.Buffer)); Assert.Equal(-1, files.Position(first.File));
                    Assert.Equal("anticipation.mus", audio.CurrentMusicName);
                }
            }
            // 두 번째 재생 중 최종 Dispose가 실제 worker 합류→장치→파일 저장소를 모두 반납한다.
            AssertNativeShutdown(handles, audio.Dispose);
            Assert.False(audio.DeviceReady); Assert.False(state.MusicInitialized); Assert.Null(runtime.WorkerFailure);
            Assert.Equal((nint)0, handles.Thread); Assert.Equal((nint)0, handles.Shutdown);
            Assert.Equal(0u, primary.State.Field(MusicField.File)); Assert.Equal(0u, primary.State.Field(MusicField.Buffer));
            Assert.Equal(-1, files.Position(previousFile));
            Assert.Equal((nint)0, Field<nint>(device, "_device")); Assert.Equal((nint)0, Field<nint>(device, "_module"));
            Assert.Throws<ObjectDisposedException>(() => files.Open(Path.Combine(data, "music", "anticipation.mus"), new()));
            Assert.Throws<ObjectDisposedException>(() => audio.SelectMusic("anticipation.mus"));
        }
        catch (Exception error) { firstException = error; throw; }
        finally
        {
            try
            {
                // 본문 실패 뒤에도 실제 owner를 먼저 반납하고 창은 그다음에 파괴한다.
                if (audio is not null)
                    CleanupPreservingFirst(audio.Dispose, firstException, "장기 음향 최종 정리 실패");
            }
            catch (Exception error) { firstException ??= error; throw; }
            finally
            {
                CleanupPreservingFirst(() => DestroyInspectionWindow(window), firstException, "숨긴 HWND 정리 실패");
            }
        }
    }
}
