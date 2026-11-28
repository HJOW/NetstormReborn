using System.Runtime.InteropServices;
using System.Text;

namespace Netstorm.Client.Host; // 원문 time32/localtime32/asctime의 실제 Windows CRT를 사용한다.

/// <summary>25BYTE 영문 달력과 끝개행을 반환하며 CRT 실패를 빈 문자열로 감추지 않는다.</summary>
public static class WindowsMonitorCalendar
{
    /// <summary>Windows CRT의9개 signed int tm 구조다.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct NativeTm
    {
        /// <summary>초다.</summary>
        public int Second;
        /// <summary>분이다.</summary>
        public int Minute;
        /// <summary>시다.</summary>
        public int Hour;
        /// <summary>월 안의 날짜다.</summary>
        public int Day;
        /// <summary>0부터 시작하는 월이다.</summary>
        public int Month;
        /// <summary>1900년 이후 연도다.</summary>
        public int Year;
        /// <summary>0부터 시작하는 요일이다.</summary>
        public int WeekDay;
        /// <summary>0부터 시작하는 연중 날짜다.</summary>
        public int YearDay;
        /// <summary>일광 절약 시간 표식이다.</summary>
        public int DaylightSaving;
    }
    /// <summary>원문32비트 Unix 초를 읽는다.</summary>
    [DllImport("ucrtbase.dll", EntryPoint = "_time32", CallingConvention = CallingConvention.Cdecl)]
    private static extern int ReadTime(out int time);
    /// <summary>같은32비트 초를 OS 지역 달력으로 변환한다.</summary>
    [DllImport("ucrtbase.dll", EntryPoint = "_localtime32_s", CallingConvention = CallingConvention.Cdecl)]
    private static extern int LocalTime(out NativeTm local, in int time);
    /// <summary>고정26BYTE에 영문 달력·개행·NUL을 쓴다.</summary>
    [DllImport("ucrtbase.dll", EntryPoint = "asctime_s", CallingConvention = CallingConvention.Cdecl)]
    private static extern int AsciiTime([Out] byte[] text, nuint size, in NativeTm time);
    /// <summary>세 실제 CRT 호출을 순서대로 실행한다. 게임 정지/고정 프레임 시계로 OS 달력을 대체하지 않는다.</summary>
    public static string Read()
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 시간 알림 달력은 Windows CRT 전용이다");
        if (ReadTime(out int now) == -1) throw new InvalidOperationException("시간 알림 time32 실패");
        if (LocalTime(out NativeTm local, in now) != 0) throw new InvalidOperationException("시간 알림 localtime32 실패");
        var text = new byte[26];
        if (AsciiTime(text, (nuint)text.Length, in local) != 0) throw new InvalidOperationException("시간 알림 asctime 실패");
        int end = text.AsSpan().IndexOf((byte)0);
        if (end < 0) throw new InvalidOperationException("시간 알림 asctime의 실제 NUL 누락");
        return Encoding.Latin1.GetString(text, 0, end);
    }
}
