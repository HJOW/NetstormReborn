using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Client.Audio;

// 실제 Win32 이벤트·스레드를 원문 커널 입구에 연결하고 강제 중단을 협조 종료로 바꾼다.
namespace Netstorm.Client.Host;

/// <summary>게임 창·DirectSound 장치를 만들지 않는 실제 음악 worker의 Windows 커널 경계다.</summary>
[SupportedOSPlatform("windows")]
public static class WindowsMusicRuntime
{
    /// <summary>완전 합류에서만 사용하는 Win32 무한 대기다.</summary>
    private const uint Infinite = uint.MaxValue;
    /// <summary>CreateThread가 실제 별도 스레드에서 호출하는 stdcall 진입점 형식이다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate uint WorkerProc(nint parameter);
    /// <summary>native 함수 포인터가 worker 종료 전 수집되지 않게 프로세스 수명으로 보관한다.</summary>
    private static readonly WorkerProc Entry = WorkerMain;

    /// <summary>익명 수동 리셋·초기 비신호 이벤트를 만든다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "CreateEventW", SetLastError = true)]
    private static extern nint CreateEventNative(nint attributes, [MarshalAs(UnmanagedType.Bool)] bool manualReset,
        [MarshalAs(UnmanagedType.Bool)] bool initialState, nint name);
    /// <summary>관리 callback의 고정 진입점과 매개변수를 실제 별도 native 스레드에 넘긴다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "CreateThread", SetLastError = true)]
    private static extern nint CreateThreadNative(nint attributes, nuint stackBytes, WorkerProc entry, nint parameter, uint flags, ref uint threadId);
    /// <summary>현재 커널 핸들의 원문 대기 결과DWORD를 돌려준다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "WaitForSingleObject", SetLastError = true)]
    private static extern uint WaitNative(nint handle, uint milliseconds);
    /// <summary>같은 실제 종료 이벤트를 신호 상태로 만든다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "SetEvent", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SignalNative(nint handle);
    /// <summary>합류된 스레드 또는 그 뒤의 이벤트 핸들을 반납한다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "CloseHandle", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CloseNative(nint handle);

    /// <summary>callback과 그 대상 Runtime을 worker가 끝날 때까지 실제 GCHandle로 보유한다.</summary>
    private sealed class WorkerEntry
    {
        /// <summary>실제 별도 스레드에서 한 번 실행할 Runtime 진입점이다.</summary>
        public readonly Func<uint> Callback;
        /// <summary>아직 실행하지 않고 callback의 강한 참조만 보관한다.</summary>
        public WorkerEntry(Func<uint> callback) { Callback = callback; }
    }

    /// <summary>callback 예외를 native 경계 밖으로 내보내지 않으며 반환 때 GCHandle을 반드시 해제한다.</summary>
    private static uint WorkerMain(nint parameter)
    {
        GCHandle entry = GCHandle.FromIntPtr(parameter);
        try { return ((WorkerEntry)entry.Target!).Callback(); }
        catch { return 0; }
        finally { entry.Free(); }
    }

    /// <summary>한 Runtime의 실제 native 핸들과 협조 종료 표식을 소유한다.</summary>
    private sealed class Owner
    {
        /// <summary>worker와 종료 호출자가 공유하는 현재 실제 종료 이벤트다.</summary>
        private nint _event;
        /// <summary>수명 본체의 생성·닫기 호출만 변경하는 실제 스레드 핸들이다.</summary>
        private nint _thread;
        /// <summary>신호 실패도 다음500ms 대기 이후 정상 반환하게 하는 협조 종료 요청이다.</summary>
        private int _stopping;

        /// <summary>다음 수명의 종료 요청을 지우고 실제 수동 리셋 이벤트를 만든다.</summary>
        public nint CreateEvent()
        {
            Volatile.Write(ref _stopping, 0);
            nint created = CreateEventNative(0, true, false, 0);
            Volatile.Write(ref _event, created);
            return created;
        }

        /// <summary>실제 스레드가 소유할 callback을 고정한다. 생성 실패에서는 지역 소유권을 되돌리고0을 반환한다.</summary>
        public nint CreateThread(Func<uint> callback, ref uint threadId)
        {
            GCHandle entry = default;
            bool allocated = false;
            try
            {
                entry = GCHandle.Alloc(new WorkerEntry(callback)); allocated = true;
                nint created = CreateThreadNative(0, 0, Entry, GCHandle.ToIntPtr(entry), 0, ref threadId);
                if (created == 0) { entry.Free(); allocated = false; }
                else allocated = false;
                _thread = created;
                return created;
            }
            catch
            {
                if (allocated) entry.Free();
                return 0;
            }
        }

        /// <summary>원문 커널 대기를 수행하되 협조 종료 요청이 생긴 이벤트는 정상 신호로 반환한다.</summary>
        public uint Wait(nint handle, uint milliseconds)
        {
            nint shutdown = Volatile.Read(ref _event);
            if (handle == shutdown && Volatile.Read(ref _stopping) != 0) return 0;
            uint result = WaitNative(handle, milliseconds);
            return handle == shutdown && Volatile.Read(ref _stopping) != 0 ? 0u : result;
        }

        /// <summary>원문 TerminateThread 위치에서 강제 중단 없이 실제 종료 이벤트와 공유 표식만 설정한다.</summary>
        public void RequestStop(nint thread)
        {
            Volatile.Write(ref _stopping, 1);
            nint shutdown = Volatile.Read(ref _event);
            if (shutdown != 0) _ = SignalNative(shutdown);
        }

        /// <summary>실제 worker를 완전히 합류한 뒤 핸들을 닫고 이벤트는 별도 다음 호출에서 반납한다.</summary>
        public void Close(nint handle)
        {
            if (handle == _thread)
            {
                RequestStop(handle);
                _ = WaitNative(handle, Infinite);
                _ = CloseNative(handle);
                _thread = 0;
            }
            else if (handle == Volatile.Read(ref _event))
            {
                _ = CloseNative(handle);
                Volatile.Write(ref _event, 0);
            }
        }
    }

    /// <summary>실제 소유자를 함수 슬롯이 공유하게 만든다. 이 호출 자체는 이벤트·스레드·창·장치를 열지 않는다.</summary>
    public static MusicRuntimeHooks CreateHooks(Action<uint, string>? log = null, Action<string, int>? report = null,
        Action<uint, bool>? section = null)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("음악 worker의 현재 커널 경계는 Windows 전용이다");
        var owner = new Owner();
        return new()
        {
            CreateEvent = owner.CreateEvent,
            CreateThread = owner.CreateThread,
            Wait = owner.Wait,
            Signal = SignalNative,
            Terminate = owner.RequestStop,
            Close = owner.Close,
            Log = log,
            Report = report,
            Section = section,
        };
    }
}
